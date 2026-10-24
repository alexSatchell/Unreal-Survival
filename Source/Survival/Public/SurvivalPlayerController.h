#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "SurvivalPlayerController.generated.h"

/** Controls a player's interaction with a Survival match. */
UCLASS()
class SURVIVAL_API ASurvivalPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void HandleInteractInput();
	bool IsValidInteractionTarget(const AActor* TargetActor) const;
	bool TraceInteraction(FHitResult& OutHitResult, FVector& OutViewLocation, FName TraceTag) const;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionDistance = 1000.0f;

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetActor);
};
