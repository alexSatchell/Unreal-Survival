#pragma once

#include "CoreMinimal.h"
#include "SurvivalRoundState.generated.h"

UENUM(BlueprintType)
enum class ESurvivalRoundState : uint8
{
	Waiting UMETA(DisplayName = "Waiting"),
	InProgress UMETA(DisplayName = "In Progress"),
	Finished UMETA(DisplayName = "Finished")
};
