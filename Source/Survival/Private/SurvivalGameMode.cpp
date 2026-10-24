#include "SurvivalGameMode.h"
#include "SurvivalCharacter.h"
#include "SurvivalGameplayData.h"
#include "SurvivalGameState.h"
#include "SurvivalHud.h"
#include "SurvivalPlayerController.h"
#include "SurvivalPlayerState.h"

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

	WorldClockAccumulator += FMath::Max(DeltaSeconds, 0.0f);
	if (WorldClockAccumulator < 1.0f)
	{
		return;
	}

	const float ElapsedSeconds = WorldClockAccumulator;
	WorldClockAccumulator = 0.0f;
	if (ASurvivalGameState* SurvivalGameState = GetGameState<ASurvivalGameState>())
	{
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
