#include "Pickup.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

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

void APickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APickup, bIsAvailable);
}

bool APickup::TryPickup()
{
	if (!HasAuthority())
	{
		UE_LOG(LogPickup, Warning, TEXT("Rejected pickup attempt for %s because this Pickup has no authority"), *GetName());
		return false;
	}

	if (!bIsAvailable)
	{
		UE_LOG(LogPickup, Log, TEXT("Rejected pickup attempt for %s because it is unavailable"), *GetName());
		return false;
	}

	bIsAvailable = false;
	UE_LOG(LogPickup, Log, TEXT("Server accepted pickup for %s and changed IsAvailable to false"), *GetName());
	return true;
}

void APickup::Interact_Implementation(AController* InteractingController)
{
	if (TryPickup())
	{
		UE_LOG(LogPickup, Log, TEXT("Pickup %s was interacted with by controller %s"), *GetName(), InteractingController ? *InteractingController->GetName() : TEXT("None"));
	}
}

void APickup::OnRep_IsAvailable(const bool bPreviousIsAvailable) const
{
	UE_LOG(LogPickup, Log, TEXT("Client received replicated IsAvailable change for %s: %s -> %s"), *GetName(), bPreviousIsAvailable ? TEXT("true") : TEXT("false"),
		bIsAvailable ? TEXT("true") : TEXT("false"));
}
