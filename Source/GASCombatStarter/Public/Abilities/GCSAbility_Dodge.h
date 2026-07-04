// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GCSGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GCSAbility_Dodge.generated.h"

class UGameplayEffect;

/**
 * UGCSAbility_Dodge
 *
 * Dodge / evade ability with an i-frame (invincibility) window. On activation
 * the owner is launched a short distance in either their current input
 * direction (roll/dash-style dodge) or straight backward (backstep-style
 * dodge), and becomes briefly invincible to incoming damage partway through
 * the motion -- mirroring the "windup, then invincible, then recovery" pacing
 * common to action-combat dodges (Souls-likes, character actions games, etc.).
 *
 * Lifecycle:
 *   1. CanActivateAbility gates the dodge behind BlockedByTag / GCS.State.Dead
 *      / available Stamina.
 *   2. ActivateAbility commits the Stamina cost, tags the owner with
 *      GCS.State.InDodge, schedules the invincibility window to open after
 *      InvincibilityStartDelay seconds (via UGCSCombatComponent::BeginInvincibility),
 *      computes a dodge direction, and launches the character.
 *   3. After DodgeDuration seconds, EndAbility fires automatically -- clearing
 *      GCS.State.InDodge and notifying Blueprint via K2_OnDodgeEnd.
 *
 * Invincibility itself lives on UGCSCombatComponent (bIsInvincible / Begin-
 * Invincibility / EndInvincibility -- see GCSCombatComponent_DodgePatch.h for
 * the required additions to that class) rather than on this ability, so any
 * damage-pipeline code only needs to check one boolean regardless of which
 * ability/source granted invincibility.
 *
 * This class does not play montages itself, matching the pattern established
 * by UGCSAbility_MeleeAttack -- root motion / montage-driven dodges should be
 * implemented in a Blueprint child (or a C++ subclass using
 * UAbilityTask_PlayMontageAndWait) that calls Super::ActivateAbility and then
 * layers montage playback on top. The LaunchCharacter-based movement here is a
 * simple, working default suitable for capsule-based dodges without root motion.
 */
UCLASS(Abstract, Blueprintable)
class GASCOMBATSTARTER_API UGCSAbility_Dodge : public UGCSGameplayAbility
{
	GENERATED_BODY()

public:
	UGCSAbility_Dodge();

	//~ Begin UGameplayAbility interface
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility interface

	// ----------------------------------------------------------------------
	// Tuning
	// ----------------------------------------------------------------------

	/** How far, in Unreal units, the dodge travels over DodgeDuration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Dodge")
	float DodgeDistance = 400.f;

	/** Total duration, in seconds, of the dodge motion. The ability auto-ends after this elapses. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Dodge")
	float DodgeDuration = 0.35f;

	/** How long, in seconds, the invincibility (i-frame) window stays open. Should be <= DodgeDuration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Dodge|Invincibility")
	float InvincibilityDuration = 0.25f;

	/**
	 * Delay, in seconds, after activation before the invincibility window opens.
	 * Modeling a brief non-invincible "commitment" frame before i-frames kick in
	 * reads as more deliberate/readable than instant invincibility, and avoids
	 * players spamming dodge as a frame-perfect no-cost damage-avoidance button.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Dodge|Invincibility")
	float InvincibilityStartDelay = 0.05f;

	/** GameplayEffect that costs Stamina (or whatever resource this project uses) for dodging. Optional -- if unset, dodge is free. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Dodge|Cost")
	TSubclassOf<UGameplayEffect> StaminaCostEffectClass;

	/**
	 * Gameplay tag that, if present on the owner's ability system component,
	 * prevents this ability from activating (in addition to the hardcoded
	 * GCS.State.Dead check). Defaults to GCS.State.Staggered -- you can't dodge
	 * out of a hit-stagger in most action-combat games.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Dodge", meta = (Categories = "GCS.State"))
	FGameplayTag BlockedByTag;

	/**
	 * If true, dodges in the direction of the owner's current movement input
	 * (directional roll/dash -- forward, back, or strafe, whichever the player
	 * is holding). If false (or if there is no current input), dodges straight
	 * backward from the character's facing (a fixed backstep).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Dodge")
	bool bUseInputDirection = true;

protected:
	/** Blueprint hook fired from EndAbility, after GCS.State.InDodge has been removed. Use for dodge-recovery VFX/SFX/animation cleanup. */
	UFUNCTION(BlueprintImplementableEvent, Category = "GCS|Dodge", meta = (DisplayName = "On Dodge End"))
	void K2_OnDodgeEnd();

	/**
	 * Computes the world-space direction the dodge should travel in.
	 * If bUseInputDirection is true and the owner has non-zero current
	 * movement input / velocity, uses that direction. Otherwise falls back to
	 * a backstep (-ActorForwardVector). Always returns a normalized, purely
	 * horizontal (Z = 0) vector so the launch doesn't fling the character
	 * upward/downward on slopes.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GCS|Dodge")
	FVector ComputeDodgeDirection() const;

	/** Applies StaminaCostEffectClass to self, if set. Mirrors UGCSAbility_Block::ApplyBlockState's pattern for spec creation/application. */
	void ApplyStaminaCost();

	/** Timer callback: opens the invincibility window on the owner's UGCSCombatComponent. */
	void OpenInvincibilityWindow();

	/** Timer callback: ends the ability once the dodge motion has finished playing out. */
	void FinishDodge();

	/** Handle for the delayed invincibility-window-open timer. */
	UPROPERTY()
	FTimerHandle InvincibilityStartTimerHandle;

	/** Handle for the auto-EndAbility-after-DodgeDuration timer. */
	UPROPERTY()
	FTimerHandle DodgeDurationTimerHandle;

	/** Cached copies of the handle/info this activation needs inside timer-delegate callbacks (which do not receive them as parameters). */
	FGameplayAbilitySpecHandle CachedAbilityHandle;
	FGameplayAbilityActorInfo CachedActorInfo;
	FGameplayAbilityActivationInfo CachedActivationInfo;
};
