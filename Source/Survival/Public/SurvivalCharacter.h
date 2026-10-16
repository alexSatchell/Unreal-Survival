#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "SurvivalCharacter.generated.h"

class USurvivalInventoryComponent;
class USurvivalVitalComponent;

UCLASS()
class SURVIVAL_API ASurvivalCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASurvivalCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USurvivalInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitals")
	TObjectPtr<USurvivalVitalComponent> VitalComponent;

	virtual void PossessedBy(AController* NewController) override;
	USurvivalInventoryComponent* GetInventoryComponent() const;
	USurvivalVitalComponent* GetVitalComponent() const;

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void HandleMove(const FInputActionValue& InputActionValue);
	void HandleLook(const FInputActionValue& InputActionValue);
	void HandleUseItemInput();
	void HandleDropInput();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DropAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> UseItemAction;

	UFUNCTION(Server, Reliable)
	void ServerDropItem(FGuid ItemInstanceId);

	UFUNCTION(Server, Reliable)
	void ServerUseItem(FGuid ItemInstanceId);
};
