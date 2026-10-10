#include "Pickup.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Controller.h"

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
	DisplayName = NSLOCTEXT("Survival", "DefaultPickupDisplayName", "Item");

	bReplicates = true;
}

void APickup::Interact_Implementation(AController* InteractingController)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected interaction with pickup %s because it has no authority"), *GetName());
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
	PromptData.TargetText = DisplayName.IsEmpty() ? FText::FromString(GetName()) : DisplayName;
	PromptData.WorldOffset = PromptWorldOffset;
	return PromptData;
}
