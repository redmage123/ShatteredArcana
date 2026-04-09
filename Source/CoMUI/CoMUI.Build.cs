// Copyright Mythforge Studios. All Rights Reserved.

using UnrealBuildTool;

public class CoMUI : ModuleRules
{
	public CoMUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CoMCore",
			"UMG",
			"Slate",
			"SlateCore",
			"InputCore",
			"GameplayTags",
		});
	}
}
