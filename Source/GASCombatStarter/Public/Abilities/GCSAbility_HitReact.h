// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GCSGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GCSAbility_HitReact.generated.h"

class UAnimMontage;
class UAnimInstance;
class UGameplayEffect;
class UGCSCombatComponent;

/**
 * UGCSAbility_HitReact
 *
 * Receiving-side counterpart to UGCSAbility_MeleeAttack::ApplyDamageToTargets.
 * That ability sends a GCS.Event.HitReact GameplayEvent (via
 * SendGameplayEventToActor) to whatever it hits; this ability is what turns
 * that event into an actual directional hit-react montage, and escalates into
 * a stagger state if the owner is being hit faster than it can recover.
 *
 * -----------------------------------------------------------------------
 * GRANTING / TRIGGERING (must be configured on the Ability Spec, not just
 * this class):
 *
 *   ActivationPolicy is set to EGCSAbilityActivationPolicy::OnEvent purely as
 *   documentation/intent -- GAS itself does not read ActivationPolicy to
 *   decide activation. To actually make this ability respond to
 *   GCS.Event.HitReact, it MUST be granted with a matching entry in the
 *   FGameplayAbilitySpec::Ability's AbilityTriggers array (inherited from
 *   UGameplayAbility, EditDefaultsOnly on the CDO). Concretely, in the
 *   Blueprint (or C++ constructor of a subclass) that owns the ability
 *   default object, add to AbilityTriggers:
 *
 *       FAbilityTriggerData TriggerData;
 *       TriggerData.TriggerTag = GCSGameplayTags::Event_HitReact; // "GCS.Event.HitReact"
 *       TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
 *       AbilityTriggers.Add(TriggerData);
 *
 *   This is normally done once on the ability's CDO (i.e. in this class's
 *   constructor, see GCSAbility_HitReact.cpp) so every project that grants
 *   this ability gets correct event-triggering for free without needing to
 *   remember to configure it per-instance in the editor. It is still safe
 *   (and harmless) to also add the same trigger entry from the Blueprint
 *   Class Defaults panel -- AbilityTriggers is just an array, duplicate
 *   identical entries are a no-op in practice since GAS matches on tag+source.
 *
 *   The ability must also be granted to the target's UAbilitySystemComponent
 *   ahead of time (e.g. via UGCSCombatConfig / InitializeFromCombatConfig, or
 *   an explicit GiveAbility call) -- GameplayEvents only trigger abilities
 *   that are already granted; they cannot grant an ability on the fly.
 * -----------------------------------------------------------------------
 *
 * Directional selection: the ability compares the instigator's location
 * against the owner's forward/right vectors to classify the hit as
 * Front/Back/Left/Right (see GetHitDirectionTag), then looks up the
 * corresponding montage in HitDirectionMontages. If no entry matches (empty
 * map, missing tag, or TriggerEventData->Instigator is null), it falls back
 * to DefaultHitReactMontage.
 *
 * Stagger escalation: every activation calls
 * UGCSCombatComponent::RecordHitTaken() and locally tracks how many hits have
 * landed within the trailing StaggerWindow seconds. If that count exceeds
 * StaggerThreshold, the ability treats this as a "juggled"/overwhelmed state:
 * it cancels whatever hit-react montage is currently playing, optionally
 * applies HitStaggerEffectClass (e.g. a GE that grants GCS.State.Staggered
 * and blocks input), and calls UGCSCombatComponent::BeginStagger(StaggerDuration).
 *
 * This class does not clear cooldown/cost -- hit reacts are not activated via
 * player input and are not expected to have a cost/cooldown GameplayEffect,
 * but CommitAbility is deliberately NOT called here so nothing blocks this
 * ability from re-triggering rapidly (which is exactly the scenario the
 * stagger-escalation logic above is designed to detect and handle).
 */
UCLASS(Abstract, Blueprintable)
class GASCOMBATSTARTER_API UGCSAbility_HitReact : public UGCSGameplayAbility
{
	GENERATED_BODY()

public:
	UGCSAbility_HitReact();

	//~ Begin UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility interface

	// ----------------------------------------------------------------------
	// Montage selection
	// ----------------------------------------------------------------------

	/**
	 * Maps a hit-direction tag (GCS.HitReact.Front / Back / Left / Right) to
	 * the montage that should play for a hit coming from that direction.
	 * Populate all four for full directional coverage; any missing entries
	 * fall back to DefaultHitReactMontage.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitReact|Montage", meta = (Categories = "GCS.HitReact"))
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> HitDirectionMontages;

	/** Fallback montage used when HitDirectionMontages has no entry for the computed direction tag (or the instigator is unknown). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitReact|Montage")
	TObjectPtr<UAnimMontage> DefaultHitReactMontage;

	// ----------------------------------------------------------------------
	// Stagger escalation tuning
	// ----------------------------------------------------------------------

	/** Number of hits within StaggerWindow seconds required to trigger a stagger. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitReact|Stagger")
	float StaggerThreshold = 3.f;

	/** Trailing time window, in seconds, over which hits are counted toward StaggerThreshold. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitReact|Stagger")
	float StaggerWindow = 1.5f;

	/** Duration, in seconds, of the stagger state applied via UGCSCombatComponent::BeginStagger when the threshold is exceeded. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitReact|Stagger")
	float StaggerDuration = 0.6f;

	/**
	 * Optional GameplayEffect applied to the owner when a stagger triggers.
	 * Typically a duration GE that grants GCS.State.Staggered and/or applies
	 * a movement-speed or damage-taken modifier for the stagger's duration.
	 * If unset, only UGCSCombatComponent::BeginStagger's boolean state is used.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitReact|Stagger")
	TSubclassOf<UGameplayEffect> HitStaggerEffectClass;

	/**
	 * Computes which directional hit-react tag best matches the given
	 * instigator's position relative to the owning avatar's facing.
	 *
	 * Method: builds the 2D (horizontal-plane) vector from the owner to the
	 * instigator, normalizes it, and dot-products it against the owner's
	 * forward and right vectors. The axis (forward/back vs left/right) with
	 * the larger absolute dot product wins; the sign of that dot product then
	 * picks the specific direction (e.g. positive Forward-dot => Front,
	 * negative => Back).
	 *
	 * Returns an empty (invalid) FGameplayTag if Instigator is null, is at
	 * the exact same location as the owner (zero-length direction vector), or
	 * the owning avatar cannot be resolved -- callers should treat an invalid
	 * return as "use DefaultHitReactMontage".
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GCS|HitReact")
	FGameplayTag GetHitDirectionTag(AActor* Instigator) const;

protected:
	/** Selects and plays the appropriate hit-react montage for TriggerEventData->Instigator, returning the montage that was played (or nullptr if none). */
	UAnimMontage* PlayHitReactMontage(const FGameplayEventData* TriggerEventData);

	/**
	 * Records this hit's timestamp locally (in HitTimestamps) and returns true
	 * if the number of hits recorded within the trailing StaggerWindow seconds
	 * (including this one) exceeds StaggerThreshold.
	 */
	bool ShouldTriggerStagger();

	/** Cancels the currently-playing hit-react montage (if any), applies HitStaggerEffectClass, and begins the stagger state on the owner's UGCSCombatComponent. */
	void TriggerStagger();

	/** Bound to the active montage's OnMontageEnded/OnMontageBlendingOut so this ability ends itself once the hit-react animation finishes (or is interrupted). */
	UFUNCTION()
	void OnHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Cached handles/info captured at ActivateAbility time, since the montage-ended delegate does not receive them as parameters. */
	FGameplayAbilitySpecHandle CachedAbilityHandle;
	FGameplayAbilityActorInfo CachedActorInfo;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	/** The montage instance currently playing for this activation, if any. Tracked so it can be explicitly stopped when a stagger pre-empts it. */
	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentlyPlayingMontage;

	/**
	 * Rolling log of world-time timestamps (GetWorld()->GetTimeSeconds()) for
	 * hits taken, trimmed to StaggerWindow seconds each activation. Kept on
	 * the ability instance (InstancingPolicy == InstancedPerActor, inherited
	 * from UGCSGameplayAbility) so it naturally resets per-owner and does not
	 * need explicit replication -- stagger evaluation only needs to be
	 * correct on the authoritative side (server), matching where
	 * UGCSCombatComponent::BeginStagger already asserts HasAuthority().
	 */
	TArray<float> HitTimestamps;
};
