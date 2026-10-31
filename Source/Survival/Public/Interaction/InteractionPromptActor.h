#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionPromptActor.generated.h"

class UInteractionPromptWidget;
class UWidgetComponent;
struct FInteractionPromptData;

/** Owns one local world-space interaction prompt and keeps it facing the owning player's camera. */
UCLASS()
class SURVIVAL_API AInteractionPromptActor : public AActor
{
	GENERATED_BODY()

public:
	AInteractionPromptActor();

	void SetPromptWidgetClass(TSubclassOf<UInteractionPromptWidget> InWidgetClass);
	void SetPromptWorldScale(float InWorldScale);
	void ShowPrompt(const FVector& WorldLocation, const FVector& CameraLocation, const FInteractionPromptData& InPromptData);
	void HidePrompt();

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> WidgetComponent;
};
