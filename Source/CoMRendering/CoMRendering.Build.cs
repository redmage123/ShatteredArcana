// Copyright Mythforge Studios. All Rights Reserved.

using UnrealBuildTool;

public class CoMRendering : ModuleRules
{
	public CoMRendering(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Expose module root so subdirs can use relative includes (e.g. "RHI/", "Benchmark/")
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CoMCore",
			"RenderCore",
			"RHI",
			"Renderer",
			"Niagara",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Benchmark harness — JSON output (CoMBenchmarkWriter)
			"Json",
			"JsonUtilities",
		});

		// Platform-agnostic: always go through UE5 RHI
		// Never call DX12/Vulkan/Metal APIs directly
	}
}
