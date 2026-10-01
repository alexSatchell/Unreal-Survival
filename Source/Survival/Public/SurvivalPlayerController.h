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

	UFUNCTION(Server, Reliable)
	void ServerSetReady();
};
