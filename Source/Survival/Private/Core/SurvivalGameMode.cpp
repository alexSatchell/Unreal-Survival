#include "Core/SurvivalGameMode.h"
#include "Character/SurvivalCharacter.h"
#include "Core/SurvivalGameplayData.h"
#include "Core/SurvivalGameState.h"
#include "UI/SurvivalHud.h"
#include "Interaction/SurvivalPlayerController.h"
#include "Core/SurvivalPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalGameMode, Log, All);

ASurvivalGameMode::ASurvivalGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = ASurvivalCharacter::StaticClass();
	GameStateClass = ASurvivalGameState::StaticClass();
	HUDClass = ASurvivalHud::StaticClass();
	PlayerControllerClass = ASurvivalPlayerController::StaticClass();
	PlayerStateClass = ASurvivalPlayerState::StaticClass();
}

const USurvivalGameplayData* ASurvivalGameMode::GetGameplayData() const
{
	return GameplayData ? GameplayData : GetDefault<USurvivalGameplayData>();
}

void ASurvivalGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ASurvivalGameState* SurvivalGameState = GetGameState<ASurvivalGameState>())
	{
		const USurvivalGameplayData* SurvivalData = GetGameplayData();
		SurvivalGameState->InitializeWorldClock(SurvivalData->GameDayDurationSeconds, SurvivalData->DaylightDurationSeconds, SurvivalData->StartingWorldTimeSeconds);
		TransitionToRoundState(ESurvivalRoundState::InProgress);
	}
}

void ASurvivalGameMode::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Accumulate frame time so the world clock advances from elapsed game time rather than from frame count.
	WorldClockAccumulator += FMath::Max(DeltaSeconds, 0.0f);
	// Wait until at least one second has elapsed before updating the clock to avoid unnecessary per-frame work.
	if (WorldClockAccumulator < 1.0f)
	{
		return;
	}

	// Consume the accumulated time as one update and start measuring the next interval.
	const float ElapsedSeconds = WorldClockAccumulator;
	WorldClockAccumulator = 0.0f;
	if (ASurvivalGameState* SurvivalGameState = GetGameState<ASurvivalGameState>())
	{
		// GameState owns and replicates the authoritative world time to connected clients.
		SurvivalGameState->AdvanceWorldTime(ElapsedSeconds);
	}
}

void ASurvivalGameMode::TransitionToRoundState(const ESurvivalRoundState NewState)
{
	if (ASurvivalGameState* SurvivalGameState = GetGameState<ASurvivalGameState>())
	{
		SurvivalGameState->SetRoundState(NewState);
	}

}
