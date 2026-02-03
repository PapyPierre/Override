using UnrealBuildTool;

public class OverrideEditor : ModuleRules
{
	public OverrideEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"EditorSubsystem",
			"Sockets",
			"Networking",
			"Override",
			"LevelEditor",
			"InputCore",
			"Json",
			"JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
        {
			"Override",
			"EditorFramework"
		});
	}
}