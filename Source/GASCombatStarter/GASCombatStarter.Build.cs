// Copyright Fallen Signal Studios. All Rights Reserved.

using UnrealBuildTool;

public class GASCombatStarter : ModuleRules
{
	public GASCombatStarter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"NetCore"
		});

		// NOTE ON AbilitySystemGlobals:
		// GAS requires UAbilitySystemGlobals::Get().InitGlobalData() to be called once at
		// startup (typically from your game module's StartupModule, or automatically via
		// the "AbilitySystemGlobals" InitGlobalDataOnGameStart config flag). If your project
		// does not already do this (e.g. no other GAS-based plugin sets it up), add the
		// following to Config/DefaultGame.ini:
		//
		//     [/Script/GameplayAbilities.AbilitySystemGlobals]
		//     bInitGlobalDataOnGameStart=true
		//
		// Without this, GameplayEffect execution calculations and global cue containers
		// will not be initialized, and you may see asserts related to FGameplayEffectContext
		// or FGameplayAbilityTargetDataHandle at runtime.

		bEnableUndefinedIdentifierWarnings = false;
	}
}
