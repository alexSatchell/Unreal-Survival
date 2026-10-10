#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivalInventoryComponent.generated.h"

/** Owns a character's server-authoritative carried-item state and replicates it to the owning client. */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class SURVIVAL_API USurvivalInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalInventoryComponent();
};
