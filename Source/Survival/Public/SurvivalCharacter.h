#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SurvivalCharacter.generated.h"

/** Represents the player-controlled character in a Survival match. */
UCLASS()
class SURVIVAL_API ASurvivalCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	virtual void PossessedBy(AController* NewController) override;
};
