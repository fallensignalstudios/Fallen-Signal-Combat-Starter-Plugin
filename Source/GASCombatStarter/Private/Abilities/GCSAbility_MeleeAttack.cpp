// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Abilities/GCSAbility_MeleeAttack.h"
#include "Components/GCSCombatComponent.h"
#include "AbilitySystem/GCSAbilitySystemComponent.h"
#include "GCSNativeGameplayTags.h"
#include "GASCombatStarterModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"

UGCSAbility_MeleeAttack::UGCSAbility_MeleeAttack()
{
	ActivationPolicy = EGCSAbilityActivationPolicy::OnInput;

	// Default to tracing against Pawns; project can override per-ability instance.
	TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
}

void UGCSAbility_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		return;
	}

	// Default flow: trace immediately on activation. For animation-driven combos,
	// override this in a Blueprint child to instead wait for an anim notify (via
	// AbilityTask_WaitGameplayEvent or a montage notify binding) before calling
	// PerformMeleeTrace() / ApplyDamageToTargets().
	const TArray<AActor*> Targets = PerformMeleeTrace();
	if (Targets.Num() > 0)
	{
		ApplyDamageToTargets(Targets);
	}

	if (UGCSCombatComponent* CombatComponent = GetCombatComponent())
	{
		CombatComponent->AdvanceCombo();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGCSAbility_MeleeAttack::GetTraceTransform(FVector& OutOrigin, FVector& OutForward) const
{
	OutOrigin = FVector::ZeroVector;
	OutForward = FVector::ForwardVector;

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	if (TraceSocketName != NAME_None)
	{
		if (const ACharacter* Character = Cast<ACharacter>(Avatar))
		{
			if (USkeletalMeshComponent* Mesh = Character->GetMesh())
			{
				if (Mesh->DoesSocketExist(TraceSocketName))
				{
					OutOrigin = Mesh->GetSocketLocation(TraceSocketName);
					OutForward = Mesh->GetSocketRotation(TraceSocketName).Vector();
					return;
				}
			}
		}
	}

	OutOrigin = Avatar->GetActorLocation();
	OutForward = Avatar->GetActorForwardVector();
}

TArray<AActor*> UGCSAbility_MeleeAttack::PerformMeleeTrace()
{
	TArray<AActor*> Results;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !GetWorld())
	{
		return Results;
	}

	FVector Origin, Forward;
	GetTraceTransform(Origin, Forward);
	const FVector TraceEnd = Origin + (Forward * TraceForwardDistance);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Avatar);

	TArray<FHitResult> HitResults;
	const EDrawDebugTrace::Type DebugType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	bool bHit = false;
	if (TraceShape == EGCSMeleeTraceShape::Sphere)
	{
		bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
			Avatar, Origin, TraceEnd, TraceRadius, TraceObjectTypes,
			false, ActorsToIgnore, DebugType, HitResults, true);
	}
	else
	{
		bHit = UKismetSystemLibrary::BoxTraceMultiForObjects(
			Avatar, Origin, TraceEnd, TraceBoxHalfExtent, Forward.Rotation(), TraceObjectTypes,
			false, ActorsToIgnore, DebugType, HitResults, true);
	}

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				Results.AddUnique(HitActor);
			}
		}
	}

	return Results;
}

void UGCSAbility_MeleeAttack::ApplyDamageToTargets(const TArray<AActor*>& Targets)
{
	if (!DamageEffectClass)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbility_MeleeAttack::ApplyDamageToTargets called with no DamageEffectClass set on %s."), *GetName());
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

	for (AActor* Target : Targets)
	{
		if (!Target)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!TargetASC)
		{
			continue; // Target has no ASC -- cannot receive a GameplayEffect. Skip silently.
		}

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, DamageEffectLevel, ContextHandle);
		if (SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			OnHitConfirmed(Target, SpecHandle);
		}
	}
}

void UGCSAbility_MeleeAttack::OnHitConfirmed_Implementation(AActor* Target, const FGameplayEffectSpecHandle& EffectSpecHandle)
{
	if (!Target)
	{
		return;
	}

	// Notify the target's hit-react ability/animation via GameplayEvent so the
	// receiving side decides how to react (montage, stagger, death) without this
	// ability needing to know anything about the target's implementation.
	FGameplayEventData EventData;
	EventData.EventTag = GCSGameplayTags::Event_HitReact;
	EventData.Instigator = GetAvatarActorFromActorInfo();
	EventData.Target = Target;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, EventData.EventTag, EventData);

	if (UGCSCombatComponent* TargetCombatComponent = Target->FindComponentByClass<UGCSCombatComponent>())
	{
		TargetCombatComponent->RecordHitTaken();
	}
}
