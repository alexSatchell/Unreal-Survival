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

void AInteractionPromptActor::ShowPrompt(const FVector& WorldLocation, const FVector& CameraLocation, const FInteractionPromptData& InPromptData)
{
	SetActorLocation(WorldLocation);

	FVector ToCamera = CameraLocation - WorldLocation;
	// Only calculate a facing rotation when the camera has a meaningful direction from the prompt.
	// IsNearlyZero also handles overlapping or nearly overlapping positions, where a zero-length vector has no valid facing direction.
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
