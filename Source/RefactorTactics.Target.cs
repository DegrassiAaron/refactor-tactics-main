using UnrealBuildTool;
using System.Collections.Generic;

public class RefactorTacticsTarget : TargetRules
{
	public RefactorTacticsTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("RefactorTactics");
	}
}
