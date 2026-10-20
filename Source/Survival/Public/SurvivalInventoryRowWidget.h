#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SurvivalItemTypes.h"
#include "SurvivalInventoryRowWidget.generated.h"

class ASurvivalCharacter;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSurvivalInventoryAction, FGuid, ItemInstanceId, bool, bUse);

/** One compact inventory row with the currently supported Use and Drop actions. */
UCLASS()
class SURVIVAL_API USurvivalInventoryRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeRow(ASurvivalCharacter* InCharacter, const FSurvivalItemInstance& InItemInstance);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnSurvivalInventoryAction OnActionRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidget();
	void RefreshRow();

	UFUNCTION()
	void HandleUseClicked();

	UFUNCTION()
	void HandleDropClicked();

	TWeakObjectPtr<ASurvivalCharacter> Character;
	FSurvivalItemInstance ItemInstance;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ItemLabel;
	UPROPERTY(Transient)
	TObjectPtr<UButton> UseButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> DropButton;
};
