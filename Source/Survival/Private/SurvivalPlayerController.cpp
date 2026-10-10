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
#include "SurvivalPlayerState.h"

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
	}
	else
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Enhanced Input component is unavailable for %s"), *GetName());
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
