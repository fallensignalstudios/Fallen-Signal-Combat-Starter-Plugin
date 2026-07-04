// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Native GameplayTag declarations for the GAS Combat Starter plugin.
 *
 * These mirror the tags defined in Config/DefaultGASCombatStarterTags.ini.
 * Declaring them natively lets C++ code reference tags without brittle
 * FGameplayTag::RequestGameplayTag(FName) string lookups, while Blueprints
 * can still use the same tags via the tag picker (they resolve to the same
 * underlying FGameplayTag). If you add a tag to the ini, consider adding a
 * matching declaration here for any tag C++ code needs to reference directly.
 */
namespace GCSGameplayTags
{
	// Ability tags
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Melee);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Projectile);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Block);

	// State tags
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Blocking);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Staggered);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InParryWindow);

	// NOTE: State_InDodge is new -- added for UGCSAbility_Dodge (Dodge/iFrame ability).
	// This tag must ALSO be added to Config/DefaultGASCombatStarterTags.ini as:
	//   +GameplayTagList=(Tag="GCS.State.InDodge",Comment="Applied for the duration of an active dodge/evade maneuver.")
	// so it is registered with UGameplayTagsSettings and visible in the tag picker
	// even before this native definition is compiled in (e.g. for content-only
	// consumers of the plugin). See GCSNativeGameplayTags.cpp for the definition.
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InDodge);

	// Hit-react direction tags
	// NOTE: New for UGCSAbility_HitReact. Also added to
	// Config/DefaultGASCombatStarterTags.ini as GCS.HitReact.Front/Back/Left/Right
	// so they are registered with UGameplayTagsSettings and visible in the tag
	// picker even before this native definition is compiled in.
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_Front);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_Back);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_Left);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_Right);

	// Event tags
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_ParrySuccess);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_ComboAdvance);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);

	// Effect tags
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Heal);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Stagger);

	// Input tags
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_PrimaryAttack);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Block);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Dodge);

	// Cooldown tags
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Melee);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Projectile);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Block);

	// SetByCaller tags -- used to pass raw numeric magnitudes into GameplayEffects
	// at spec-creation time (see UGCSExecCalc_Damage / UGCSExecCalc_Heal). These
	// must ALSO be added to Config/DefaultGASCombatStarterTags.ini:
	//   +GameplayTagList=(Tag="GCS.SetByCaller.BaseDamage",Comment="Raw/base damage magnitude passed into GEs via SetByCaller before AttackPower scaling and DefensePower mitigation.")
	//   +GameplayTagList=(Tag="GCS.SetByCaller.BaseHeal",Comment="Raw/base heal magnitude passed into GEs via SetByCaller.")
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_BaseDamage);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_BaseHeal);

	// NOTE: Effect_CriticalHit is new -- added for UGCSExecCalc_Damage's critical
	// hit roll (see Source/GASCombatStarter/Public/Calculations/GCSExecCalc_Damage.h).
	// Applied to the target's output tag container for one execution when a crit lands.
	// This tag must ALSO be added to Config/DefaultGASCombatStarterTags.ini as:
	//   +GameplayTagList=(Tag="GCS.Effect.CriticalHit",Comment="Applied to the target's tags for one execution when a damage calculation rolls a critical hit. Useful for triggering VFX/SFX/UI gameplay cues.")
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_CriticalHit);

	// GameplayCue tags
	// NOTE: New for UGCSGameplayCueNotify_HitImpact. The "GameplayCue." root is
	// MANDATORY -- UGameplayCueManager/UAbilitySystemComponent only treat tags
	// rooted at "GameplayCue." as valid cue identifiers; a tag like
	// "GCS.HitImpact" would never reach a UGameplayCueNotify_Static/Actor no
	// matter how it's tagged on the class. The "GCS." segment underneath that
	// root is just this plugin's namespace to avoid colliding with cue tags
	// from other plugins. Also added to Config/DefaultGASCombatStarterTags.ini
	// as GameplayCue.GCS.HitImpact / GameplayCue.GCS.CriticalHit.
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_HitImpact);
	GASCOMBATSTARTER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_CriticalHit);
}
