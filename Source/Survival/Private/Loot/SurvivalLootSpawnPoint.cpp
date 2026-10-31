#include "Loot/SurvivalLootSpawnPoint.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "Loot/SurvivalLootTable.h"
#include "Items/Pickup.h"
#include "Items/SurvivalItemDefinition.h"
#include "Loot/SurvivalLootWorldSubsystem.h"

ASurvivalLootSpawnPoint::ASurvivalLootSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	PlacementMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("PlacementMarker"));
	PlacementMarker->ArrowSize = 1.0f;
	PlacementMarker->SetArrowColor(FLinearColor(0.1f, 0.8f, 1.0f, 1.0f));
	PlacementMarker->SetHiddenInGame(true);
	RootComponent = PlacementMarker;
	AActor::SetActorHiddenInGame(true);
	SetCanBeDamaged(false);
}

void ASurvivalLootSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	if (const UWorld* World = GetWorld())
	{
		World->GetSubsystem<USurvivalLootWorldSubsystem>()->RegisterSpawnPoint(this);
	}
}

void ASurvivalLootSpawnPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetSubsystem<USurvivalLootWorldSubsystem>()->UnregisterSpawnPoint(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool ASurvivalLootSpawnPoint::HasContents() const
{
	return IsValid(ActivePickup);
}

void ASurvivalLootSpawnPoint::HandleActivePickupDestroyed(AActor* DestroyedActor)
{
	if (ActivePickup == DestroyedActor)
	{
		ActivePickup = nullptr;
	}
}

void ASurvivalLootSpawnPoint::TryPopulate(FRandomStream& RandomStream)
{
	if (!HasAuthority() || HasContents() || !IsValid(LootTable))
	{
		return;
	}

	TArray<USurvivalItemDefinition*> RolledDefinitions;
	LootTable->RollItems(ContextTags, RandomStream, 1, RolledDefinitions);
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
	NewPickup->OnDestroyed.AddDynamic(this, &ASurvivalLootSpawnPoint::HandleActivePickupDestroyed);
	ActivePickup = NewPickup;
}
