// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GCSAbilityTask_PlayMontageAndWaitForEvent.generated.h"

class UAnimMontage;
class UAbilitySystemComponent;
struct FGameplayTag;
struct FGameplayEventData;

/**
 * Multicast delegate signature used by every output pin on this task.
 *
 * EventTag / EventData are only meaningful for OnEventReceived (they describe
 * the GameplayEvent that fired). For the montage-lifecycle pins (Completed,
 * BlendOut, Interrupted, Cancelled) EventTag/EventData are left at their
 * default-constructed values -- they exist on this shared signature purely so
 * Blueprint graphs can use a single delegate type for every pin.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGCSPlayMontageAndWaitForEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

/**
 * UGCSAbilityTask_PlayMontageAndWaitForEvent
 *
 * Plays a montage on the avatar actor AND simultaneously listens for
 * GameplayEvents (typically sent from anim notifies via
 * AGCSCharacterBase/AnimNotify_SendGameplayEvent-style notifies, or from
 * gameplay code) while that montage is playing. This is the workhorse task
 * for combo/melee abilities: it lets a single ability wait on "montage
 * finished" AND "notify fired" at the same time, instead of needing two
 * separate ability tasks wired together by hand.
 *
 * Modeled after Lyra's ULyraAbilityTask_PlayMontageAndWait (montage bookkeeping,
 * bStopWhenAbilityEnds semantics, root motion scale handling) with the engine's
 * UAbilityTask_WaitGameplayEvent's tag-container listening merged in.
 *
 * ---------------------------------------------------------------------------
 * USAGE EXAMPLE -- UGCSAbility_MeleeAttack combo step (Blueprint or C++)
 * ---------------------------------------------------------------------------
 * See the full walkthrough in the header comment of the .cpp file. Summary:
 *
 *   1. ActivateAbility calls:
 *        PlayMontageAndWaitForEvent(this, NAME_None, ComboMontage,
 *            FGameplayTagContainer{GCSGameplayTags::Event_HitReact, GCSGameplayTags::Event_ComboAdvance},
 *            1.f, NAME_None, true, 1.f);
 *
 *   2. Bind OnEventReceived -> when EventTag matches the "notify triggers a
 *      hit check" tag (e.g. an anim notify mid-swing sends GCS.Event.HitReact
 *      to the instigator itself as a cue), call PerformMeleeTrace() then
 *      ApplyDamageToTargets() on the owning UGCSAbility_MeleeAttack.
 *
 *   3. Bind OnCompleted and OnInterrupted (and typically OnCancelled) -> call
 *      EndAbility so the ability's lifetime is tied to the montage.
 * ---------------------------------------------------------------------------
 */
UCLASS()
class GASCOMBATSTARTER_API UGCSAbilityTask_PlayMontageAndWaitForEvent : public UAbilityTask
{
	GENERATED_BODY()

public:
	UGCSAbilityTask_PlayMontageAndWaitForEvent(const FObjectInitializer& ObjectInitializer);

	/** Montage finished playing naturally (reached its end, not interrupted/cancelled). */
	UPROPERTY(BlueprintAssignable)
	FGCSPlayMontageAndWaitForEventDelegate OnCompleted;

	/** Montage started blending out (either finishing naturally or being interrupted -- fires before OnCompleted/OnInterrupted). */
	UPROPERTY(BlueprintAssignable)
	FGCSPlayMontageAndWaitForEventDelegate OnBlendOut;

	/** Montage was interrupted by another montage/anim state before finishing (e.g. a stagger montage overriding it). */
	UPROPERTY(BlueprintAssignable)
	FGCSPlayMontageAndWaitForEventDelegate OnInterrupted;

	/** The owning ability was cancelled while this task was active. */
	UPROPERTY(BlueprintAssignable)
	FGCSPlayMontageAndWaitForEventDelegate OnCancelled;

	/** A GameplayEvent whose tag matches (or is a child of) one of the EventTags fired while the montage was playing. */
	UPROPERTY(BlueprintAssignable)
	FGCSPlayMontageAndWaitForEventDelegate OnEventReceived;

	/**
	 * Plays MontageToPlay on the ability's avatar actor and listens for any
	 * GameplayEvent tagged with (or a child of) a tag in EventTags for as long
	 * as this task is active.
	 *
	 * @param OwningAbility						Ability instantiating this task (auto-filled by BlueprintInternalUseOnly/DefaultToSelf in ability graphs).
	 * @param TaskInstanceName					Optional name used to look up/end this task instance later (e.g. via AbilityTask lookup helpers). Can be NAME_None.
	 * @param MontageToPlay						Montage to play on the avatar's AnimInstance.
	 * @param EventTags							GameplayEvents matching any of these tags (exact match or child tag) will broadcast OnEventReceived while this task is active.
	 * @param Rate								Playback rate multiplier passed to PlayMontage.
	 * @param StartSection						Optional montage section to start playback from. NAME_None plays from the start.
	 * @param bStopWhenAbilityEnds				If true, the montage is stopped (if it's still the active montage) when the owning ability ends for any reason and this task hasn't already finished.
	 * @param AnimRootMotionTranslationScale	Scales root motion translation extracted from the montage while it plays (e.g. to stretch/shrink a dash distance).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UGCSAbilityTask_PlayMontageAndWaitForEvent* PlayMontageAndWaitForEvent(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		UAnimMontage* MontageToPlay,
		FGameplayTagContainer EventTags,
		float Rate = 1.f,
		FName StartSection = NAME_None,
		bool bStopWhenAbilityEnds = true,
		float AnimRootMotionTranslationScale = 1.f);

	//~ Begin UAbilityTask interface
	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	virtual FString GetDebugString() const override;
	//~ End UAbilityTask interface

protected:
	/** Montage to play on activation. */
	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay;

	/** Tags this task listens for GameplayEvents on while active. */
	UPROPERTY()
	FGameplayTagContainer EventTags;

	/** Playback rate passed to PlayMontage. */
	UPROPERTY()
	float Rate = 1.f;

	/** Section to begin montage playback at. */
	UPROPERTY()
	FName StartSection = NAME_None;

	/** Root motion translation scale applied for the duration of the montage. */
	UPROPERTY()
	float AnimRootMotionTranslationScale = 1.f;

	/** Whether to force-stop the montage when the owning ability ends and this task is still active. */
	UPROPERTY()
	bool bStopWhenAbilityEnds = true;

private:
	/** Bound to the AbilitySystemComponent's montage-blending-out notification. */
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	/** Bound to the AnimInstance's montage-ended notification (covers both natural completion and interruption). */
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Bound via AddGameplayEventTagContainerDelegate; fires for any GameplayEvent matching EventTags. */
	void OnGameplayEventReceived(FGameplayTag MatchingTag, const FGameplayEventData* Payload);

	/** Called when the owning ability is cancelled while the montage is still playing. */
	void OnAbilityCancelled();

	/**
	 * True once one of the terminal montage callbacks (blend out / ended) has
	 * already run once for this play-through, used to guard against the well
	 * known double-fire of OnMontageEnded / OnMontageBlendingOut when a
	 * montage is interrupted by another Play (see comment in .cpp Activate()).
	 */
	bool bMontageEndedOrInterrupted = false;

	/** Cached ASC this task registered its callbacks on, so OnDestroy can cleanly unregister even if GetAbilitySystemComponent() would otherwise return null (e.g. avatar destroyed). */
	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

	/** Handle for the delegate registered via AddGameplayEventTagContainerDelegate, needed to unregister it in OnDestroy. */
	FDelegateHandle EventHandle;

	/** Handle for the ability-cancelled callback, needed to unregister it in OnDestroy. */
	FDelegateHandle CancelledHandle;

	/** Delegate bound to the AnimInstance's OnMontageEnded multicast delegate. */
	FOnMontageEnded MontageEndedDelegate;

	/** Delegate bound to the AbilitySystemComponent's OnMontageBlendingOut for our specific montage instance. */
	FOnMontageBlendingOutStarted BlendingOutDelegate;

	/** Returns the AnimInstance driving MontageToPlay on the avatar, or nullptr if unavailable. */
	UAnimInstance* GetTargetAnimInstance() const;
};
