#include "SurvivalCharacter.h"
#include "CollisionQueryParams.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"
#include "Pickup.h"
#include "SurvivalInventoryComponent.h"
#include "SurvivalConsumableItemDefinition.h"
#include "SurvivalItemDefinition.h"
#include "SurvivalVitalComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalCharacter, Log, All);

ASurvivalCharacter::ASurvivalCharacter()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);
	CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	CameraComponent->bUsePawnControlRotation = true;

	InventoryComponent = CreateDefaultSubobject<USurvivalInventoryComponent>(TEXT("InventoryComponent"));
	VitalComponent = CreateDefaultSubobject<USurvivalVitalComponent>(TEXT("VitalComponent"));
}

USurvivalInventoryComponent* ASurvivalCharacter::GetInventoryComponent() const
{
	return FindComponentByClass<USurvivalInventoryComponent>();
}

USurvivalVitalComponent* ASurvivalCharacter::GetVitalComponent() const
{
	return FindComponentByClass<USurvivalVitalComponent>();
}

void ASurvivalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::HandleMove);
		}
		else
		{
			UE_LOG(LogSurvivalCharacter, Warning, TEXT("Enhanced Input MoveAction is not configured for %s"), *GetName());
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::HandleLook);
		}
		else
		{
			UE_LOG(LogSurvivalCharacter, Warning, TEXT("Enhanced Input LookAction is not configured for %s"), *GetName());
		}

		if (DropAction)
		{
			EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &ASurvivalCharacter::HandleDropInput);
		}
		else
		{
			UE_LOG(LogSurvivalCharacter, Warning, TEXT("Enhanced Input DropAction is not configured for %s"), *GetName());
		}

		if (UseItemAction)
		{
			EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Started, this, &ASurvivalCharacter::HandleUseItemInput);
		}
		else
		{
			UE_LOG(LogSurvivalCharacter, Warning, TEXT("Enhanced Input UseItemAction is not configured for %s"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Enhanced Input component is unavailable for %s"), *GetName());
	}
}

void ASurvivalCharacter::HandleMove(const FInputActionValue& InputActionValue)
{
	const FVector2D MovementVector = InputActionValue.Get<FVector2D>();
	const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ASurvivalCharacter::HandleLook(const FInputActionValue& InputActionValue)
{
	const FVector2D LookVector = InputActionValue.Get<FVector2D>();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->AddYawInput(LookVector.X);
		PlayerController->AddPitchInput(LookVector.Y);
	}
}

void ASurvivalCharacter::HandleUseItemInput()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	const USurvivalInventoryComponent* SurvivalInventoryComponent = GetInventoryComponent();
	if (!SurvivalInventoryComponent || SurvivalInventoryComponent->Items.IsEmpty())
	{
		UE_LOG(LogSurvivalCharacter, Log, TEXT("Use input ignored because the inventory is empty"));
		return;
	}

	const FSurvivalItemInstance& FirstSurvivalItemInstance = SurvivalInventoryComponent->Items[0];
	if (FirstSurvivalItemInstance.IsValid())
	{
		RequestUseItem(FirstSurvivalItemInstance.InstanceId);
	}
}

bool ASurvivalCharacter::TryApplyConsumableEffects(const USurvivalConsumableItemDefinition* ConsumableItemDefinition)
{
	if (!HasAuthority() || !ConsumableItemDefinition || !VitalComponent)
	{
		return false;
	}

	bool bAppliedEffect = false;
	bAppliedEffect |= VitalComponent->TryRestoreHydration(ConsumableItemDefinition->HydrationRestoration);
	bAppliedEffect |= VitalComponent->TryRestoreHunger(ConsumableItemDefinition->HungerRestoration);
	bAppliedEffect |= VitalComponent->TryApplyHealthDelta(ConsumableItemDefinition->HealthEffect);
	bAppliedEffect |= VitalComponent->TryApplyIllness(ConsumableItemDefinition->SicknessTag, ConsumableItemDefinition->SicknessSeverity, ConsumableItemDefinition->SicknessDurationGameSeconds);
	bAppliedEffect |= VitalComponent->TryCureIllnesses(ConsumableItemDefinition->SicknessCureTags);
	return bAppliedEffect;
}

void ASurvivalCharacter::RequestUseItem(const FGuid& ItemInstanceId)
{
	if (IsLocallyControlled() && ItemInstanceId.IsValid())
	{
		ServerUseItem(ItemInstanceId);
	}
}

void ASurvivalCharacter::RequestDropItem(const FGuid& ItemInstanceId)
{
	if (IsLocallyControlled() && ItemInstanceId.IsValid())
	{
		ServerDropItem(ItemInstanceId);
	}
}

void ASurvivalCharacter::HandleDropInput()
{
	if (!IsLocallyControlled())
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Drop input rejected because the character is not locally controlled"));
		return;
	}

	const USurvivalInventoryComponent* SurvivalInventoryComponent = GetInventoryComponent();
	if (!SurvivalInventoryComponent || SurvivalInventoryComponent->Items.IsEmpty())
	{
		UE_LOG(LogSurvivalCharacter, Log, TEXT("Drop input ignored because the replicated inventory is empty"));
		return;
	}

	const FSurvivalItemInstance& FirstSurvivalItemInstance = SurvivalInventoryComponent->Items[0];
	if (!FirstSurvivalItemInstance.IsValid())
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Drop input rejected because the first inventory item is invalid"));
		return;
	}

	RequestDropItem(FirstSurvivalItemInstance.InstanceId);
}

void ASurvivalCharacter::ServerDropItem_Implementation(FGuid ItemInstanceId)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!ItemInstanceId.IsValid())
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected drop because the request contained an invalid item instance ID"));
		return;
	}

	USurvivalInventoryComponent* SurvivalInventoryComponent = GetInventoryComponent();
	if (!SurvivalInventoryComponent)
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected drop because the character has no inventory component"));
		return;
	}

	FSurvivalItemInstance SurvivalItemInstance;
	if (!SurvivalInventoryComponent->FindItemByInstanceId(ItemInstanceId, SurvivalItemInstance))
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected drop because item instance %s is not in the inventory"), *ItemInstanceId.ToString());
		return;
	}

	USurvivalItemDefinition* SurvivalItemDefinition = SurvivalInventoryComponent->FindDefinitionAssetByInstanceId(ItemInstanceId);
	if (!IsValid(SurvivalItemDefinition))
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected drop because the server definition for item instance %s is missing"), *ItemInstanceId.ToString());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	constexpr float DropForwardDistance = 100.0f;
	constexpr float GroundTraceUpDistance = 200.0f;
	constexpr float GroundTraceDownDistance = 400.0f;
	const FVector DropTraceOrigin = GetActorLocation() + GetActorForwardVector() * DropForwardDistance;
	const FVector GroundTraceStart = DropTraceOrigin + FVector(0.0f, 0.0f, GroundTraceUpDistance);
	const FVector GroundTraceEnd = DropTraceOrigin - FVector(0.0f, 0.0f, GroundTraceDownDistance);
	// Ignore this character and trace only world-static geometry so the item is placed on the nearby ground.
	FCollisionQueryParams GroundTraceParams(FName(TEXT("SurvivalDropGroundTrace")), false, this);
	FHitResult GroundHit;
	if (!World->LineTraceSingleByChannel(GroundHit, GroundTraceStart, GroundTraceEnd, ECC_WorldStatic, GroundTraceParams))
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected drop because no ground was found in front of %s"), *GetName());
		return;
	}

	// The interaction box may have padding, so never use it to place the item on the floor.
	// Use the visible mesh bounds so its lowest point rests on the ground hit instead.
	float MeshBottomOffset = 0.0f;
	if (SurvivalItemDefinition->PickupMesh)
	{
		// Convert the mesh bounds into the vertical offset from its pivot to its lowest visible point.
		const FBoxSphereBounds PickupMeshBounds = SurvivalItemDefinition->PickupMesh->GetBounds();
		MeshBottomOffset = PickupMeshBounds.Origin.Z - PickupMeshBounds.BoxExtent.Z;
	}

	// The trace gives us the surface point; compensate for the mesh pivot so the item does not float or sink.
	FVector DropLocation = GroundHit.ImpactPoint;
	DropLocation.Z -= MeshBottomOffset;

	// Remove the item only after finding a valid drop location; later spawn failures will restore this instance.
	FSurvivalItemInstance RemovedSurvivalItemInstance;
	if (!SurvivalInventoryComponent->TryRemoveItem(ItemInstanceId, RemovedSurvivalItemInstance))
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected drop because the inventory no longer contained the requested item for %s"), *GetName());
		return;
	}

	// Preserve the character's rotation while placing the pickup at the corrected ground location.
	const FTransform DropTransform(GetActorRotation(), DropLocation);
	// Defer spawning so the item instance can be initialized before the pickup finishes its actor lifecycle.
	APickup* DroppedPickup = World->SpawnActorDeferred<APickup>(APickup::StaticClass(), DropTransform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!DroppedPickup)
	{
		// The inventory was already changed, so restore the item when no pickup actor was created.
		SurvivalInventoryComponent->TryAddItem(RemovedSurvivalItemInstance, SurvivalItemDefinition);
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected drop because the pickup could not spawn for %s"), *GetName());
		return;
	}

	if (!DroppedPickup->InitializeItemInstance(RemovedSurvivalItemInstance, SurvivalItemDefinition))
	{
		// Destroy the partially spawned actor and restore the inventory item if initialization rejects it.
		DroppedPickup->Destroy();
		SurvivalInventoryComponent->TryAddItem(RemovedSurvivalItemInstance, SurvivalItemDefinition);
		UE_LOG(LogSurvivalCharacter, Error, TEXT("Rejected drop because the spawned pickup could not be initialized"));
		return;
	}

	// Finish the deferred spawn and request prompt replication immediately for connected clients.
	DroppedPickup->FinishSpawning(DropTransform);
	DroppedPickup->ForceNetUpdate();
	UE_LOG(LogSurvivalCharacter, Log, TEXT("Dropped item instance %s (%s) from %s"), *ItemInstanceId.ToString(), *SurvivalItemDefinition->GetName(), *GetName());
}

void ASurvivalCharacter::ServerUseItem_Implementation(FGuid ItemInstanceId)
{
	if (!HasAuthority())
	{
		return;
	}

	USurvivalInventoryComponent* SurvivalInventoryComponent = GetInventoryComponent();
	USurvivalVitalComponent* SurvivalVitalComponent = GetVitalComponent();
	if (!SurvivalInventoryComponent || !SurvivalVitalComponent || !ItemInstanceId.IsValid())
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected use request because the inventory, vital component, or item instance is invalid"));
		return;
	}

	FSurvivalItemInstance SurvivalItemInstance;
	if (!SurvivalInventoryComponent->FindItemByInstanceId(ItemInstanceId, SurvivalItemInstance))
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected use because item instance %s is not in the inventory"), *ItemInstanceId.ToString());
		return;
	}

	USurvivalItemDefinition* SurvivalItemDefinition = SurvivalInventoryComponent->FindDefinitionAssetByInstanceId(ItemInstanceId);
	const USurvivalConsumableItemDefinition* ConsumableItemDefinition = Cast<USurvivalConsumableItemDefinition>(SurvivalItemDefinition);
	if (!ConsumableItemDefinition)
	{
		UE_LOG(LogSurvivalCharacter, Log, TEXT("Rejected use because item instance %s is not a consumable"), *ItemInstanceId.ToString());
		return;
	}

	const FSurvivalVitalSnapshot PreviousVitalState = SurvivalVitalComponent->CaptureSnapshot();
	if (!TryApplyConsumableEffects(ConsumableItemDefinition))
	{
		UE_LOG(LogSurvivalCharacter, Log, TEXT("Rejected use of %s because it produced no vital change"), *ConsumableItemDefinition->GetName());
		return;
	}

	FSurvivalItemInstance RemovedSurvivalItemInstance;
	if (!SurvivalInventoryComponent->TryRemoveItem(ItemInstanceId, RemovedSurvivalItemInstance))
	{
		SurvivalVitalComponent->RestoreSnapshot(PreviousVitalState);
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Use of %s applied hydration but could not remove item instance %s"), *ConsumableItemDefinition->GetName(), *ItemInstanceId.ToString());
		return;
	}

	UE_LOG(LogSurvivalCharacter, Log, TEXT("Used %s for %s"), *ConsumableItemDefinition->GetName(), *GetName());
}

void ASurvivalCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogSurvivalCharacter, Log, TEXT("Character %s was possessed by controller %s"), *GetName(), NewController ? *NewController->GetName() : TEXT("None"));
}

void ASurvivalCharacter::Jump()
{
	Super::Jump();
	if (VitalComponent && GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		VitalComponent->NotifyJumpActivity();
	}
}
