#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SurvivalItemDefinition.h"
#include "SurvivalConsumableItemDefinition.generated.h"

/** Defines item data for an item that can be consumed and apply effects to the character's vital state. */
UCLASS(BlueprintType)
class SURVIVAL_API USurvivalConsumableItemDefinition : public USurvivalItemDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Hydration points restored when this item is consumed."))
	float HydrationRestoration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Hunger points restored when this item is consumed."))
	float HungerRestoration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (UIMin = "-100.0", ToolTip = "Immediate health change when consumed. Positive values heal; negative values cause damage; zero has no effect."))
	float HealthEffect = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness", meta = (ToolTip = "Gameplay tag identifying the sickness applied by this item. Leave None when the item does not cause sickness."))
	FGameplayTag SicknessTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", ToolTip = "Sickness severity points applied when consumed, from zero to one hundred. It is used only when Sickness Tag is valid."))
	float SicknessSeverity = 0.0f;

	/** A value of zero makes the sickness persistent until a matching cure is consumed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "How long the sickness lasts in in-game seconds. Zero means it persists until cured."))
	float SicknessDurationGameSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness|Recovery", meta = (ToolTip = "Sickness tags this item gradually reduces over time."))
	FGameplayTagContainer SicknessRecoveryTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness|Recovery", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", ToolTip = "Total sickness points this item gradually recovers. This is separate from Health Effect."))
	float SicknessRecoveryPoints = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness|Recovery", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Game-time seconds over which sickness recovery is applied. Zero disables gradual recovery."))
	float SicknessRecoveryDurationGameSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness", meta = (ToolTip = "Sickness tags removed immediately when this item is consumed."))
	FGameplayTagContainer SicknessCureTags;

	virtual void GetWorldInteractionOptions(TArray<FInteractionOptionData>& OutOptions) const override;
};
