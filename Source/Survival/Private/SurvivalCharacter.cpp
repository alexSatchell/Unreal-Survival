#include "SurvivalCharacter.h"
#include "EnhancedInputComponent.h"
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
		ServerUseItem(FirstSurvivalItemInstance.InstanceId);
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

	ServerDropItem(FirstSurvivalItemInstance.InstanceId);
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

	USurvivalItemDefinition* SurvivalItemDefinition = SurvivalInventoryComponent->FindItemDefinitionByInstanceId(ItemInstanceId);
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

	const FVector DropLocation = GetActorLocation() + GetActorForwardVector() * 100.0f;
	FSurvivalItemInstance RemovedSurvivalItemInstance;
	if (!SurvivalInventoryComponent->TryRemoveItem(ItemInstanceId, RemovedSurvivalItemInstance))
	{
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected drop because the inventory no longer contained the requested item for %s"), *GetName());
		return;
	}

	const FTransform DropTransform(GetActorRotation(), DropLocation);
	APickup* DroppedPickup = World->SpawnActorDeferred<APickup>(APickup::StaticClass(), DropTransform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!DroppedPickup)
	{
		SurvivalInventoryComponent->TryAddItem(RemovedSurvivalItemInstance, SurvivalItemDefinition);
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Rejected drop because the pickup could not spawn for %s"), *GetName());
		return;
	}

	if (!DroppedPickup->InitializeItemInstance(RemovedSurvivalItemInstance, SurvivalItemDefinition))
	{
		DroppedPickup->Destroy();
		SurvivalInventoryComponent->TryAddItem(RemovedSurvivalItemInstance, SurvivalItemDefinition);
		UE_LOG(LogSurvivalCharacter, Error, TEXT("Rejected drop because the spawned pickup could not be initialized"));
		return;
	}

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

	USurvivalItemDefinition* SurvivalItemDefinition = SurvivalInventoryComponent->FindItemDefinitionByInstanceId(ItemInstanceId);
	const USurvivalConsumableItemDefinition* ConsumableItemDefinition = Cast<USurvivalConsumableItemDefinition>(SurvivalItemDefinition);
	if (!ConsumableItemDefinition || ConsumableItemDefinition->HydrationRestoration <= 0.0f)
	{
		UE_LOG(LogSurvivalCharacter, Log, TEXT("Rejected use because item instance %s is not a hydrating consumable"), *ItemInstanceId.ToString());
		return;
	}

	const float HydrationRestoration = ConsumableItemDefinition->HydrationRestoration;
	const float PreviousHydration = SurvivalVitalComponent->CurrentHydration;
	if (!SurvivalVitalComponent->TryRestoreHydration(HydrationRestoration))
	{
		UE_LOG(LogSurvivalCharacter, Log, TEXT("Rejected use of %s because hydration is already full"), *ConsumableItemDefinition->GetName());
		return;
	}

	FSurvivalItemInstance RemovedSurvivalItemInstance;
	if (!SurvivalInventoryComponent->TryRemoveItem(ItemInstanceId, RemovedSurvivalItemInstance))
	{
		SurvivalVitalComponent->SetCurrentHydration(PreviousHydration);
		UE_LOG(LogSurvivalCharacter, Warning, TEXT("Use of %s applied hydration but could not remove item instance %s"), *ConsumableItemDefinition->GetName(), *ItemInstanceId.ToString());
		return;
	}

	UE_LOG(LogSurvivalCharacter, Log, TEXT("Used %s; hydration restored by %.1f for %s"), *ConsumableItemDefinition->GetName(), HydrationRestoration, *GetName());
}

void ASurvivalCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogSurvivalCharacter, Log, TEXT("Character %s was possessed by controller %s"), *GetName(), NewController ? *NewController->GetName() : TEXT("None"));
}
