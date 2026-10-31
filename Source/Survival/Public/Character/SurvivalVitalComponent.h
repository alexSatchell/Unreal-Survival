#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SurvivalVitalComponent.generated.h"

USTRUCT(BlueprintType)
struct SURVIVAL_API FSurvivalIllnessState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Sickness")
	FGameplayTag IllnessTag;

	UPROPERTY(BlueprintReadOnly, Category = "Sickness", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Severity = 0.0f;

	/** A value of zero means this illness persists until a matching cure is applied. */
	UPROPERTY(BlueprintReadOnly, Category = "Sickness", meta = (ClampMin = "0.0"))
	float RemainingDurationGameSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct SURVIVAL_API FSurvivalSicknessRecoveryState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Sickness|Recovery")
	FGameplayTagContainer RecoveryTags;

	UPROPERTY(BlueprintReadOnly, Category = "Sickness|Recovery", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float RemainingRecoveryPoints = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Sickness|Recovery", meta = (ClampMin = "0.0"))
	float RecoveryRatePointsPerGameSecond = 0.0f;
};

USTRUCT(BlueprintType)
struct SURVIVAL_API FSurvivalVitalSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Vitals")
	float Health = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vitals")
	float Hunger = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vitals")
	float Hydration = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vitals")
	TArray<FSurvivalIllnessState> ActiveIllnesses;

	UPROPERTY(BlueprintReadOnly, Category = "Vitals|Sickness")
	TArray<FSurvivalSicknessRecoveryState> ActiveSicknessRecoveries;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSurvivalVitalsChanged, float, HealthPercent, float, HungerPercent, float, HydrationPercent, float, SicknessPercent);

/** Owns replicated character vitals and applies server-authoritative survival effects over world time. */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class SURVIVAL_API USurvivalVitalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalVitalComponent();

	UFUNCTION(BlueprintCallable, Category = "Vitals|Health")
	bool TryApplyHealthDelta(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Vitals|Health")
	void SetCurrentHealth(float NewHealth);

	UFUNCTION(BlueprintPure, Category = "Vitals|Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Vitals|Hunger")
	bool TryRestoreHunger(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Vitals|Hunger")
	void SetCurrentHunger(float NewHunger);

	UFUNCTION(BlueprintPure, Category = "Vitals|Hunger")
	float GetHungerPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Vitals|Hydration")
	bool TryRestoreHydration(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Vitals|Hydration")
	void SetCurrentHydration(float NewHydration);

	UFUNCTION(BlueprintPure, Category = "Vitals|Hydration")
	float GetHydrationPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Vitals|Sickness")
	bool TryApplyIllness(FGameplayTag IllnessTag, float Severity, float DurationGameSeconds = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Vitals|Sickness")
	bool TryCureIllnesses(const FGameplayTagContainer& CureTags);

	UFUNCTION(BlueprintCallable, Category = "Vitals|Sickness")
	bool TryApplySicknessRecovery(const FGameplayTagContainer& RecoveryTags, float RecoveryPoints, float DurationGameSeconds);

	UFUNCTION(BlueprintPure, Category = "Vitals|Sickness")
	float GetSicknessPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Vitals|Activity")
	void NotifyJumpActivity();

	UFUNCTION(BlueprintCallable, Category = "Vitals|Activity")
	void NotifyPunchActivity();

	UFUNCTION(BlueprintPure, Category = "Vitals")
	FSurvivalVitalSnapshot CaptureSnapshot() const;

	UFUNCTION(BlueprintCallable, Category = "Vitals")
	void RestoreSnapshot(const FSurvivalVitalSnapshot& Snapshot);

	UPROPERTY(BlueprintAssignable, Category = "Vitals")
	FOnSurvivalVitalsChanged OnVitalsChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Vitals|Health", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHealth, Category = "Vitals|Health", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CurrentHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Vitals|Hunger", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHunger = 100.0f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHunger, Category = "Vitals|Hunger", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CurrentHunger = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Vitals|Hydration", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHydration = 100.0f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHydration, Category = "Vitals|Hydration", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CurrentHydration = 50.0f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ActiveIllnesses, Category = "Vitals|Sickness")
	TArray<FSurvivalIllnessState> ActiveIllnesses;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeStartingVitals();
	void ApplyWorldTimeDelta(float InElapsedGameSeconds);
	bool ApplySicknessRecovery(float InElapsedGameSeconds);
	void ApplyActivityCost(float InCost);
	void BroadcastVitalsChanged();
	void ApplyMovementPenalty();
	void HandleWorldTimeAdvanced(float InElapsedGameSeconds);
	void HandleDeath();
	float GetActivityMultiplier() const;
	float GetSicknessSeverityTotal() const;
	bool HasMatchingIllness(const FGameplayTagContainer& RecoveryTags) const;
	const class USurvivalGameplayData* GetGameplayData() const;

	TArray<FSurvivalSicknessRecoveryState> ActiveSicknessRecoveries;

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void OnRep_CurrentHunger();

	UFUNCTION()
	void OnRep_CurrentHydration();

	UFUNCTION()
	void OnRep_ActiveIllnesses();

	float NormalMovementSpeed = 0.0f;
	bool bHasHandledDeath = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
