using UnrealBuildTool;
using System.Collections.Generic;

public class OverrideClientTarget : TargetRules
{
    public OverrideClientTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Client;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        CppStandard = CppStandardVersion.Cpp20;
        ExtraModuleNames.Add("Override");
    }
}