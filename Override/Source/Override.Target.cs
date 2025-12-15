using UnrealBuildTool;

public class OverrideTarget : TargetRules
{
	public OverrideTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange(new string[] { "Override", "HttpServer" });
        CppStandard = CppStandardVersion.Latest;
	}
}