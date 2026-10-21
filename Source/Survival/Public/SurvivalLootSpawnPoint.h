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

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", UIMin = "1", ToolTip = "Maximum number of item instances this point may contain during one population roll."))
	int32 MaximumContents = 1;

	bool HasContents() const;
	void TryPopulate(FRandomStream& RandomStream);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<APickup> ActivePickup;
};
