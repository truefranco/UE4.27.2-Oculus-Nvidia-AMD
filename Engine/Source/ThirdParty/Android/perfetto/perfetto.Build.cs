// Copyright Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class perfetto : ModuleRules
{
	public perfetto(ReadOnlyTargetRules Target) : base(Target)
	{	
		string perfettopath = Target.UEThirdPartySourceDirectory + "Android/perfetto/";
		PublicIncludePaths.Add(perfettopath);
    }
}
