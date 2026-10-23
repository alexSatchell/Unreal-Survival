#include "Loot/SurvivalLootContainer.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "Items/Pickup.h"
#include "Character/SurvivalCharacter.h"
#include "Items/SurvivalConsumableItemDefinition.h"
#include "Inventory/SurvivalInventoryComponent.h"
#include "Loot/SurvivalLootTable.h"
#include "Loot/SurvivalLootWorldSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalLootContainer, Log, All);

namespace SurvivalLootContainerActions
{
	static const FName Search = FName(TEXT("Search"));
}

ASurvivalLootContainer::ASurvivalLootContainer()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	RootComponent = SceneRoot;
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetMobility(EComponentMobility::Static);
	CollisionComponent->SetBoxExtent(FVector(50.0f));
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionComponent->SetupAttachment(SceneRoot);
	bReplicates = true;
	ContainerDisplayName = NSLOCTEXT("Survival", "LootContainerDefaultName", "Container");
}

void ASurvivalLootContainer::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		World->GetSubsystem<USurvivalLootWorldSubsystem>()->RegisterContainer(this);
	}
}

void ASurvivalLootContainer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetSubsystem<USurvivalLootWorldSubsystem>()->UnregisterContainer(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ASurvivalLootContainer::Interact_Implementation(AController* InteractingController)
{
	InteractWithAction_Implementation(InteractingController, bHasBeenSearched ? NAME_None : SurvivalLootContainerActions::Search);
}

void ASurvivalLootContainer::InteractWithAction_Implementation(AController* InteractingController, const FName ActionId)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!bHasBeenSearched && (ActionId == SurvivalLootContainerActions::Search || ActionId.IsNone()))
	{
		bHasBeenSearched = true;
		GenerateContents();
		ForceNetUpdate();
		return;
	}

	FString Action;
	int32 ContentIndex = INDEX_NONE;
	if (!ResolveContentAction(ActionId, Action, ContentIndex))
	{
		return;
	}

	if (Action == TEXT("Store"))
	{
		TryStoreContent(InteractingController, ContentIndex);
	}
	else if (Action == TEXT("Use"))
	{
		TryUseContent(InteractingController, ContentIndex);
	}
}

FInteractionPromptData ASurvivalLootContainer::GetInteractionPrompt_Implementation(AController*) const
{
	FInteractionPromptData PromptData;
	PromptData.Style = EInteractionPromptStyle::Container;
	PromptData.State = EInteractionPromptState::Available;
	PromptData.TargetText = ContainerDisplayName;

	if (!bHasBeenSearched)
	{
		PromptData.ActionText = NSLOCTEXT("Survival", "SearchContainerAction", "Search");
		PromptData.DefaultActionId = SurvivalLootContainerActions::Search;
		FInteractionOptionData SearchOption;
		SearchOption.ActionId = SurvivalLootContainerActions::Search;
		SearchOption.DisplayText = FText::Format(NSLOCTEXT("Survival", "SearchContainerOption", "Search {0}"), ContainerDisplayName);
		PromptData.Options.Add(SearchOption);
		return PromptData;
	}

	if (Contents.IsEmpty())
	{
		PromptData.State = EInteractionPromptState::Unavailable;
		PromptData.ActionText = NSLOCTEXT("Survival", "EmptyContainerAction", "Empty");
		PromptData.DescriptionText = FText::Format(NSLOCTEXT("Survival", "EmptyContainerDescription", "The {0} is empty."), ContainerDisplayName);
		return PromptData;
	}

	PromptData.ActionText = NSLOCTEXT("Survival", "TakeContainerAction", "Take");
	for (int32 ContentIndex = 0; ContentIndex < Contents.Num(); ++ContentIndex)
	{
		const FSurvivalItemInstance& Content = Contents[ContentIndex];
		const FText ItemName = Content.DisplayName.IsEmpty() ? NSLOCTEXT("Survival", "UnknownLootItem", "Item") : Content.DisplayName;
		FInteractionOptionData StoreOption;
		StoreOption.ActionId = FName(*FString::Printf(TEXT("Store_%d"), ContentIndex));
		StoreOption.DisplayText = FText::Format(NSLOCTEXT("Survival", "StoreContainerItem", "Store {0}"), ItemName);
		PromptData.Options.Add(StoreOption);
		if (Content.bIsConsumable)
		{
			FInteractionOptionData UseOption;
			UseOption.ActionId = FName(*FString::Printf(TEXT("Use_%d"), ContentIndex));
			UseOption.DisplayText = FText::Format(NSLOCTEXT("Survival", "UseContainerItem", "Use {0}"), ItemName);
			PromptData.Options.Add(UseOption);
		}
	}
	PromptData.DefaultActionId = PromptData.Options.Num() > 0 ? PromptData.Options[0].ActionId : NAME_None;
	return PromptData;
}

void ASurvivalLootContainer::TryRefill()
{
	if (HasAuthority() && bHasBeenSearched && Contents.IsEmpty())
	{
		GenerateContents();
	}
}

void ASurvivalLootContainer::GenerateContents()
{
	if (!HasAuthority() || !IsValid(LootTable) || !Contents.IsEmpty())
	{
		return;
	}

	FRandomStream RandomStream;
	RandomStream.Initialize(GetUniqueID() ^ FMath::Rand());
	TArray<USurvivalItemDefinition*> RolledDefinitions;
	LootTable->RollItems(ContextTags, RandomStream, MaximumContents, RolledDefinitions);
	ContentDefinitions.Reset();
	Contents.Reset();
	for (USurvivalItemDefinition* ItemDefinition : RolledDefinitions)
	{
		if (!IsValid(ItemDefinition))
		{
			continue;
		}

		FSurvivalItemInstance ItemInstance;
		ItemInstance.InstanceId = FGuid::NewGuid();
		ItemInstance.DefinitionAssetId = ItemDefinition->GetPrimaryAssetId();
		ItemInstance.DisplayName = ItemDefinition->DisplayName;
		ItemInstance.bIsConsumable = ItemDefinition->IsA<USurvivalConsumableItemDefinition>();
		Contents.Add(ItemInstance);
		ContentDefinitions.Add(ItemDefinition);
	}
	ForceNetUpdate();
}

bool ASurvivalLootContainer::TryStoreContent(AController* InteractingController, const int32 ContentIndex)
{
	ASurvivalCharacter* Character = InteractingController ? Cast<ASurvivalCharacter>(InteractingController->GetPawn()) : nullptr;
	USurvivalInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
	if (!Character || !Inventory || !Contents.IsValidIndex(ContentIndex) || !ContentDefinitions.IsValidIndex(ContentIndex) || !IsValid(ContentDefinitions[ContentIndex]))
	{
		return false;
	}

	if (!Inventory->TryAddItem(Contents[ContentIndex], ContentDefinitions[ContentIndex]))
	{
		return false;
	}
	Contents.RemoveAt(ContentIndex);
	ContentDefinitions.RemoveAt(ContentIndex);
	ForceNetUpdate();
	return true;
}

bool ASurvivalLootContainer::TryUseContent(AController* InteractingController, const int32 ContentIndex)
{
	ASurvivalCharacter* Character = InteractingController ? Cast<ASurvivalCharacter>(InteractingController->GetPawn()) : nullptr;
	const USurvivalConsumableItemDefinition* Consumable = ContentDefinitions.IsValidIndex(ContentIndex) ? Cast<USurvivalConsumableItemDefinition>(ContentDefinitions[ContentIndex]) : nullptr;
	if (!Character || !Consumable || !Contents.IsValidIndex(ContentIndex) || !Character->TryApplyConsumableEffects(Consumable))
	{
		return false;
	}

	Contents.RemoveAt(ContentIndex);
	ContentDefinitions.RemoveAt(ContentIndex);
	ForceNetUpdate();
	return true;
}

bool ASurvivalLootContainer::ResolveContentAction(const FName ActionId, FString& OutAction, int32& OutIndex) const
{
	FString ActionString = ActionId.ToString();
	FString IndexString;
	if (!ActionString.Split(TEXT("_"), &OutAction, &IndexString) || !IndexString.IsNumeric())
	{
		return false;
	}
	OutIndex = FCString::Atoi(*IndexString);
	return Contents.IsValidIndex(OutIndex) && (OutAction == TEXT("Store") || OutAction == TEXT("Use"));
}

void ASurvivalLootContainer::OnRep_Contents()
{
}

void ASurvivalLootContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASurvivalLootContainer, Contents);
	DOREPLIFETIME(ASurvivalLootContainer, bHasBeenSearched);
}
