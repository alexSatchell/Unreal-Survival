#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivalVitalComponent.generated.h"

/** Owns body hydration state so the character can apply physical effects without placing survival state in PlayerState. */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class SURVIVAL_API USurvivalVitalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalVitalComponent();

	UFUNCTION(BlueprintCallable, Category = "Vitals|Hydration")
	bool TryRestoreHydration(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Vitals|Hydration")
	void SetCurrentHydration(float NewHydration);

	UFUNCTION(BlueprintPure, Category = "Vitals|Hydration")
	float GetHydrationPercent() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Vitals|Hydration", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHydration = 100.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Vitals|Hydration", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CurrentHydration = 50.0f;

protected:
	virtual void BeginPlay() override;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
