// Copyright Mythforge Studios. All Rights Reserved.

using UnrealBuildTool;

public class ShatteredArcana : ModuleRules
{
	public ShatteredArcana(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CoMCore",
			"CoMAI",
			"CoMUI",
			"CoMRendering",
			"EnhancedInput",
		});
	}
}
