#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SurvivalPlayerState.generated.h"

UCLASS()
class SURVIVAL_API ASurvivalPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void AddPoint();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetIsAlive(const bool bNewIsAlive);
	
	UFUNCTION(BlueprintPure)
	bool GetIsAlive() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetIsReady(const bool bNewIsReady);

	UFUNCTION(BlueprintPure)
	bool GetIsReady() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_IsAlive(const bool bPreviousIsAlive) const;

	UFUNCTION()
	void OnRep_IsReady(const bool bPreviousIsReady) const;
	
	UPROPERTY(ReplicatedUsing = OnRep_IsAlive)
	bool bIsAlive = true;

	UPROPERTY(ReplicatedUsing = OnRep_IsReady)
	bool bIsReady = false;
};
