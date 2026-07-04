// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Calculations/GCSExecCalc_Heal.h"

#include "AttributeSets/GCSCombatAttributeSet.h"
#include "GCSNativeGameplayTags.h"
#include "GameplayEffectTypes.h"

/**
 * This execution calculation is intentionally simple - it has no attribute
 * captures at all, since the heal amount comes purely from SetByCaller and is
 * applied as a flat Additive modifier. If you later want heal scaling (e.g. by
 * a "HealingPower" stat, similar to AttackPower for damage), follow the exact
 * pattern used in GCSExecCalc_Damage.cpp: declare a capture-definitions struct
 * with DECLARE_ATTRIBUTE_CAPTUREDEF / DEFINE_ATTRIBUTE_CAPTUREDEF, register it
 * in RelevantAttributesToCapture in the constructor, and read it via
 * AttemptCalculateCapturedAttributeMagnitude in Execute_Implementation.
 */

// SetByCaller tag used to pass the raw/base heal magnitude into this execution
// calculation from whichever GameplayEffect triggers it. Declared natively in
// GCSGameplayTags (see GCSNativeGameplayTags.h/.cpp) and registered in
// Config/DefaultGASCombatStarterTags.ini. See GCSExecCalc_Heal.h for full usage
// instructions.

UGCSExecCalc_Heal::UGCSExecCalc_Heal()
{
	// No RelevantAttributesToCapture needed for the minimal SetByCaller-only
	// implementation below. If you add heal-scaling attributes later (see the
	// comment block above), register their capture definitions here.
}

void UGCSExecCalc_Heal::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// ------------------------------------------------------------------
	// BaseHeal = SetByCaller(GCS.SetByCaller.BaseHeal)
	// ------------------------------------------------------------------
	// As with damage, bWarnIfNotFound is false and we default to 0.f ourselves,
	// so a missing SetByCaller tag fails soft (no heal applied) instead of
	// asserting - watch for the warning log below during content iteration.
	const float BaseHeal = Spec.GetSetByCallerMagnitude(GCSGameplayTags::SetByCaller_BaseHeal, false, 0.f);

	if (BaseHeal <= 0.f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UGCSExecCalc_Heal: GE '%s' executed with BaseHeal <= 0. Did you forget to call SetSetByCallerMagnitude(GCS.SetByCaller.BaseHeal, ...) before applying the spec?"),
			*Spec.Def->GetName());
		return;
	}

	// ------------------------------------------------------------------
	// Output the heal amount.
	// ------------------------------------------------------------------
	// As shipped, UGCSCombatAttributeSet does NOT yet have an IncomingHeal
	// meta-attribute, so this default implementation uses OPTION B: it outputs
	// directly to Health (Additive). UGCSCombatAttributeSet::PreAttributeChange
	// already clamps Health to [0, MaxHealth] on every change (see
	// GCSCombatAttributeSet.cpp), so overheal is automatically capped there -
	// no extra clamping is required in this execution calculation.
	//
	// To switch to OPTION A (RECOMMENDED for production - see the full rationale
	// in GCSExecCalc_Heal.h): add an IncomingHeal FGameplayAttributeData meta-attribute
	// to UGCSCombatAttributeSet (mirroring IncomingDamage), give it a branch in
	// PostGameplayEffectExecute that reads+resets it and then applies the clamped
	// result onto Health, and change the line below from GetHealthAttribute() to
	// GetIncomingHealAttribute(). This centralizes heal-specific side effects
	// (overheal-to-shield conversion, heal-received modifiers, lifesteal caps,
	// heal-received GameplayCues, etc.) in one place, exactly like IncomingDamage
	// does for the damage pipeline.
	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UGCSCombatAttributeSet::GetHealthAttribute(),
			EGameplayModOp::Additive,
			BaseHeal));

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Verbose,
		TEXT("UGCSExecCalc_Heal: Source=%s Target=%s BaseHeal=%.2f"),
		SourceActor ? *SourceActor->GetName() : TEXT("None"),
		TargetActor ? *TargetActor->GetName() : TEXT("None"),
		BaseHeal);
#endif
}
