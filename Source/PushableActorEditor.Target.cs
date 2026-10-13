// Copyright 2015 Greg Mladucky

using UnrealBuildTool;
using System.Collections.Generic;

public class PushableActorEditorTarget : TargetRules
{
	public PushableActorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;

		ExtraModuleNames.AddRange( new string[] { "PushableActor", "PushableActorEditor" } );
	}
}
