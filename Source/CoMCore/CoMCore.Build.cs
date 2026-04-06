// Copyright Mythforge Studios. All Rights Reserved.

using UnrealBuildTool;

public class CoMCore : ModuleRules
{
	public CoMCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Expose module root so CoMCore subdirs can use relative includes (e.g. "Data/", "CoreTypes/")
		PublicIncludePaths.Add(ModuleDirectory);


		// Pure game logic — zero rendering dependencies
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"GameplayAbilities",
			"EnhancedInput",   // ACoMHumanPlayerController input binding stubs (Sprint 1)
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"NetCore",
		});

		// Automation tests — only compiled for test targets
		if (Target.Configuration == UnrealTargetConfiguration.Test ||
		    Target.Configuration == UnrealTargetConfiguration.Development ||
		    Target.Configuration == UnrealTargetConfiguration.Debug ||
		    Target.Configuration == UnrealTargetConfiguration.DebugGame)
		{
			PrivateDependencyModuleNames.Add("AutomationController");
		}

		// No UMG, no Renderer, no RHI — CoMCore is purely logic
	}
}
