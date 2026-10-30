#include "Character/SurvivalVitalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Character/SurvivalCharacter.h"
#include "Core/SurvivalGameMode.h"
#include "Core/SurvivalGameState.h"
#include "Core/SurvivalGameplayData.h"
#include "Core/SurvivalPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalVital, Log, All);

USurvivalVitalComponent::USurvivalVitalComponent()
{
	// Components do not use the actor's bReplicates property directly; this marks this component for replication when its owner replicates.
	SetIsReplicatedByDefault(true);
}

void USurvivalVitalComponent::BeginPlay()
{
	// Let the engine initialize the component before we cache owner-dependent state or bind delegates.
	Super::BeginPlay();

	if (ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
	{
		if (CharacterOwner->GetCharacterMovement())
		{
			// Preserve the character's authored movement speed so exhaustion can temporarily apply a multiplier and later restore it.
			NormalMovementSpeed = CharacterOwner->GetCharacterMovement()->MaxWalkSpeed;
		}
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// Only the server initializes and mutates authoritative vital values; clients receive the replicated results.
		InitializeStartingVitals();

		if (ASurvivalGameState* SurvivalGameState = GetWorld()->GetGameState<ASurvivalGameState>())
		{
			// Subscribe to the server's world-clock updates so hunger, hydration, sickness, and health decay with game time.
			SurvivalGameState->OnWorldTimeAdvanced.AddUObject(this, &USurvivalVitalComponent::HandleWorldTimeAdvanced);
		}

		// Apply any starting movement penalty and notify local listeners that the initial values are ready.
		ApplyMovementPenalty();
		BroadcastVitalsChanged();
	}
}

void USurvivalVitalComponent::InitializeStartingVitals()
{
	const USurvivalGameplayData* SurvivalData = GetGameplayData();
	// Characters begin at full health, while hunger and hydration start within the configured ranges.
	CurrentHealth = FMath::Clamp(MaxHealth, 0.0f, MaxHealth);
	CurrentHunger = FMath::FRandRange(SurvivalData->StartingHungerMinimum, SurvivalData->StartingHungerMaximum);
	CurrentHydration = FMath::FRandRange(SurvivalData->StartingHydrationMinimum, SurvivalData->StartingHydrationMaximum);
	// Avoid starting with hunger and hydration effectively tied, which would make the initial state less varied.
	for (int32 Attempt = 0; Attempt < 8 && FMath::Abs(CurrentHunger - CurrentHydration) < 1.0f; ++Attempt)
	{
		// Retry hydration a limited number of times if the random values are within one point of each other.
		CurrentHydration = FMath::FRandRange(SurvivalData->StartingHydrationMinimum, SurvivalData->StartingHydrationMaximum);
	}
	if (FMath::Abs(CurrentHunger - CurrentHydration) < 1.0f)
	{
		// Guarantee a non-tied starting state if all random attempts still produced nearly equal values.
		CurrentHydration = FMath::Clamp(CurrentHunger + 1.0f, SurvivalData->StartingHydrationMinimum, SurvivalData->StartingHydrationMaximum);
		if (FMath::IsNearlyEqual(CurrentHunger, CurrentHydration))
		{
			// If the hydration range prevented that adjustment, move hunger in the opposite direction instead.
			CurrentHunger = FMath::Clamp(CurrentHydration - 1.0f, SurvivalData->StartingHungerMinimum, SurvivalData->StartingHungerMaximum);
		}
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

	const float ClampedHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	if (FMath::IsNearlyEqual(CurrentHealth, ClampedHealth))
	{
		return;
	}

	CurrentHealth = ClampedHealth;
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

	const float ClampedHunger = FMath::Clamp(NewHunger, 0.0f, MaxHunger);
	if (FMath::IsNearlyEqual(CurrentHunger, ClampedHunger))
	{
		return;
	}

	CurrentHunger = ClampedHunger;
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
	// A full hydration meter still counts as a successful hydration effect so the same consumable can provide sickness recovery or other benefits.
	return Amount > 0.0f && (CurrentHydration > PreviousHydration || FMath::IsNearlyEqual(CurrentHydration, MaxHydration));
}

void USurvivalVitalComponent::SetCurrentHydration(const float NewHydration)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const float ClampedHydration = FMath::Clamp(NewHydration, 0.0f, MaxHydration);
	if (FMath::IsNearlyEqual(CurrentHydration, ClampedHydration))
	{
		return;
	}

	CurrentHydration = ClampedHydration;
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

	// Illness severity is stored as points from 0 to 100; the HUD converts the total back to a 0-to-1 percentage.
	const float ClampedSeverity = FMath::Clamp(Severity, 0.0f, 100.0f);
	for (FSurvivalIllnessState& Illness : ActiveIllnesses)
	{
		if (Illness.IllnessTag != IllnessTag)
		{
			// Keep searching until the requested illness tag is found.
			continue;
		}

		// Reapplying an existing illness updates that entry instead of adding a duplicate illness state.
		const float PreviousSeverity = Illness.Severity;
		const float PreviousDuration = Illness.RemainingDurationGameSeconds;
		// Reapplying an illness adds severity, but the normalized severity cannot exceed 100 points.
		Illness.Severity = FMath::Clamp(Illness.Severity + ClampedSeverity, 0.0f, 100.0f);
		// A zero duration remains permanent; otherwise retain the longer of the existing and incoming durations.
		Illness.RemainingDurationGameSeconds = Illness.RemainingDurationGameSeconds <= 0.0f || DurationGameSeconds <= 0.0f ? 0.0f : FMath::Max(Illness.RemainingDurationGameSeconds, DurationGameSeconds);
		const bool bChanged = !FMath::IsNearlyEqual(PreviousSeverity, Illness.Severity) || !FMath::IsNearlyEqual(PreviousDuration, Illness.RemainingDurationGameSeconds);
		if (bChanged)
		{
			BroadcastVitalsChanged();
		}
		return true;
	}

	// No matching illness exists, so create a new active illness state.
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

	// Store recovery as a timed effect instead of removing severity immediately; world-time updates apply it gradually.
	FSurvivalSicknessRecoveryState NewRecovery;
	NewRecovery.RecoveryTags = RecoveryTags;
	// Recovery points use the same 0-to-100 scale as illness severity.
	NewRecovery.RemainingRecoveryPoints = FMath::Clamp(RecoveryPoints, 0.0f, 100.0f);
	// Spread the available recovery points evenly across the requested game-time duration.
	NewRecovery.RecoveryRatePointsPerGameSecond = NewRecovery.RemainingRecoveryPoints / DurationGameSeconds;
	ActiveSicknessRecoveries.Add(MoveTemp(NewRecovery));
	// The effect was accepted and will be processed by ApplySicknessRecovery during future time updates.
	return true;
}

bool USurvivalVitalComponent::TryCureIllnesses(const FGameplayTagContainer& CureTags)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || CureTags.IsEmpty())
	{
		return false;
	}

	const int32 PreviousCount = ActiveIllnesses.Num();
	// Remove every active illness whose gameplay tag is covered by the supplied cure tags.
	ActiveIllnesses.RemoveAll([&CureTags](const FSurvivalIllnessState& Illness) { return CureTags.HasTag(Illness.IllnessTag); });
	if (PreviousCount == ActiveIllnesses.Num())
	{
		// No illness was removed, so there is no state change to replicate or broadcast.
		return false;
	}

	ActiveSicknessRecoveries.RemoveAll([this](const FSurvivalSicknessRecoveryState& Recovery) { return !HasMatchingIllness(Recovery.RecoveryTags); });
	// At least one illness was removed, so notify the HUD and other listeners.
	BroadcastVitalsChanged();
	return true;
}

float USurvivalVitalComponent::GetSicknessPercent() const
{
	// Convert the 0-to-100 severity scale into the normalized 0-to-1 value used by the HUD.
	return FMath::Clamp(GetSicknessSeverityTotal() / 100.0f, 0.0f, 1.0f);
}

void USurvivalVitalComponent::NotifyJumpActivity()
{
	if (const USurvivalGameplayData* SurvivalData = GetGameplayData())
	{
		// Jumping consumes the shared one-time activity cost from the survival rules asset.
		ApplyActivityCost(SurvivalData->OneTimeActivityCost);
	}
}

void USurvivalVitalComponent::NotifyPunchActivity()
{
	if (const USurvivalGameplayData* SurvivalData = GetGameplayData())
	{
		// Punching currently uses the same one-time activity cost as jumping.
		ApplyActivityCost(SurvivalData->OneTimeActivityCost);
	}
}

FSurvivalVitalSnapshot USurvivalVitalComponent::CaptureSnapshot() const
{
	FSurvivalVitalSnapshot Snapshot;
	// Copy all mutable vital state, including illnesses, so a failed transaction can be rolled back exactly.
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

	// Restore the complete server-side state in one operation instead of triggering separate setter broadcasts.
	CurrentHealth = FMath::Clamp(Snapshot.Health, 0.0f, MaxHealth);
	CurrentHunger = FMath::Clamp(Snapshot.Hunger, 0.0f, MaxHunger);
	CurrentHydration = FMath::Clamp(Snapshot.Hydration, 0.0f, MaxHydration);
	ActiveIllnesses = Snapshot.ActiveIllnesses;
	ActiveSicknessRecoveries = Snapshot.ActiveSicknessRecoveries;
	// Keep the death guard consistent with the restored health value.
	bHasHandledDeath = CurrentHealth <= 0.0f;
	// Recalculate derived movement state and notify listeners once after the full rollback is complete.
	ApplyMovementPenalty();
	BroadcastVitalsChanged();
}

void USurvivalVitalComponent::ApplyWorldTimeDelta(const float InElapsedGameSeconds)
{
	// Only the authoritative living character can process time-based survival effects.
	if (!GetOwner() || !GetOwner()->HasAuthority() || InElapsedGameSeconds <= 0.0f || bHasHandledDeath)
	{
		return;
	}

	const USurvivalGameplayData* SurvivalData = GetGameplayData();
	// Ensure the day length is a safe divisor even if the configured value is invalid.
	const float GameDayDuration = FMath::Max(SurvivalData->GameDayDurationSeconds, 1.0f);
	// Moving increases the amount of hunger and hydration drained during this time slice.
	const float ActivityMultiplier = GetActivityMultiplier();
	// Convert elapsed time into the fraction of a game day that should be drained this update.
	const float DrainScale = InElapsedGameSeconds / GameDayDuration * ActivityMultiplier;
	const float PreviousHunger = CurrentHunger;
	const float PreviousHydration = CurrentHydration;
	bool bSicknessChanged = ApplySicknessRecovery(InElapsedGameSeconds);
	// Apply each vital's configured per-day drain, clamping at zero so values cannot become negative.
	CurrentHunger = FMath::Clamp(CurrentHunger - SurvivalData->HungerDrainPerGameDay * DrainScale, 0.0f, MaxHunger);
	CurrentHydration = FMath::Clamp(CurrentHydration - SurvivalData->HydrationDrainPerGameDay * DrainScale, 0.0f, MaxHydration);

	// Iterate backward because expired illnesses may be removed from the array while it is being traversed.
	for (int32 Index = ActiveIllnesses.Num() - 1; Index >= 0; --Index)
	{
		FSurvivalIllnessState& Illness = ActiveIllnesses[Index];
		if (Illness.RemainingDurationGameSeconds > 0.0f)
		{
			// Timed illnesses count down with world time; a zero duration remains permanent until cured.
			Illness.RemainingDurationGameSeconds = FMath::Max(0.0f, Illness.RemainingDurationGameSeconds - InElapsedGameSeconds);
			if (Illness.RemainingDurationGameSeconds <= 0.0f)
			{
				// Remove the illness once its countdown reaches zero.
				ActiveIllnesses.RemoveAt(Index);
				bSicknessChanged = true;
			}
		}
	}
	ActiveSicknessRecoveries.RemoveAll([this](const FSurvivalSicknessRecoveryState& Recovery) { return !HasMatchingIllness(Recovery.RecoveryTags); });

	// Count how many tracked vitals are empty so each empty vital can contribute damage.
	const int32 EmptyVitalCount = (CurrentHunger <= 0.0f ? 1 : 0) + (CurrentHydration <= 0.0f ? 1 : 0);
	// Empty-vital damage uses the same per-day scaling as hunger and hydration drain.
	const float EmptyVitalDamage = EmptyVitalCount * SurvivalData->EmptyVitalHealthDamagePerGameDay * DrainScale;
	// Sickness damage uses normalized total illness severity and the fraction of a game day that elapsed.
	const float SicknessDamage = GetSicknessSeverityTotal() / 100.0f * SurvivalData->SicknessHealthDamagePerGameDay * InElapsedGameSeconds / GameDayDuration;
	const float PreviousHealth = CurrentHealth;
	// Apply both damage sources together, keeping health within its valid range.
	CurrentHealth = FMath::Clamp(CurrentHealth - EmptyVitalDamage - SicknessDamage, 0.0f, MaxHealth);

	// Hunger or hydration reaching zero may change the character's movement speed.
	ApplyMovementPenalty();
	if (CurrentHealth <= 0.0f)
	{
		// Transition the character into the handled-death state once health reaches zero.
		HandleDeath();
	}

	// Notify the HUD when a tracked value changed, damage was processed, or gradual recovery changed sickness.
	if (!FMath::IsNearlyEqual(PreviousHunger, CurrentHunger) || !FMath::IsNearlyEqual(PreviousHydration, CurrentHydration) || !FMath::IsNearlyEqual(PreviousHealth, CurrentHealth) || EmptyVitalCount > 0 || SicknessDamage > 0.0f || bSicknessChanged)
	{
		BroadcastVitalsChanged();
	}
}

bool USurvivalVitalComponent::ApplySicknessRecovery(const float InElapsedGameSeconds)
{
	bool bChanged = false;
	// Process recovery effects backward because completed effects may be removed from the array.
	for (int32 RecoveryIndex = ActiveSicknessRecoveries.Num() - 1; RecoveryIndex >= 0; --RecoveryIndex)
	{
		FSurvivalSicknessRecoveryState& Recovery = ActiveSicknessRecoveries[RecoveryIndex];
		// Limit this update to both the recovery points remaining and the rate for the elapsed time slice.
		float RemainingToApply = FMath::Min(Recovery.RemainingRecoveryPoints, Recovery.RecoveryRatePointsPerGameSecond * InElapsedGameSeconds);
		const float RequestedRecovery = RemainingToApply;
		// Distribute this update across every matching active illness until the recovery budget is exhausted.
		for (int32 IllnessIndex = ActiveIllnesses.Num() - 1; IllnessIndex >= 0 && RemainingToApply > KINDA_SMALL_NUMBER; --IllnessIndex)
		{
			FSurvivalIllnessState& Illness = ActiveIllnesses[IllnessIndex];
			if (!Recovery.RecoveryTags.HasTag(Illness.IllnessTag))
			{
				// This recovery effect does not apply to the current illness tag.
				continue;
			}

			// Do not apply recovery beyond the portion of this illness that exists during the current time slice.
			const float AvailableSeconds = Illness.RemainingDurationGameSeconds > 0.0f ? FMath::Min(InElapsedGameSeconds, Illness.RemainingDurationGameSeconds) : InElapsedGameSeconds;
			const float AvailableRecovery = Recovery.RecoveryRatePointsPerGameSecond * AvailableSeconds;
			// Limit recovery by the remaining budget, available time, and illness severity.
			const float AppliedRecovery = FMath::Min3(RemainingToApply, AvailableRecovery, Illness.Severity);
			if (AppliedRecovery <= 0.0f)
			{
				continue;
			}

			// Reduce severity by the amount applied and remove the illness when it is effectively cured.
			Illness.Severity = FMath::Max(0.0f, Illness.Severity - AppliedRecovery);
			RemainingToApply -= AppliedRecovery;
			bChanged = true;
			if (Illness.Severity <= KINDA_SMALL_NUMBER)
			{
				ActiveIllnesses.RemoveAt(IllnessIndex);
			}
		}

		// Consume only the recovery points that were actually applied this update.
		Recovery.RemainingRecoveryPoints = FMath::Max(0.0f, Recovery.RemainingRecoveryPoints - (RequestedRecovery - RemainingToApply));
		// Stop tracking recovery when its budget is spent or no matching illness remains.
		if (Recovery.RemainingRecoveryPoints <= KINDA_SMALL_NUMBER || !HasMatchingIllness(Recovery.RecoveryTags))
		{
			ActiveSicknessRecoveries.RemoveAt(RecoveryIndex);
		}
	}

	return bChanged;
}

void USurvivalVitalComponent::ApplyActivityCost(const float InCost)
{
	// Activity costs are authoritative, must be positive, and stop once this character has handled death.
	if (!GetOwner() || !GetOwner()->HasAuthority() || InCost <= 0.0f || bHasHandledDeath)
	{
		return;
	}

	const USurvivalGameplayData* SurvivalData = GetGameplayData();
	// Remember whether either vital was already empty before this activity occurred.
	const bool bWasHungerEmpty = CurrentHunger <= 0.0f;
	const bool bWasHydrationEmpty = CurrentHydration <= 0.0f;
	// FMath::Max selects the larger value, so subtracting the cost cannot take either vital below zero.
	CurrentHunger = FMath::Max(CurrentHunger - InCost, 0.0f);
	CurrentHydration = FMath::Max(CurrentHydration - InCost, 0.0f);
	if (bWasHungerEmpty || bWasHydrationEmpty)
	{
		// Continuing activity while already dehydrated or hungry causes the configured immediate health damage.
		CurrentHealth = FMath::Max(CurrentHealth - SurvivalData->EmptyVitalActivityHealthDamage, 0.0f);
	}

	// Recalculate exhaustion movement speed after the activity cost changes the vitals.
	ApplyMovementPenalty();
	if (CurrentHealth <= 0.0f)
	{
		// Handle death immediately if the activity damage depleted health.
		HandleDeath();
	}
	// Activity costs are immediate events, so broadcast the resulting state once.
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
