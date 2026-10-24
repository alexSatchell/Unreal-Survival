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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HydrationRestoration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HungerRestoration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (UIMin = "-100.0"))
	float HealthEffect = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness")
	FGameplayTag SicknessTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SicknessSeverity = 0.0f;

	/** A value of zero makes the sickness persistent until a matching cure is consumed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SicknessDurationGameSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable|Sickness")
	FGameplayTagContainer SicknessCureTags;

	virtual void GetWorldInteractionOptions(TArray<FInteractionOptionData>& OutOptions) const override;
};
