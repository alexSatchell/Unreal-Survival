#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/Interactable.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "SurvivalPlayerController.generated.h"

/** Controls a player's interaction with a Survival match. */
UCLASS()
class SURVIVAL_API ASurvivalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	const AActor* GetFocusedInteractable() const { return FocusedInteractable.Get(); }
	const FInteractionPromptData& GetFocusedInteractionPrompt() const { return FocusedInteractionPrompt; }

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SelectInteractionOption(FName ActionId);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

private:
	void HandleInteractInput();
	void HandleInteractionOptionsInput(const FInputActionValue& InputActionValue);
	void HandleInteractionOptionsAxis(float AxisValue);
	void UpdateInteractionFocus();
	void OpenInteractionMenu();
	void CloseInteractionMenu();
	void HandleInventoryToggleInput();
	void OpenInventoryOverlay();
	void CloseInventoryOverlay();
	bool IsValidInteractionTarget(const AActor* TargetActor) const;
	AActor* ResolveInteractionTarget(AActor* HitActor) const;
	bool TraceInteraction(FHitResult& OutHitResult, FVector& OutViewLocation, FName TraceTag) const;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractionOptionsAction;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionDistance = 1000.0f;

	/** Radius in centimeters used by the camera sweep to make small interactables easier to target. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float InteractionTraceRadius = 12.0f;

	/** Temporarily draws the interaction sweep in-game so its targeting volume can be tuned visually. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Debug")
	bool bDrawInteractionTrace = false;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "0.01"))
	float FocusTraceInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|UI")
	TSubclassOf<class UInteractionPromptWidget> InteractionPromptWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|UI")
	TSubclassOf<class USurvivalVitalsWidget> VitalsWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|UI")
	TSubclassOf<class USurvivalInventoryOverlayWidget> InventoryOverlayWidgetClass;

	FTimerHandle FocusTraceTimerHandle;
	TWeakObjectPtr<AActor> FocusedInteractable;
	FInteractionPromptData FocusedInteractionPrompt;
	bool bInteractionMenuOpen = false;
	bool bInventoryOverlayOpen = false;
	int32 SelectedInteractionOptionIndex = 0;

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetActor, FName ActionId);
};
