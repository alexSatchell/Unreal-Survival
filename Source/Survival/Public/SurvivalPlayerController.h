#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/Interactable.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "SurvivalPlayerController.generated.h"

class USurvivalItemDefinition;

/** Controls a player's interaction with a Survival match. */
UCLASS()
class SURVIVAL_API ASurvivalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	const AActor* GetFocusedInteractable() const { return FocusedInteractable.Get(); }
	const FInteractionPromptData& GetFocusedInteractionPrompt() const { return FocusedInteractionPrompt; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

private:
	void HandleInteractInput();
	void HandleUseItemInput();
	void HandleDropInput();
	void CreateInteractionPromptActor();
	void UpdateInteractionFocus();
	bool IsValidInteractionTarget(const AActor* TargetActor) const;
	bool TraceInteraction(FHitResult& OutHitResult, FVector& OutViewLocation, FName TraceTag) const;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DropAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> UseItemAction;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionDistance = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.01"))
	float FocusTraceInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|UI", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float InteractionPromptWorldScale = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|UI")
	TSubclassOf<class UInteractionPromptWidget> InteractionPromptWidgetClass;

	FTimerHandle FocusTraceTimerHandle;
	TWeakObjectPtr<AActor> FocusedInteractable;
	FInteractionPromptData FocusedInteractionPrompt;
	UPROPERTY(Transient)
	TObjectPtr<class AInteractionPromptActor> InteractionPromptActor;

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetActor);

	UFUNCTION(Server, Reliable)
	void ServerDropItem(FGuid ItemInstanceId);

	UFUNCTION(Server, Reliable)
	void ServerUseItem(FGuid ItemInstanceId);
};
