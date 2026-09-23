#include "Door.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoor, Log, All);

ADoor::ADoor()
{
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetBoxExtent(FVector(50.0f));
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = CollisionComponent;
	bReplicates = true;
}

void ADoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADoor, bIsOpen);
}

void ADoor::SetIsOpen(const bool bNewIsOpen)
{
	if (!HasAuthority())
	{
		UE_LOG(LogDoor, Warning, TEXT("Rejected IsOpen change for %s because this Door has no authority"), *GetName());
		return;
	}

	if (bIsOpen == bNewIsOpen)
	{
		return;
	}

	const bool bPreviousIsOpen = bIsOpen;
	bIsOpen = bNewIsOpen;

	UE_LOG(LogDoor, Log, TEXT("Server changed IsOpen for %s: %s -> %s"), *GetName(), bPreviousIsOpen ? TEXT("true") : TEXT("false"), bIsOpen ? TEXT("true") : TEXT("false"));
}

void ADoor::Interact_Implementation(AController* InteractingController)
{
	UE_LOG(LogDoor, Log, TEXT("Door %s was interacted with by controller %s"), *GetName(), InteractingController ? *InteractingController->GetName() : TEXT("None"));
	SetIsOpen(!bIsOpen);
}

void ADoor::OnRep_IsOpen(const bool bPreviousIsOpen) const
{
	UE_LOG(LogDoor, Log, TEXT("Client received replicated IsOpen change for %s: %s -> %s"), *GetName(), bPreviousIsOpen ? TEXT("true") : TEXT("false"), bIsOpen ? TEXT("true") : TEXT("false"));
}
