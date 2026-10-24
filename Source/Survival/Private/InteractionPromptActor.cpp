#include "InteractionPromptActor.h"
#include "Components/WidgetComponent.h"
#include "InteractionPromptWidget.h"

AInteractionPromptActor::AInteractionPromptActor()
{
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetDrawAtDesiredSize(true);
	WidgetComponent->SetTwoSided(true);
	WidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComponent->SetVisibility(false);
	RootComponent = WidgetComponent;
	bReplicates = false;
}

void AInteractionPromptActor::SetPromptWidgetClass(TSubclassOf<UInteractionPromptWidget> InWidgetClass)
{
	WidgetComponent->SetWidgetClass(InWidgetClass);
	WidgetComponent->InitWidget();
}

void AInteractionPromptActor::SetPromptWorldScale(float InWorldScale)
{
	WidgetComponent->SetRelativeScale3D(FVector(FMath::Max(InWorldScale, 0.01f)));
}

/**
 * Positions the world-space prompt, rotates it to face the player's camera, then shows and updates the widget.
 * WorldLocation is the prompt's desired position, while CameraLocation is the player's camera position.
 * Subtracting them creates a direction vector from the prompt to the camera. FVector::Rotation() converts that
 * direction into an FRotator, and SetActorRotation applies it so the prompt's forward direction faces the camera.
 * Roll is cleared to keep the prompt upright. If both positions are nearly identical, the direction has no useful
 * facing rotation, so the existing actor rotation is preserved.
 */
void AInteractionPromptActor::ShowPrompt(const FVector& WorldLocation, const FVector& CameraLocation, const FInteractionPromptData& InPromptData)
{
	SetActorLocation(WorldLocation);

	FVector ToCamera = CameraLocation - WorldLocation;
	if (!ToCamera.IsNearlyZero())
	{
		FRotator TargetRotation = ToCamera.Rotation();
		TargetRotation.Roll = 0.0f;
		SetActorRotation(TargetRotation);
	}

	WidgetComponent->SetVisibility(true);

	if (UInteractionPromptWidget* PromptWidget = Cast<UInteractionPromptWidget>(WidgetComponent->GetUserWidgetObject()))
	{
		PromptWidget->SetPromptData(InPromptData);
	}
}

void AInteractionPromptActor::HidePrompt()
{
	WidgetComponent->SetVisibility(false);
}
