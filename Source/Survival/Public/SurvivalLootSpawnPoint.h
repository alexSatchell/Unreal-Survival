#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "SurvivalLootSpawnPoint.generated.h"

class USurvivalItemDefinition;
class USurvivalLootTable;
class APickup;

UCLASS()
class SURVIVAL_API ASurvivalLootSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ASurvivalLootSpawnPoint();

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot")
	FGameplayTagContainer ContextTags;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USurvivalLootTable> LootTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", UIMin = "1"))
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
