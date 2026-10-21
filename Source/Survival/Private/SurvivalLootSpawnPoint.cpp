#include "SurvivalLootSpawnPoint.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "SurvivalLootTable.h"
#include "Pickup.h"
#include "SurvivalItemDefinition.h"
#include "SurvivalLootWorldSubsystem.h"

ASurvivalLootSpawnPoint::ASurvivalLootSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	PlacementMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("PlacementMarker"));
	PlacementMarker->ArrowSize = 1.0f;
	PlacementMarker->SetArrowColor(FLinearColor(0.1f, 0.8f, 1.0f, 1.0f));
	PlacementMarker->SetHiddenInGame(true);
	RootComponent = PlacementMarker;
	SetActorHiddenInGame(true);
	SetCanBeDamaged(false);
}

void ASurvivalLootSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		World->GetSubsystem<USurvivalLootWorldSubsystem>()->RegisterSpawnPoint(this);
	}
}

void ASurvivalLootSpawnPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetSubsystem<USurvivalLootWorldSubsystem>()->UnregisterSpawnPoint(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool ASurvivalLootSpawnPoint::HasContents() const
{
	return IsValid(ActivePickup);
}

void ASurvivalLootSpawnPoint::TryPopulate(FRandomStream& RandomStream)
{
	if (!HasAuthority() || HasContents() || !IsValid(LootTable))
	{
		return;
	}

	TArray<USurvivalItemDefinition*> RolledDefinitions;
	LootTable->RollItems(ContextTags, RandomStream, MaximumContents, RolledDefinitions);
	if (RolledDefinitions.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTransform SpawnTransform = GetActorTransform();
	APickup* NewPickup = World->SpawnActorDeferred<APickup>(APickup::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!NewPickup)
	{
		return;
	}

	NewPickup->ItemDefinition = RolledDefinitions[0];
	NewPickup->FinishSpawning(SpawnTransform);
	ActivePickup = NewPickup;
}
