// Copyright Mythforge Studios. All Rights Reserved.

using UnrealBuildTool;

public class CoMModKit : ModuleRules
{
	public CoMModKit(ReadOnlyTargetRules Target) : base(Target)
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
