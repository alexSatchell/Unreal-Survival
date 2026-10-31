#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "SurvivalLootManagerComponent.generated.h"

class ASurvivalGameState;
class ASurvivalLootContainer;
class ASurvivalLootSpawnPoint;

/** Manages loot population and recycling for the authoritative game world. */
UCLASS(ClassGroup = (Survival))
class SURVIVAL_API USurvivalLootManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalLootManagerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Registers an authored point for initial population. */
	void RegisterSpawnPoint(ASurvivalLootSpawnPoint* SpawnPoint);
	/** Cancels pending work for a point leaving the world. */
	void UnregisterSpawnPoint(ASurvivalLootSpawnPoint* SpawnPoint);
	/** Schedules an empty point after its configured real-time cooldown. */
	void ScheduleEmptySpawnPoint(ASurvivalLootSpawnPoint* SpawnPoint);
	/** Registers a searchable container for periodic refill. */
	void RegisterContainer(ASurvivalLootContainer* Container);
	/** Removes a container leaving the world. */
	void UnregisterContainer(ASurvivalLootContainer* Container);
	/** Returns whether the initially loaded points have been processed. */
	bool IsInitialPopulationComplete() const { return bInitialPopulationComplete; }

private:
	struct FPendingSpawn
	{
		TWeakObjectPtr<ASurvivalLootSpawnPoint> SpawnPoint;
		double Deadline = 0.0;
		uint64 Sequence = 0;
		bool operator<(const FPendingSpawn& Other) const { return Deadline < Other.Deadline; }
	};

	void FinishInitialRegistration();
	void BeginRecycleBatch();
	void ProcessSpawnPoint(ASurvivalLootSpawnPoint* SpawnPoint);
	void HandleWorldTimeAdvanced(float InElapsedSeconds);
	void EvaluateEmptyContainers();

	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "1.0", UIMin = "1.0", ToolTip = "Real seconds between checks for due spawn points. Due points may wait up to this interval before processing."))
	float RecycleBatchIntervalSeconds = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = "1", UIMin = "1", ToolTip = "Maximum initial and recycle population attempts processed per frame. Must be at least 1."))
	int32 MaxPopulationAttemptsPerFrame = 50;

	TSet<TWeakObjectPtr<ASurvivalLootSpawnPoint>> SpawnPoints;
	TSet<TWeakObjectPtr<ASurvivalLootContainer>> Containers;
	TArray<TWeakObjectPtr<ASurvivalLootSpawnPoint>> InitialQueue;
	TArray<FPendingSpawn> PendingSpawns;
	TMap<TWeakObjectPtr<ASurvivalLootSpawnPoint>, uint64> ScheduledSequences;
	TWeakObjectPtr<ASurvivalGameState> SurvivalGameState;
	FTimerHandle RecycleTimer;
	FRandomStream RandomStream;
	uint64 NextSequence = 0;
	int32 InitialQueueIndex = 0;
	float TimeSinceLastRecycle = 0.0f;
	double BatchCutoff = 0.0;
	bool bInitialRegistrationFinished = false;
	bool bInitialPopulationComplete = false;
	bool bProcessingRecycleBatch = false;
};
