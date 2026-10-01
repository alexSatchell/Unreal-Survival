#include "SurvivalPlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalPlayerState, Log, All);

void ASurvivalPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ASurvivalPlayerState, bIsAlive);
}

void ASurvivalPlayerState::AddPoint()
{
	if (!HasAuthority())
	{
		UE_LOG(LogSurvivalPlayerState, Warning, TEXT("Rejected AddPoint because this PlayerState has no authority"));
		return;
	}
	
	const float PreviousScore = GetScore();
	SetScore(PreviousScore + 1.0f);

	UE_LOG(LogSurvivalPlayerState, Log, TEXT("Server changed score for %s: %.0f -> %.0f"), *GetPlayerName(), PreviousScore, GetScore());
}

void ASurvivalPlayerState::SetIsAlive(const bool bNewIsAlive)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSurvivalPlayerState, Warning, TEXT("Rejected IsAlive change because this PlayerState has no authority"));
		return;
	}

	if (bIsAlive == bNewIsAlive)
	{
		return;
	}

	const bool bPreviousIsAlive = bIsAlive;
	bIsAlive = bNewIsAlive;

	UE_LOG(LogSurvivalPlayerState, Log, TEXT("Server changed IsAlive for %s: %s -> %s"), *GetPlayerName(), bPreviousIsAlive ? TEXT("true") : TEXT("false"), bIsAlive ? TEXT("true") : TEXT("false"));
}

bool ASurvivalPlayerState::GetIsAlive() const
{
	return bIsAlive;
}

void ASurvivalPlayerState::OnRep_IsAlive(const bool bPreviousIsAlive) const
{
	const TCHAR* PreviousState = bPreviousIsAlive ? TEXT("true") : TEXT("false");
	const TCHAR* CurrentState = bIsAlive ? TEXT("true") : TEXT("false");
	UE_LOG(LogSurvivalPlayerState, Log, TEXT("Client received replicated IsAlive change for %s: %s -> %s"), *GetPlayerName(), PreviousState, CurrentState);
}
