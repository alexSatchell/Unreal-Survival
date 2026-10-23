#include "Loot/SurvivalLootWorldSubsystem.h"
#include "Core/SurvivalGameState.h"
#include "Core/SurvivalGameMode.h"
#include "Core/SurvivalGameplayData.h"
#include "Loot/SurvivalLootSpawnPoint.h"
#include "Loot/SurvivalLootContainer.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Stats/Stats.h"

void USurvivalLootWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	RandomStream.Initialize(FMath::Rand());
	if (InWorld.GetAuthGameMode<ASurvivalGameMode>())
	{
		SurvivalGameState = InWorld.GetGameState<ASurvivalGameState>();
		if (SurvivalGameState.IsValid())
		{
			SurvivalGameState->OnWorldTimeAdvanced.AddUObject(this, &USurvivalLootWorldSubsystem::HandleWorldTimeAdvanced);
		}
	}
}

void USurvivalLootWorldSubsystem::Deinitialize()
{
	if (SurvivalGameState.IsValid())
	{
		SurvivalGameState->OnWorldTimeAdvanced.RemoveAll(this);
	}
	SpawnPoints.Reset();
	NextSpawnPointRecycleTimes.Reset();
	Containers.Reset();
	Super::Deinitialize();
}

void USurvivalLootWorldSubsystem::RegisterSpawnPoint(ASurvivalLootSpawnPoint* InSpawnPoint)
{
	if (!InSpawnPoint || !InSpawnPoint->HasAuthority() || SpawnPoints.Contains(InSpawnPoint))
	{
		return;
	}

	SpawnPoints.Add(InSpawnPoint);
	InSpawnPoint->TryPopulate(RandomStream);
	if (InSpawnPoint->IsRecyclingEnabled())
	{
		NextSpawnPointRecycleTimes.Add(InSpawnPoint, FPlatformTime::Seconds() + InSpawnPoint->GetRecycleIntervalRealSeconds());
	}
}

void USurvivalLootWorldSubsystem::UnregisterSpawnPoint(ASurvivalLootSpawnPoint* InSpawnPoint)
{
	SpawnPoints.Remove(InSpawnPoint);
	NextSpawnPointRecycleTimes.Remove(InSpawnPoint);
}

void USurvivalLootWorldSubsystem::RegisterContainer(ASurvivalLootContainer* InContainer)
{
	if (!InContainer || !InContainer->HasAuthority() || Containers.Contains(InContainer))
	{
		return;
	}

	Containers.Add(InContainer);
}

void USurvivalLootWorldSubsystem::UnregisterContainer(ASurvivalLootContainer* InContainer)
{
	Containers.Remove(InContainer);
}

void USurvivalLootWorldSubsystem::Tick(const float DeltaTime)
{
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}

	EvaluateDueSpawnPoints();
}

TStatId USurvivalLootWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USurvivalLootWorldSubsystem, STATGROUP_Tickables);
}

void USurvivalLootWorldSubsystem::HandleWorldTimeAdvanced(const float InElapsedSeconds)
{
	if (InElapsedSeconds <= 0.0f)
	{
		return;
	}

	TimeSinceLastRecycle += InElapsedSeconds;
	const USurvivalGameplayData* GameplayData = GetDefault<USurvivalGameplayData>();
	const ASurvivalGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASurvivalGameMode>() : nullptr;
	const float RecycleInterval = GameMode && GameMode->GetGameplayData() ? GameMode->GetGameplayData()->LootRecycleIntervalGameSeconds : GameplayData->LootRecycleIntervalGameSeconds;
	if (TimeSinceLastRecycle < FMath::Max(RecycleInterval, 1.0f))
	{
		return;
	}

	TimeSinceLastRecycle = 0.0f;
	EvaluateEmptyContainers();
}

void USurvivalLootWorldSubsystem::EvaluateDueSpawnPoints()
{
	const double CurrentRealTimeSeconds = FPlatformTime::Seconds();
	for (ASurvivalLootSpawnPoint* SpawnPoint : SpawnPoints)
	{
		if (!IsValid(SpawnPoint))
		{
			continue;
		}

		if (!SpawnPoint->IsRecyclingEnabled())
		{
			NextSpawnPointRecycleTimes.Remove(SpawnPoint);
			continue;
		}

		double* NextRecycleTime = NextSpawnPointRecycleTimes.Find(SpawnPoint);
		if (!NextRecycleTime)
		{
			NextSpawnPointRecycleTimes.Add(SpawnPoint, CurrentRealTimeSeconds + SpawnPoint->GetRecycleIntervalRealSeconds());
			continue;
		}

		if (CurrentRealTimeSeconds >= *NextRecycleTime)
		{
			SpawnPoint->TryPopulate(RandomStream);
			*NextRecycleTime = CurrentRealTimeSeconds + SpawnPoint->GetRecycleIntervalRealSeconds();
		}
	}
}

void USurvivalLootWorldSubsystem::EvaluateEmptyContainers()
{
	for (ASurvivalLootContainer* Container : Containers)
	{
		if (IsValid(Container))
		{
			Container->TryRefill();
		}
	}
}
