#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Enums/SurvivalRoundState.h"
#include "SurvivalGameState.generated.h"

UCLASS()
class SURVIVAL_API ASurvivalGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(ReplicatedUsing = OnRep_RoundState, BlueprintReadOnly, Category = "Round")
	ESurvivalRoundState RoundState = ESurvivalRoundState::Waiting;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	friend class ASurvivalGameMode;

	void SetRoundState(const ESurvivalRoundState NewState);

	UFUNCTION()
	void OnRep_RoundState(const ESurvivalRoundState PreviousRoundState);

	bool bHasInitializedRoundState = false;
};
