// Copyright Fallen Signal Studios. All Rights Reserved.

// -----------------------------------------------------------------------
// PATCH NOTE -- New GameplayTags introduced by this file / the Hit React
// feature set. These must exist in Config/DefaultGASCombatStarterTags.ini
// (and, for tags C++ needs to reference directly, in
// Source/GASCombatStarter/Public/GCSNativeGameplayTags.h) -- both have
// already been updated to include them as part of this change:
//
//   GCS.HitReact.Front            (UGCSAbility_HitReact directional montage key)
//   GCS.HitReact.Back             (UGCSAbility_HitReact directional montage key)
//   GCS.HitReact.Left             (UGCSAbility_HitReact directional montage key)
//   GCS.HitReact.Right            (UGCSAbility_HitReact directional montage key)
//   GameplayCue.GCS.HitImpact     (this class's own cue tag -- burst hit VFX/SFX/camera shake)
//   GameplayCue.GCS.CriticalHit   (companion cue broadcast for crits -- see OnExecute_Implementation)
//
// IMPORTANT: GameplayCue tags MUST be rooted at "GameplayCue." -- this is a
// hard requirement of UAbilitySystemComponent/UGameplayCueManager, which only
// route tags under that root to GameplayCue Notify classes at all (tags like
// "GCS.HitImpact" would silently never fire, regardless of how this class is
// tagged). The "GCS." segment beneath that root is just this plugin's
// namespace to avoid clashing with other plugins' cues.
// -----------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GCSGameplayCueNotify_HitImpact.generated.h"

class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;

/**
 * UGCSGameplayCueNotify_HitImpact
 *
 * Static (stateless, non-actor-spawning) GameplayCueNotify that provides the
 * standard "burst" cosmetic feedback for a landed hit: impact VFX, impact
 * sound, and an optional camera shake for the instigating player. This is
 * deliberately a UGameplayCueNotify_Static rather than
 * UGameplayCueNotify_Actor because hit impacts are fire-and-forget --
 * there is no ongoing state to track for the duration of anything, so a
 * pooled actor per-cue would be pure overhead.
 *
 * Cue tag: GameplayCue.GCS.HitImpact (see the patch note above -- the
 * "GameplayCue." root is mandatory). Set this class's GameplayCueTag property
 * (inherited from UGameplayCueNotify_Static, editable in the Class Defaults
 * panel of any Blueprint child) to GameplayCue.GCS.HitImpact.
 *
 * Triggering: fire this cue from gameplay code with
 * UAbilitySystemComponent::ExecuteGameplayCue(Tag, Parameters) or
 * K2_ExecuteGameplayCueWithParams (Blueprint), or via
 * UGCSGameplayCueManager::ExecuteGameplayCueOnActor for a convenience
 * wrapper that also works on actors without their own ASC. Typical call
 * site: UGCSAbility_MeleeAttack::OnHitConfirmed_Implementation, immediately
 * after (or instead of, if you don't need gameplay-affecting behavior)
 * sending the GCS.Event.HitReact GameplayEvent.
 *
 * As with all "Burst" cues (OnExecute only, no OnActive/OnRemove/WhileActive
 * behavior needed), keep bAutoDestroyOnRemove/duration semantics as their
 * class defaults -- Static cue notifies without OnActive/WhileActive overrides
 * naturally behave as instantaneous bursts.
 */
UCLASS(Blueprintable, meta = (DisplayName = "GCS Hit Impact Cue (Static)"))
class GASCOMBATSTARTER_API UGCSGameplayCueNotify_HitImpact : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGCSGameplayCueNotify_HitImpact();

	//~ Begin UGameplayCueNotify_Static interface
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	//~ End UGameplayCueNotify_Static interface

	/** Niagara VFX spawned at the hit location (Parameters.Location) when this cue executes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitImpact|VFX")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	/** Sound played at the hit location when this cue executes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitImpact|Audio")
	TObjectPtr<USoundBase> ImpactSound;

	/** Optional camera shake applied to the hit instigator's local player camera. Left unset = no camera shake (common for hits the local player receives rather than deals, if you don't want self-shake on both ends). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitImpact|Camera")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	/** Scale applied to HitCameraShake when started (maps to UGameplayStatics::PlayerCameraManager::StartCameraShake's Scale parameter). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitImpact|Camera")
	float CameraShakeScale = 0.5f;

	/**
	 * RawMagnitude threshold above which this cue also broadcasts
	 * GameplayCue.GCS.CriticalHit (see the pattern documented on
	 * OnExecute_Implementation below). Parameters.RawMagnitude is typically
	 * populated from the GameplayEffectSpec's captured magnitude when the cue
	 * is triggered via an executed GameplayEffect (see
	 * UGCSExecCalc_Damage) -- if your call site sets RawMagnitude to the
	 * final damage dealt, this acts as a simple "big hit" crit-FX threshold.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|HitImpact|CriticalHit")
	float CritThreshold = 50.f;
};
