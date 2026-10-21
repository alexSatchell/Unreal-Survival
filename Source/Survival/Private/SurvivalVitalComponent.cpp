#include "SurvivalVitalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "SurvivalCharacter.h"
#include "SurvivalGameMode.h"
#include "SurvivalGameState.h"
#include "SurvivalGameplayData.h"
#include "SurvivalPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalVital, Log, All);

USurvivalVitalComponent::USurvivalVitalComponent()
{
	SetIsReplicatedByDefault(true);
}

void USurvivalVitalComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
	{
		if (CharacterOwner->GetCharacterMovement())
		{
			NormalMovementSpeed = CharacterOwner->GetCharacterMovement()->MaxWalkSpeed;
		}
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		const USurvivalGameplayData* SurvivalData = GetGameplayData();
		CurrentHealth = FMath::Clamp(MaxHealth, 0.0f, MaxHealth);
		CurrentHunger = FMath::FRandRange(SurvivalData->StartingHungerMinimum, SurvivalData->StartingHungerMaximum);
		CurrentHydration = FMath::FRandRange(SurvivalData->StartingHydrationMinimum, SurvivalData->StartingHydrationMaximum);
		for (int32 Attempt = 0; Attempt < 8 && FMath::Abs(CurrentHunger - CurrentHydration) < 1.0f; ++Attempt)
		{
			CurrentHydration = FMath::FRandRange(SurvivalData->StartingHydrationMinimum, SurvivalData->StartingHydrationMaximum);
		}
		if (FMath::Abs(CurrentHunger - CurrentHydration) < 1.0f)
		{
			CurrentHydration = FMath::Clamp(CurrentHunger + 1.0f, SurvivalData->StartingHydrationMinimum, SurvivalData->StartingHydrationMaximum);
			if (FMath::IsNearlyEqual(CurrentHunger, CurrentHydration))
			{
				CurrentHunger = FMath::Clamp(CurrentHydration - 1.0f, SurvivalData->StartingHungerMinimum, SurvivalData->StartingHungerMaximum);
			}
		}

		if (ASurvivalGameState* SurvivalGameState = GetWorld()->GetGameState<ASurvivalGameState>())
		{
			SurvivalGameState->OnWorldTimeAdvanced.AddUObject(this, &USurvivalVitalComponent::HandleWorldTimeAdvanced);
		}

		ApplyMovementPenalty();
		BroadcastVitalsChanged();
	}
}

void USurvivalVitalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ASurvivalGameState* SurvivalGameState = World->GetGameState<ASurvivalGameState>())
		{
			SurvivalGameState->OnWorldTimeAdvanced.RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool USurvivalVitalComponent::TryApplyHealthDelta(const float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || FMath::IsNearlyZero(Amount))
	{
		return false;
	}

	const float PreviousHealth = CurrentHealth;
	SetCurrentHealth(CurrentHealth + Amount);
	return !FMath::IsNearlyEqual(PreviousHealth, CurrentHealth);
}

void USurvivalVitalComponent::SetCurrentHealth(const float NewHealth)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}
	BroadcastVitalsChanged();
}

float USurvivalVitalComponent::GetHealthPercent() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

bool USurvivalVitalComponent::TryRestoreHunger(const float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.0f)
	{
		return false;
	}

	const float PreviousHunger = CurrentHunger;
	SetCurrentHunger(CurrentHunger + Amount);
	return !FMath::IsNearlyEqual(PreviousHunger, CurrentHunger);
}

void USurvivalVitalComponent::SetCurrentHunger(const float NewHunger)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CurrentHunger = FMath::Clamp(NewHunger, 0.0f, MaxHunger);
	ApplyMovementPenalty();
	BroadcastVitalsChanged();
}

float USurvivalVitalComponent::GetHungerPercent() const
{
	return MaxHunger > 0.0f ? CurrentHunger / MaxHunger : 0.0f;
}

bool USurvivalVitalComponent::TryRestoreHydration(const float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.0f)
	{
		return false;
	}

	const float PreviousHydration = CurrentHydration;
	SetCurrentHydration(CurrentHydration + Amount);
	return Amount > 0.0f && (CurrentHydration > PreviousHydration || FMath::IsNearlyEqual(CurrentHydration, MaxHydration));
}

void USurvivalVitalComponent::SetCurrentHydration(const float NewHydration)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CurrentHydration = FMath::Clamp(NewHydration, 0.0f, MaxHydration);
	ApplyMovementPenalty();
	BroadcastVitalsChanged();
}

float USurvivalVitalComponent::GetHydrationPercent() const
{
	return MaxHydration > 0.0f ? CurrentHydration / MaxHydration : 0.0f;
}

bool USurvivalVitalComponent::TryApplyIllness(const FGameplayTag IllnessTag, const float Severity, const float DurationGameSeconds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IllnessTag.IsValid() || Severity <= 0.0f)
	{
		return false;
	}

	const float ClampedSeverity = FMath::Clamp(Severity, 0.0f, 100.0f);
	for (FSurvivalIllnessState& Illness : ActiveIllnesses)
	{
		if (Illness.IllnessTag != IllnessTag)
		{
			continue;
		}

		const float PreviousSeverity = Illness.Severity;
		const float PreviousDuration = Illness.RemainingDurationGameSeconds;
		Illness.Severity = FMath::Max(Illness.Severity, ClampedSeverity);
		Illness.RemainingDurationGameSeconds = Illness.RemainingDurationGameSeconds <= 0.0f || DurationGameSeconds <= 0.0f ? 0.0f : FMath::Max(Illness.RemainingDurationGameSeconds, DurationGameSeconds);
		const bool bChanged = !FMath::IsNearlyEqual(PreviousSeverity, Illness.Severity) || !FMath::IsNearlyEqual(PreviousDuration, Illness.RemainingDurationGameSeconds);
		if (bChanged)
		{
			BroadcastVitalsChanged();
		}
		return true;
	}

	FSurvivalIllnessState NewIllness;
	NewIllness.IllnessTag = IllnessTag;
	NewIllness.Severity = ClampedSeverity;
	NewIllness.RemainingDurationGameSeconds = FMath::Max(DurationGameSeconds, 0.0f);
	ActiveIllnesses.Add(NewIllness);
	BroadcastVitalsChanged();
	return true;
}

bool USurvivalVitalComponent::TryApplySicknessRecovery(const FGameplayTagContainer& RecoveryTags, const float RecoveryPoints, const float DurationGameSeconds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || RecoveryTags.IsEmpty() || RecoveryPoints <= 0.0f || DurationGameSeconds <= 0.0f || !HasMatchingIllness(RecoveryTags))
	{
		return false;
	}

	FSurvivalSicknessRecoveryState NewRecovery;
	NewRecovery.RecoveryTags = RecoveryTags;
	NewRecovery.RemainingRecoveryPoints = FMath::Clamp(RecoveryPoints, 0.0f, 100.0f);
	NewRecovery.RecoveryRatePointsPerGameSecond = NewRecovery.RemainingRecoveryPoints / DurationGameSeconds;
	ActiveSicknessRecoveries.Add(MoveTemp(NewRecovery));
	return true;
}

bool USurvivalVitalComponent::TryCureIllnesses(const FGameplayTagContainer& CureTags)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || CureTags.IsEmpty())
	{
		return false;
	}

	const int32 PreviousCount = ActiveIllnesses.Num();
	ActiveIllnesses.RemoveAll([&CureTags](const FSurvivalIllnessState& Illness) { return CureTags.HasTag(Illness.IllnessTag); });
	if (PreviousCount == ActiveIllnesses.Num())
	{
		return false;
	}

	ActiveSicknessRecoveries.RemoveAll([this](const FSurvivalSicknessRecoveryState& Recovery) { return !HasMatchingIllness(Recovery.RecoveryTags); });
	BroadcastVitalsChanged();
	return true;
}

float USurvivalVitalComponent::GetSicknessPercent() const
{
	return FMath::Clamp(GetSicknessSeverityTotal() / 100.0f, 0.0f, 1.0f);
}

void USurvivalVitalComponent::NotifyJumpActivity()
{
	if (const USurvivalGameplayData* SurvivalData = GetGameplayData())
	{
		ApplyActivityCost(SurvivalData->OneTimeActivityCost);
	}
}

void USurvivalVitalComponent::NotifyPunchActivity()
{
	if (const USurvivalGameplayData* SurvivalData = GetGameplayData())
	{
		ApplyActivityCost(SurvivalData->OneTimeActivityCost);
	}
}

FSurvivalVitalSnapshot USurvivalVitalComponent::CaptureSnapshot() const
{
	FSurvivalVitalSnapshot Snapshot;
	Snapshot.Health = CurrentHealth;
	Snapshot.Hunger = CurrentHunger;
	Snapshot.Hydration = CurrentHydration;
	Snapshot.ActiveIllnesses = ActiveIllnesses;
	Snapshot.ActiveSicknessRecoveries = ActiveSicknessRecoveries;
	return Snapshot;
}

void USurvivalVitalComponent::RestoreSnapshot(const FSurvivalVitalSnapshot& Snapshot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CurrentHealth = FMath::Clamp(Snapshot.Health, 0.0f, MaxHealth);
	CurrentHunger = FMath::Clamp(Snapshot.Hunger, 0.0f, MaxHunger);
	CurrentHydration = FMath::Clamp(Snapshot.Hydration, 0.0f, MaxHydration);
	ActiveIllnesses = Snapshot.ActiveIllnesses;
	ActiveSicknessRecoveries = Snapshot.ActiveSicknessRecoveries;
	bHasHandledDeath = CurrentHealth <= 0.0f;
	ApplyMovementPenalty();
	BroadcastVitalsChanged();
}

void USurvivalVitalComponent::ApplyWorldTimeDelta(const float InElapsedGameSeconds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || InElapsedGameSeconds <= 0.0f || bHasHandledDeath)
	{
		return;
	}

	const USurvivalGameplayData* SurvivalData = GetGameplayData();
	const float GameDayDuration = FMath::Max(SurvivalData->GameDayDurationSeconds, 1.0f);
	const float ActivityMultiplier = GetActivityMultiplier();
	const float DrainScale = InElapsedGameSeconds / GameDayDuration * ActivityMultiplier;
	const float PreviousHunger = CurrentHunger;
	const float PreviousHydration = CurrentHydration;
	bool bSicknessChanged = ApplySicknessRecovery(InElapsedGameSeconds);
	CurrentHunger = FMath::Clamp(CurrentHunger - SurvivalData->HungerDrainPerGameDay * DrainScale, 0.0f, MaxHunger);
	CurrentHydration = FMath::Clamp(CurrentHydration - SurvivalData->HydrationDrainPerGameDay * DrainScale, 0.0f, MaxHydration);

	for (int32 Index = ActiveIllnesses.Num() - 1; Index >= 0; --Index)
	{
		FSurvivalIllnessState& Illness = ActiveIllnesses[Index];
		if (Illness.RemainingDurationGameSeconds > 0.0f)
		{
			Illness.RemainingDurationGameSeconds = FMath::Max(0.0f, Illness.RemainingDurationGameSeconds - InElapsedGameSeconds);
			if (Illness.RemainingDurationGameSeconds <= 0.0f)
			{
				ActiveIllnesses.RemoveAt(Index);
				bSicknessChanged = true;
			}
		}
	}
	ActiveSicknessRecoveries.RemoveAll([this](const FSurvivalSicknessRecoveryState& Recovery) { return !HasMatchingIllness(Recovery.RecoveryTags); });

	const int32 EmptyVitalCount = (CurrentHunger <= 0.0f ? 1 : 0) + (CurrentHydration <= 0.0f ? 1 : 0);
	const float EmptyVitalDamage = EmptyVitalCount * SurvivalData->EmptyVitalHealthDamagePerGameDay * DrainScale;
	const float SicknessDamage = GetSicknessSeverityTotal() / 100.0f * SurvivalData->SicknessHealthDamagePerGameDay * InElapsedGameSeconds / GameDayDuration;
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - EmptyVitalDamage - SicknessDamage, 0.0f, MaxHealth);

	ApplyMovementPenalty();
	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}

	if (!FMath::IsNearlyEqual(PreviousHunger, CurrentHunger) || !FMath::IsNearlyEqual(PreviousHydration, CurrentHydration) || !FMath::IsNearlyEqual(PreviousHealth, CurrentHealth) || EmptyVitalCount > 0 || SicknessDamage > 0.0f || bSicknessChanged)
	{
		BroadcastVitalsChanged();
	}
}

bool USurvivalVitalComponent::ApplySicknessRecovery(const float InElapsedGameSeconds)
{
	bool bChanged = false;
	for (int32 RecoveryIndex = ActiveSicknessRecoveries.Num() - 1; RecoveryIndex >= 0; --RecoveryIndex)
	{
		FSurvivalSicknessRecoveryState& Recovery = ActiveSicknessRecoveries[RecoveryIndex];
		float RemainingToApply = FMath::Min(Recovery.RemainingRecoveryPoints, Recovery.RecoveryRatePointsPerGameSecond * InElapsedGameSeconds);
		const float RequestedRecovery = RemainingToApply;
		for (int32 IllnessIndex = ActiveIllnesses.Num() - 1; IllnessIndex >= 0 && RemainingToApply > KINDA_SMALL_NUMBER; --IllnessIndex)
		{
			FSurvivalIllnessState& Illness = ActiveIllnesses[IllnessIndex];
			if (!Recovery.RecoveryTags.HasTag(Illness.IllnessTag))
			{
				continue;
			}

			const float AvailableSeconds = Illness.RemainingDurationGameSeconds > 0.0f ? FMath::Min(InElapsedGameSeconds, Illness.RemainingDurationGameSeconds) : InElapsedGameSeconds;
			const float AvailableRecovery = Recovery.RecoveryRatePointsPerGameSecond * AvailableSeconds;
			const float AppliedRecovery = FMath::Min3(RemainingToApply, AvailableRecovery, Illness.Severity);
			if (AppliedRecovery <= 0.0f)
			{
				continue;
			}

			Illness.Severity = FMath::Max(0.0f, Illness.Severity - AppliedRecovery);
			RemainingToApply -= AppliedRecovery;
			bChanged = true;
			if (Illness.Severity <= KINDA_SMALL_NUMBER)
			{
				ActiveIllnesses.RemoveAt(IllnessIndex);
			}
		}

		Recovery.RemainingRecoveryPoints = FMath::Max(0.0f, Recovery.RemainingRecoveryPoints - (RequestedRecovery - RemainingToApply));
		if (Recovery.RemainingRecoveryPoints <= KINDA_SMALL_NUMBER || !HasMatchingIllness(Recovery.RecoveryTags))
		{
			ActiveSicknessRecoveries.RemoveAt(RecoveryIndex);
		}
	}

	return bChanged;
}

void USurvivalVitalComponent::ApplyActivityCost(const float InCost)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || InCost <= 0.0f || bHasHandledDeath)
	{
		return;
	}

	const USurvivalGameplayData* SurvivalData = GetGameplayData();
	const bool bWasHungerEmpty = CurrentHunger <= 0.0f;
	const bool bWasHydrationEmpty = CurrentHydration <= 0.0f;
	CurrentHunger = FMath::Max(CurrentHunger - InCost, 0.0f);
	CurrentHydration = FMath::Max(CurrentHydration - InCost, 0.0f);
	if (bWasHungerEmpty || bWasHydrationEmpty)
	{
		CurrentHealth = FMath::Max(CurrentHealth - SurvivalData->EmptyVitalActivityHealthDamage, 0.0f);
	}

	ApplyMovementPenalty();
	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}
	BroadcastVitalsChanged();
}

void USurvivalVitalComponent::BroadcastVitalsChanged()
{
	OnVitalsChanged.Broadcast(GetHealthPercent(), GetHungerPercent(), GetHydrationPercent(), GetSicknessPercent());
}

void USurvivalVitalComponent::ApplyMovementPenalty()
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* MovementComponent = CharacterOwner ? CharacterOwner->GetCharacterMovement() : nullptr;
	if (!MovementComponent || NormalMovementSpeed <= 0.0f)
	{
		return;
	}

	const USurvivalGameplayData* SurvivalData = GetGameplayData();
	const bool bIsExhausted = CurrentHunger <= 0.0f || CurrentHydration <= 0.0f;
	MovementComponent->MaxWalkSpeed = NormalMovementSpeed * (bIsExhausted ? SurvivalData->ExhaustedMovementSpeedMultiplier : 1.0f);
}

void USurvivalVitalComponent::HandleWorldTimeAdvanced(const float InElapsedGameSeconds)
{
	ApplyWorldTimeDelta(InElapsedGameSeconds);
}

void USurvivalVitalComponent::HandleDeath()
{
	if (bHasHandledDeath)
	{
		return;
	}

	bHasHandledDeath = true;
	if (ASurvivalCharacter* SurvivalCharacter = Cast<ASurvivalCharacter>(GetOwner()))
	{
		if (ASurvivalPlayerState* SurvivalPlayerState = Cast<ASurvivalPlayerState>(SurvivalCharacter->GetPlayerState()))
		{
			SurvivalPlayerState->SetIsAlive(false);
		}

		if (UCharacterMovementComponent* MovementComponent = SurvivalCharacter->GetCharacterMovement())
		{
			MovementComponent->DisableMovement();
		}
	}

	UE_LOG(LogSurvivalVital, Log, TEXT("Character %s died because health reached zero"), GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}

float USurvivalVitalComponent::GetActivityMultiplier() const
{
	const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* MovementComponent = CharacterOwner ? CharacterOwner->GetCharacterMovement() : nullptr;
	if (!MovementComponent || MovementComponent->Velocity.SizeSquared2D() <= FMath::Square(5.0f))
	{
		return 1.0f;
	}

	const USurvivalGameplayData* SurvivalData = GetGameplayData();
	const float RunningSpeedThreshold = NormalMovementSpeed * 0.75f;
	return MovementComponent->Velocity.Size2D() >= RunningSpeedThreshold ? SurvivalData->RunningActivityMultiplier : SurvivalData->WalkingActivityMultiplier;
}

float USurvivalVitalComponent::GetSicknessSeverityTotal() const
{
	float TotalSeverity = 0.0f;
	for (const FSurvivalIllnessState& Illness : ActiveIllnesses)
	{
		TotalSeverity += Illness.Severity;
	}

	return TotalSeverity;
}

bool USurvivalVitalComponent::HasMatchingIllness(const FGameplayTagContainer& RecoveryTags) const
{
	for (const FSurvivalIllnessState& Illness : ActiveIllnesses)
	{
		if (Illness.Severity > KINDA_SMALL_NUMBER && RecoveryTags.HasTag(Illness.IllnessTag))
		{
			return true;
		}
	}

	return false;
}

const USurvivalGameplayData* USurvivalVitalComponent::GetGameplayData() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const ASurvivalGameMode* SurvivalGameMode = Cast<ASurvivalGameMode>(World->GetAuthGameMode()))
		{
			return SurvivalGameMode->GetGameplayData();
		}
	}

	return GetDefault<USurvivalGameplayData>();
}

void USurvivalVitalComponent::OnRep_CurrentHealth()
{
	BroadcastVitalsChanged();
}

void USurvivalVitalComponent::OnRep_CurrentHunger()
{
	ApplyMovementPenalty();
	BroadcastVitalsChanged();
}

void USurvivalVitalComponent::OnRep_CurrentHydration()
{
	ApplyMovementPenalty();
	BroadcastVitalsChanged();
}

void USurvivalVitalComponent::OnRep_ActiveIllnesses()
{
	BroadcastVitalsChanged();
}

void USurvivalVitalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USurvivalVitalComponent, MaxHealth);
	DOREPLIFETIME(USurvivalVitalComponent, CurrentHealth);
	DOREPLIFETIME(USurvivalVitalComponent, MaxHunger);
	DOREPLIFETIME(USurvivalVitalComponent, CurrentHunger);
	DOREPLIFETIME(USurvivalVitalComponent, MaxHydration);
	DOREPLIFETIME(USurvivalVitalComponent, CurrentHydration);
	DOREPLIFETIME(USurvivalVitalComponent, ActiveIllnesses);
}
