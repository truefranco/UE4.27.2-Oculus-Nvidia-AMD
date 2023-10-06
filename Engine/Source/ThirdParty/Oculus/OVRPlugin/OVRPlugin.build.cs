// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OVRPlugin : ModuleRules
{
	public OVRPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		string SourceDirectory = Target.UEThirdPartySourceDirectory + "Oculus/OVRPlugin/OVRPlugin/";

		PublicIncludePaths.Add(SourceDirectory + "Include");

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			RuntimeDependencies.Add(SourceDirectory + "Lib/armeabi-v7a/OpenXR/libOVRPlugin.so");
			RuntimeDependencies.Add(SourceDirectory + "Lib/arm64-v8a/OpenXR/libOVRPlugin.so");
		}
	}
}