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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Scene root for the container actor. Use the actor Transform to position the container in the level."))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Collision volume used by the interaction trace. It blocks the Visibility channel and should cover the container."))
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Name shown in the interaction prompt while the player is looking at this container."))
	FText ContainerDisplayName;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Context tags passed to the loot table when this container is searched. Use them to filter eligible loot entries."))
	FGameplayTagContainer ContextTags;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Loot table rolled when this container is first searched. Leave empty to make the container contain no generated loot."))
	TObjectPtr<USurvivalLootTable> LootTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", UIMin = "1", ClampMax = "3", UIMax = "3", ToolTip = "Maximum number of item instances this container can generate when it is searched."))
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
