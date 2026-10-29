#include "SurvivalInventoryRowWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "SurvivalCharacter.h"

TSharedRef<SWidget> USurvivalInventoryRowWidget::RebuildWidget()
{
	BuildWidget();
	return Super::RebuildWidget();
}

void USurvivalInventoryRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidget();
	RefreshRow();
}

void USurvivalInventoryRowWidget::InitializeRow(ASurvivalCharacter* InCharacter, const FSurvivalItemInstance& InItemInstance)
{
	Character = InCharacter;
	ItemInstance = InItemInstance;
	RefreshRow();
}

void USurvivalInventoryRowWidget::BuildWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	WidgetTree->RootWidget = Row;

	ItemLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ItemLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(ItemLabel);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LabelSlot->SetPadding(FMargin(8.0f, 4.0f));

	UseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* UseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UseLabel->SetText(FText::FromString(TEXT("Use")));
	UseButton->AddChild(UseLabel);
	Row->AddChildToHorizontalBox(UseButton)->SetPadding(FMargin(3.0f, 2.0f));
	UseButton->OnClicked.AddDynamic(this, &USurvivalInventoryRowWidget::HandleUseClicked);

	DropButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* DropLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DropLabel->SetText(FText::FromString(TEXT("Drop")));
	DropButton->AddChild(DropLabel);
	Row->AddChildToHorizontalBox(DropButton)->SetPadding(FMargin(3.0f, 2.0f));
	DropButton->OnClicked.AddDynamic(this, &USurvivalInventoryRowWidget::HandleDropClicked);
}

void USurvivalInventoryRowWidget::RefreshRow()
{
	if (!ItemLabel)
	{
		return;
	}

	const FText DisplayName = ItemInstance.DisplayName.IsEmpty() ? FText::FromString(TEXT("Unknown Item")) : ItemInstance.DisplayName;
	ItemLabel->SetText(DisplayName);
	if (UseButton)
	{
		UseButton->SetVisibility(ItemInstance.bIsConsumable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void USurvivalInventoryRowWidget::HandleUseClicked()
{
	OnActionRequested.Broadcast(ItemInstance.InstanceId, true);
}

void USurvivalInventoryRowWidget::HandleDropClicked()
{
	OnActionRequested.Broadcast(ItemInstance.InstanceId, false);
}
