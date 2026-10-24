#include "SurvivalVitalsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "SurvivalCharacter.h"
#include "SurvivalRadialIndicatorWidget.h"
#include "SurvivalVitalComponent.h"

TSharedRef<SWidget> USurvivalVitalsWidget::RebuildWidget()
{
	BuildWidget();
	return Super::RebuildWidget();
}

void USurvivalVitalsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidget();
	BindToCharacter();
}

void USurvivalVitalsWidget::NativeDestruct()
{
	if (bIsBoundToVitals && Character.IsValid() && Character->GetVitalComponent())
	{
		Character->GetVitalComponent()->OnVitalsChanged.RemoveDynamic(this, &USurvivalVitalsWidget::HandleVitalsChanged);
	}
	bIsBoundToVitals = false;
	Super::NativeDestruct();
}

void USurvivalVitalsWidget::InitializeForCharacter(ASurvivalCharacter* InCharacter)
{
	Character = InCharacter;
	BindToCharacter();
}

void USurvivalVitalsWidget::BuildWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = RootCanvas;
	UHorizontalBox* VitalsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UCanvasPanelSlot* VitalsCanvasSlot = RootCanvas->AddChildToCanvas(VitalsBox);
	VitalsCanvasSlot->SetAnchors(FAnchors(1.0f, 1.0f));
	VitalsCanvasSlot->SetAlignment(FVector2D(1.0f, 1.0f));
	VitalsCanvasSlot->SetPosition(FVector2D(-24.0f, -24.0f));
	VitalsCanvasSlot->SetAutoSize(true);

	const TArray<FString> Labels = { TEXT("Health"), TEXT("Thirst"), TEXT("Hunger"), TEXT("Sickness") };
	const TArray<FLinearColor> Colors = { FLinearColor(0.85f, 0.12f, 0.12f), FLinearColor(0.10f, 0.45f, 1.0f), FLinearColor(1.0f, 0.65f, 0.10f), FLinearColor(0.65f, 0.25f, 0.85f) };
	TArray<TObjectPtr<USurvivalRadialIndicatorWidget>*> Indicators = { &HealthIndicator, &ThirstIndicator, &HungerIndicator, &SicknessIndicator };

	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		UVerticalBox* VitalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBoxSlot* VitalBoxSlot = VitalsBox->AddChildToHorizontalBox(VitalBox);
		VitalBoxSlot->SetPadding(FMargin(5.0f, 0.0f));
		VitalBoxSlot->SetHorizontalAlignment(HAlign_Center);

		USizeBox* IndicatorSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IndicatorSizeBox->SetWidthOverride(58.0f);
		IndicatorSizeBox->SetHeightOverride(58.0f);
		UVerticalBoxSlot* IndicatorSlot = VitalBox->AddChildToVerticalBox(IndicatorSizeBox);
		IndicatorSlot->SetHorizontalAlignment(HAlign_Center);

		USurvivalRadialIndicatorWidget* Indicator = WidgetTree->ConstructWidget<USurvivalRadialIndicatorWidget>(USurvivalRadialIndicatorWidget::StaticClass());
		Indicator->FillColor = Colors[Index];
		Indicator->OutlineColor = FLinearColor(0.04f, 0.04f, 0.04f, 0.85f);
		IndicatorSizeBox->AddChild(Indicator);
		*Indicators[Index] = Indicator;

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Labels[Index]));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo LabelFont = Label->GetFont();
		LabelFont.Size = 12;
		Label->SetFont(LabelFont);
		UVerticalBoxSlot* LabelSlot = VitalBox->AddChildToVerticalBox(Label);
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void USurvivalVitalsWidget::BindToCharacter()
{
	if (!Character.IsValid() && GetOwningPlayer())
	{
		Character = Cast<ASurvivalCharacter>(GetOwningPlayerPawn());
	}

	USurvivalVitalComponent* VitalComponent = Character.IsValid() ? Character->GetVitalComponent() : nullptr;
	if (!VitalComponent || bIsBoundToVitals)
	{
		return;
	}

	VitalComponent->OnVitalsChanged.AddDynamic(this, &USurvivalVitalsWidget::HandleVitalsChanged);
	bIsBoundToVitals = true;
	HandleVitalsChanged(VitalComponent->GetHealthPercent(), VitalComponent->GetHungerPercent(), VitalComponent->GetHydrationPercent(), VitalComponent->GetSicknessPercent());
}

void USurvivalVitalsWidget::HandleVitalsChanged(const float HealthPercent, const float HungerPercent, const float HydrationPercent, const float SicknessPercent)
{
	SetIndicator(HealthIndicator, HealthPercent);
	SetIndicator(ThirstIndicator, HydrationPercent);
	SetIndicator(HungerIndicator, HungerPercent);
	SetIndicator(SicknessIndicator, SicknessPercent);
}

void USurvivalVitalsWidget::SetIndicator(USurvivalRadialIndicatorWidget* Indicator, const float Percent)
{
	if (Indicator)
	{
		Indicator->SetPercent(Percent);
	}
}
