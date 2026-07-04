// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Abilities/GCSAbility_Dodge.h"
#include "Components/GCSCombatComponent.h"
#include "AbilitySystem/GCSAbilitySystemComponent.h"
#include "AttributeSets/GCSCombatAttributeSet.h"
#include "GCSNativeGameplayTags.h"
#include "GASCombatStarterModule.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"

UGCSAbility_Dodge::UGCSAbility_Dodge()
{
	ActivationPolicy = EGCSAbilityActivationPolicy::OnInput;

	// Default to blocking dodge while staggered -- matches the "can't dodge out
	// of a hit-stagger" convention common to action-combat games. Project can
	// override per-Blueprint-child if a different rule is desired.
	BlockedByTag = GCSGameplayTags::State_Staggered;
}

bool UGCSAbility_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return false;
	}

	// Dead actors never get to dodge, regardless of BlockedByTag configuration.
	if (ASC->HasMatchingGameplayTag(GCSGameplayTags::State_Dead))
	{
		return false;
	}

	// Configurable block tag (defaults to Staggered, but a project could repoint
	// this at e.g. GCS.State.Blocking if blocking and dodging are meant to be
	// mutually exclusive).
	if (BlockedByTag.IsValid() && ASC->HasMatchingGameplayTag(BlockedByTag))
	{
		return false;
	}

	// If a stamina cost is configured, the owner must have at least some Stamina
	// to attempt the dodge. This is a cheap pre-check; CommitAbility (via
	// StaminaCostEffectClass's own Cost definition, if authored as an attribute-
	// cost GE) is still the authoritative gate against exact cost affordability.
	if (StaminaCostEffectClass)
	{
		if (const UGCSCombatAttributeSet* CombatAttributeSet = ASC->GetSet<UGCSCombatAttributeSet>())
		{
			if (CombatAttributeSet->GetStamina() <= 0.f)
			{
				return false;
			}
		}
	}

	return true;
}

void UGCSAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		return;
	}

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Cache activation context so the timer-driven callbacks (which fire outside
	// the normal ActivateAbility/EndAbility call stack and therefore don't
	// receive these as parameters) can still call EndAbility / touch the ASC
	// correctly later.
	CachedAbilityHandle = Handle;
	CachedActorInfo = *ActorInfo;
	CachedActivationInfo = ActivationInfo;

	// 1) Pay the Stamina cost, if configured.
	ApplyStaminaCost();

	// 2) Tag the owner as "in dodge" for the duration of the motion. Using a
	// loose tag (rather than a GameplayEffect-granted tag) keeps this simple
	// and instantaneous -- no GE asset is required just to carry a state tag,
	// and it's trivially removed in EndAbility regardless of how the ability
	// terminates (finished normally vs. cancelled).
	if (UGCSAbilitySystemComponent* GCSASC = GetGCSAbilitySystemComponent())
	{
		GCSASC->AddLooseGameplayTag(GCSGameplayTags::State_InDodge);
	}

	// 3) Schedule the invincibility window to open after InvincibilityStartDelay.
	// A zero/negative delay opens it immediately on the next tick via a 0-second
	// timer, which still defers one frame -- acceptable for this use case and
	// avoids special-casing "immediate" vs "delayed" open logic.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InvincibilityStartTimerHandle,
			this,
			&UGCSAbility_Dodge::OpenInvincibilityWindow,
			FMath::Max(InvincibilityStartDelay, 0.f),
			false);
	}

	// 4) Compute the dodge direction (input-relative roll, or backstep fallback).
	const FVector DodgeDirection = ComputeDodgeDirection();

	// 5) Launch the character along that direction, covering DodgeDistance over
	// DodgeDuration. LaunchCharacter sets velocity directly (in cm/s), so the
	// speed required is simply distance / time.
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		const float DodgeSpeed = DodgeDuration > 0.f ? (DodgeDistance / DodgeDuration) : DodgeDistance;
		const FVector LaunchVelocity = DodgeDirection * DodgeSpeed;

		// bXYOverride/bZOverride = true so this fully replaces existing horizontal
		// velocity (a clean, decisive dodge rather than additive momentum), while
		// leaving vertical velocity alone (false) so falling/gravity still applies
		// normally -- i.e. you can still dodge while airborne without floating.
		Character->LaunchCharacter(LaunchVelocity, /*bXYOverride=*/true, /*bZOverride=*/false);
	}
	else
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbility_Dodge::ActivateAbility: avatar for %s is not an ACharacter; skipping LaunchCharacter."), *GetName());
	}

	// 6) Schedule automatic EndAbility once the dodge motion has finished.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DodgeDurationTimerHandle,
			this,
			&UGCSAbility_Dodge::FinishDodge,
			FMath::Max(DodgeDuration, 0.01f),
			false);
	}
}

void UGCSAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Clear any still-pending timers regardless of why we're ending (normal
	// completion vs. external cancellation) so a cancelled dodge can't leak a
	// delayed OpenInvincibilityWindow/FinishDodge call into a later activation.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InvincibilityStartTimerHandle);
		World->GetTimerManager().ClearTimer(DodgeDurationTimerHandle);
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		if (UGCSAbilitySystemComponent* GCSASC = GetGCSAbilitySystemComponent())
		{
			GCSASC->RemoveLooseGameplayTag(GCSGameplayTags::State_InDodge);
		}

		// If the ability is ending early (e.g. cancelled by a stagger applied
		// mid-dodge) while invincibility is still open, close it immediately
		// rather than leaving the owner invincible for longer than intended.
		if (UGCSCombatComponent* CombatComponent = GetCombatComponent())
		{
			if (CombatComponent->bIsInvincible)
			{
				CombatComponent->EndInvincibility();
			}
		}
	}

	K2_OnDodgeEnd();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FVector UGCSAbility_Dodge::ComputeDodgeDirection() const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return FVector::ForwardVector;
	}

	FVector Direction = FVector::ZeroVector;

	if (bUseInputDirection)
	{
		if (const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			// GetLastInputVector() reflects the most recent consumed movement input
			// direction (set by AddMovementInput), which is a more accurate
			// "which way is the player pressing" signal than instantaneous
			// Velocity (which can be affected by external forces, sliding, etc.).
			//
			// CAVEAT: this vector is NOT replicated -- it is only meaningful on the
			// locally controlled client. Because this ability runs LocalPredicted
			// (see UGCSGameplayAbility), the owning client's activation reads a
			// valid value here, but the server-side execution of the same ability
			// will typically see a zero vector and fall through to the Velocity
			// check below, then ultimately to the backstep fallback. For fully
			// authoritative direction control (e.g. anti-cheat-sensitive projects),
			// replicate the desired dodge direction explicitly (e.g. via the
			// FGameplayEventData / TriggerEventData payload passed into this
			// ability on activation) instead of relying on this helper server-side.
			FVector InputDirection = MovementComponent->GetLastInputVector();
			InputDirection.Z = 0.f;

			if (!InputDirection.IsNearlyZero())
			{
				Direction = InputDirection.GetSafeNormal();
			}
			else
			{
				// No active input -- fall back to current horizontal velocity direction
				// so a dodge triggered mid-slide/mid-run still goes somewhere sensible.
				FVector VelocityDirection = MovementComponent->Velocity;
				VelocityDirection.Z = 0.f;

				if (!VelocityDirection.IsNearlyZero())
				{
					Direction = VelocityDirection.GetSafeNormal();
				}
			}
		}
	}

	// Backstep fallback: no movement component, no input, and no velocity (or
	// bUseInputDirection == false) all resolve here.
	if (Direction.IsNearlyZero())
	{
		Direction = -Character->GetActorForwardVector();
		Direction.Z = 0.f;
		Direction = Direction.GetSafeNormal();
	}

	return Direction;
}

void UGCSAbility_Dodge::ApplyStaminaCost()
{
	if (!StaminaCostEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(StaminaCostEffectClass, GetAbilityLevel(), ContextHandle);
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	else
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbility_Dodge::ApplyStaminaCost: failed to build outgoing spec for %s on %s."), *GetNameSafe(StaminaCostEffectClass), *GetName());
	}
}

void UGCSAbility_Dodge::OpenInvincibilityWindow()
{
	if (UGCSCombatComponent* CombatComponent = GetCombatComponent())
	{
		CombatComponent->BeginInvincibility(InvincibilityDuration);
	}
}

void UGCSAbility_Dodge::FinishDodge()
{
	EndAbility(CachedAbilityHandle, &CachedActorInfo, CachedActivationInfo, true, false);
}
