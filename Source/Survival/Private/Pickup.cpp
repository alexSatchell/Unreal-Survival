#include "Pickup.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "SurvivalItemDefinition.h"
#include "SurvivalConsumableItemDefinition.h"
#include "SurvivalCharacter.h"
#include "SurvivalInventoryComponent.h"
#include "SurvivalVitalComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPickup, Log, All);

APickup::APickup()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetBoxExtent(FVector(50.0f));
	// This box is a query-only interaction volume. It does not participate in floor physics or item placement.
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionComponent->SetupAttachment(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bReplicates = true;
}

void APickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyItemDefinitionState(ItemDefinition);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		return;
	}

	ApplyItemDefinitionState(ItemDefinition);

	if (!ItemInstanceId.IsValid())
	{
		ItemInstanceId = FGuid::NewGuid();
	}
}

USurvivalItemDefinition* APickup::GetItemDefinition() const
{
	if (IsValid(ItemDefinition))
	{
		return ItemDefinition;
	}

	return nullptr;
}

void APickup::ApplyItemDefinitionState(USurvivalItemDefinition* InItemDefinition)
{
	if (!IsValid(InItemDefinition))
	{
		// Clear the visible presentation so an invalid definition cannot leave an old mesh or collision volume behind.
		ApplyItemPresentation(nullptr);
		return;
	}

	// Copy the definition's stable identity and display data into the pickup state that can be replicated to clients.
	DefinitionAssetId = InItemDefinition->GetPrimaryAssetId();
	ReplicatedDisplayName = InItemDefinition->DisplayName;
	ReplicatedPickupMesh = InItemDefinition->PickupMesh;

	// Ask the definition to build the actions this item supports, such as Store for all items or Consume for consumables.
	ReplicatedInteractionOptions.Reset();
	InItemDefinition->GetWorldInteractionOptions(ReplicatedInteractionOptions);
	ReplicatedDefaultActionId = ReplicatedInteractionOptions.Num() > 0 ? ReplicatedInteractionOptions[0].ActionId : NAME_None;

	// Apply the definition's mesh and resize the interaction volume to match it.
	ApplyItemPresentation(InItemDefinition);
}

void APickup::ApplyItemPresentation(USurvivalItemDefinition* InItemDefinition)
{
	MeshComponent->SetStaticMesh(IsValid(InItemDefinition) ? InItemDefinition->PickupMesh : nullptr);
	UpdateCollisionBounds();
}

void APickup::UpdateCollisionBounds()
{
	if (!MeshComponent || !CollisionComponent || !MeshComponent->GetStaticMesh())
	{
		// Without a mesh, use a centered fallback volume so the pickup still has a predictable query area.
		CollisionComponent->SetRelativeLocation(FVector::ZeroVector);
		CollisionComponent->SetBoxExtent(FVector(50.0f));
		return;
	}

	FVector BoundsMin;
	FVector BoundsMax;
	MeshComponent->GetLocalBounds(BoundsMin, BoundsMax);

	// Move the sibling collision component to the center of the mesh's local-space bounds.
	CollisionComponent->SetRelativeLocation((BoundsMin + BoundsMax) * 0.5f);

	// SetBoxExtent expects half-extents, so add the configured padding to each side of the mesh bounds.
	CollisionComponent->SetBoxExtent((BoundsMax - BoundsMin) * 0.5f + CollisionPadding);
}

void APickup::OnRep_DefinitionAssetId()
{
	UpdatePickupMesh();
}

void APickup::OnRep_PickupMesh()
{
	UpdatePickupMesh();
}

void APickup::UpdatePickupMesh()
{
	MeshComponent->SetStaticMesh(ReplicatedPickupMesh);
	UpdateCollisionBounds();
}

bool APickup::InitializeItemInstance(const FSurvivalItemInstance& InItemInstance, USurvivalItemDefinition* InItemDefinition)
{
	if (!HasAuthority() || !InItemInstance.IsValid() || !IsValid(InItemDefinition) || InItemDefinition->GetPrimaryAssetId() != InItemInstance.DefinitionAssetId)
	{
		return false;
	}

	ItemInstanceId = InItemInstance.InstanceId;
	ItemDefinition = InItemDefinition;
	ApplyItemDefinitionState(InItemDefinition);
	ForceNetUpdate();
	return true;
}

void APickup::Interact_Implementation(AController* InteractingController)
{
	InteractWithAction_Implementation(InteractingController, SurvivalItemInteractionActions::Store);
}

void APickup::InteractWithAction_Implementation(AController* InteractingController, FName ActionId)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected interaction with pickup %s because it has no authority"), *GetName());
		return;
	}

	if (ActionId == SurvivalItemInteractionActions::Store)
	{
		TryStoreItem(InteractingController);
		return;
	}

	if (ActionId == SurvivalItemInteractionActions::Consume)
	{
		TryConsumeItem(InteractingController);
		return;
	}

	UE_LOG(LogPickup, Warning, TEXT("Rejected unknown action %s for pickup %s"), *ActionId.ToString(), *GetName());
}

bool APickup::TryStoreItem(AController* InteractingController)
{
	USurvivalItemDefinition* PickupItemDefinition = GetItemDefinition();
	if (!IsValid(PickupItemDefinition))
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected interaction with pickup %s because no item definition is assigned"), *GetName());
		return false;
	}

	ASurvivalCharacter* SurvivalCharacter = InteractingController ? Cast<ASurvivalCharacter>(InteractingController->GetPawn()) : nullptr;
	USurvivalInventoryComponent* SurvivalInventoryComponent = SurvivalCharacter ? SurvivalCharacter->GetInventoryComponent() : nullptr;
	if (!SurvivalCharacter || !SurvivalInventoryComponent)
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected interaction with pickup %s because the interacting character has no inventory"), *GetName());
		return false;
	}
	FSurvivalItemInstance ItemInstance;
	ItemInstance.InstanceId = ItemInstanceId;
	ItemInstance.DefinitionAssetId = DefinitionAssetId;
	if (!SurvivalInventoryComponent->TryAddItem(ItemInstance, PickupItemDefinition))
	{
		UE_LOG(LogPickup, Log, TEXT("Inventory rejected pickup %s"), *GetName());
		return false;
	}

	const FString PickupName = GetName();
	UE_LOG(LogPickup, Log, TEXT("Pickup %s was interacted with by controller %s"), *PickupName, InteractingController ? *InteractingController->GetName() : TEXT("None"));

	const bool bDestroySucceeded = Destroy();
	if (bDestroySucceeded || IsActorBeingDestroyed())
	{
		UE_LOG(LogPickup, Log, TEXT("Server destroyed consumed pickup %s"), *PickupName);
	}
	else
	{
		FSurvivalItemInstance RolledBackItem;
		if (!SurvivalInventoryComponent->TryRemoveItem(ItemInstanceId, RolledBackItem))
		{
			UE_LOG(LogPickup, Error, TEXT("Pickup %s could not be destroyed and its inventory item could not be rolled back"), *PickupName);
			return false;
		}

		UE_LOG(LogPickup, Warning, TEXT("Server could not destroy consumed pickup %s"), *PickupName);
		return false;
	}
	return true;
}

bool APickup::TryConsumeItem(AController* InteractingController)
{
	const USurvivalConsumableItemDefinition* ConsumableItemDefinition = Cast<USurvivalConsumableItemDefinition>(GetItemDefinition());
	ASurvivalCharacter* SurvivalCharacter = InteractingController ? Cast<ASurvivalCharacter>(InteractingController->GetPawn()) : nullptr;
	if (!ConsumableItemDefinition || !SurvivalCharacter)
	{
		UE_LOG(LogPickup, Log, TEXT("Rejected consume action for pickup %s because it is not a consumable or the character is invalid"), *GetName());
		return false;
	}

	USurvivalVitalComponent* SurvivalVitalComponent = SurvivalCharacter->GetVitalComponent();
	if (!SurvivalVitalComponent)
	{
		return false;
	}

	const FSurvivalVitalSnapshot PreviousVitalState = SurvivalVitalComponent->CaptureSnapshot();
	if (!SurvivalCharacter->TryApplyConsumableEffects(ConsumableItemDefinition))
	{
		return false;
	}

	const FString PickupName = GetName();
	if (!Destroy())
	{
		SurvivalVitalComponent->RestoreSnapshot(PreviousVitalState);
		UE_LOG(LogPickup, Warning, TEXT("Rejected consume action because pickup %s could not be destroyed"), *PickupName);
		return false;
	}

	UE_LOG(LogPickup, Log, TEXT("Consumed %s directly from pickup for %s"), *ConsumableItemDefinition->GetName(), *SurvivalCharacter->GetName());
	return true;
}

FInteractionPromptData APickup::GetInteractionPrompt_Implementation(AController*) const
{
	FInteractionPromptData PromptData;
	PromptData.Style = EInteractionPromptStyle::Pickup;
	PromptData.State = EInteractionPromptState::Available;
	PromptData.ActionText = NSLOCTEXT("Survival", "PickupAction", "Pick up");
	const USurvivalItemDefinition* PickupItemDefinition = GetItemDefinition();
	if (PickupItemDefinition && !PickupItemDefinition->DisplayName.IsEmpty())
	{
		PromptData.TargetText = PickupItemDefinition->DisplayName;
	}
	else
	{
		PromptData.TargetText = ReplicatedDisplayName.IsEmpty() ? NSLOCTEXT("Survival", "DefaultPickupTarget", "Item") : ReplicatedDisplayName;
	}
	PromptData.DefaultActionId = ReplicatedDefaultActionId;
	PromptData.Options = ReplicatedInteractionOptions;
	if (IsValid(PickupItemDefinition))
	{
		PromptData.Options.Reset();
		PickupItemDefinition->GetWorldInteractionOptions(PromptData.Options);
		PromptData.DefaultActionId = PromptData.Options.Num() > 0 ? PromptData.Options[0].ActionId : NAME_None;
	}
	return PromptData;
}

void APickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APickup, ItemInstanceId);
	DOREPLIFETIME(APickup, DefinitionAssetId);
	DOREPLIFETIME(APickup, ReplicatedDisplayName);
	DOREPLIFETIME(APickup, ReplicatedPickupMesh);
	DOREPLIFETIME(APickup, ReplicatedInteractionOptions);
	DOREPLIFETIME(APickup, ReplicatedDefaultActionId);
}
