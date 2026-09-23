#include "SurvivalPlayerController.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Interfaces/Interactable.h"
#include "SurvivalPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalPlayerController, Log, All);

void ASurvivalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ASurvivalPlayerController::HandleReadyInput);
		InputComponent->BindKey(EKeys::I, IE_Pressed, this, &ASurvivalPlayerController::HandleInteractInput);
	}
}

void ASurvivalPlayerController::HandleReadyInput()
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Local player pressed R to become ready"));
	ServerSetReady();
}

void ASurvivalPlayerController::HandleInteractInput()
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Local player pressed I to interact"));

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * 1000.0f;
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InteractTrace), true, GetPawn());

	if (!GetWorld() || !GetWorld()->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		UE_LOG(LogSurvivalPlayerController, Log, TEXT("Interaction trace found no actor"));
		return;
	}

	AActor* TargetActor = HitResult.GetActor();
	if (!TargetActor || !TargetActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		UE_LOG(LogSurvivalPlayerController, Log, TEXT("Interaction trace hit a non-interactable actor"));
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Interaction trace found interactable actor %s"), *TargetActor->GetName());
	ServerInteract(TargetActor);
}

void ASurvivalPlayerController::ServerInteract_Implementation(AActor* TargetActor)
{
	if (!HasAuthority())
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction because controller %s has no authority"), *GetName());
		return;
	}

	if (!IsValid(TargetActor))
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction because the target actor is invalid"));
		return;
	}

	if (TargetActor->GetWorld() != GetWorld())
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction with %s because it is in a different world"), *TargetActor->GetName());
		return;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Rejected interaction with %s because it does not implement Interactable"), *TargetActor->GetName());
		return;
	}

	UE_LOG(LogSurvivalPlayerController, Log, TEXT("Server interacting with target actor %s"), *TargetActor->GetName());
	IInteractable::Execute_Interact(TargetActor, this);
}

void ASurvivalPlayerController::ServerSetReady_Implementation()
{
	ASurvivalPlayerState* SurvivalPlayerState = GetPlayerState<ASurvivalPlayerState>();
	if (!SurvivalPlayerState)
	{
		UE_LOG(LogSurvivalPlayerController, Warning, TEXT("Could not set player ready because SurvivalPlayerState is unavailable"));
		return;
	}

	SurvivalPlayerState->SetIsReady(true);
}
