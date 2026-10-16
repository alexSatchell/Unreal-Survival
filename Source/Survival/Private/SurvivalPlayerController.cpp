#include "SurvivalPlayerController.h"
#include "CollisionQueryParams.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InteractionPromptActor.h"
#include "InteractionPromptWidget.h"
#include "InputCoreTypes.h"
#include "Interfaces/Interactable.h"
#include "Pickup.h"
#include "SurvivalCharacter.h"
#include "SurvivalInventoryComponent.h"
#include "SurvivalItemDefinition.h"
#include "SurvivalPlayerState.h"
#include "SurvivalVitalComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalPlayerController, Log, All);

void ASurvivalPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(FocusTraceTimerHandle, this, &ASurvivalPlayerController::UpdateInteractionFocus, FMath::Max(FocusTraceInterval, 0.01f), true, 0.0f);
	CreateInteractionPromptActor();

	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Enhanced Input local player is unavailable for %s"), *GetName());
		return;
	}

	if (!PlayerMappingContext)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Enhanced Input PlayerMappingContext is not configured for %s"), *GetName());
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
	}
}

void ASurvivalPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FocusTraceTimerHandle);
	if (IsValid(InteractionPromptActor))
	{
		InteractionPromptActor->Destroy();
	}
	Super::EndPlay(EndPlayReason);
}

void ASurvivalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ASurvivalPlayerController::HandleInteractInput);
		}
		else
		{
			UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Enhanced Input InteractAction is not configured for %s"), *GetName());
		}

		if (DropAction)
		{
			EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &ASurvivalPlayerController::HandleDropInput);
		}
		else
		{
			UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Enhanced Input DropAction is not configured for %s"), *GetName());
		}

		if (UseItemAction)
		{
			EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Started, this, &ASurvivalPlayerController::HandleUseItemInput);
		}
		else
		{
			UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Enhanced Input UseItemAction is not configured for %s"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Enhanced Input component is unavailable for %s"), *GetName());
	}
}

void ASurvivalPlayerController::HandleUseItemInput()
{
	if (!IsLocalController() || !GetPawn())
	{
		return;
	}

	const ASurvivalCharacter* SurvivalCharacter = Cast<ASurvivalCharacter>(GetPawn());
	const USurvivalInventoryComponent* SurvivalInventoryComponent = SurvivalCharacter ? SurvivalCharacter->GetInventoryComponent() : nullptr;
	if (!SurvivalInventoryComponent || SurvivalInventoryComponent->Items.IsEmpty())
	{
		UE_LOG(LogSurvivalPlayerController, Log, TEXT("Use input ignored because the inventory is empty"));
		return;
	}

	const FSurvivalItemInstance& FirstSurvivalItemInstance = SurvivalInventoryComponent->Items[0];
	if (FirstSurvivalItemInstance.IsValid())
	{
		ServerUseItem(FirstSurvivalItemInstance.InstanceId);
	}
}

void ASurvivalPlayerController::HandleInteractInput()
{
	if (!IsLocalController())
	{
		return;
	}

	FVector ViewLocation;
	FHitResult HitResult;
	if (!TraceInteraction(HitResult, ViewLocation, FName(TEXT("InteractTrace"))))
	{
		UE_LOG(LogSurvivalPlayerController, Log, TEXT("Interaction trace found no actor"));
		return;
	}

	AActor* TargetActor = HitResult.GetActor();
	if (!IsValidInteractionTarget(TargetActor))
	{
		UE_LOG(LogSurvivalPlayerController, Log, TEXT("Interaction trace hit a non-interactable actor"));
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Interaction trace found interactable actor %s"), *TargetActor->GetName());
	ServerInteract(TargetActor);
}

void ASurvivalPlayerController::HandleDropInput()
{
	if (!IsLocalController() || !GetPawn())
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Drop input rejected because the controller is not local or has no pawn"));
		return;
	}

	ASurvivalCharacter* SurvivalCharacter = Cast<ASurvivalCharacter>(GetPawn());
	USurvivalInventoryComponent* SurvivalInventoryComponent = SurvivalCharacter ? SurvivalCharacter->GetInventoryComponent() : nullptr;
	if (!SurvivalInventoryComponent)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Drop input rejected because the controlled character has no inventory component"));
		return;
	}

	if (SurvivalInventoryComponent->Items.IsEmpty())
	{
		UE_LOG(LogSurvivalPlayerController, Log, TEXT("Drop input ignored because the replicated inventory is empty"));
		return;
	}

	const FSurvivalItemInstance& FirstSurvivalItemInstance = SurvivalInventoryComponent->Items[0];
	if (!FirstSurvivalItemInstance.IsValid())
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Drop input rejected because the first inventory item is invalid"));
		return;
	}

	ServerDropItem(FirstSurvivalItemInstance.InstanceId);
}

void ASurvivalPlayerController::CreateInteractionPromptActor()
{
	if (!InteractionPromptWidgetClass)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Interaction prompt widget class is not configured for %s"), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	InteractionPromptActor = World->SpawnActor<AInteractionPromptActor>(AInteractionPromptActor::StaticClass(), FTransform::Identity, SpawnParameters);
	if (InteractionPromptActor)
	{
		InteractionPromptActor->SetPromptWidgetClass(InteractionPromptWidgetClass);
		InteractionPromptActor->SetPromptWorldScale(InteractionPromptWorldScale);
	}
}

void ASurvivalPlayerController::UpdateInteractionFocus()
{
	if (!IsLocalController())
	{
		return;
	}

	FVector ViewLocation;
	FHitResult HitResult;
	AActor* TargetActor = nullptr;
	if (TraceInteraction(HitResult, ViewLocation, FName(TEXT("InteractionFocusTrace"))))
	{
		TargetActor = HitResult.GetActor();
		if (!IsValidInteractionTarget(TargetActor))
		{
			TargetActor = nullptr;
		}
	}

	AActor* PreviousTargetActor = FocusedInteractable.Get();
	if (PreviousTargetActor != TargetActor)
	{
		if (TargetActor)
		{
			UE_LOG(LogSurvivalPlayerController, Log, TEXT("Local interaction focus changed to %s"), *TargetActor->GetName());
		}
		else if (IsValid(PreviousTargetActor))
		{
			UE_LOG(LogSurvivalPlayerController, Log, TEXT("Local interaction focus cleared from %s"), *PreviousTargetActor->GetName());
		}
	}

	FocusedInteractable = TargetActor;
	FocusedInteractionPrompt = TargetActor ? IInteractable::Execute_GetInteractionPrompt(TargetActor, this) : FInteractionPromptData();
	if (InteractionPromptActor)
	{
		if (TargetActor)
		{
			FVector BoundsOrigin;
			FVector BoundsExtent;
			TargetActor->GetActorBounds(false, BoundsOrigin, BoundsExtent);
			const FVector PromptLocation = BoundsOrigin + FocusedInteractionPrompt.WorldOffset;
			InteractionPromptActor->ShowPrompt(PromptLocation, ViewLocation, FocusedInteractionPrompt);
		}
		else
		{
			InteractionPromptActor->HidePrompt();
		}
	}
}

void ASurvivalPlayerController::ServerInteract_Implementation(AActor* TargetActor)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction because controller %s has no authority"), *GetName());
		return;
	}

	if (!IsValidInteractionTarget(TargetActor))
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction because the target actor is invalid or not interactable"));
		return;
	}

	FVector ServerViewLocation;
	FHitResult ServerHitResult;
	if (!TraceInteraction(ServerHitResult, ServerViewLocation, FName(TEXT("ServerInteractTrace"))) || ServerHitResult.GetActor() != TargetActor)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction with %s because the server trace did not reach the target"), *TargetActor->GetName());
		return;
	}

	if (FVector::DistSquared(ServerViewLocation, TargetActor->GetActorLocation()) > FMath::Square(InteractionDistance))
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction with %s because it is out of range"), *TargetActor->GetName());
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Server interacting with target actor %s"), *TargetActor->GetName());
	IInteractable::Execute_Interact(TargetActor, this);
}

void ASurvivalPlayerController::ServerDropItem_Implementation(FGuid ItemInstanceId)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!ItemInstanceId.IsValid())
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected drop because the request contained an invalid item instance ID"));
		return;
	}

	ASurvivalCharacter* SurvivalCharacter = Cast<ASurvivalCharacter>(GetPawn());
	USurvivalInventoryComponent* SurvivalInventoryComponent = SurvivalCharacter ? SurvivalCharacter->GetInventoryComponent() : nullptr;
	if (!SurvivalCharacter || !SurvivalInventoryComponent)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected drop request for %s"), *GetName());
		return;
	}

	FSurvivalItemInstance SurvivalItemInstance;
	if (!SurvivalInventoryComponent->FindItemByInstanceId(ItemInstanceId, SurvivalItemInstance))
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected drop because item instance %s is not in the inventory"), *ItemInstanceId.ToString());
		return;
	}

	USurvivalItemDefinition* SurvivalItemDefinition = SurvivalInventoryComponent->FindItemDefinitionByInstanceId(ItemInstanceId);
	if (!IsValid(SurvivalItemDefinition))
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected drop because the server definition for item instance %s is missing"), *ItemInstanceId.ToString());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector DropLocation = SurvivalCharacter->GetActorLocation() + SurvivalCharacter->GetActorForwardVector() * 100.0f;
	FSurvivalItemInstance RemovedSurvivalItemInstance;
	if (!SurvivalInventoryComponent->TryRemoveItem(ItemInstanceId, RemovedSurvivalItemInstance))
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected drop because the inventory no longer contained the requested item for %s"), *GetName());
		return;
	}

	const FTransform DropTransform(SurvivalCharacter->GetActorRotation(), DropLocation);
	APickup* DroppedPickup = World->SpawnActorDeferred<APickup>(APickup::StaticClass(), DropTransform, this, SurvivalCharacter, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!DroppedPickup)
	{
		SurvivalInventoryComponent->TryAddItem(RemovedSurvivalItemInstance, SurvivalItemDefinition);
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected drop because the pickup could not spawn for %s"), *GetName());
		return;
	}

	if (!DroppedPickup->InitializeItemInstance(RemovedSurvivalItemInstance, SurvivalItemDefinition))
	{
		DroppedPickup->Destroy();
		SurvivalInventoryComponent->TryAddItem(RemovedSurvivalItemInstance, SurvivalItemDefinition);
		UE_LOG(LogSurvivalPlayerController, Error, TEXT("Rejected drop because the spawned pickup could not be initialized"));
		return;
	}

	DroppedPickup->FinishSpawning(DropTransform);
	DroppedPickup->ForceNetUpdate();
	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Dropped item instance %s (%s) from %s"), *ItemInstanceId.ToString(), *SurvivalItemDefinition->GetName(), *GetName());
}

void ASurvivalPlayerController::ServerUseItem_Implementation(FGuid ItemInstanceId)
{
	if (!HasAuthority())
	{
		return;
	}

	ASurvivalCharacter* SurvivalCharacter = Cast<ASurvivalCharacter>(GetPawn());
	USurvivalInventoryComponent* SurvivalInventoryComponent = SurvivalCharacter ? SurvivalCharacter->GetInventoryComponent() : nullptr;
	USurvivalVitalComponent* SurvivalVitalComponent = SurvivalCharacter ? SurvivalCharacter->GetVitalComponent() : nullptr;
	if (!SurvivalCharacter || !SurvivalInventoryComponent || !SurvivalVitalComponent || !ItemInstanceId.IsValid())
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected use request for %s because the character, inventory, vital component, or item instance is invalid"), *GetName());
		return;
	}

	FSurvivalItemInstance SurvivalItemInstance;
	if (!SurvivalInventoryComponent->FindItemByInstanceId(ItemInstanceId, SurvivalItemInstance))
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected use because item instance %s is not in the inventory"), *ItemInstanceId.ToString());
		return;
	}

	USurvivalItemDefinition* SurvivalItemDefinition = SurvivalInventoryComponent->FindItemDefinitionByInstanceId(ItemInstanceId);
	if (!IsValid(SurvivalItemDefinition) || SurvivalItemDefinition->HydrationRestoration <= 0.0f)
	{
		UE_LOG(LogSurvivalPlayerController, Log, TEXT("Rejected use because item instance %s is not a hydrating consumable"), *ItemInstanceId.ToString());
		return;
	}

	const float HydrationRestoration = SurvivalItemDefinition->HydrationRestoration;
	const float PreviousHydration = SurvivalVitalComponent->CurrentHydration;
	if (!SurvivalVitalComponent->TryRestoreHydration(HydrationRestoration))
	{
		UE_LOG(LogSurvivalPlayerController, Log, TEXT("Rejected use of %s because hydration is already full"), *SurvivalItemDefinition->GetName());
		return;
	}

	FSurvivalItemInstance RemovedSurvivalItemInstance;
	if (!SurvivalInventoryComponent->TryRemoveItem(ItemInstanceId, RemovedSurvivalItemInstance))
	{
		SurvivalVitalComponent->SetCurrentHydration(PreviousHydration);
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Use of %s applied hydration but could not remove item instance %s"), *SurvivalItemDefinition->GetName(), *ItemInstanceId.ToString());
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Used %s; hydration restored by %.1f for %s"), *SurvivalItemDefinition->GetName(), HydrationRestoration, *GetName());
}

bool ASurvivalPlayerController::IsValidInteractionTarget(const AActor* TargetActor) const
{
	return IsValid(TargetActor) && TargetActor->GetWorld() == GetWorld() && TargetActor->GetClass()->ImplementsInterface(UInteractable::StaticClass());
}

bool ASurvivalPlayerController::TraceInteraction(FHitResult& OutHitResult, FVector& OutViewLocation, FName TraceTag) const
{
	const UWorld* World = GetWorld();
	const APawn* ControlledPawn = GetPawn();
	if (!World || !ControlledPawn)
	{
		return false;
	}

	FRotator ViewRotation;
	GetPlayerViewPoint(OutViewLocation, ViewRotation);
	const FVector TraceEnd = OutViewLocation + ViewRotation.Vector() * InteractionDistance;
	const FCollisionQueryParams QueryParams(TraceTag, true, ControlledPawn);
	return World->LineTraceSingleByChannel(OutHitResult, OutViewLocation, TraceEnd, ECC_Visibility, QueryParams);
}
