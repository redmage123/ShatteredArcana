// Copyright Mythforge Studios. All Rights Reserved.

using UnrealBuildTool;

public class CoMEditor : ModuleRules
{
	public CoMEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"CoMCore",
			"PropertyEditor",
			"EditorStyle",
			"AssetTools",
		});
	}
}
