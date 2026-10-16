#include "SurvivalVitalComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalVital, Log, All);

USurvivalVitalComponent::USurvivalVitalComponent()
{
	SetIsReplicatedByDefault(true);
}

void USurvivalVitalComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CurrentHydration = FMath::Clamp(CurrentHydration, 0.0f, MaxHydration);
	}
}

bool USurvivalVitalComponent::TryRestoreHydration(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.0f)
	{
		return false;
	}

	const float PreviousHydration = CurrentHydration;
	const float NewHydration = FMath::Clamp(CurrentHydration + Amount, 0.0f, MaxHydration);
	if (FMath::IsNearlyEqual(NewHydration, PreviousHydration))
	{
		return false;
	}

	SetCurrentHydration(NewHydration);
	UE_LOG(LogSurvivalVital, Log, TEXT("Hydration changed for %s from %.1f to %.1f"), *GetOwner()->GetName(), PreviousHydration, CurrentHydration);
	return true;
}

void USurvivalVitalComponent::SetCurrentHydration(float NewHydration)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CurrentHydration = FMath::Clamp(NewHydration, 0.0f, MaxHydration);
}

float USurvivalVitalComponent::GetHydrationPercent() const
{
	return MaxHydration > 0.0f ? CurrentHydration / MaxHydration : 0.0f;
}

void USurvivalVitalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USurvivalVitalComponent, MaxHydration);
	DOREPLIFETIME(USurvivalVitalComponent, CurrentHydration);
}
