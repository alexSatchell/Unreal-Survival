#pragma once

#include "CoreMinimal.h"
#include "Enums/SurvivalRoundState.h"
#include "GameFramework/GameModeBase.h"
#include "SurvivalGameMode.generated.h"

class USurvivalGameplayData;

UCLASS()
class SURVIVAL_API ASurvivalGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASurvivalGameMode();

	const USurvivalGameplayData* GetGameplayData() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void TransitionToRoundState(const ESurvivalRoundState NewState);

	UPROPERTY(EditDefaultsOnly, Category = "Survival")
	TObjectPtr<USurvivalGameplayData> GameplayData;

	float WorldClockAccumulator = 0.0f;
};
