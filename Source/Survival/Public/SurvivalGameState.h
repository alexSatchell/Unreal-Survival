#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Enums/SurvivalRoundState.h"
#include "SurvivalGameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSurvivalWorldTimeAdvanced, float);

UCLASS()
class SURVIVAL_API ASurvivalGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(ReplicatedUsing = OnRep_RoundState, BlueprintReadOnly, Category = "Round")
	ESurvivalRoundState RoundState = ESurvivalRoundState::Waiting;

	UPROPERTY(ReplicatedUsing = OnRep_WorldTime, BlueprintReadOnly, Category = "World Time", meta = (ClampMin = "0.0"))
	float WorldTimeSeconds = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "World Time", meta = (ClampMin = "1.0"))
	float GameDayDurationSeconds = 10800.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "World Time", meta = (ClampMin = "0.0"))
	float DaylightDurationSeconds = 9000.0f;

	UFUNCTION(BlueprintPure, Category = "World Time")
	float GetWorldTimeSeconds() const { return WorldTimeSeconds; }

	UFUNCTION(BlueprintPure, Category = "World Time")
	float GetWorldDayFraction() const;

	UFUNCTION(BlueprintPure, Category = "World Time")
	bool IsNight() const;

	FOnSurvivalWorldTimeAdvanced OnWorldTimeAdvanced;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	friend class ASurvivalGameMode;

	void SetRoundState(const ESurvivalRoundState NewState);
	void InitializeWorldClock(float InGameDayDurationSeconds, float InDaylightDurationSeconds, float InStartingWorldTimeSeconds);
	void AdvanceWorldTime(float InElapsedSeconds);

	UFUNCTION()
	void OnRep_RoundState(const ESurvivalRoundState PreviousRoundState) const;

	UFUNCTION()
	void OnRep_WorldTime() const;

	bool bHasInitializedRoundState = false;
};
