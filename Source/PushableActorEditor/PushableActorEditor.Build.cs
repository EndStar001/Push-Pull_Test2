// Copyright 2015 Greg Mladucky

using UnrealBuildTool;

public class PushableActorEditor : ModuleRules
{
	public PushableActorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"PushableActor"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AssetTools",
			"EditorStyle",
			"Projects",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
