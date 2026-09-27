#include "SurvivalGameMode.h"
#include "SurvivalGameState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalGameMode, Log, All);

ASurvivalGameMode::ASurvivalGameMode()
{
	GameStateClass = ASurvivalGameState::StaticClass();
}

void ASurvivalGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (GetGameState<ASurvivalGameState>())
	{
		TransitionToRoundState(ESurvivalRoundState::Waiting);
	}
	else
	{
		UE_LOG(LogSurvivalGameMode, Error, TEXT("SurvivalGameState is unavailable; round state timers were not started"));
		return;
	}

	GetWorldTimerManager().SetTimer(RoundStateTimerHandle, FTimerDelegate::CreateUObject(this, &ASurvivalGameMode::TransitionToRoundState, ESurvivalRoundState::InProgress), 5.0f, false);
}

void ASurvivalGameMode::TransitionToRoundState(const ESurvivalRoundState NewState)
{
	if (ASurvivalGameState* SurvivalGameState = GetGameState<ASurvivalGameState>())
	{
		SurvivalGameState->SetRoundState(NewState);
	}
	else
	{
		UE_LOG(LogSurvivalGameMode, Error, TEXT("SurvivalGameState is unavailable; round state transition was not applied"));
		return;
	}

	if (NewState == ESurvivalRoundState::InProgress)
	{
		GetWorldTimerManager().SetTimer(RoundStateTimerHandle, FTimerDelegate::CreateUObject(this, &ASurvivalGameMode::TransitionToRoundState, ESurvivalRoundState::Finished), 60.0f, false);
	}
}
