#include "SurvivalInventoryOverlayWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "SurvivalCharacter.h"
#include "SurvivalInventoryComponent.h"
#include "SurvivalInventoryRowWidget.h"

TSharedRef<SWidget> USurvivalInventoryOverlayWidget::RebuildWidget()
{
	BuildWidget();
	return Super::RebuildWidget();
}

void USurvivalInventoryOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidget();
	BindToInventory();
	SetVisibility(ESlateVisibility::Collapsed);
}

void USurvivalInventoryOverlayWidget::NativeDestruct()
{
	if (bIsBoundToInventory && Character.IsValid() && Character->GetInventoryComponent())
	{
		Character->GetInventoryComponent()->OnInventoryChanged.RemoveDynamic(this, &USurvivalInventoryOverlayWidget::HandleInventoryChanged);
	}
	bIsBoundToInventory = false;
	Super::NativeDestruct();
}

void USurvivalInventoryOverlayWidget::InitializeForCharacter(ASurvivalCharacter* InCharacter)
{
	if (Character.Get() != InCharacter)
	{
		if (bIsBoundToInventory && Character.IsValid() && Character->GetInventoryComponent())
		{
			Character->GetInventoryComponent()->OnInventoryChanged.RemoveDynamic(this, &USurvivalInventoryOverlayWidget::HandleInventoryChanged);
		}

		bIsBoundToInventory = false;
	}

	Character = InCharacter;
	BindToInventory();
	RefreshInventory();
}

void USurvivalInventoryOverlayWidget::SetOverlayOpen(const bool bInOpen)
{
	SetVisibility(bInOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bInOpen)
	{
		RefreshInventory();
	}
}

void USurvivalInventoryOverlayWidget::BuildWidget()
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
	BackgroundSlot->SetPosition(FVector2D::ZeroVector);
	BackgroundSlot->SetSize(FVector2D(560.0f, 440.0f));

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Background->AddChild(Content);

	UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Heading->SetText(FText::FromString(TEXT("Inventory")));
	Heading->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo HeadingFont = Heading->GetFont();
	HeadingFont.Size = 24;
	Heading->SetFont(HeadingFont);
	Content->AddChildToVerticalBox(Heading)->SetPadding(FMargin(16.0f, 14.0f, 16.0f, 8.0f));

	ItemList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	UVerticalBoxSlot* ListSlot = Content->AddChildToVerticalBox(ItemList);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ListSlot->SetPadding(FMargin(12.0f, 0.0f));

	UTextBlock* Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Footer->SetText(FText::FromString(TEXT("Tab: close    Click Use or Drop on an item")));
	Footer->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f)));
	Content->AddChildToVerticalBox(Footer)->SetPadding(FMargin(16.0f, 8.0f, 16.0f, 14.0f));
}

void USurvivalInventoryOverlayWidget::BindToInventory()
{
	if (!Character.IsValid() && GetOwningPlayer())
	{
		Character = Cast<ASurvivalCharacter>(GetOwningPlayerPawn());
	}

	USurvivalInventoryComponent* InventoryComponent = Character.IsValid() ? Character->GetInventoryComponent() : nullptr;
	if (!InventoryComponent || bIsBoundToInventory)
	{
		return;
	}

	InventoryComponent->OnInventoryChanged.AddDynamic(this, &USurvivalInventoryOverlayWidget::HandleInventoryChanged);
	bIsBoundToInventory = true;
}

void USurvivalInventoryOverlayWidget::RefreshInventory()
{
	if (!ItemList || !Character.IsValid() || !Character->GetInventoryComponent())
	{
		return;
	}

	ItemList->ClearChildren();
	for (const FSurvivalItemInstance& ItemInstance : Character->GetInventoryComponent()->Items)
	{
		USurvivalInventoryRowWidget* Row = CreateWidget<USurvivalInventoryRowWidget>(GetOwningPlayer(), USurvivalInventoryRowWidget::StaticClass());
		if (!Row)
		{
			continue;
		}

		Row->InitializeRow(Character.Get(), ItemInstance);
		Row->OnActionRequested.AddDynamic(this, &USurvivalInventoryOverlayWidget::HandleRowAction);
		ItemList->AddChild(Row);
	}
}

void USurvivalInventoryOverlayWidget::HandleInventoryChanged()
{
	RefreshInventory();
}

void USurvivalInventoryOverlayWidget::HandleRowAction(FGuid ItemInstanceId, const bool bUse)
{
	if (!Character.IsValid())
	{
		return;
	}

	if (bUse)
	{
		Character->RequestUseItem(ItemInstanceId);
	}
	else
	{
		Character->RequestDropItem(ItemInstanceId);
	}
}
