#pragma once

#include "CoreMinimal.h"
#include "Enums/SurvivalRoundState.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "SurvivalGameMode.generated.h"

UCLASS()
class SURVIVAL_API ASurvivalGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASurvivalGameMode();

protected:
	virtual void BeginPlay() override;

private:
	void TransitionToRoundState(const ESurvivalRoundState NewState);

	FTimerHandle RoundStateTimerHandle;
};
