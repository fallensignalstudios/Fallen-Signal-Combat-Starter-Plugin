// Copyright Fallen Signal Studios. All Rights Reserved.

#include "AttributeSets/GCSCombatAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GASCombatStarterModule.h"

UGCSCombatAttributeSet::UGCSCombatAttributeSet()
{
	// Sensible defaults. Override per-character via a GameplayEffect applied at
	// spawn (recommended), or by editing these defaults directly for quick testing.
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitShield(0.f);
	InitMaxShield(0.f);
	InitEnergy(100.f);
	InitMaxEnergy(100.f);
	InitStamina(100.f);
	InitMaxStamina(100.f);
	InitAttackPower(10.f);
	InitDefensePower(0.f);
	InitCritChance(0.05f);
	InitCritMultiplier(1.5f);
	InitIncomingDamage(0.f);
}

void UGCSCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_None: all attributes here are relevant to all clients (health bars, shields,
	// etc. are typically visible to everyone). Switch to COND_OwnerOnly for anything
	// you consider private to the owning client (e.g. exact Energy/Stamina values).
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, Energy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, MaxEnergy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, DefensePower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGCSCombatAttributeSet, CritMultiplier, COND_None, REPNOTIFY_Always);

	// IncomingDamage is a meta attribute (execution scratch space only) and is
	// intentionally NOT replicated.
}

void UGCSCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Clamp current values against their Max counterpart whenever either changes.
	// This handles direct attribute writes (e.g. SetHealth calls, GE modifiers that
	// aren't routed through PostGameplayEffectExecute).
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxShield());
	}
	else if (Attribute == GetEnergyAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxEnergy());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetCritChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
}

void UGCSCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// Mirrors PreAttributeChange for the "base value" (permanent, non-buffed) channel.
	// Extend here if you introduce attributes with base-only clamping requirements.
}

void UGCSCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	AActor* Instigator = Context.GetOriginalInstigator();
	AActor* Target = Data.Target.GetAvatarActor();

	// -----------------------------------------------------------------
	// IncomingDamage meta attribute -> Shield/Health pipeline.
	//
	// Damage GameplayEffects (see UGCSEffect_DamageBase) should write their
	// final computed damage into IncomingDamage via a
	// UGameplayEffectExecutionCalculation. This function converts that value
	// into actual resource loss: Shield absorbs first, remainder hits Health.
	// -----------------------------------------------------------------
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float DamageDone = GetIncomingDamage();
		SetIncomingDamage(0.f); // Reset meta attribute immediately; it is scratch space only.

		if (DamageDone > 0.f)
		{
			const float CurrentShield = GetShield();
			const float DamageAfterShield = FMath::Max(0.f, DamageDone - CurrentShield);
			const float ShieldAbsorbed = DamageDone - DamageAfterShield;

			if (ShieldAbsorbed > 0.f)
			{
				SetShield(FMath::Clamp(CurrentShield - ShieldAbsorbed, 0.f, GetMaxShield()));
			}

			if (DamageAfterShield > 0.f)
			{
				const float NewHealth = FMath::Clamp(GetHealth() - DamageAfterShield, 0.f, GetMaxHealth());
				SetHealth(NewHealth);

				UE_LOG(LogGASCombatStarter, Verbose, TEXT("%s took %.1f damage from %s. Health: %.1f/%.1f"),
					Target ? *Target->GetName() : TEXT("Unknown"),
					DamageAfterShield,
					Instigator ? *Instigator->GetName() : TEXT("Unknown"),
					GetHealth(), GetMaxHealth());

				// NOTE: Death detection belongs in the owning ability system component
				// or character (e.g. bind to Health attribute change and check <= 0),
				// so that death handling (ragdoll, GCS.State.Dead tag, ability removal)
				// stays game-specific and out of the attribute set itself.
			}
		}
	}
}

void UGCSCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, Health, OldValue);
}

void UGCSCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, MaxHealth, OldValue);
}

void UGCSCombatAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, Shield, OldValue);
}

void UGCSCombatAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, MaxShield, OldValue);
}

void UGCSCombatAttributeSet::OnRep_Energy(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, Energy, OldValue);
}

void UGCSCombatAttributeSet::OnRep_MaxEnergy(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, MaxEnergy, OldValue);
}

void UGCSCombatAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, Stamina, OldValue);
}

void UGCSCombatAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, MaxStamina, OldValue);
}

void UGCSCombatAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, AttackPower, OldValue);
}

void UGCSCombatAttributeSet::OnRep_DefensePower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, DefensePower, OldValue);
}

void UGCSCombatAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, CritChance, OldValue);
}

void UGCSCombatAttributeSet::OnRep_CritMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGCSCombatAttributeSet, CritMultiplier, OldValue);
}
