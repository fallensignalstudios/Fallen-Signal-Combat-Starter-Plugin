// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Abilities/GCSAbility_HitReact.h"
#include "Components/GCSCombatComponent.h"
#include "AbilitySystem/GCSAbilitySystemComponent.h"
#include "GCSNativeGameplayTags.h"
#include "GASCombatStarterModule.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemComponent.h"

UGCSAbility_HitReact::UGCSAbility_HitReact()
{
	// Documentation-only intent flag (see class comment for the full
	// explanation of why this alone is not sufficient to make GAS call this
	// ability in response to the event).
	ActivationPolicy = EGCSAbilityActivationPolicy::OnEvent;

	// This is the part that actually matters: register GCS.Event.HitReact as
	// a GameplayEvent trigger on this ability's CDO. As long as an instance of
	// this ability (or a Blueprint child of it) is granted to a target's
	// UAbilitySystemComponent, sending GCS.Event.HitReact to that actor (as
	// UGCSAbility_MeleeAttack::OnHitConfirmed_Implementation already does)
	// will activate it automatically -- no per-project setup required.
	FAbilityTriggerData HitReactTriggerData;
	HitReactTriggerData.TriggerTag = GCSGameplayTags::Event_HitReact;
	HitReactTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(HitReactTriggerData);

	// Hit reacts should never be blocked by their own previous instance still
	// running (replaceable), and multiple rapid hits should be able to
	// re-trigger this ability so the stagger-escalation logic in
	// ActivateAbility gets a chance to run on every hit rather than being
	// starved out while a prior hit-react montage is still playing.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UGCSAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		// Hit reacts are cosmetically driven by montages that replicate via
		// the owning character's AnimInstance/mesh in the normal GAS way once
		// the server plays them (or via a GameplayCue for pure cosmetics).
		// Running the stagger-accounting logic below only on the
		// authoritative side keeps UGCSCombatComponent::BeginStagger's
		// HasAuthority() check meaningful and avoids double-counting hits.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Cache for use in the montage-ended callback, which does not receive
	// these as parameters.
	CachedAbilityHandle = Handle;
	CachedActorInfo = ActorInfo ? *ActorInfo : FGameplayAbilityActorInfo();
	CachedActivationInfo = ActivationInfo;

	UGCSCombatComponent* CombatComponent = GetCombatComponent();

	// Step 3 (order-wise it's convenient to record the hit before deciding on
	// stagger, since ShouldTriggerStagger consults the timestamp log this
	// call feeds): record that a hit landed, for any external systems (UI,
	// analytics, other abilities) that watch UGCSCombatComponent::LastHitTime.
	if (CombatComponent)
	{
		CombatComponent->RecordHitTaken();
	}

	// Step 4: evaluate stagger BEFORE committing to a hit-react montage. If
	// the owner is being hit too fast to recover, skip/interrupt the normal
	// hit-react animation in favor of the stagger state -- getting juggled by
	// a flurry of hits should read as "staggered", not as a queue of
	// individually-replayed hit-react clips.
	if (ShouldTriggerStagger())
	{
		TriggerStagger();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Steps 1 + 2: pick the directional (or default) montage and play it.
	CurrentlyPlayingMontage = PlayHitReactMontage(TriggerEventData);

	if (!CurrentlyPlayingMontage)
	{
		// No montage configured at all (neither directional nor default) --
		// nothing to wait on, so end immediately rather than leaving the
		// ability active forever.
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbility_HitReact::ActivateAbility on %s has no HitDirectionMontages or DefaultHitReactMontage configured; ending immediately."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}

	// If a montage IS playing, EndAbility is deferred to OnHitReactMontageEnded
	// (step 5), bound from within PlayHitReactMontage.
}

void UGCSAbility_HitReact::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Defensive cleanup: if this ability is ended externally (e.g. death,
	// forced-cancel by another system) while a hit-react montage is still
	// playing, stop it so the character doesn't get stuck mid-animation.
	if (CurrentlyPlayingMontage)
	{
		if (const FGameplayAbilityActorInfo* Info = ActorInfo)
		{
			if (UAnimInstance* AnimInstance = Info->GetAnimInstance())
			{
				if (AnimInstance->Montage_IsPlaying(CurrentlyPlayingMontage))
				{
					AnimInstance->Montage_Stop(0.15f, CurrentlyPlayingMontage);
				}
			}
		}
		CurrentlyPlayingMontage = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FGameplayTag UGCSAbility_HitReact::GetHitDirectionTag(AActor* Instigator) const
{
	const AActor* Owner = GetAvatarActorFromActorInfo();
	if (!Owner || !Instigator)
	{
		return FGameplayTag();
	}

	// Build a horizontal-plane (Z-flattened) direction from the owner to the
	// instigator so pitch differences (e.g. a hit from a flying enemy) don't
	// skew the front/back/left/right classification.
	FVector ToInstigator = Instigator->GetActorLocation() - Owner->GetActorLocation();
	ToInstigator.Z = 0.f;

	if (ToInstigator.IsNearlyZero())
	{
		// Instigator is at (effectively) the same location as the owner --
		// there is no meaningful direction to compute (e.g. self-inflicted
		// damage, or a hit with no valid instigator actor transform).
		return FGameplayTag();
	}

	ToInstigator.Normalize();

	FVector OwnerForward = Owner->GetActorForwardVector();
	FVector OwnerRight = Owner->GetActorRightVector();
	OwnerForward.Z = 0.f;
	OwnerRight.Z = 0.f;
	OwnerForward.Normalize();
	OwnerRight.Normalize();

	const float ForwardDot = FVector::DotProduct(OwnerForward, ToInstigator);
	const float RightDot = FVector::DotProduct(OwnerRight, ToInstigator);

	// Whichever axis has the larger magnitude dot product tells us whether
	// the hit is primarily in front/behind (ForwardDot dominant) or to the
	// side (RightDot dominant); the sign of that dominant dot product then
	// selects the specific direction along that axis.
	if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
	{
		return (ForwardDot >= 0.f) ? GCSGameplayTags::HitReact_Front : GCSGameplayTags::HitReact_Back;
	}
	else
	{
		return (RightDot >= 0.f) ? GCSGameplayTags::HitReact_Right : GCSGameplayTags::HitReact_Left;
	}
}

UAnimMontage* UGCSAbility_HitReact::PlayHitReactMontage(const FGameplayEventData* TriggerEventData)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return nullptr;
	}

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (!AnimInstance)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbility_HitReact::PlayHitReactMontage on %s: owning avatar has no AnimInstance (no SkeletalMeshComponent?)."), *GetName());
		return nullptr;
	}

	// Step 1: extract hit direction from TriggerEventData->Instigator
	// relative to the owner's facing, and select the matching montage.
	AActor* Instigator = (TriggerEventData ? const_cast<AActor*>(TriggerEventData->Instigator.Get()) : nullptr);
	UAnimMontage* SelectedMontage = nullptr;

	if (Instigator)
	{
		const FGameplayTag DirectionTag = GetHitDirectionTag(Instigator);
		if (DirectionTag.IsValid())
		{
			if (const TObjectPtr<UAnimMontage>* FoundMontage = HitDirectionMontages.Find(DirectionTag))
			{
				SelectedMontage = *FoundMontage;
			}
		}
	}

	if (!SelectedMontage)
	{
		SelectedMontage = DefaultHitReactMontage;
	}

	if (!SelectedMontage)
	{
		return nullptr;
	}

	// Step 2: play the selected montage.
	const float PlayedLength = AnimInstance->Montage_Play(SelectedMontage);
	if (PlayedLength <= 0.f)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbility_HitReact::PlayHitReactMontage on %s: Montage_Play failed for %s."), *GetName(), *SelectedMontage->GetName());
		return nullptr;
	}

	// Step 5 (setup half): bind so EndAbility fires automatically once this
	// montage finishes playing or is interrupted (e.g. by a stagger
	// pre-empting it, or death cancelling all abilities).
	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &UGCSAbility_HitReact::OnHitReactMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, SelectedMontage);

	return SelectedMontage;
}

void UGCSAbility_HitReact::OnHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	CurrentlyPlayingMontage = nullptr;

	// Step 5: end the ability once the montage ends, whether it finished
	// naturally (bInterrupted == false) or was cut short (bInterrupted ==
	// true, e.g. by TriggerStagger's Montage_Stop or an external cancel).
	EndAbility(CachedAbilityHandle, &CachedActorInfo, CachedActivationInfo, true, bInterrupted);
}

bool UGCSAbility_HitReact::ShouldTriggerStagger()
{
	if (!GetWorld())
	{
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	HitTimestamps.Add(Now);

	// Trim anything older than StaggerWindow seconds off the front of the log.
	const float WindowStart = Now - StaggerWindow;
	HitTimestamps.RemoveAll([WindowStart](float Timestamp)
	{
		return Timestamp < WindowStart;
	});

	return static_cast<float>(HitTimestamps.Num()) > StaggerThreshold;
}

void UGCSAbility_HitReact::TriggerStagger()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	// Step 4a: cancel whatever hit-react montage is currently playing so the
	// stagger state reads clearly instead of blending awkwardly out of a
	// half-finished directional hit react.
	if (CurrentlyPlayingMontage && ActorInfo)
	{
		if (UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance())
		{
			if (AnimInstance->Montage_IsPlaying(CurrentlyPlayingMontage))
			{
				AnimInstance->Montage_Stop(0.1f, CurrentlyPlayingMontage);
			}
		}
		CurrentlyPlayingMontage = nullptr;
	}

	// Step 4b: apply the optional stagger GameplayEffect to self.
	if (HitStaggerEffectClass)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddSourceObject(this);
			ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

			const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HitStaggerEffectClass, GetAbilityLevel(), ContextHandle);
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// Step 4c: flip the owner into the stagger state on UGCSCombatComponent,
	// which is the single source of truth other systems (movement lock,
	// ability CanActivate checks, UI) should query.
	if (UGCSCombatComponent* CombatComponent = GetCombatComponent())
	{
		CombatComponent->BeginStagger(StaggerDuration);
	}

	// A fresh stagger clears the rolling hit log -- the punishment (the
	// stagger itself) has already been applied, so there is no need to keep
	// counting toward a second, compounding stagger from the same flurry.
	HitTimestamps.Reset();
}
