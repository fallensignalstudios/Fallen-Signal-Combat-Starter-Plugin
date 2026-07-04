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
}
