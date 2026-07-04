// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Abilities/GCSGameplayAbility.h"
#include "AbilitySystem/GCSAbilitySystemComponent.h"
#include "Components/GCSCombatComponent.h"
#include "Characters/GCSCharacterBase.h"

UGCSGameplayAbility::UGCSGameplayAbility()
{
	// Instanced-per-actor is the standard choice for combat abilities: it allows
	// per-activation state (e.g. combo tracking, parry timers) to live safely on
	// the ability instance without colliding across multiple concurrent users of
	// the same ability class.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Combat abilities are latent by nature (montages, traces, delays) and almost
	// always require server authority to be meaningful (damage, cooldowns).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGCSGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	K2_OnAbilityActivated();

	// NOTE: This base implementation intentionally does not call CommitAbility or
	// EndAbility -- derived abilities (melee/projectile/block) or Blueprint logic
	// are responsible for committing cost/cooldown and ending the ability once
	// their specific activation logic (traces, montages, projectile spawn) completes.
}

void UGCSGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	K2_OnAbilityEnd(bWasCancelled);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGCSGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

UGCSCombatComponent* UGCSGameplayAbility::GetCombatComponent() const
{
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		return Avatar->FindComponentByClass<UGCSCombatComponent>();
	}
	return nullptr;
}

AGCSCharacterBase* UGCSGameplayAbility::GetOwningCharacter() const
{
	return Cast<AGCSCharacterBase>(GetAvatarActorFromActorInfo());
}

UGCSAbilitySystemComponent* UGCSGameplayAbility::GetGCSAbilitySystemComponent() const
{
	return Cast<UGCSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}
