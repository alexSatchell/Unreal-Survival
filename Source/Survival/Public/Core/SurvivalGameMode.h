#pragma once

#include "CoreMinimal.h"
#include "Enums/SurvivalRoundState.h"
#include "GameFramework/GameModeBase.h"
#include "SurvivalGameMode.generated.h"

class USurvivalGameplayData;
class USurvivalLootManagerComponent;

UCLASS()
class SURVIVAL_API ASurvivalGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASurvivalGameMode();

	const USurvivalGameplayData* GetGameplayData() const;
	USurvivalLootManagerComponent* GetLootManager() const { return LootManager; }
	void HandleInitialLootReady();
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void TransitionToRoundState(const ESurvivalRoundState NewState);

	UPROPERTY(EditDefaultsOnly, Category = "Survival")
	TObjectPtr<USurvivalGameplayData> GameplayData;

	UPROPERTY(VisibleAnywhere, Category = "Loot", meta = (ToolTip = "Server-owned component that populates and recycles world loot."))
	TObjectPtr<USurvivalLootManagerComponent> LootManager;

	TArray<TWeakObjectPtr<APlayerController>> PlayersWaitingForLoot;

	float WorldClockAccumulator = 0.0f;
};
