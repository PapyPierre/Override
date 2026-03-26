using UnrealBuildTool;

[SupportedPlatforms(UnrealPlatformClass.Server)]
public class OverrideServerTarget : TargetRules
{
    public OverrideServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.AddRange(new string[] { "Override", "HTTPServer" });
        CppStandard = CppStandardVersion.Latest;
        
        bUseLoggingInShipping = true;
    }
}