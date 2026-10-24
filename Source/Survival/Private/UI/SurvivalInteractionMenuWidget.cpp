#include "UI/SurvivalInteractionMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Interaction/SurvivalPlayerController.h"
#include "UI/SurvivalInteractionOptionRowWidget.h"

TSharedRef<SWidget> USurvivalInteractionMenuWidget::RebuildWidget()
{
	BuildWidget();
	return Super::RebuildWidget();
}

void USurvivalInteractionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidget();
	SetVisibility(ESlateVisibility::Collapsed);
}

void USurvivalInteractionMenuWidget::InitializeForController(ASurvivalPlayerController* InController)
{
	PlayerController = InController;
	RefreshOptions();
}

void USurvivalInteractionMenuWidget::SetPromptData(const FInteractionPromptData& InPromptData)
{
	PromptData = InPromptData;
	RefreshOptions();
}

void USurvivalInteractionMenuWidget::SetMenuOpen(const bool bInOpen)
{
	SetVisibility(bInOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bInOpen)
	{
		RefreshOptions();
	}
}

void USurvivalInteractionMenuWidget::BuildWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = RootCanvas;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.96f));
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	BackgroundSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	BackgroundSlot->SetPosition(FVector2D(0.0f, 40.0f));
	BackgroundSlot->SetSize(FVector2D(460.0f, 320.0f));

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Background->AddChild(Content);
	Heading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Heading->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo HeadingFont = Heading->GetFont();
	HeadingFont.Size = 24;
	Heading->SetFont(HeadingFont);
	Content->AddChildToVerticalBox(Heading)->SetPadding(FMargin(16.0f, 14.0f, 16.0f, 8.0f));

	OptionList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	UVerticalBoxSlot* ListSlot = Content->AddChildToVerticalBox(OptionList);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ListSlot->SetPadding(FMargin(12.0f, 0.0f));

	UTextBlock* Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Footer->SetText(FText::FromString(TEXT("Scroll: select    Click an action    Move away: close")));
	Footer->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f)));
	Content->AddChildToVerticalBox(Footer)->SetPadding(FMargin(16.0f, 8.0f, 16.0f, 14.0f));
}

void USurvivalInteractionMenuWidget::RefreshOptions()
{
	if (!OptionList)
	{
		return;
	}

	if (Heading)
	{
		Heading->SetText(PromptData.TargetText);
	}

	OptionList->ClearChildren();
	OptionRows.Reset();
	for (int32 OptionIndex = 0; OptionIndex < PromptData.Options.Num(); ++OptionIndex)
	{
		USurvivalInteractionOptionRowWidget* Row = CreateWidget<USurvivalInteractionOptionRowWidget>(GetOwningPlayer(), USurvivalInteractionOptionRowWidget::StaticClass());
		if (!Row)
		{
			continue;
		}

		Row->InitializeOption(PromptData.Options[OptionIndex], OptionIndex == PromptData.SelectedOptionIndex);
		Row->OnOptionSelected.AddDynamic(this, &USurvivalInteractionMenuWidget::HandleOptionSelected);
		OptionRows.Add(Row);
		OptionList->AddChild(Row);
	}
}

void USurvivalInteractionMenuWidget::HandleOptionSelected(const FName ActionId)
{
	if (PlayerController.IsValid())
	{
		PlayerController->SelectInteractionOption(ActionId);
	}
}
