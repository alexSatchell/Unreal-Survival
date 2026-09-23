#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SurvivalHud.generated.h"

/** Displays the player-facing HUD for a Survival match. */
UCLASS()
class SURVIVAL_API ASurvivalHud : public AHUD
{
	GENERATED_BODY()

public:
	void LogLocalPlayerState() const;

protected:
	virtual void BeginPlay() override;
};
