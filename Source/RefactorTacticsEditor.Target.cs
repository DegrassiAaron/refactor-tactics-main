using UnrealBuildTool;
using System.Collections.Generic;

public class RefactorTacticsEditorTarget : TargetRules
{
	public RefactorTacticsEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("RefactorTactics");
	}
}
