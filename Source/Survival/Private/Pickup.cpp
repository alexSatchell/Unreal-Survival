#include "Pickup.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "SurvivalItemDefinition.h"
#include "SurvivalCharacter.h"
#include "SurvivalInventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPickup, Log, All);

APickup::APickup()
{
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetBoxExtent(FVector(50.0f));
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
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

void APickup::ApplyItemDefinitionState(USurvivalItemDefinition* InItemDefinition)
{
	if (!IsValid(InItemDefinition))
	{
		return;
	}

	ItemDefinitionId = InItemDefinition->GetPrimaryAssetId();
	ReplicatedDisplayName = InItemDefinition->DisplayName;
	ReplicatedPickupMesh = InItemDefinition->PickupMesh;
	if (InItemDefinition->PickupMesh)
	{
		MeshComponent->SetStaticMesh(InItemDefinition->PickupMesh);
	}
}

void APickup::OnRep_ItemDefinitionId()
{
	MeshComponent->SetStaticMesh(ReplicatedPickupMesh);
}

void APickup::OnRep_PickupPresentation()
{
	MeshComponent->SetStaticMesh(ReplicatedPickupMesh);
}

bool APickup::InitializeItemInstance(const FSurvivalItemInstance& InItemInstance, USurvivalItemDefinition* InItemDefinition)
{
	if (!HasAuthority() || !InItemInstance.IsValid() || !IsValid(InItemDefinition) || InItemDefinition->GetPrimaryAssetId() != InItemInstance.ItemDefinitionId)
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
	if (!HasAuthority())
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected interaction with pickup %s because it has no authority"), *GetName());
		return;
	}

	if (!IsValid(ItemDefinition))
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected interaction with pickup %s because no item definition is assigned"), *GetName());
		return;
	}

	ASurvivalCharacter* Character = InteractingController ? Cast<ASurvivalCharacter>(InteractingController->GetPawn()) : nullptr;
	USurvivalInventoryComponent* CharacterInventory = Character ? Character->GetInventoryComponent() : nullptr;

	if (!Character || !CharacterInventory)
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected interaction with pickup %s because the interacting character has no inventory"), *GetName());
		return;
	}

	FSurvivalItemInstance ItemInstance;
	ItemInstance.InstanceId = ItemInstanceId;
	ItemInstance.ItemDefinitionId = ItemDefinitionId;
	if (!CharacterInventory->TryAddItem(ItemInstance, ItemDefinition))
	{
		UE_LOG(LogPickup, Log, TEXT("Inventory rejected pickup %s"), *GetName());
		return;
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
		if (!CharacterInventory->TryRemoveItem(ItemInstanceId, RolledBackItem))
		{
			UE_LOG(LogPickup, Error, TEXT("Pickup %s could not be destroyed and its inventory item could not be rolled back"), *PickupName);
			return;
		}

		UE_LOG(LogPickup, Warning, TEXT("Server could not destroy consumed pickup %s"), *PickupName);
	}
}

FInteractionPromptData APickup::GetInteractionPrompt_Implementation(AController*) const
{
	FInteractionPromptData PromptData;
	PromptData.Style = EInteractionPromptStyle::Pickup;
	PromptData.State = EInteractionPromptState::Available;
	PromptData.ActionText = NSLOCTEXT("Survival", "PickupAction", "Pick up");
	if (IsValid(ItemDefinition) && !ItemDefinition->DisplayName.IsEmpty())
	{
		PromptData.TargetText = ItemDefinition->DisplayName;
	}
	else
	{
		PromptData.TargetText = ReplicatedDisplayName.IsEmpty() ? NSLOCTEXT("Survival", "DefaultPickupTarget", "Item") : ReplicatedDisplayName;
	}
	PromptData.WorldOffset = PromptWorldOffset;
	return PromptData;
}

void APickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APickup, ItemInstanceId);
	DOREPLIFETIME(APickup, ItemDefinitionId);
	DOREPLIFETIME(APickup, ReplicatedDisplayName);
	DOREPLIFETIME(APickup, ReplicatedPickupMesh);
}
