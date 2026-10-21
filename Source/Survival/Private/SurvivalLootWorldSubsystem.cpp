#include "SurvivalLootWorldSubsystem.h"
#include "SurvivalGameState.h"
#include "SurvivalGameMode.h"
#include "SurvivalGameplayData.h"
#include "SurvivalLootSpawnPoint.h"
#include "SurvivalLootContainer.h"
#include "Engine/World.h"

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
}

void USurvivalLootWorldSubsystem::UnregisterSpawnPoint(ASurvivalLootSpawnPoint* InSpawnPoint)
{
	SpawnPoints.Remove(InSpawnPoint);
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
	EvaluateEmptySpawnPoints();
}

void USurvivalLootWorldSubsystem::EvaluateEmptySpawnPoints()
{
	for (ASurvivalLootSpawnPoint* SpawnPoint : SpawnPoints)
	{
		if (IsValid(SpawnPoint))
		{
			SpawnPoint->TryPopulate(RandomStream);
		}
	}
	for (ASurvivalLootContainer* Container : Containers)
	{
		if (IsValid(Container))
		{
			Container->TryRefill();
		}
	}
}
