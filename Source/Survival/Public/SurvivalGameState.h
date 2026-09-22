#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SurvivalGameState.generated.h"

UENUM(BlueprintType)
enum class ESurvivalRoundState : uint8
{
	Waiting UMETA(DisplayName = "Waiting"),
	InProgress UMETA(DisplayName = "In Progress"),
	Finished UMETA(DisplayName = "Finished")
};

UCLASS()
class SURVIVAL_API ASurvivalGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(ReplicatedUsing = OnRep_RoundState, BlueprintReadOnly, Category = "Round")
	ESurvivalRoundState RoundState = ESurvivalRoundState::Waiting;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	friend class ASurvivalGameMode;

	void SetRoundState(ESurvivalRoundState NewState);

	UFUNCTION()
	void OnRep_RoundState(ESurvivalRoundState PreviousRoundState);

	bool bHasInitializedRoundState = false;
};
