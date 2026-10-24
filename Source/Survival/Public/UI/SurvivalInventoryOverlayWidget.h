#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SurvivalInventoryOverlayWidget.generated.h"

class ASurvivalCharacter;
class UScrollBox;

/** Minimal Tab inventory overlay for inspecting carried items and invoking the existing server actions. */
UCLASS()
class SURVIVAL_API USurvivalInventoryOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForCharacter(ASurvivalCharacter* InCharacter);
	void SetOverlayOpen(bool bInOpen);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildWidget();
	void BindToInventory();
	void RefreshInventory();

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleRowAction(FGuid ItemInstanceId, bool bUse);

	TWeakObjectPtr<ASurvivalCharacter> Character;
	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> ItemList;
	bool bIsBoundToInventory = false;
};
