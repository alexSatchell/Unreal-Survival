#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "SurvivalGameMode.generated.h"

UCLASS()
class SURVIVAL_API ASurvivalGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASurvivalGameMode();

	virtual void BeginPlay() override;

private:
	void BeginInProgressState();
	void BeginFinishedState();

	FTimerHandle RoundStateTimerHandle;
};
