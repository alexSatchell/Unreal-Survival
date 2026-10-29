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
	return !FMath::IsNearlyEqual(PreviousHydration, CurrentHydration);
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

	// Keep illness severity within the normalized range used by the vitals UI and sickness calculations.
	const float ClampedSeverity = FMath::Clamp(Severity, 0.0f, 1.0f);
	for (FSurvivalIllnessState& Illness : ActiveIllnesses)
	{
		if (Illness.IllnessTag != IllnessTag)
		{
			// This is a different illness, so continue searching for an existing instance of the requested tag.
			continue;
		}

		// Reapplying an existing illness updates that entry instead of adding a duplicate illness state.
		const float PreviousSeverity = Illness.Severity;
		const float PreviousDuration = Illness.RemainingDurationGameSeconds;
		// Reapplying an illness can increase its severity, but should never weaken the existing illness.
		Illness.Severity = FMath::Max(Illness.Severity, ClampedSeverity);
		// A duration of zero means the illness persists until cured; if either duration is permanent, keep it permanent.
		const bool bIsPermanent = Illness.RemainingDurationGameSeconds <= 0.0f || DurationGameSeconds <= 0.0f;
		Illness.RemainingDurationGameSeconds = bIsPermanent ? 0.0f : FMath::Max(Illness.RemainingDurationGameSeconds, DurationGameSeconds);
		// Only notify listeners when reapplying the illness changed its severity or remaining duration.
		const bool bChanged = !FMath::IsNearlyEqual(PreviousSeverity, Illness.Severity) || !FMath::IsNearlyEqual(PreviousDuration, Illness.RemainingDurationGameSeconds);
		if (bChanged)
		{
			BroadcastVitalsChanged();
		}
		return bChanged;
	}

	// No matching tag exists, so create the first active state for this illness.
	FSurvivalIllnessState NewIllness;
	NewIllness.IllnessTag = IllnessTag;
	NewIllness.Severity = ClampedSeverity;
	NewIllness.RemainingDurationGameSeconds = FMath::Max(DurationGameSeconds, 0.0f);
	ActiveIllnesses.Add(NewIllness);
	BroadcastVitalsChanged();
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

	BroadcastVitalsChanged();
	return true;
}

float USurvivalVitalComponent::GetSicknessPercent() const
{
	return FMath::Clamp(GetSicknessSeverityTotal(), 0.0f, 1.0f);
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
			}
		}
	}

	const int32 EmptyVitalCount = (CurrentHunger <= 0.0f ? 1 : 0) + (CurrentHydration <= 0.0f ? 1 : 0);
	const float EmptyVitalDamage = EmptyVitalCount * SurvivalData->EmptyVitalHealthDamagePerGameDay * DrainScale;
	const float SicknessDamage = GetSicknessSeverityTotal() * SurvivalData->SicknessHealthDamagePerGameDay * InElapsedGameSeconds / GameDayDuration;
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - EmptyVitalDamage - SicknessDamage, 0.0f, MaxHealth);

	ApplyMovementPenalty();
	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}

	if (!FMath::IsNearlyEqual(PreviousHunger, CurrentHunger) || !FMath::IsNearlyEqual(PreviousHydration, CurrentHydration) || !FMath::IsNearlyEqual(PreviousHealth, CurrentHealth) || EmptyVitalCount > 0 || SicknessDamage > 0.0f)
	{
		BroadcastVitalsChanged();
	}
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
