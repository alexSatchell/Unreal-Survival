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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Time", meta = (ClampMin = "1.0", UIMin = "1.0", ToolTip = "Length of one complete in-game day, measured in real seconds."))
	float GameDayDurationSeconds = 10800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Time", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "How many seconds of the game day use the daylight portion of the world clock."))
	float DaylightDurationSeconds = 9000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Time", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "World-clock time at which a new game starts, measured in seconds from the beginning of the day."))
	float StartingWorldTimeSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1.0", UIMin = "1.0", ToolTip = "In-game seconds between evaluations of searched, empty loot containers. Minimum 1; unsearched or nonempty containers are unchanged."))
	float LootRecycleIntervalGameSeconds = 7200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", ToolTip = "Lowest random hunger value assigned when a player spawns."))
	float StartingHungerMinimum = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", ToolTip = "Highest random hunger value assigned when a player spawns."))
	float StartingHungerMaximum = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", ToolTip = "Lowest random hydration value assigned when a player spawns."))
	float StartingHydrationMinimum = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", ToolTip = "Highest random hydration value assigned when a player spawns."))
	float StartingHydrationMaximum = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Hunger points removed over one complete in-game day before activity modifiers."))
	float HungerDrainPerGameDay = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Hydration points removed over one complete in-game day before activity modifiers."))
	float HydrationDrainPerGameDay = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Multiplier applied to vital drain while walking."))
	float WalkingActivityMultiplier = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Multiplier applied to vital drain while running."))
	float RunningActivityMultiplier = 1.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Decay", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Immediate hunger and hydration cost applied by a discrete activity event."))
	float OneTimeActivityCost = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Health", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Health damage applied over one complete in-game day while a hunger or hydration vital is empty."))
	float EmptyVitalHealthDamagePerGameDay = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Health", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Health damage applied over one complete in-game day while the player is sick."))
	float SicknessHealthDamagePerGameDay = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Health", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Additional health damage applied when an activity drain event occurs while a vital is empty."))
	float EmptyVitalActivityHealthDamage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vitals|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Movement-speed multiplier used when hunger or hydration reaches zero."))
	float ExhaustedMovementSpeedMultiplier = 0.5f;
};
