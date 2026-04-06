// Copyright Mythforge Studios. All Rights Reserved.

using UnrealBuildTool;

public class CoMAI : ModuleRules
{
	public CoMAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CoMCore",
			"GameplayTags",
		});
	}
}
