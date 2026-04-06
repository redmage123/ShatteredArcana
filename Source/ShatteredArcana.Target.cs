// Copyright Mythforge Studios. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ShatteredArcanaTarget : TargetRules
{
	public ShatteredArcanaTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;

		ExtraModuleNames.AddRange(new string[]
		{
			"ShatteredArcana",
			"CoMCore",
			"CoMAI",
			"CoMUI",
			"CoMRendering",
		});
	}
}
