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
	if (IsValid(ItemDefinition))
	{
		ItemDefinitionId = ItemDefinition->GetPrimaryAssetId();
		ReplicatedDisplayName = ItemDefinition->DisplayName;
		ReplicatedPickupMesh = ItemDefinition->PickupMesh;
	}

	ApplyItemPresentation(ItemDefinition);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		return;
	}

	if (IsValid(ItemDefinition))
	{
		ItemDefinitionId = ItemDefinition->GetPrimaryAssetId();
		ReplicatedDisplayName = ItemDefinition->DisplayName;
		ReplicatedPickupMesh = ItemDefinition->PickupMesh;
	}

	if (!ItemInstanceId.IsValid())
	{
		ItemInstanceId = FGuid::NewGuid();
	}

	ApplyItemPresentation(GetItemDefinition());
}

USurvivalItemDefinition* APickup::GetItemDefinition() const
{
	if (IsValid(ItemDefinition))
	{
		return ItemDefinition;
	}

	return nullptr;
}

void APickup::ApplyItemPresentation(USurvivalItemDefinition* InItemDefinition)
{
	if (IsValid(InItemDefinition) && InItemDefinition->PickupMesh)
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
	ItemDefinitionId = InItemInstance.ItemDefinitionId;
	ItemDefinition = InItemDefinition;
	ReplicatedDisplayName = InItemDefinition->DisplayName;
	ReplicatedPickupMesh = InItemDefinition->PickupMesh;
	ApplyItemPresentation(InItemDefinition);
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

	USurvivalItemDefinition* PickupItemDefinition = GetItemDefinition();
	if (!IsValid(PickupItemDefinition))
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected interaction with pickup %s because no item definition is assigned"), *GetName());
		return;
	}

	const ASurvivalCharacter* SurvivalCharacter = InteractingController ? Cast<ASurvivalCharacter>(InteractingController->GetPawn()) : nullptr;
	USurvivalInventoryComponent* SurvivalInventoryComponent = SurvivalCharacter ? SurvivalCharacter->GetInventoryComponent() : nullptr;
	if (!SurvivalCharacter || !SurvivalInventoryComponent)
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected interaction with pickup %s because the interacting character has no inventory"), *GetName());
		return;
	}
	FSurvivalItemInstance ItemInstance;
	ItemInstance.InstanceId = ItemInstanceId;
	ItemInstance.ItemDefinitionId = ItemDefinitionId;
	if (!SurvivalInventoryComponent->TryAddItem(ItemInstance, PickupItemDefinition))
	{
		UE_LOG(LogPickup, Log, TEXT("Inventory rejected pickup %s"), *GetName());
		return;
	}

	const FString PickupName = GetName();
	UE_LOG(LogPickup, Log, TEXT("Pickup %s was interacted with by controller %s"), *PickupName, InteractingController ? *InteractingController->GetName() : TEXT("None"));

	if (Destroy())
	{
		UE_LOG(LogPickup, Log, TEXT("Server destroyed consumed pickup %s"), *PickupName);
	}
	else
	{
		UE_LOG(LogPickup, Warning, TEXT("Server could not destroy consumed pickup %s"), *PickupName);
	}
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
