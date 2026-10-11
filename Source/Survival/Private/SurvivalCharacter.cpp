#include "SurvivalCharacter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"
#include "SurvivalInventoryComponent.h"

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
}

USurvivalInventoryComponent* ASurvivalCharacter::GetInventoryComponent() const
{
	return FindComponentByClass<USurvivalInventoryComponent>();
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

void ASurvivalCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogSurvivalCharacter, Log, TEXT("Character %s was possessed by controller %s"), *GetName(), NewController ? *NewController->GetName() : TEXT("None"));
}
