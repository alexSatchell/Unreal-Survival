#include "SurvivalPlayerController.h"
#include "InputCoreTypes.h"
#include "SurvivalPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalPlayerController, Log, All);

void ASurvivalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ASurvivalPlayerController::HandleReadyInput);
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
