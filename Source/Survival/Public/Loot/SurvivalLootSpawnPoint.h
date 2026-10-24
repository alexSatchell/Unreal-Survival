#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "SurvivalLootSpawnPoint.generated.h"

class USurvivalItemDefinition;
class USurvivalLootTable;
class APickup;
class UArrowComponent;

UCLASS()
class SURVIVAL_API ASurvivalLootSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ASurvivalLootSpawnPoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Editor-only arrow showing this spawn point's location and forward direction. It is hidden during gameplay."))
	TObjectPtr<UArrowComponent> PlacementMarker;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Context tags supplied to the loot table when this point rolls. Use them to restrict or exclude entries."))
	FGameplayTagContainer ContextTags;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Loot table rolled when this point is populated. Leave empty to disable spawning at this point."))
	TObjectPtr<USurvivalLootTable> LootTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot|Recycling", meta = (ToolTip = "Enables this point to respawn one pickup after its configured real-time interval. Disabled points only populate initially."))
	bool bEnableRecycling = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot|Recycling", meta = (ClampMin = "1.0", UIMin = "1.0", ToolTip = "Real-world seconds between recycle checks for this point. Use 60 for a one-minute test interval; 3600 is one real-time hour."))
	float RecycleIntervalRealSeconds = 3600.0f;

	bool HasContents() const;
	bool IsRecyclingEnabled() const { return bEnableRecycling; }
	float GetRecycleIntervalRealSeconds() const { return FMath::Max(RecycleIntervalRealSeconds, 1.0f); }
	void TryPopulate(FRandomStream& RandomStream);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleActivePickupDestroyed(AActor* DestroyedActor);

	UPROPERTY(Transient)
	TObjectPtr<APickup> ActivePickup;
};
