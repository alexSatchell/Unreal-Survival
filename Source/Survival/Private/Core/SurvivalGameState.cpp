#include "Core/SurvivalGameState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalGameState, Log, All);

namespace UE::Survival::Private
{
	FString RoundStateToString(const ESurvivalRoundState State)
	{
		const UEnum* RoundStateEnum = StaticEnum<ESurvivalRoundState>();
		return RoundStateEnum ? RoundStateEnum->GetNameStringByValue(static_cast<int64>(State)) : TEXT("Unknown");
	}
}

void ASurvivalGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASurvivalGameState, RoundState);
	DOREPLIFETIME(ASurvivalGameState, WorldTimeSeconds);
	DOREPLIFETIME(ASurvivalGameState, GameDayDurationSeconds);
	DOREPLIFETIME(ASurvivalGameState, DaylightDurationSeconds);
}

float ASurvivalGameState::GetWorldDayFraction() const
{
	// Convert the current position within the repeating game day to a normalized 0-to-1 value for clocks, lighting, or other world-time visuals.
	return GameDayDurationSeconds > 0.0f ? WorldTimeSeconds / GameDayDurationSeconds : 0.0f;
}

bool ASurvivalGameState::IsNight() const
{
	// WorldTimeSeconds wraps at GameDayDurationSeconds, so any time after daylight ends is within the night's portion of the current game day.
	return WorldTimeSeconds >= DaylightDurationSeconds;
}

void ASurvivalGameState::SetRoundState(const ESurvivalRoundState NewState)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSurvivalGameState, Warning, TEXT("Rejected round state change to %s because this GameState has no authority"), *UE::Survival::Private::RoundStateToString(NewState));
		return;
	}

	if (bHasInitializedRoundState && RoundState == NewState)
	{
		return;
	}

	const FString PreviousState = bHasInitializedRoundState ? UE::Survival::Private::RoundStateToString(RoundState) : TEXT("Uninitialized");
	const FString NewStateName = UE::Survival::Private::RoundStateToString(NewState);

	RoundState = NewState;
	bHasInitializedRoundState = true;

	UE_LOG(LogSurvivalGameState, Log, TEXT("Server round state changed: %s -> %s"), *PreviousState, *NewStateName);
}

void ASurvivalGameState::InitializeWorldClock(const float InGameDayDurationSeconds, const float InDaylightDurationSeconds, const float InStartingWorldTimeSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	GameDayDurationSeconds = FMath::Max(InGameDayDurationSeconds, 1.0f);
	DaylightDurationSeconds = FMath::Clamp(InDaylightDurationSeconds, 0.0f, GameDayDurationSeconds);
	WorldTimeSeconds = FMath::Fmod(FMath::Max(InStartingWorldTimeSeconds, 0.0f), GameDayDurationSeconds);
	ForceNetUpdate();
}

void ASurvivalGameState::AdvanceWorldTime(const float InElapsedSeconds)
{
	if (!HasAuthority() || InElapsedSeconds <= 0.0f || GameDayDurationSeconds <= 0.0f)
	{
		return;
	}

	WorldTimeSeconds = FMath::Fmod(WorldTimeSeconds + InElapsedSeconds, GameDayDurationSeconds);
	// Notify server-side systems, such as vital decay, that the authoritative game clock has advanced by this amount.
	OnWorldTimeAdvanced.Broadcast(InElapsedSeconds);
}

void ASurvivalGameState::OnRep_RoundState(const ESurvivalRoundState PreviousRoundState) const
{
	const FString PreviousState = UE::Survival::Private::RoundStateToString(PreviousRoundState);
	const FString CurrentState = UE::Survival::Private::RoundStateToString(RoundState);
	UE_LOG(LogSurvivalGameState, Log, TEXT("Client received round state change: %s -> %s"), *PreviousState, *CurrentState);
}

void ASurvivalGameState::OnRep_WorldTime() const
{
	UE_LOG(LogSurvivalGameState, VeryVerbose, TEXT("Client received world time %.1f seconds"), WorldTimeSeconds);
}
