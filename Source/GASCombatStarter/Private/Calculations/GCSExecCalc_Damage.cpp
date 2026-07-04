// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Calculations/GCSExecCalc_Damage.h"

#include "AttributeSets/GCSCombatAttributeSet.h"
#include "GCSNativeGameplayTags.h"
#include "GameplayEffectTypes.h"
#include "Math/UnrealMathUtility.h"

/**
 * Local capture-definition struct.
 *
 * This is the standard GAS pattern (see ActionRPG's GDCurveAttributeSet-based
 * execution calcs, or Lyra's ULyraDamageExecution) for declaring which attributes
 * this execution calculation needs, and how they should be captured:
 *   - Source vs Target
 *   - Snapshot vs non-snapshot
 *
 * Snapshot = TRUE:  the attribute's value is frozen at the moment the
 *                    GameplayEffectSpec is created (e.g. when the ability activates).
 *                    Used here for AttackPower, since we want the attacker's power
 *                    AT THE TIME THE HIT WAS THROWN, not whatever it changes to by
 *                    the time the effect actually executes (relevant for projectiles,
 *                    DoTs, or anything with travel/application delay).
 *
 * Snapshot = FALSE: the attribute is captured live, at the moment of Execute_Implementation.
 *                    Used here for DefensePower/CritChance/CritMultiplier, since we want
 *                    the TARGET's current defensive stats at the moment the damage actually
 *                    lands (e.g. if the target buffed their defense mid-flight, that should count).
 */
struct FGCSDamageCaptureDefinitions
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DefensePower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritMultiplier);

	FGCSDamageCaptureDefinitions()
	{
		// Source, Snapshot = true
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGCSCombatAttributeSet, AttackPower, Source, true);

		// Target, Snapshot = false
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGCSCombatAttributeSet, DefensePower, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGCSCombatAttributeSet, CritChance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGCSCombatAttributeSet, CritMultiplier, Target, false);
	}
};

// Function-local static accessor for the capture definitions singleton.
// This is the idiomatic GAS pattern to avoid static-initialization-order fiasco
// with the global UProperty system (attributes must be fully initialized first).
static const FGCSDamageCaptureDefinitions& GCSDamageCaptureDefinitions()
{
	static FGCSDamageCaptureDefinitions Definitions;
	return Definitions;
}

// SetByCaller tag used to pass the raw/base damage magnitude into this execution
// calculation from whichever GameplayEffect triggers it, and the tag applied to
// the target's output tags when a critical hit lands. Both are declared natively
// in GCSGameplayTags (see GCSNativeGameplayTags.h/.cpp) and registered in
// Config/DefaultGASCombatStarterTags.ini. See GCSExecCalc_Damage.h for full
// usage instructions.

UGCSExecCalc_Damage::UGCSExecCalc_Damage()
{
	// Register which captured attributes this execution calculation relies on.
	// GAS uses this list to know what to gather before Execute_Implementation runs.
	RelevantAttributesToCapture.Add(GCSDamageCaptureDefinitions().AttackPowerDef);
	RelevantAttributesToCapture.Add(GCSDamageCaptureDefinitions().DefensePowerDef);
	RelevantAttributesToCapture.Add(GCSDamageCaptureDefinitions().CritChanceDef);
	RelevantAttributesToCapture.Add(GCSDamageCaptureDefinitions().CritMultiplierDef);
}

void UGCSExecCalc_Damage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// Gather tags for magnitude calculations that use SetByCaller / tag-based
	// scaling (e.g. Set/Reference curves keyed off SourceTags or TargetTags).
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// ------------------------------------------------------------------
	// 1. BaseDamage = SetByCaller(GCS.SetByCaller.BaseDamage)
	// ------------------------------------------------------------------
	// GetSetByCallerMagnitude's third parameter (bWarnIfNotFound) is set to false
	// and we explicitly default to 0.f ourselves so a missing SetByCaller tag on
	// the triggering GE fails soft (zero damage) rather than asserting/crashing -
	// useful during content iteration, but keep an eye on log warnings below.
	float BaseDamage = Spec.GetSetByCallerMagnitude(GCSGameplayTags::SetByCaller_BaseDamage, false, 0.f);

	if (BaseDamage <= 0.f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UGCSExecCalc_Damage: GE '%s' executed with BaseDamage <= 0. Did you forget to call SetSetByCallerMagnitude(GCS.SetByCaller.BaseDamage, ...) before applying the spec?"),
			*Spec.Def->GetName());
	}

	// ------------------------------------------------------------------
	// Capture attribute magnitudes.
	// ------------------------------------------------------------------
	float AttackPower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GCSDamageCaptureDefinitions().AttackPowerDef, EvaluationParameters, AttackPower);
	AttackPower = FMath::Max(AttackPower, 0.f);

	float DefensePower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GCSDamageCaptureDefinitions().DefensePowerDef, EvaluationParameters, DefensePower);
	DefensePower = FMath::Max(DefensePower, 0.f);

	float CritChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GCSDamageCaptureDefinitions().CritChanceDef, EvaluationParameters, CritChance);
	CritChance = FMath::Clamp(CritChance, 0.f, 1.f);

	float CritMultiplier = 1.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GCSDamageCaptureDefinitions().CritMultiplierDef, EvaluationParameters, CritMultiplier);
	CritMultiplier = FMath::Max(CritMultiplier, 1.f);

	// ------------------------------------------------------------------
	// 2. AttackScaled = BaseDamage * (AttackPower / 100.f)
	// ------------------------------------------------------------------
	// AttackPower of 100 is defined as the "neutral" baseline (1.0x multiplier),
	// so a character with default/unbuffed AttackPower deals exactly BaseDamage.
	const float AttackScaled = BaseDamage * (AttackPower / 100.f);

	// ------------------------------------------------------------------
	// 3. DefenseMitigated = AttackScaled * (1 - Clamp(DefensePower / (DefensePower + 100), 0, 0.85))
	// ------------------------------------------------------------------
	// This is a classic "soft" mitigation curve: mitigation approaches but never
	// reaches 100% as DefensePower grows, giving diminishing returns on stacking
	// defense, while the explicit Clamp(..., 0.f, 0.85f) enforces a hard damage
	// floor of 15% of AttackScaled always getting through, regardless of how much
	// defense the target stacks. This prevents defense from ever making a target
	// fully immune to damage.
	const float MitigationFraction = FMath::Clamp(DefensePower / (DefensePower + 100.f), 0.f, 0.85f);
	const float DefenseMitigated = AttackScaled * (1.f - MitigationFraction);

	// ------------------------------------------------------------------
	// 4. Critical hit roll.
	// ------------------------------------------------------------------
	// FMath::FRand() returns a pseudo-random float in [0, 1). If it lands below
	// CritChance, the hit is a critical and DamageAfterCrit is scaled up by
	// CritMultiplier. Note that per this system's design, CritChance/CritMultiplier
	// are captured from the TARGET rather than the attacker - conceptually this
	// models the target's "vulnerability to critical hits" rather than the
	// attacker's precision/luck. Keep that design intent in mind when authoring
	// CritChance/CritMultiplier values on the target's attribute set / GEs. If
	// your game instead wants attacker-driven crit chance, change the capture
	// definitions for CritChance/CritMultiplier at the top of this file from
	// Target to Source.
	float DamageAfterCrit = DefenseMitigated;
	const bool bIsCriticalHit = FMath::FRand() < CritChance;

	if (bIsCriticalHit)
	{
		DamageAfterCrit *= CritMultiplier;

		// Add the critical-hit tag to the target's tag output for this execution.
		// This allows GameplayCues, ability triggers, or UI to react to crits
		// (e.g. spawn "CRIT!" floating text, play a distinct hit sound/VFX).
		// AddOutputModifier is not used for tags - target tags are added via the
		// dedicated tag container output on FGameplayEffectCustomExecutionOutput.
		FGameplayTagContainer CriticalHitTags;
		CriticalHitTags.AddTag(GCSGameplayTags::Effect_CriticalHit);
		OutExecutionOutput.AddOutputTags(CriticalHitTags);
	}

	// Final damage floor: never output negative damage.
	const float FinalDamage = FMath::Max(DamageAfterCrit, 0.f);

	// ------------------------------------------------------------------
	// 5. Output to IncomingDamage (Additive). UGCSCombatAttributeSet::
	//    PostGameplayEffectExecute is responsible for routing this value through
	//    Shield -> Health and then resetting IncomingDamage back to zero.
	// ------------------------------------------------------------------
	if (FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				UGCSCombatAttributeSet::GetIncomingDamageAttribute(),
				EGameplayModOp::Additive,
				FinalDamage));
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Verbose,
		TEXT("UGCSExecCalc_Damage: Source=%s Target=%s Base=%.2f AtkPow=%.2f AtkScaled=%.2f DefPow=%.2f MitigationFrac=%.2f Mitigated=%.2f Crit=%s(x%.2f) Final=%.2f"),
		SourceActor ? *SourceActor->GetName() : TEXT("None"),
		TargetActor ? *TargetActor->GetName() : TEXT("None"),
		BaseDamage, AttackPower, AttackScaled, DefensePower, MitigationFraction, DefenseMitigated,
		bIsCriticalHit ? TEXT("true") : TEXT("false"), CritMultiplier, FinalDamage);
#endif
}
