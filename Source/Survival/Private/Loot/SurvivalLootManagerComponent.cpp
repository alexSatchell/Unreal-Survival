#include "Loot/SurvivalLootManagerComponent.h"
#include "Core/SurvivalGameMode.h"
#include "Core/SurvivalGameState.h"
#include "Core/SurvivalGameplayData.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Loot/SurvivalLootContainer.h"
#include "Loot/SurvivalLootSpawnPoint.h"
#include "Loot/SurvivalLootTable.h"

USurvivalLootManagerComponent::USurvivalLootManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USurvivalLootManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	RandomStream.Initialize(FMath::Rand());
	// Level actors register during BeginPlay; begin processing after that pass completes.
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &USurvivalLootManagerComponent::FinishInitialRegistration);
}

void USurvivalLootManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SurvivalGameState.IsValid())
	{
		SurvivalGameState->OnWorldTimeAdvanced.RemoveAll(this);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	SpawnPoints.Reset();
	Containers.Reset();
	InitialQueue.Reset();
	PendingSpawns.Reset();
	ScheduledSequences.Reset();
	Super::EndPlay(EndPlayReason);
}

void USurvivalLootManagerComponent::FinishInitialRegistration()
{
	bInitialRegistrationFinished = true;
	SurvivalGameState = GetWorld()->GetGameState<ASurvivalGameState>();
	if (SurvivalGameState.IsValid())
	{
		SurvivalGameState->OnWorldTimeAdvanced.AddUObject(this, &USurvivalLootManagerComponent::HandleWorldTimeAdvanced);
	}
	GetWorld()->GetTimerManager().SetTimer(RecycleTimer, this, &USurvivalLootManagerComponent::BeginRecycleBatch, FMath::Max(RecycleBatchIntervalSeconds, 1.0f), true);
	SetComponentTickEnabled(true);
}

void USurvivalLootManagerComponent::RegisterSpawnPoint(ASurvivalLootSpawnPoint* SpawnPoint)
{
	if (!IsValid(SpawnPoint) || SpawnPoints.Contains(SpawnPoint))
	{
		return;
	}
	SpawnPoints.Add(SpawnPoint);
	InitialQueue.Add(SpawnPoint);
	if (bInitialRegistrationFinished)
	{
		SetComponentTickEnabled(true);
	}
}

void USurvivalLootManagerComponent::UnregisterSpawnPoint(ASurvivalLootSpawnPoint* SpawnPoint)
{
	SpawnPoints.Remove(SpawnPoint);
	ScheduledSequences.Remove(SpawnPoint);
}

void USurvivalLootManagerComponent::ScheduleEmptySpawnPoint(ASurvivalLootSpawnPoint* SpawnPoint)
{
	if (!IsValid(SpawnPoint) || !SpawnPoints.Contains(SpawnPoint) || !SpawnPoint->IsRecyclingEnabled() || SpawnPoint->HasContents() || !IsValid(SpawnPoint->LootTable))
	{
		return;
	}
	FPendingSpawn Pending;
	Pending.SpawnPoint = SpawnPoint;
	Pending.Deadline = FPlatformTime::Seconds() + SpawnPoint->GetRecycleIntervalRealSeconds();
	Pending.Sequence = ++NextSequence;
	ScheduledSequences.Add(SpawnPoint, Pending.Sequence);
	PendingSpawns.HeapPush(MoveTemp(Pending));
}

void USurvivalLootManagerComponent::RegisterContainer(ASurvivalLootContainer* Container)
{
	if (IsValid(Container))
	{
		Containers.Add(Container);
	}
}

void USurvivalLootManagerComponent::UnregisterContainer(ASurvivalLootContainer* Container)
{
	Containers.Remove(Container);
}

void USurvivalLootManagerComponent::BeginRecycleBatch()
{
	BatchCutoff = FPlatformTime::Seconds();
	bProcessingRecycleBatch = true;
	SetComponentTickEnabled(true);
}

void USurvivalLootManagerComponent::ProcessSpawnPoint(ASurvivalLootSpawnPoint* SpawnPoint)
{
	if (!IsValid(SpawnPoint) || !SpawnPoints.Contains(SpawnPoint))
	{
		return;
	}
	SpawnPoint->TryPopulate(RandomStream);
	if (!SpawnPoint->HasContents())
	{
		ScheduleEmptySpawnPoint(SpawnPoint);
	}
}

void USurvivalLootManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	int32 Remaining = FMath::Max(MaxPopulationAttemptsPerFrame, 1);
	while (Remaining > 0 && InitialQueueIndex < InitialQueue.Num())
	{
		ProcessSpawnPoint(InitialQueue[InitialQueueIndex++].Get());
		--Remaining;
	}
	if (InitialQueueIndex == InitialQueue.Num())
	{
		InitialQueue.Reset();
		InitialQueueIndex = 0;
		if (bInitialRegistrationFinished && !bInitialPopulationComplete)
		{
			bInitialPopulationComplete = true;
			if (ASurvivalGameMode* GameMode = Cast<ASurvivalGameMode>(GetOwner()))
			{
				GameMode->HandleInitialLootReady();
			}
		}
	}
	while (Remaining > 0 && bProcessingRecycleBatch && !PendingSpawns.IsEmpty() && PendingSpawns.HeapTop().Deadline <= BatchCutoff)
	{
		FPendingSpawn Pending;
		PendingSpawns.HeapPop(Pending, EAllowShrinking::No);
		const uint64* CurrentSequence = ScheduledSequences.Find(Pending.SpawnPoint);
		if (CurrentSequence && *CurrentSequence == Pending.Sequence)
		{
			ScheduledSequences.Remove(Pending.SpawnPoint);
			ProcessSpawnPoint(Pending.SpawnPoint.Get());
		}
		--Remaining;
	}
	if (bProcessingRecycleBatch && (PendingSpawns.IsEmpty() || PendingSpawns.HeapTop().Deadline > BatchCutoff))
	{
		bProcessingRecycleBatch = false;
	}
	if (InitialQueue.IsEmpty() && !bProcessingRecycleBatch)
	{
		SetComponentTickEnabled(false);
	}
}

void USurvivalLootManagerComponent::HandleWorldTimeAdvanced(const float InElapsedSeconds)
{
	if (InElapsedSeconds <= 0.0f)
	{
		return;
	}
	TimeSinceLastRecycle += InElapsedSeconds;
	const ASurvivalGameMode* GameMode = Cast<ASurvivalGameMode>(GetOwner());
	const USurvivalGameplayData* GameplayData = GameMode ? GameMode->GetGameplayData() : GetDefault<USurvivalGameplayData>();
	if (TimeSinceLastRecycle >= FMath::Max(GameplayData->LootRecycleIntervalGameSeconds, 1.0f))
	{
		TimeSinceLastRecycle = 0.0f;
		EvaluateEmptyContainers();
	}
}

void USurvivalLootManagerComponent::EvaluateEmptyContainers()
{
	for (const TWeakObjectPtr<ASurvivalLootContainer>& Container : Containers)
	{
		if (Container.IsValid())
		{
			Container->TryRefill();
		}
	}
}
