#include "SurvivalHud.h"
#include "InteractionPromptWidget.h"
#include "GameFramework/PlayerController.h"
#include "SurvivalCharacter.h"
#include "SurvivalInventoryOverlayWidget.h"
#include "SurvivalVitalsWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalHud, Log, All);

void ASurvivalHud::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* OwningPlayerController = GetOwningPlayerController();
	UE_LOG(LogSurvivalHud, Log, TEXT("HUD %s began play for owning player controller %s"), *GetName(), OwningPlayerController ? *OwningPlayerController->GetName() : TEXT("None"));
	if (!OwningPlayerController || !OwningPlayerController->IsLocalController())
	{
		return;
	}

	InitializeVitalsWidget(nullptr);
	InitializeInventoryOverlayWidget(nullptr);
	UE_LOG(LogSurvivalHud, Log, TEXT("Created code-built vitals and inventory overlays for %s"), *OwningPlayerController->GetName());
}

void ASurvivalHud::InitializeInteractionPrompt(TSubclassOf<UInteractionPromptWidget> InWidgetClass)
{
	if (!InWidgetClass || !GetOwningPlayerController() || InteractionPromptWidget)
	{
		return;
	}

	InteractionPromptWidget = CreateWidget<UInteractionPromptWidget>(GetOwningPlayerController(), InWidgetClass);
	if (!InteractionPromptWidget)
	{
		UE_LOG(LogSurvivalHud, Warning, TEXT("Could not create interaction prompt widget for %s"), *GetName());
		return;
	}

	InteractionPromptWidget->AddToViewport(100);
	// The root Canvas Panel and CenterOverlay own the screen layout in the widget Blueprint.
	InteractionPromptWidget->SetInteractionState(false, false, InteractionDotOpacity);
}

void ASurvivalHud::UpdateInteractionPrompt(const FInteractionPromptData& InPromptData, bool bInFocused, bool bInMenuOpen)
{
	if (!InteractionPromptWidget)
	{
		return;
	}

	InteractionPromptWidget->SetPromptData(InPromptData);
	InteractionPromptWidget->SetInteractionState(bInFocused, bInMenuOpen, InteractionDotOpacity);
}

void ASurvivalHud::InitializeVitalsWidget(TSubclassOf<USurvivalVitalsWidget> InWidgetClass)
{
	APlayerController* OwningPlayerController = GetOwningPlayerController();
	if (!OwningPlayerController)
	{
		return;
	}

	if (VitalsWidget)
	{
		VitalsWidget->InitializeForCharacter(Cast<ASurvivalCharacter>(OwningPlayerController->GetPawn()));
		return;
	}

	TSubclassOf<USurvivalVitalsWidget> WidgetClass = InWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = USurvivalVitalsWidget::StaticClass();
	}
	VitalsWidget = CreateWidget<USurvivalVitalsWidget>(GetOwningPlayerController(), WidgetClass);
	if (!VitalsWidget)
	{
		UE_LOG(LogSurvivalHud, Warning, TEXT("Could not create vitals widget for %s"), *GetName());
		return;
	}

	VitalsWidget->AddToViewport(10);
	VitalsWidget->InitializeForCharacter(Cast<ASurvivalCharacter>(OwningPlayerController->GetPawn()));
}

void ASurvivalHud::InitializeInventoryOverlayWidget(TSubclassOf<USurvivalInventoryOverlayWidget> InWidgetClass)
{
	APlayerController* OwningPlayerController = GetOwningPlayerController();
	if (!OwningPlayerController)
	{
		return;
	}

	if (InventoryOverlayWidget)
	{
		InventoryOverlayWidget->InitializeForCharacter(Cast<ASurvivalCharacter>(OwningPlayerController->GetPawn()));
		return;
	}

	TSubclassOf<USurvivalInventoryOverlayWidget> WidgetClass = InWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = USurvivalInventoryOverlayWidget::StaticClass();
	}
	InventoryOverlayWidget = CreateWidget<USurvivalInventoryOverlayWidget>(GetOwningPlayerController(), WidgetClass);
	if (!InventoryOverlayWidget)
	{
		UE_LOG(LogSurvivalHud, Warning, TEXT("Could not create inventory overlay widget for %s"), *GetName());
		return;
	}

	InventoryOverlayWidget->AddToViewport(50);
	InventoryOverlayWidget->InitializeForCharacter(Cast<ASurvivalCharacter>(OwningPlayerController->GetPawn()));
	InventoryOverlayWidget->SetOverlayOpen(false);
}

void ASurvivalHud::SetInventoryOverlayVisible(const bool bInVisible)
{
	if (InventoryOverlayWidget)
	{
		InventoryOverlayWidget->SetOverlayOpen(bInVisible);
	}
}

bool ASurvivalHud::IsInventoryOverlayVisible() const
{
	return InventoryOverlayWidget && InventoryOverlayWidget->GetVisibility() == ESlateVisibility::Visible;
}
