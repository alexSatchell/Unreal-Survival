#include "SurvivalHud.h"
#include "GameFramework/PlayerController.h"
#include "SurvivalPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalHud, Log, All);

void ASurvivalHud::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogSurvivalHud, Log, TEXT("HUD %s began play for owning player controller %s"), *GetName(), GetOwningPlayerController() ? *GetOwningPlayerController()->GetName() : TEXT("None"));
	// LogLocalPlayerState();
}

void ASurvivalHud::LogLocalPlayerState() const
{
	const APlayerController* OwningPlayerController = GetOwningPlayerController();
	if (!OwningPlayerController)
	{
		UE_LOG(LogSurvivalHud, Warning, TEXT("Could not log local PlayerState because the owning PlayerController is unavailable"));
		return;
	}

	const ASurvivalPlayerState* SurvivalPlayerState = OwningPlayerController->GetPlayerState<ASurvivalPlayerState>();
	if (!SurvivalPlayerState)
	{
		UE_LOG(LogSurvivalHud, Warning, TEXT("Could not log local PlayerState because ASurvivalPlayerState is unavailable"));
		return;
	}

	const TCHAR* AliveState = SurvivalPlayerState->GetIsAlive() ? TEXT("true") : TEXT("false");
	UE_LOG(LogSurvivalHud, Log, TEXT("Local PlayerState: PlayerName=%s Score=%.0f IsAlive=%s"), *SurvivalPlayerState->GetPlayerName(), SurvivalPlayerState->GetScore(), AliveState);
}
