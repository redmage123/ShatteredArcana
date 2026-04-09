// Copyright Mythforge Studios. All Rights Reserved.

using UnrealBuildTool;

public class CoMAI : ModuleRules
{
	public CoMAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Expose module root so subdirs (Strategic/, Tactical/, Difficulty/) can use
		// relative includes and other modules can include "CoMAI/CoMAISubsystem.h".
		PublicIncludePaths.Add(ModuleDirectory);

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
