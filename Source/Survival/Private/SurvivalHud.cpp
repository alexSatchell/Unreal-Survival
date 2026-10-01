#include "SurvivalHud.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalHud, Log, All);

void ASurvivalHud::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogSurvivalHud, Log, TEXT("HUD %s began play for owning player controller %s"), *GetName(), GetOwningPlayerController() ? *GetOwningPlayerController()->GetName() : TEXT("None"));
}
