// Copyright 2015 Greg Mladucky

using UnrealBuildTool;
using System.Collections.Generic;

public class PushableActorTarget : TargetRules
{
	public PushableActorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;

		ExtraModuleNames.AddRange( new string[] { "PushableActor" } );
	}
}
