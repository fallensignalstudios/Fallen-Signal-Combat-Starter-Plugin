// Copyright Fallen Signal Studios. All Rights Reserved.

#include "AbilitySystem/GCSAbilitySystemComponent.h"
#include "Abilities/GCSGameplayAbility.h"
#include "GASCombatStarterModule.h"

UGCSAbilitySystemComponent::UGCSAbilitySystemComponent()
{
	// Mixed replication mode is the standard choice for combat-heavy GAS setups:
	// it replicates GameplayEffects via the normal ASC path while allowing
	// abilities to run predicted client-side logic (important for responsive
	// melee/dodge input). Switch to Full if you need fully authoritative,
	// non-predicted abilities only.
	ReplicationMode = EGameplayEffectReplicationMode::Mixed;
}

void UGCSAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGCSAbilitySystemComponent::InitializeDefaultAbilities(const TArray<TSubclassOf<UGCSGameplayAbility>>& DefaultAbilities, int32 AbilityLevel)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		// Ability granting must happen on the server -- the ASC replicates granted
		// abilities down to owning/relevant clients automatically.
		return;
	}

	for (const TSubclassOf<UGCSGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		// Skip if already granted (matched by class) to keep this function idempotent.
		bool bAlreadyGranted = false;
		for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
			{
				bAlreadyGranted = true;
				break;
			}
		}

		if (!bAlreadyGranted)
		{
			GrantAbility(AbilityClass, AbilityLevel, false);
		}
	}
}

FGameplayAbilitySpecHandle UGCSAbilitySystemComponent::GrantAbility(TSubclassOf<UGCSGameplayAbility> AbilityClass, int32 AbilityLevel, bool bRemoveAfterActivation)
{
	if (!AbilityClass)
	{
		return FGameplayAbilitySpecHandle();
	}

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("GrantAbility called on non-authority for %s. Ignoring."), *AbilityClass->GetName());
		return FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpec AbilitySpec(AbilityClass, AbilityLevel, INDEX_NONE, GetOwner());
	AbilitySpec.RemoveAfterActivation = bRemoveAfterActivation;

	const FGameplayAbilitySpecHandle Handle = GiveAbility(AbilitySpec);
	DefaultAbilityHandles.AddUnique(Handle);
	return Handle;
}

void UGCSAbilitySystemComponent::RemoveAbility(FGameplayAbilitySpecHandle AbilityHandle)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (AbilityHandle.IsValid())
	{
		ClearAbility(AbilityHandle);
		DefaultAbilityHandles.Remove(AbilityHandle);
	}
}

void UGCSAbilitySystemComponent::RemoveAbilitiesByClass(TSubclassOf<UGCSGameplayAbility> AbilityClass)
{
	if (!AbilityClass || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> HandlesToRemove;
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
		{
			HandlesToRemove.Add(Spec.Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : HandlesToRemove)
	{
		RemoveAbility(Handle);
	}
}

TArray<FGameplayAbilitySpecHandle> UGCSAbilitySystemComponent::GetActiveAbilitiesByTag(FGameplayTag Tag) const
{
	TArray<FGameplayAbilitySpecHandle> Result;

	if (!Tag.IsValid())
	{
		return Result;
	}

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(Tag) && Spec.IsActive())
		{
			Result.Add(Spec.Handle);
		}
	}

	return Result;
}

void UGCSAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);
}

void UGCSAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);
}

void UGCSAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);
	OnGCSAbilityActivated.Broadcast(Ability);
}

void UGCSAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);
	OnGCSAbilityEnded.Broadcast(Ability, bWasCancelled, !bWasCancelled);
}
