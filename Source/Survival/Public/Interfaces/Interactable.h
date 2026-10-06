#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class AController;

UINTERFACE(Blueprintable)
class SURVIVAL_API UInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Defines an actor that can receive a server-authoritative interaction. */
class SURVIVAL_API IInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AController* InteractingController);
};
