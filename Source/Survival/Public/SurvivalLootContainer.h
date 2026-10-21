#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "SurvivalItemTypes.h"
#include "SurvivalLootContainer.generated.h"

class USurvivalItemDefinition;
class USurvivalLootTable;

UCLASS()
class SURVIVAL_API ASurvivalLootContainer : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASurvivalLootContainer();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot")
	FText ContainerDisplayName;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot")
	FGameplayTagContainer ContextTags;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USurvivalLootTable> LootTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", UIMin = "1", ClampMax = "3", UIMax = "3"))
	int32 MaximumContents = 3;

	UPROPERTY(ReplicatedUsing = OnRep_Contents, BlueprintReadOnly, Category = "Loot")
	TArray<FSurvivalItemInstance> Contents;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Loot")
	bool bHasBeenSearched = false;

	virtual void Interact_Implementation(AController* InteractingController) override;
	virtual void InteractWithAction_Implementation(AController* InteractingController, FName ActionId) override;
	virtual FInteractionPromptData GetInteractionPrompt_Implementation(AController* InteractingController) const override;
	void TryRefill();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_Contents();

	void GenerateContents();
	bool TryStoreContent(AController* InteractingController, int32 ContentIndex);
	bool TryUseContent(AController* InteractingController, int32 ContentIndex);
	bool ResolveContentAction(FName ActionId, FString& OutAction, int32& OutIndex) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USurvivalItemDefinition>> ContentDefinitions;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
