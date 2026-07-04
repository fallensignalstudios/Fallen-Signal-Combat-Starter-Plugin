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
}
