#include "SurvivalPlayerController.h"
#include "Door.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Pickup.h"
#include "SurvivalPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalPlayerController, Log, All);

void ASurvivalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ASurvivalPlayerController::HandleReadyInput);
		InputComponent->BindKey(EKeys::I, IE_Pressed, this, &ASurvivalPlayerController::HandleInteractInput);
	}
}

void ASurvivalPlayerController::HandleReadyInput()
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Local player pressed R to become ready"));
	ServerSetReady();
}

void ASurvivalPlayerController::HandleInteractInput()
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Local player pressed I to interact"));
	ServerInteract();
}

void ASurvivalPlayerController::ServerInteract_Implementation()
{
	if (!HasAuthority())
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction because controller %s has no authority"), *GetName());
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction because the world is unavailable"));
		return;
	}

	for (TActorIterator<APickup> PickupIterator(World); PickupIterator; ++PickupIterator)
	{
		if (APickup* Pickup = *PickupIterator)
		{
			Pickup->TryPickup();
			return;
		}
	}

	for (TActorIterator<ADoor> DoorIterator(World); DoorIterator; ++DoorIterator)
	{
		if (ADoor* Door = *DoorIterator)
		{
			Door->SetIsOpen(!Door->bIsOpen);
			return;
		}
	}

	UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Interaction found no Door instance in the world"));
}

void ASurvivalPlayerController::ServerSetReady_Implementation()
{
	ASurvivalPlayerState* SurvivalPlayerState = GetPlayerState<ASurvivalPlayerState>();
	if (!SurvivalPlayerState)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Could not set player ready because SurvivalPlayerState is unavailable"));
		return;
	}

	SurvivalPlayerState->SetIsReady(true);
}
