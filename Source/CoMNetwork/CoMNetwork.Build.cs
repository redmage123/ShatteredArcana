// CoMNetwork.Build.cs
// Owner: Network Architect
// Deps: CoMCore + OnlineSubsystem/Utils
// NEVER import CoMAI, CoMUI, or CoMRendering

using UnrealBuildTool;

public class CoMNetwork : ModuleRules
{
    public CoMNetwork(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "CoMCore",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
        });

        // CoMNetwork must NEVER depend on CoMAI, CoMUI, or CoMRendering.
        // Enforce at code review.
    }
}
