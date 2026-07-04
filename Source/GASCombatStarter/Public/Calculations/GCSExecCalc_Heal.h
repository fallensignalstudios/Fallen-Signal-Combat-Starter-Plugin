// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GCSExecCalc_Heal.generated.h"

/**
 * UGCSExecCalc_Heal
 *
 * Minimal healing execution calculation for GASCombatStarter.
 *
 * Pipeline:
 *   1. Pulls a raw "BaseHeal" value from the triggering GameplayEffectSpec via
 *      SetByCaller (tag: GCS.SetByCaller.BaseHeal).
 *   2. Writes that value out as an Additive modifier.
 *
 * ---------------------------------------------------------------------------
 * WHERE DOES THE OUTPUT GO? TWO OPTIONS:
 * ---------------------------------------------------------------------------
 * Option A (RECOMMENDED for production - mirrors the IncomingDamage pattern):
 *   Add a new meta-attribute, IncomingHeal, to UGCSCombatAttributeSet, exactly
 *   like IncomingDamage:
 *
 *     UPROPERTY(BlueprintReadOnly, Category = "GCS|Attributes|Meta")
 *     FGameplayAttributeData IncomingHeal;
 *     ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, IncomingHeal)
 *
 *   Then in UGCSCombatAttributeSet::PostGameplayEffectExecute, branch on
 *   Data.EvaluatedData.Attribute == GetIncomingHealAttribute(): clamp/apply the
 *   value onto Health (respecting MaxHealth), then zero IncomingHeal back out -
 *   the same "meta-attribute funnel" pattern already used for IncomingDamage.
 *   This keeps ALL attribute mutation logic centralized in PostGameplayEffectExecute,
 *   which is the idiomatic GAS approach (see Lyra's ULyraHealthSet / ActionRPG), and
 *   gives you one place to add heal-specific side effects later (overheal-to-shield
 *   conversion, heal-received modifiers, lifesteal caps, heal GameplayCues, etc.).
 *   To switch to this option: add IncomingHeal as described above, then change the
 *   output attribute in the .cpp from GetHealthAttribute() to GetIncomingHealAttribute().
 *
 * Option B (SIMPLER, less flexible - THIS IS WHAT IS IMPLEMENTED BELOW BY DEFAULT):
 *   Skip the meta-attribute entirely and output an Additive modifier straight onto
 *   Health. UGCSCombatAttributeSet::PreAttributeChange already clamps Health to
 *   [0, MaxHealth] on every change (see GCSCombatAttributeSet.cpp), so overheal is
 *   automatically capped without any extra work here. This avoids adding a new
 *   attribute, but means heal logic and damage logic are applied through different
 *   paths, and you lose a central interception point for heal-specific side effects.
 *
 * ---------------------------------------------------------------------------
 * HOW TO FEED THIS FROM A GAMEPLAY EFFECT (SetByCaller usage):
 * ---------------------------------------------------------------------------
 *     FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
 *         HealGameplayEffectClass, GetAbilityLevel(), MakeEffectContext());
 *
 *     SpecHandle.Data->SetSetByCallerMagnitude(
 *         FGameplayTag::RequestGameplayTag(FName("GCS.SetByCaller.BaseHeal")),
 *         35.f); // raw heal amount
 *
 *     SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
 *
 * ---------------------------------------------------------------------------
 * REQUIRED GAMEPLAY TAGS:
 * ---------------------------------------------------------------------------
 * This calculation references one tag declared natively in the GCSGameplayTags
 * namespace (Source/GASCombatStarter/Public/GCSNativeGameplayTags.h and its .cpp):
 *   - GCSGameplayTags::SetByCaller_BaseHeal -> "GCS.SetByCaller.BaseHeal"
 *
 * It must also be registered in Config/DefaultGASCombatStarterTags.ini (this has
 * already been done as part of adding this execution calculation):
 *
 * [/Script/GameplayTags.GameplayTagsSettings]
 * +GameplayTagList=(Tag="GCS.SetByCaller.BaseHeal",Comment="Raw/base heal magnitude passed into GEs via SetByCaller.")
 *
 * Native declarations and the ini must stay in sync -- if you rename this tag,
 * update both places (see the NOTE comments in GCSNativeGameplayTags.h/.cpp).
 */
UCLASS()
class GASCOMBATSTARTER_API UGCSExecCalc_Heal : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UGCSExecCalc_Heal();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
