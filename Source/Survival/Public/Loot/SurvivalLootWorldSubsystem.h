#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SurvivalLootWorldSubsystem.generated.h"

class ASurvivalLootSpawnPoint;
class ASurvivalLootContainer;
class ASurvivalGameState;

UCLASS()
class SURVIVAL_API USurvivalLootWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;
	void RegisterSpawnPoint(ASurvivalLootSpawnPoint* InSpawnPoint);
	void UnregisterSpawnPoint(ASurvivalLootSpawnPoint* InSpawnPoint);
	void RegisterContainer(ASurvivalLootContainer* InContainer);
	void UnregisterContainer(ASurvivalLootContainer* InContainer);

private:
	void HandleWorldTimeAdvanced(float InElapsedSeconds);
	void EvaluateDueSpawnPoints();
	void EvaluateEmptyContainers();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASurvivalLootSpawnPoint>> SpawnPoints;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASurvivalLootContainer>> Containers;

	TWeakObjectPtr<ASurvivalGameState> SurvivalGameState;
	TMap<TObjectPtr<ASurvivalLootSpawnPoint>, double> NextSpawnPointRecycleTimes;
	FRandomStream RandomStream;
	float TimeSinceLastRecycle = 0.0f;
};
