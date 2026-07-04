// Copyright Fallen Signal Studios. All Rights Reserved.

#include "GCSNativeGameplayTags.h"

namespace GCSGameplayTags
{
	// Ability tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Melee, "GCS.Ability.Melee", "Tag granted to / identifying melee attack abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Projectile, "GCS.Ability.Projectile", "Tag granted to / identifying projectile attack abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Block, "GCS.Ability.Block", "Tag granted to / identifying block abilities.");

	// State tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Blocking, "GCS.State.Blocking", "Applied while the actor is actively blocking.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Staggered, "GCS.State.Staggered", "Applied while the actor is staggered / hit-reacting and unable to act normally.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "GCS.State.Dead", "Applied when the actor's Health has reached zero.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_InParryWindow, "GCS.State.InParryWindow", "Applied during the brief window in which an incoming attack can be parried.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_InDodge, "GCS.State.InDodge", "Applied for the duration of an active dodge/evade maneuver (see UGCSAbility_Dodge). Also needs a matching entry in DefaultGASCombatStarterTags.ini.");

	// Hit-react direction tags (see UGCSAbility_HitReact)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact_Front, "GCS.HitReact.Front", "Hit-react direction: instigator was primarily in front of the target.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact_Back, "GCS.HitReact.Back", "Hit-react direction: instigator was primarily behind the target.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact_Left, "GCS.HitReact.Left", "Hit-react direction: instigator was primarily to the target's left.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact_Right, "GCS.HitReact.Right", "Hit-react direction: instigator was primarily to the target's right.");

	// Event tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_HitReact, "GCS.Event.HitReact", "GameplayEvent sent to a target when it is hit, to trigger hit-react montages/abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_ParrySuccess, "GCS.Event.ParrySuccess", "GameplayEvent sent when a parry window successfully blocks/counters an attack.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_ComboAdvance, "GCS.Event.ComboAdvance", "GameplayEvent sent to advance a melee combo to its next stage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Death, "GCS.Event.Death", "GameplayEvent sent when an actor dies.");

	// Effect tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage, "GCS.Effect.Damage", "Asset tag applied to damage GameplayEffects.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Heal, "GCS.Effect.Heal", "Asset tag applied to healing GameplayEffects.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Stagger, "GCS.Effect.Stagger", "Asset tag applied to stagger/hit-react GameplayEffects.");

	// Input tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_PrimaryAttack, "GCS.Input.PrimaryAttack", "Input tag bound to the primary attack action (melee or ranged).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Block, "GCS.Input.Block", "Input tag bound to the block action.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Dodge, "GCS.Input.Dodge", "Input tag bound to the dodge/evade action.");

	// Cooldown tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Melee, "GCS.Cooldown.Melee", "Cooldown tag for melee attack abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Projectile, "GCS.Cooldown.Projectile", "Cooldown tag for projectile attack abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Block, "GCS.Cooldown.Block", "Cooldown tag for block abilities.");

	// SetByCaller tags (see UGCSExecCalc_Damage / UGCSExecCalc_Heal)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_BaseDamage, "GCS.SetByCaller.BaseDamage", "Raw/base damage magnitude passed into GEs via SetByCaller before AttackPower scaling and DefensePower mitigation.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_BaseHeal, "GCS.SetByCaller.BaseHeal", "Raw/base heal magnitude passed into GEs via SetByCaller.");

	// Effect tags (continued -- see UGCSExecCalc_Damage's critical hit roll)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_CriticalHit, "GCS.Effect.CriticalHit", "Applied to the target's tags for one execution when a damage calculation rolls a critical hit. Useful for triggering VFX/SFX/UI gameplay cues.");

	// GameplayCue tags (see UGCSGameplayCueNotify_HitImpact). "GameplayCue." root is mandatory.
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_HitImpact, "GameplayCue.GCS.HitImpact", "Burst GameplayCue for melee/ranged hit impact feedback (VFX/SFX/camera shake). See UGCSGameplayCueNotify_HitImpact.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_CriticalHit, "GameplayCue.GCS.CriticalHit", "Burst GameplayCue broadcast alongside GameplayCue.GCS.HitImpact when a hit's RawMagnitude exceeds the critical-hit threshold. See UGCSGameplayCueNotify_HitImpact.");
}
