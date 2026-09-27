#include "SurvivalGameState.h"
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

void ASurvivalGameState::OnRep_RoundState(const ESurvivalRoundState PreviousRoundState)
{
	const FString PreviousState = UE::Survival::Private::RoundStateToString(PreviousRoundState);
	const FString CurrentState = UE::Survival::Private::RoundStateToString(RoundState);
	UE_LOG(LogSurvivalGameState, Log, TEXT("Client received round state change: %s -> %s"), *PreviousState, *CurrentState);
}
