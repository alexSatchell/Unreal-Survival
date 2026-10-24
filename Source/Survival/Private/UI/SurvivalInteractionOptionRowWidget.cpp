#include "UI/SurvivalInteractionOptionRowWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> USurvivalInteractionOptionRowWidget::RebuildWidget()
{
	BuildWidget();
	return Super::RebuildWidget();
}

void USurvivalInteractionOptionRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidget();
	RefreshRow();
}

void USurvivalInteractionOptionRowWidget::InitializeOption(const FInteractionOptionData& InOption, const bool bInSelected)
{
	Option = InOption;
	bIsSelected = bInSelected;
	RefreshRow();
}

void USurvivalInteractionOptionRowWidget::BuildWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	OptionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	WidgetTree->RootWidget = OptionButton;
	OptionLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OptionLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	OptionButton->AddChild(OptionLabel);
	OptionButton->OnClicked.AddDynamic(this, &USurvivalInteractionOptionRowWidget::HandleClicked);
}

void USurvivalInteractionOptionRowWidget::RefreshRow()
{
	if (!OptionLabel)
	{
		return;
	}

	OptionLabel->SetText(Option.DisplayText);
	OptionLabel->SetJustification(ETextJustify::Center);
	if (OptionButton)
	{
		OptionButton->SetColorAndOpacity(bIsSelected ? FLinearColor(0.12f, 0.32f, 0.55f, 1.0f) : FLinearColor(0.08f, 0.1f, 0.14f, 1.0f));
	}
}

void USurvivalInteractionOptionRowWidget::HandleClicked()
{
	OnOptionSelected.Broadcast(Option.ActionId);
}
