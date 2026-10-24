#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class AController;

UENUM(BlueprintType)
enum class EInteractionPromptStyle : uint8
{
	Default,
	Pickup,
	Door,
	Container
};

UENUM(BlueprintType)
enum class EInteractionPromptState : uint8
{
	Available,
	Locked,
	Unavailable
};

USTRUCT(BlueprintType)
struct SURVIVAL_API FInteractionOptionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	FName ActionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	FText DisplayText;
};

USTRUCT(BlueprintType)
struct SURVIVAL_API FInteractionPromptData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	EInteractionPromptStyle Style = EInteractionPromptStyle::Default;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	EInteractionPromptState State = EInteractionPromptState::Available;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	FText ActionText;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	FText TargetText;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	FText DescriptionText;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	FName DefaultActionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	TArray<FInteractionOptionData> Options;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Interaction")
	int32 SelectedOptionIndex = 0;
};

UINTERFACE(Blueprintable)
class SURVIVAL_API UInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Defines an actor that can receive a server-authoritative interaction and provide its local interaction prompt. */
class SURVIVAL_API IInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AController* InteractingController);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void InteractWithAction(AController* InteractingController, FName ActionId);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FInteractionPromptData GetInteractionPrompt(AController* InteractingController) const;
};
