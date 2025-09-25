using UnrealBuildTool;
using System.Collections.Generic;

public class OverrideClientTarget : TargetRules
{
    public OverrideClientTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Client;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.AddRange( new string[] { "Override" } );
        CppStandard = CppStandardVersion.Cpp20;
    }
}