#include "SurvivalPlayerController.h"
#include "CollisionShape.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InteractionPromptWidget.h"
#include "InputCoreTypes.h"
#include "Interfaces/Interactable.h"
#include "Math/RotationMatrix.h"
#include "SurvivalHud.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalPlayerController, Log, All);

void ASurvivalPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(FocusTraceTimerHandle, this, &ASurvivalPlayerController::UpdateInteractionFocus, FMath::Max(FocusTraceInterval, 0.01f), true, 0.0f);
	if (ASurvivalHud* SurvivalHud = GetHUD<ASurvivalHud>())
	{
		SurvivalHud->InitializeInteractionPrompt(InteractionPromptWidgetClass);
		SurvivalHud->InitializeVitalsWidget(VitalsWidgetClass);
		SurvivalHud->InitializeInventoryOverlayWidget(InventoryOverlayWidgetClass);
	}

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
	CloseInventoryOverlay();
	CloseInteractionMenu();
	Super::EndPlay(EndPlayReason);
}

void ASurvivalPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsLocalController())
	{
		if (ASurvivalHud* SurvivalHud = GetHUD<ASurvivalHud>())
		{
			SurvivalHud->InitializeVitalsWidget(VitalsWidgetClass);
			SurvivalHud->InitializeInventoryOverlayWidget(InventoryOverlayWidgetClass);
		}
	}
}

void ASurvivalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ASurvivalPlayerController::HandleInventoryToggleInput);

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

		if (InteractionOptionsAction)
		{
			EnhancedInputComponent->BindAction(InteractionOptionsAction, ETriggerEvent::Triggered, this, &ASurvivalPlayerController::HandleInteractionOptionsInput);
		}
		else
		{
			// Keep wheel selection usable while the Enhanced Input action is being added to the project assets.
			InputComponent->BindAxis(TEXT("MouseWheelAxis"), this, &ASurvivalPlayerController::HandleInteractionOptionsAxis);
		}

	}
	else
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Enhanced Input component is unavailable for %s"), *GetName());
	}
}

void ASurvivalPlayerController::HandleInteractionOptionsAxis(float AxisValue)
{
	HandleInteractionOptionsInput(FInputActionValue(AxisValue));
}

void ASurvivalPlayerController::HandleInteractInput()
{
	if (!IsLocalController() || bInventoryOverlayOpen)
	{
		return;
	}

	AActor* TargetActor = FocusedInteractable.Get();
	FVector ViewLocation;
	FHitResult HitResult;
	if (!IsValidInteractionTarget(TargetActor) || !TraceInteraction(HitResult, ViewLocation, FName(TEXT("InteractTrace"))) || HitResult.GetActor() != TargetActor)
	{
		UE_LOG(LogSurvivalPlayerController, Log, TEXT("Interaction input found no focused actor"));
		return;
	}

	const FInteractionPromptData PromptData = IInteractable::Execute_GetInteractionPrompt(TargetActor, this);
	if (PromptData.DefaultActionId.IsNone() && PromptData.Options.Num() > 0)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Interaction target %s has options but no default action"), *TargetActor->GetName());
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Interaction trace found interactable actor %s"), *TargetActor->GetName());
	ServerInteract(TargetActor, PromptData.DefaultActionId);
}

void ASurvivalPlayerController::HandleInteractionOptionsInput(const FInputActionValue& InputActionValue)
{
	if (bInventoryOverlayOpen)
	{
		return;
	}

	const float ScrollValue = InputActionValue.Get<float>();
	if (FMath::IsNearlyZero(ScrollValue) || !IsValid(FocusedInteractable.Get()) || FocusedInteractionPrompt.Options.Num() < 2)
	{
		return;
	}

	if (!bInteractionMenuOpen)
	{
		OpenInteractionMenu();
	}

	const int32 OptionCount = FocusedInteractionPrompt.Options.Num();
	const int32 ScrollDirection = ScrollValue > 0.0f ? 1 : -1;
	SelectedInteractionOptionIndex = (SelectedInteractionOptionIndex + ScrollDirection + OptionCount) % OptionCount;
	FocusedInteractionPrompt.SelectedOptionIndex = SelectedInteractionOptionIndex;
	if (ASurvivalHud* SurvivalHud = GetHUD<ASurvivalHud>())
	{
		SurvivalHud->UpdateInteractionPrompt(FocusedInteractionPrompt, true, true);
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
		if (bInteractionMenuOpen)
		{
			CloseInteractionMenu();
		}

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
	if (TargetActor)
	{
		SelectedInteractionOptionIndex = FMath::Clamp(SelectedInteractionOptionIndex, 0, FMath::Max(FocusedInteractionPrompt.Options.Num() - 1, 0));
		FocusedInteractionPrompt.SelectedOptionIndex = SelectedInteractionOptionIndex;
	}
	else
	{
		SelectedInteractionOptionIndex = 0;
	}

	if (ASurvivalHud* SurvivalHud = GetHUD<ASurvivalHud>())
	{
		SurvivalHud->UpdateInteractionPrompt(FocusedInteractionPrompt, TargetActor != nullptr, bInteractionMenuOpen);
	}
}

void ASurvivalPlayerController::OpenInteractionMenu()
{
	if (bInteractionMenuOpen || bInventoryOverlayOpen || !IsValidInteractionTarget(FocusedInteractable.Get()) || FocusedInteractionPrompt.Options.Num() < 2)
	{
		return;
	}

	bInteractionMenuOpen = true;
	SelectedInteractionOptionIndex = 0;
	FocusedInteractionPrompt.SelectedOptionIndex = SelectedInteractionOptionIndex;
	SetIgnoreLookInput(true);
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	if (ASurvivalHud* SurvivalHud = GetHUD<ASurvivalHud>())
	{
		SurvivalHud->UpdateInteractionPrompt(FocusedInteractionPrompt, true, true);
	}
}

void ASurvivalPlayerController::HandleInventoryToggleInput()
{
	if (!IsLocalController())
	{
		return;
	}

	if (bInventoryOverlayOpen)
	{
		CloseInventoryOverlay();
	}
	else
	{
		OpenInventoryOverlay();
	}
}

void ASurvivalPlayerController::OpenInventoryOverlay()
{
	if (bInventoryOverlayOpen)
	{
		return;
	}

	CloseInteractionMenu();
	bInventoryOverlayOpen = true;
	SetIgnoreLookInput(true);
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	if (ASurvivalHud* SurvivalHud = GetHUD<ASurvivalHud>())
	{
		SurvivalHud->SetInventoryOverlayVisible(true);
	}
}

void ASurvivalPlayerController::CloseInventoryOverlay()
{
	if (!bInventoryOverlayOpen)
	{
		return;
	}

	bInventoryOverlayOpen = false;
	SetIgnoreLookInput(false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	if (ASurvivalHud* SurvivalHud = GetHUD<ASurvivalHud>())
	{
		SurvivalHud->SetInventoryOverlayVisible(false);
	}
}

void ASurvivalPlayerController::CloseInteractionMenu()
{
	if (!bInteractionMenuOpen && !bShowMouseCursor)
	{
		return;
	}

	bInteractionMenuOpen = false;
	SetIgnoreLookInput(false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	if (ASurvivalHud* SurvivalHud = GetHUD<ASurvivalHud>())
	{
		SurvivalHud->UpdateInteractionPrompt(FocusedInteractionPrompt, FocusedInteractable.IsValid(), false);
	}
}

void ASurvivalPlayerController::SelectInteractionOption(FName ActionId)
{
	if (!IsLocalController() || !bInteractionMenuOpen || ActionId.IsNone() || !IsValidInteractionTarget(FocusedInteractable.Get()))
	{
		return;
	}

	for (const FInteractionOptionData& Option : FocusedInteractionPrompt.Options)
	{
		if (Option.ActionId == ActionId)
		{
			AActor* TargetActor = FocusedInteractable.Get();
			CloseInteractionMenu();
			ServerInteract(TargetActor, ActionId);
			return;
		}
	}
}

void ASurvivalPlayerController::ServerInteract_Implementation(AActor* TargetActor, FName ActionId)
{
	// This RPC is the server-side entry point; clients may request an interaction but cannot execute it directly.
	if (!HasAuthority())
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction because controller %s has no authority"), *GetName());
		return;
	}

	// Revalidate the client-provided actor before calling any interaction interface methods on it.
	if (!IsValidInteractionTarget(TargetActor))
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction because the target actor is invalid or not interactable"));
		return;
	}

	// Trace again from the server's view so the client cannot interact with an actor it can no longer see or reach.
	FVector ServerViewLocation;
	FHitResult ServerHitResult;
	if (!TraceInteraction(ServerHitResult, ServerViewLocation, FName(TEXT("ServerInteractTrace"))) || ServerHitResult.GetActor() != TargetActor)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction with %s because the server trace did not reach the target"), *TargetActor->GetName());
		return;
	}

	// Use the hit result's measured distance when available; otherwise calculate the distance to the exact impact point.
	const float ServerHitDistance = ServerHitResult.Distance > 0.0f ? ServerHitResult.Distance : FVector::Distance(ServerViewLocation, ServerHitResult.ImpactPoint);
	if (ServerHitDistance > InteractionDistance)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction with %s because the hit surface is out of range"), *TargetActor->GetName());
		return;
	}

	// Recalculate the actor's current interaction data on the server because the client's data or requested action may be stale or forged.
	const FInteractionPromptData ServerPromptData = IInteractable::Execute_GetInteractionPrompt(TargetActor, this);
	if (!ActionId.IsNone())
	{
		// An explicit action is valid only when the target currently advertises that action.
		const bool bActionIsAvailable = ServerPromptData.Options.ContainsByPredicate([ActionId](const FInteractionOptionData& Option) { return Option.ActionId == ActionId; });
		if (!bActionIsAvailable)
		{
			UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected unavailable interaction action %s for %s"), *ActionId.ToString(), *TargetActor->GetName());
			return;
		}
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Server interacting with target actor %s using action %s"), *TargetActor->GetName(), ActionId.IsNone() ? TEXT("Default") : *ActionId.ToString());
	// No action ID preserves the simple interaction path; an explicit ID routes through the option-aware interface method.
	if (ActionId.IsNone())
	{
		IInteractable::Execute_Interact(TargetActor, this);
	}
	else
	{
		IInteractable::Execute_InteractWithAction(TargetActor, this, ActionId);
	}
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
	// A short sphere sweep gives the player forgiving aim without making every pickup's collision volume oversized.
	const FCollisionShape TraceShape = FCollisionShape::MakeSphere(InteractionTraceRadius);
	TArray<FHitResult> HitResults;
	World->SweepMultiByChannel(HitResults, OutViewLocation, TraceEnd, FQuat::Identity, ECC_Visibility, TraceShape, QueryParams);
	const FVector TraceDirection = (TraceEnd - OutViewLocation).GetSafeNormal();
	float BlockingDistance = InteractionDistance;
	for (const FHitResult& HitResult : HitResults)
	{
		if (HitResult.GetActor() && !IsValidInteractionTarget(HitResult.GetActor()))
		{
			BlockingDistance = FMath::Min(BlockingDistance, HitResult.Distance > 0.0f ? HitResult.Distance : FVector::Distance(OutViewLocation, HitResult.ImpactPoint));
		}
	}

	float BestAlignment = -1.0f;
	float BestDistance = BIG_NUMBER;
	AActor* BestTargetActor = nullptr;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* CandidateActor = HitResult.GetActor();
		if (!IsValidInteractionTarget(CandidateActor))
		{
			continue;
		}

		const float CandidateDistance = HitResult.Distance > 0.0f ? HitResult.Distance : FVector::Distance(OutViewLocation, HitResult.ImpactPoint);
		if (CandidateDistance > BlockingDistance + InteractionTraceRadius)
		{
			continue;
		}

		const FVector CandidateDirection = (CandidateActor->GetActorLocation() - OutViewLocation).GetSafeNormal();
		const float CandidateAlignment = FVector::DotProduct(TraceDirection, CandidateDirection);
		if (CandidateAlignment > BestAlignment || (FMath::IsNearlyEqual(CandidateAlignment, BestAlignment) && CandidateDistance < BestDistance))
		{
			BestAlignment = CandidateAlignment;
			BestDistance = CandidateDistance;
			BestTargetActor = CandidateActor;
			OutHitResult = HitResult;
		}
	}

	const bool bSelectedTarget = BestTargetActor != nullptr;
	if (bDrawInteractionTrace && IsLocalController())
	{
		// Draw the swept capsule, which makes the configured radius visible instead of showing only a thin center line.
		const float TraceLength = FVector::Distance(OutViewLocation, TraceEnd);
		const FVector CapsuleCenter = (OutViewLocation + TraceEnd) * 0.5f;
		const FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(TraceDirection).ToQuat();
		const FColor TraceColor = bSelectedTarget ? FColor::Green : FColor::Red;
		const float DrawDuration = FMath::Max(FocusTraceInterval * 1.5f, 0.1f);
		DrawDebugCapsule(World, CapsuleCenter, TraceLength * 0.5f, InteractionTraceRadius, CapsuleRotation, TraceColor, false, DrawDuration, 0, 1.0f);
	}
	return bSelectedTarget;
}
