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

	if (ASurvivalGameState* SurvivalGameState = GetGameState<ASurvivalGameState>())
	{
		SurvivalGameState->SetRoundState(ESurvivalRoundState::Waiting);
	}
	else
	{
		UE_LOG(LogSurvivalGameMode, Error, TEXT("SurvivalGameState is unavailable; round state timers were not started"));
		return;
	}

	GetWorldTimerManager().SetTimer(
		RoundStateTimerHandle,
		this,
		&ASurvivalGameMode::BeginInProgressState,
		5.0f,
		false);
}

void ASurvivalGameMode::BeginInProgressState()
{
	if (ASurvivalGameState* SurvivalGameState = GetGameState<ASurvivalGameState>())
	{
		SurvivalGameState->SetRoundState(ESurvivalRoundState::InProgress);
	}

	GetWorldTimerManager().SetTimer(
		RoundStateTimerHandle,
		this,
		&ASurvivalGameMode::BeginFinishedState,
		60.0f,
		false);
}

void ASurvivalGameMode::BeginFinishedState()
{
	if (ASurvivalGameState* SurvivalGameState = GetGameState<ASurvivalGameState>())
	{
		SurvivalGameState->SetRoundState(ESurvivalRoundState::Finished);
	}
}

