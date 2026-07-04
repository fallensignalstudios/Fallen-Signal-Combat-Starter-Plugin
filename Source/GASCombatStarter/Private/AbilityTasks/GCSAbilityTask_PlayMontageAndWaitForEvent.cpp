// Copyright Fallen Signal Studios. All Rights Reserved.

// =============================================================================
// USAGE EXAMPLE -- how UGCSAbility_MeleeAttack (or a Blueprint combo child of
// it) would drive an anim-notify-triggered combo attack using this task.
// =============================================================================
//
// This task exists specifically to remove the need to wire up a separate
// PlayMontageAndWait + WaitGameplayEvent pair (and manually manage which one
// ends the ability) every time a melee/combo ability wants "play this swing
// animation, and when the swing notify fires mid-animation, do the hit
// trace". One task, one set of output pins, correctly ordered relative to
// montage playback.
//
// --- Step 1: Ability activation (C++ ActivateAbility override, or Blueprint
//             ActivateAbility event graph) ---
//
//     void UGCSAbility_ComboStep1::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
//         const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
//         const FGameplayEventData* TriggerEventData)
//     {
//         Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
//
//         FGameplayTagContainer WaitTags;
//         WaitTags.AddTag(GCSGameplayTags::Event_HitReact);      // "notify says: check for a hit now"
//         WaitTags.AddTag(GCSGameplayTags::Event_ComboAdvance);  // "notify says: combo window is open"
//
//         UGCSAbilityTask_PlayMontageAndWaitForEvent* Task =
//             UGCSAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
//                 this, NAME_None, ComboMontage, WaitTags,
//                 /*Rate=*/1.f, /*StartSection=*/NAME_None,
//                 /*bStopWhenAbilityEnds=*/true, /*AnimRootMotionTranslationScale=*/1.f);
//
//         Task->OnEventReceived.AddDynamic(this, &UGCSAbility_ComboStep1::HandleComboEvent);
//         Task->OnCompleted.AddDynamic(this, &UGCSAbility_ComboStep1::HandleMontageFinished);
//         Task->OnInterrupted.AddDynamic(this, &UGCSAbility_ComboStep1::HandleMontageFinished);
//         Task->OnCancelled.AddDynamic(this, &UGCSAbility_ComboStep1::HandleMontageFinished);
//         Task->ReadyForActivation();
//     }
//
// --- Step 2: Bind OnEventReceived -> trace + apply damage ---
//
//     void UGCSAbility_ComboStep1::HandleComboEvent(FGameplayTag EventTag, FGameplayEventData EventData)
//     {
//         if (EventTag.MatchesTag(GCSGameplayTags::Event_HitReact))
//         {
//             // Inherited from UGCSAbility_MeleeAttack: sweep + apply DamageEffectClass
//             // to anything hit, which in turn sends GCS.Event.HitReact to each target.
//             const TArray<AActor*> Targets = PerformMeleeTrace();
//             if (Targets.Num() > 0)
//             {
//                 ApplyDamageToTargets(Targets);
//             }
//         }
//         else if (EventTag.MatchesTag(GCSGameplayTags::Event_ComboAdvance))
//         {
//             if (UGCSCombatComponent* Combat = GetCombatComponent())
//             {
//                 Combat->AdvanceCombo();
//             }
//         }
//     }
//
// --- Step 3: Bind terminal montage pins -> end the ability ---
//
//     void UGCSAbility_ComboStep1::HandleMontageFinished(FGameplayTag EventTag, FGameplayEventData EventData)
//     {
//         EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
//     }
//
// In a pure Blueprint ability, this is the same graph shape: call the
// "Play Montage And Wait For Event" node, wire the "Event Received" exec pin
// to a Switch on EventTag (or a sequence of Tag == checks) that calls
// PerformMeleeTrace / ApplyDamageToTargets, and wire Completed/Interrupted/
// Cancelled to End Ability.
// =============================================================================

#include "AbilityTasks/GCSAbilityTask_PlayMontageAndWaitForEvent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GASCombatStarterModule.h"

UGCSAbilityTask_PlayMontageAndWaitForEvent::UGCSAbilityTask_PlayMontageAndWaitForEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// This task drives visible/replicated animation state, so make sure it
	// ticks/replicates the way other montage tasks in GAS do. bTickingTask is
	// left false (default) -- we are purely event-driven (delegates), we don't
	// need a per-frame Tick override.
	Rate = 1.f;
	AnimRootMotionTranslationScale = 1.f;
	bStopWhenAbilityEnds = true;
}

UGCSAbilityTask_PlayMontageAndWaitForEvent* UGCSAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName,
	UAnimMontage* MontageToPlay,
	FGameplayTagContainer EventTags,
	float Rate,
	FName StartSection,
	bool bStopWhenAbilityEnds,
	float AnimRootMotionTranslationScale)
{
	// NewAbilityTask is the standard GAS factory helper: it constructs the
	// task, associates it with OwningAbility, and sets InstanceName for later
	// lookup (e.g. UAbilityTask::FindAbilityTask style patterns).
	UGCSAbilityTask_PlayMontageAndWaitForEvent* Task = NewAbilityTask<UGCSAbilityTask_PlayMontageAndWaitForEvent>(OwningAbility, TaskInstanceName);
	Task->MontageToPlay = MontageToPlay;
	Task->EventTags = MoveTemp(EventTags);
	Task->Rate = Rate;
	Task->StartSection = StartSection;
	Task->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	Task->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	return Task;
}

UAnimInstance* UGCSAbilityTask_PlayMontageAndWaitForEvent::GetTargetAnimInstance() const
{
	// GetAbilitySystemComponent() is a UAbilityTask helper resolving to the
	// ASC of the ability that spawned this task -- NOT necessarily set yet if
	// the ability's ActorInfo was never valid, hence the defensive checks
	// throughout this file.
	if (const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		return ASC->AbilityActorInfo.IsValid() ? ASC->AbilityActorInfo->GetAnimInstance() : nullptr;
	}
	return nullptr;
}

void UGCSAbilityTask_PlayMontageAndWaitForEvent::Activate()
{
	// EDGE CASE: Ability object already gone (e.g. task somehow survived past
	// the ability's lifetime) -- bail out safely rather than crashing on a
	// null Ability-> dereference below.
	if (!Ability)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbilityTask_PlayMontageAndWaitForEvent activated with a null Ability. Ending task."));
		EndTask();
		return;
	}

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbilityTask_PlayMontageAndWaitForEvent activated with no AbilitySystemComponent on %s. Ending task."), *GetNameSafe(Ability));
		EndTask();
		return;
	}

	// EDGE CASE: no montage assigned (e.g. a data-driven combo asset left a
	// slot empty) -- there's nothing to play, but we can still legitimately
	// listen for events if the caller wants that... however the contract of
	// this task is "montage AND event", so treat a missing montage as a hard
	// failure the same way engine PlayMontageAndWait does.
	if (!MontageToPlay)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbilityTask_PlayMontageAndWaitForEvent activated with a null MontageToPlay on ability %s. Ending task."), *GetNameSafe(Ability));
		EndTask();
		return;
	}

	// EDGE CASE: no AnimInstance (e.g. avatar has no SkeletalMeshComponent,
	// or the mesh hasn't initialized an anim instance yet). Nothing to play
	// the montage on.
	UAnimInstance* AnimInstance = GetTargetAnimInstance();
	if (!AnimInstance)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbilityTask_PlayMontageAndWaitForEvent activated with no valid AnimInstance for ability %s. Ending task."), *GetNameSafe(Ability));
		EndTask();
		return;
	}

	bMontageEndedOrInterrupted = false;
	CachedAbilitySystemComponent = ASC;

	// PlayMontage on the ASC (rather than calling Montage_Play on the
	// AnimInstance directly) is the GAS-correct path: it handles
	// replication/prediction of montage state and returns the *actual*
	// duration the montage will play for (accounting for Rate), which is 0.f
	// or negative on failure (e.g. montage already playing something
	// incompatible, or a replay/replication edge case on a simulated proxy).
	//
	// EDGE CASE: "montage already playing" -- PlayMontage internally calls
	// Montage_Play, which itself handles the case where AnimInstance is
	// already playing *a* montage (it will stop/blend the old one per the
	// AnimInstance's own montage-blend rules). We don't need to special-case
	// that here; we only need to make sure OUR delegates get bound to OUR
	// montage instance, which the code below does via Montage == MontageToPlay
	// checks in the callbacks.
	const float MontageStartedPlayTime = ASC->PlayMontage(Ability, Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, StartSection);
	const bool bPlayedSuccessfully = MontageStartedPlayTime > 0.f;

	if (!bPlayedSuccessfully)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbilityTask_PlayMontageAndWaitForEvent failed to play montage %s for ability %s."), *GetNameSafe(MontageToPlay), *GetNameSafe(Ability));
		EndTask();
		return;
	}

	// --- Bind montage lifecycle delegates ---

	// OnMontageBlendingOut is broadcast by the ABILITYSYSTEMCOMPONENT (not the
	// AnimInstance) specifically for the montage it just started playing via
	// PlayMontage above, which makes it reliable for telling "our" montage
	// apart from any other montage the AnimInstance might play concurrently
	// on other slots.
	BlendingOutDelegate.BindUObject(this, &UGCSAbilityTask_PlayMontageAndWaitForEvent::OnMontageBlendingOut);
	ASC->AnimMontageBlendingOut.Add(BlendingOutDelegate);

	// OnMontageEnded comes from the AnimInstance itself and fires once the
	// montage instance is fully done playing (after blend-out completes),
	// covering both the "finished naturally" and "got interrupted" cases (the
	// bInterrupted bool distinguishes them). We additionally filter by
	// `Montage == MontageToPlay` inside the callback since AnimInstance's
	// OnMontageEnded is not scoped to a single montage.
	MontageEndedDelegate = FOnMontageEnded::CreateUObject(this, &UGCSAbilityTask_PlayMontageAndWaitForEvent::OnMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

	// Apply the requested root motion translation scale for the duration of
	// this montage. Restored implicitly when the next montage/task sets its
	// own value; GAS does not provide an automatic "restore previous scale"
	// hook, which mirrors Lyra's PlayMontageAndWait behavior.
	//
	// EDGE CASE: Not every avatar is an ACharacter (e.g. a non-pawn actor
	// driving its own AnimInstance directly), so this is a soft cast -- skip
	// silently if the avatar doesn't support root motion translation scaling.
	if (ACharacter* Character = Cast<ACharacter>(ASC->AbilityActorInfo.IsValid() ? ASC->AbilityActorInfo->AvatarActor.Get() : nullptr))
	{
		Character->SetAnimRootMotionTranslationScale(AnimRootMotionTranslationScale);
	}

	// --- Register the GameplayEvent listener ---
	//
	// AddGameplayEventTagContainerDelegate matches on exact tag OR any child
	// tag within EventTags (e.g. registering "GCS.Event" would also catch
	// "GCS.Event.HitReact"), which is exactly what lets a single task listen
	// for a whole family of notify-driven events without one binding per tag.
	EventHandle = ASC->AddGameplayEventTagContainerDelegate(
		EventTags,
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UGCSAbilityTask_PlayMontageAndWaitForEvent::OnGameplayEventReceived));

	// --- Register for ability cancellation ---
	//
	// If the owning ability is cancelled (e.g. a stagger GameplayEffect force-
	// cancels all of this actor's abilities with a given tag) while our
	// montage is still playing, we want to fire OnCancelled rather than
	// silently doing nothing / relying on OnDestroy's cleanup alone.
	CancelledHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &UGCSAbilityTask_PlayMontageAndWaitForEvent::OnAbilityCancelled);
}

void UGCSAbilityTask_PlayMontageAndWaitForEvent::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// Filter: AnimMontageBlendingOut is not scoped to a specific montage --
	// ignore callbacks about any montage other than the one we started.
	if (Montage != MontageToPlay)
	{
		return;
	}

	// EDGE CASE: If the ability itself is already ending (e.g. because
	// OnMontageEnded already ran and called EndTask -> OnDestroy -> this
	// object is being torn down), IsFinished()/ShouldBroadcastAbilityTaskDelegates
	// guards against firing delegates on a half-destroyed task.
	if (!ShouldBroadcastAbilityTaskDelegates())
	{
		return;
	}

	OnBlendOut.Broadcast(FGameplayTag(), FGameplayEventData());

	// NOTE: We deliberately do NOT EndTask() here. Blend-out is a
	// "heads up, wrapping up" signal, not a terminal one -- OnMontageEnded
	// (below) is the authoritative "this task's job is done" signal and is
	// guaranteed to fire after blend-out completes. This mirrors engine/Lyra
	// PlayMontageAndWait, where OnBlendOut is purely informational.
}

void UGCSAbilityTask_PlayMontageAndWaitForEvent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Filter: Montage_SetEndDelegate was bound specifically to MontageToPlay,
	// so in practice Montage should always equal MontageToPlay here, but we
	// still guard against stale callbacks firing after a new montage has
	// already reused this delegate slot.
	if (Montage != MontageToPlay)
	{
		return;
	}

	// EDGE CASE: Guard against being invoked twice for the same montage
	// instance. Some AnimInstance/Montage interruption paths (e.g. montage
	// force-stopped from two different code paths in the same frame) can
	// otherwise cause a double Broadcast+EndTask, which would assert since
	// the task will already have been marked for destruction.
	if (bMontageEndedOrInterrupted)
	{
		return;
	}
	bMontageEndedOrInterrupted = true;

	if (!ShouldBroadcastAbilityTaskDelegates())
	{
		EndTask();
		return;
	}

	if (bInterrupted)
	{
		OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
	}
	else
	{
		OnCompleted.Broadcast(FGameplayTag(), FGameplayEventData());
	}

	// Terminal event: the montage is fully done (whether it finished
	// naturally or was interrupted), so this task's work is complete.
	EndTask();
}

void UGCSAbilityTask_PlayMontageAndWaitForEvent::OnGameplayEventReceived(FGameplayTag MatchingTag, const FGameplayEventData* Payload)
{
	if (!ShouldBroadcastAbilityTaskDelegates())
	{
		return;
	}

	// Copy the payload out -- Payload is only guaranteed valid for the
	// duration of this callback, but our delegate signature passes
	// FGameplayEventData by value, so recipients get a safe, owned copy.
	const FGameplayEventData EventDataCopy = Payload ? *Payload : FGameplayEventData();

	// NON-TERMINAL: unlike the montage lifecycle callbacks, receiving a
	// GameplayEvent does NOT end this task. A single montage can (and for
	// combos, usually does) fire multiple notify-driven events -- e.g.
	// "combo window open" then "hit check" then "combo window closed" -- all
	// while the same montage instance keeps playing. The task only ends when
	// the montage itself finishes/interrupts, or the ability ends/cancels.
	OnEventReceived.Broadcast(MatchingTag, EventDataCopy);
}

void UGCSAbilityTask_PlayMontageAndWaitForEvent::OnAbilityCancelled()
{
	// EDGE CASE: ability cancelled mid-montage (e.g. a stagger effect
	// force-cancels the attacking ability). We still want the montage
	// stopped via OnDestroy's bStopWhenAbilityEnds handling, but the *pin*
	// that should fire here is OnCancelled, not OnInterrupted/OnCompleted,
	// so callers can distinguish "ability got yanked out from under me" from
	// "the animation itself naturally stopped".
	if (!bMontageEndedOrInterrupted && ShouldBroadcastAbilityTaskDelegates())
	{
		bMontageEndedOrInterrupted = true;
		OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
	}

	EndTask();
}

void UGCSAbilityTask_PlayMontageAndWaitForEvent::ExternalCancel()
{
	// ExternalCancel is invoked by the ability task framework when the task
	// is cancelled from outside the ability's normal end flow. Route it
	// through the same handling as an ability cancellation so OnCancelled
	// fires consistently regardless of *why* we were cancelled.
	OnAbilityCancelled();

	Super::ExternalCancel();
}

void UGCSAbilityTask_PlayMontageAndWaitForEvent::OnDestroy(bool bInOwnerFinished)
{
	// OnDestroy is called exactly once when this task is being torn down --
	// whether from EndTask() above, the owning ability ending, or the ASC/
	// AbilitySystemComponent itself being destroyed. All cleanup (unregister
	// delegates, stop montage) MUST happen here rather than relying on the
	// terminal callbacks alone, since those callbacks are not guaranteed to
	// run in every teardown path (e.g. avatar destroyed outright).

	if (UAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get())
	{
		// Unregister the GameplayEvent listener so this (about-to-be-garbage)
		// task doesn't keep receiving callbacks after destruction.
		if (EventHandle.IsValid())
		{
			ASC->RemoveGameplayEventTagContainerDelegate(EventTags, EventHandle);
		}

		// Unregister the montage blend-out delegate we added in Activate().
		ASC->AnimMontageBlendingOut.Remove(BlendingOutDelegate);

		// EDGE CASE: Only stop the montage if it is STILL the one we started
		// (another task/ability may have already played something else on
		// top of it, in which case stopping "our" montage here would
		// incorrectly cut off unrelated animation).
		if (bStopWhenAbilityEnds && !bMontageEndedOrInterrupted)
		{
			if (Ability && MontageToPlay)
			{
				const bool bIsStillOurMontage = ASC->GetCurrentMontage() == MontageToPlay;
				if (bIsStillOurMontage)
				{
					// StopMontageIfCurrent-style call: goes through the ASC so
					// stop replication/prediction bookkeeping happens
					// correctly, rather than calling Montage_Stop directly on
					// the AnimInstance.
					ASC->CurrentMontageStop();
				}
			}
		}
	}

	// Clear the end-of-montage delegate on the AnimInstance so a stale
	// UObject pointer can never be invoked after this task is destroyed.
	if (UAnimInstance* AnimInstance = GetTargetAnimInstance())
	{
		if (MontageToPlay && AnimInstance->Montage_IsPlaying(MontageToPlay))
		{
			AnimInstance->Montage_SetEndDelegate(FOnMontageEnded(), MontageToPlay);
		}
	}

	if (Ability && CancelledHandle.IsValid())
	{
		Ability->OnGameplayAbilityCancelled.Remove(CancelledHandle);
		CancelledHandle.Reset();
	}

	Super::OnDestroy(bInOwnerFinished);
}

FString UGCSAbilityTask_PlayMontageAndWaitForEvent::GetDebugString() const
{
	const UAnimInstance* AnimInstance = GetTargetAnimInstance();
	UAnimMontage* PlayingMontage = nullptr;
	if (AnimInstance)
	{
		if (MontageToPlay && AnimInstance->Montage_IsPlaying(MontageToPlay))
		{
			PlayingMontage = MontageToPlay;
		}
		else
		{
			PlayingMontage = AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWaitForEvent. MontageToPlay: %s (currently playing: %s). EventTags: %s"),
		*GetNameSafe(MontageToPlay),
		*GetNameSafe(PlayingMontage),
		*EventTags.ToStringSimple());
}
