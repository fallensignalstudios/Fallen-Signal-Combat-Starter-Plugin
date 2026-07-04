// Copyright Fallen Signal Studios. All Rights Reserved.

using UnrealBuildTool;

public class GASCombatStarterEditor : ModuleRules
{
	public GASCombatStarterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GASCombatStarter",
			"GameplayAbilities",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UnrealEd",
			"EditorStyle",
			"EditorSubsystem",
			"AssetTools",
			"PropertyEditor"
		});
	}
}
