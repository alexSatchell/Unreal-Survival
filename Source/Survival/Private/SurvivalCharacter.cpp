#include "SurvivalCharacter.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalCharacter, Log, All);

void ASurvivalCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	const TCHAR* ControllerName = NewController ? *NewController->GetName() : TEXT("None");
	UE_LOG(LogSurvivalCharacter, Log, TEXT("Character %s was possessed by controller %s"), *GetName(), ControllerName);
}
