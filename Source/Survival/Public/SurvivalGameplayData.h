#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SurvivalGameplayData.generated.h"

/** Central tuning asset for the persistent survival world and character vital systems. */
UCLASS(BlueprintType)
class SURVIVAL_API USurvivalGameplayData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Time", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float GameDayDurationSeconds = 10800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Time", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DaylightDurationSeconds = 9000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Time", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StartingWorldTimeSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float StartingHungerMinimum = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float StartingHungerMaximum = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float StartingHydrationMinimum = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float StartingHydrationMaximum = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HungerDrainPerGameDay = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HydrationDrainPerGameDay = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkingActivityMultiplier = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RunningActivityMultiplier = 1.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OneTimeActivityCost = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Health", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EmptyVitalHealthDamagePerGameDay = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Health", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SicknessHealthDamagePerGameDay = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Health", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EmptyVitalActivityHealthDamage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float ExhaustedMovementSpeedMultiplier = 0.5f;
};
