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
			"RenderCore",   // Minimap texture BulkData lock/unlock
			"RHI",          // PF_B8G8R8A8 pixel format, FTexture2DMipMap
		});
	}
}
