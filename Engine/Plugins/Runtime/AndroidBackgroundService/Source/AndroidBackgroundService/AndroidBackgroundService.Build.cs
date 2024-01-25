// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;



public class AndroidBackgroundService : ModuleRules
{
	public AndroidBackgroundService(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicIncludePaths.AddRange(
		new string[] {
				"Runtime/Launch/Public"
		}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Launch",
			}
			);



		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			string ModulePath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModulePath, "AndroidBackgroundService_UPL.xml"));
		}
	}
}

