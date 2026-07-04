// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Abilities/GCSAbility_Block.h"
#include "Components/GCSCombatComponent.h"
#include "GASCombatStarterModule.h"

UGCSAbility_Block::UGCSAbility_Block()
{
	ActivationPolicy = EGCSAbilityActivationPolicy::OnInput;

	// Block is a "hold" ability: it stays active until input is released or it
	// is cancelled, so it must not auto-end after ActivateAbility like the
	// melee/projectile abilities do.
}

void UGCSAbility_Block::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		return;
	}

	ApplyBlockState();

	if (UGCSCombatComponent* CombatComponent = GetCombatComponent())
	{
		CombatComponent->StartBlocking();
		CombatComponent->BeginParryWindow(ParryWindowDuration);
	}

	// Ability intentionally stays active here -- see InputReleased/EndAbility for the
	// hold-to-block lifecycle.
}

void UGCSAbility_Block::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// Releasing the block input ends the ability (equivalent to letting go of the
	// block button). Not cancelled -- this is a normal, successful end.
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGCSAbility_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		RemoveBlockState();

		if (UGCSCombatComponent* CombatComponent = GetCombatComponent())
		{
			CombatComponent->StopBlocking();
			CombatComponent->EndParryWindow();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGCSAbility_Block::ApplyBlockState()
{
	if (!BlockStateEffectClass)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbility_Block::ApplyBlockState called with no BlockStateEffectClass set on %s."), *GetName());
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(BlockStateEffectClass, GetAbilityLevel(), ContextHandle);
	if (SpecHandle.IsValid())
	{
		BlockStateEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void UGCSAbility_Block::RemoveBlockState()
{
	if (!BlockStateEffectHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveActiveGameplayEffect(BlockStateEffectHandle);
	}

	BlockStateEffectHandle.Invalidate();
}
