#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SurvivalPlayerState.generated.h"

UCLASS()
class SURVIVAL_API ASurvivalPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetIsAlive(const bool bNewIsAlive);
	
	UFUNCTION(BlueprintPure)
	bool GetIsAlive() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_IsAlive(const bool bPreviousIsAlive) const;

	UPROPERTY(ReplicatedUsing = OnRep_IsAlive)
	bool bIsAlive = true;
};
