#include "Pickup.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogPickup, Log, All);

APickup::APickup()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
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

void APickup::OnRep_IsAvailable(const bool bPreviousIsAvailable) const
{
	UE_LOG(LogPickup, Log, TEXT("Client received replicated IsAvailable change for %s: %s -> %s"), *GetName(), bPreviousIsAvailable ? TEXT("true") : TEXT("false"),
		bIsAvailable ? TEXT("true") : TEXT("false"));
}
