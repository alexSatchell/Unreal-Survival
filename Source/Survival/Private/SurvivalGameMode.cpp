#include "SurvivalGameMode.h"
#include "SurvivalCharacter.h"
#include "SurvivalGameState.h"
#include "SurvivalHud.h"
#include "SurvivalPlayerController.h"
#include "SurvivalPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalGameMode, Log, All);

ASurvivalGameMode::ASurvivalGameMode()
{
	DefaultPawnClass = ASurvivalCharacter::StaticClass();
	GameStateClass = ASurvivalGameState::StaticClass();
	HUDClass = ASurvivalHud::StaticClass();
	PlayerControllerClass = ASurvivalPlayerController::StaticClass();
	PlayerStateClass = ASurvivalPlayerState::StaticClass();
}

void ASurvivalGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GetGameState<ASurvivalGameState>())
	{
		TransitionToRoundState(ESurvivalRoundState::Waiting);
	}

	// after 5 seconds set the match to inProgress state
	GetWorldTimerManager().SetTimer(RoundStateTimerHandle, FTimerDelegate::CreateUObject(this, &ASurvivalGameMode::TransitionToRoundState, ESurvivalRoundState::InProgress), 5.0f, false);
}

void ASurvivalGameMode::TransitionToRoundState(const ESurvivalRoundState NewState)
{
	if (ASurvivalGameState* SurvivalGameState = GetGameState<ASurvivalGameState>())
	{
		SurvivalGameState->SetRoundState(NewState);
	}

	if (NewState == ESurvivalRoundState::InProgress)
	{
		// if match is in progress, set the timer to transition to finished state after 60 seconds
		GetWorldTimerManager().SetTimer(RoundStateTimerHandle, FTimerDelegate::CreateUObject(this, &ASurvivalGameMode::TransitionToRoundState, ESurvivalRoundState::Finished), 60.0f, false);
	}
}
