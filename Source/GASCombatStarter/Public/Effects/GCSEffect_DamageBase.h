// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GCSEffect_DamageBase.generated.h"

/**
 * UGCSEffect_DamageBase
 *
 * =============================================================================
 * READ ME BEFORE CREATING DAMAGE GAMEPLAY EFFECTS
 * =============================================================================
 *
 * In GAS, GameplayEffects (GEs) are data assets, not code you subclass to add
 * behavior to (with one exception: a UGameplayEffectExecutionCalculation,
 * which IS a C++/Blueprint class a GE can reference). This class exists as a
 * thin, documented C++ parent so your project can:
 *
 *   1. Give all damage GEs a common Blueprintable base with sane defaults
 *      pre-configured (Instant duration, "Damage" asset tag, etc.) so
 *      designers don't have to remember to set them by hand every time.
 *   2. Have a single place to attach a shared UGameplayEffectExecutionCalculation
 *      subclass (see below) that all damage GEs use.
 *
 * -----------------------------------------------------------------------------
 * HOW TO ACTUALLY BUILD A DAMAGE GAMEPLAY EFFECT WITH THIS BASE
 * -----------------------------------------------------------------------------
 *
 * 1. In the editor, create a new Blueprint GameplayEffect asset and reparent
 *    it to UGCSEffect_DamageBase (Class Viewer -> GCS Effect Damage Base).
 *
 * 2. Set Duration Policy to "Instant" (most damage GEs are instant -- damage
 *    over time effects should use "Duration" or "Infinite" with periodic
 *    execution instead, and can still derive from this same base).
 *
 * 3. Add a Modifier targeting UGCSCombatAttributeSet::IncomingDamage
 *    (Operation = Add). Set its Magnitude Calculation Type to
 *    "Custom Calculation Class" and point it at a
 *    UGameplayEffectExecutionCalculation subclass you write for your project,
 *    e.g. UGCSDamageExecCalculation, which should:
 *
 *      - Capture AttackPower from the Source (instigator) attribute set.
 *      - Capture DefensePower from the Target attribute set.
 *      - Capture CritChance / CritMultiplier from the Source.
 *      - Roll for critical hit, compute FinalDamage = (AttackPower - DefensePower
 *        mitigation curve of your choosing) * CritMultiplier (if crit).
 *      - Output FinalDamage as the modifier's evaluated magnitude, targeting
 *        IncomingDamage.
 *
 *    This plugin does not ship that execution calculation because the exact
 *    damage formula (linear vs. percentage mitigation, armor curves, elemental
 *    types, etc.) is precisely the kind of game-specific balancing logic this
 *    starter deliberately leaves to you. Writing your own also makes it
 *    trivial to add damage types later without touching this base class.
 *
 * 4. UGCSCombatAttributeSet::PostGameplayEffectExecute already knows how to
 *    take whatever ends up in IncomingDamage and subtract it from
 *    Shield-then-Health, so once your execution calculation writes a value
 *    there, the rest of the pipeline (including OnHitConfirmed's HitReact
 *    event) works automatically.
 *
 * 5. Tag the GE's "Asset Tags" with GCS.Effect.Damage (see
 *    Config/DefaultGASCombatStarterTags.ini) so gameplay cues, analytics, or
 *    damage-type filtering logic can identify it generically.
 *
 * For healing, follow the same pattern with GCS.Effect.Heal and a Health
 * modifier (Operation = Add, positive magnitude) -- healing does not need a
 * meta-attribute detour since it isn't mitigated by Shield/Defense.
 * =============================================================================
 */
UCLASS(Abstract, Blueprintable)
class GASCOMBATSTARTER_API UGCSEffect_DamageBase : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGCSEffect_DamageBase();
};
