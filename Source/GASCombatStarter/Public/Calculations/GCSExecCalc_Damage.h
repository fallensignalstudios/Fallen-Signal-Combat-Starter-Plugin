// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GCSExecCalc_Damage.generated.h"

/**
 * UGCSExecCalc_Damage
 *
 * Central damage-resolution execution calculation for GASCombatStarter.
 *
 * Pipeline:
 *   1. Pulls a raw "BaseDamage" value from the triggering GameplayEffectSpec via
 *      SetByCaller (tag: GCS.SetByCaller.BaseDamage).
 *   2. Scales that raw damage by the Source's AttackPower (captured as a snapshot,
 *      i.e. frozen at the moment the GameplayEffectSpec is created).
 *   3. Mitigates the result using the Target's DefensePower, using a soft
 *      diminishing-returns curve capped at 85% mitigation.
 *   4. Rolls a critical hit using the Target's CritChance / CritMultiplier
 *      (captured live/non-snapshot, i.e. re-evaluated at the moment of execution).
 *      On a crit, GCS.Effect.CriticalHit is added to the execution's output tag
 *      container via FGameplayEffectCustomExecutionOutput::AddOutputTags().
 *   5. Writes the final value into the Target's IncomingDamage meta-attribute via
 *      an Additive modifier. UGCSCombatAttributeSet::PostGameplayEffectExecute is
 *      responsible for routing IncomingDamage -> Shield -> Health.
 *
 * ---------------------------------------------------------------------------
 * HOW TO FEED THIS FROM A GAMEPLAY EFFECT (SetByCaller usage):
 * ---------------------------------------------------------------------------
 * This calculation does NOT read a hardcoded magnitude from the GE - it expects
 * the raw damage value to be supplied at spec-creation time via SetByCaller,
 * tagged with GCS.SetByCaller.BaseDamage. Example (C++):
 *
 *     FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
 *         DamageGameplayEffectClass, GetAbilityLevel(), MakeEffectContext());
 *
 *     SpecHandle.Data->SetSetByCallerMagnitude(
 *         FGameplayTag::RequestGameplayTag(FName("GCS.SetByCaller.BaseDamage")),
 *         50.f); // raw/base damage before AttackPower scaling & mitigation
 *
 *     SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
 *
 * Or from Blueprint: use "Assign Tag Set By Caller Magnitude" on the Spec Handle
 * returned by "Make Outgoing Gameplay Effect Spec", using the GCS.SetByCaller.BaseDamage
 * gameplay tag, before applying the spec to the target.
 *
 * The Gameplay Effect asset itself (e.g. GE_Damage_Melee) should have NO modifiers
 * defined on it for IncomingDamage - the modifier is generated entirely by this
 * execution calculation and added to OutExecutionOutput at runtime. The GE should
 * simply have this class assigned as one of its "Execution Calculations".
 *
 * ---------------------------------------------------------------------------
 * REQUIRED GAMEPLAY TAGS:
 * ---------------------------------------------------------------------------
 * This calculation references two tags declared natively in the GCSGameplayTags
 * namespace (Source/GASCombatStarter/Public/GCSNativeGameplayTags.h and its .cpp):
 *   - GCSGameplayTags::SetByCaller_BaseDamage  -> "GCS.SetByCaller.BaseDamage"
 *   - GCSGameplayTags::Effect_CriticalHit      -> "GCS.Effect.CriticalHit"
 *
 * Both must also be registered in Config/DefaultGASCombatStarterTags.ini (this has
 * already been done as part of adding this execution calculation):
 *
 * [/Script/GameplayTags.GameplayTagsSettings]
 * +GameplayTagList=(Tag="GCS.SetByCaller.BaseDamage",Comment="Raw/base damage magnitude passed into GEs via SetByCaller before AttackPower scaling and DefensePower mitigation.")
 * +GameplayTagList=(Tag="GCS.Effect.CriticalHit",Comment="Applied to the target's tags for one execution when a damage calculation rolls a critical hit. Useful for triggering VFX/SFX/UI gameplay cues.")
 *
 * Native declarations and the ini must stay in sync -- if you rename either tag,
 * update both places (see the NOTE comments in GCSNativeGameplayTags.h/.cpp).
 */
UCLASS()
class GASCOMBATSTARTER_API UGCSExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UGCSExecCalc_Damage();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
