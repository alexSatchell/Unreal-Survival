#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SurvivalPlayerController.generated.h"

/** Controls a player's interaction with a Survival match. */
UCLASS()
class SURVIVAL_API ASurvivalPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;

private:
	void HandleReadyInput();
	void HandleInteractInput();

	UFUNCTION(Server, Reliable)
	void ServerSetReady();

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetActor);
};
