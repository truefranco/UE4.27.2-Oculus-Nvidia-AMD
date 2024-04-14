// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
/*=================================================
* FileName: 6DoFGimbalLock.Build.cs
*
* Created by: Nebula Games Inc
* Project name: 6DoFGimbalLock
* Created on: 2024/4/18
*
* =================================================*/
// Fill out your copyright notice in the Description page of Project Settings.
using System;
using System.IO;
using UnrealBuildTool;

public class GimbalLock : ModuleRules
{
	public GimbalLock(ReadOnlyTargetRules Target) : base(Target)
	{
	PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

	PublicIncludePaths.AddRange(
		new string[] {
			// ... add public include paths required here ...
		}
		);


	PrivateIncludePaths.AddRange(
		new string[] {
			// ... add other private include paths required here ...
		}
		);


	PublicDependencyModuleNames.AddRange(
		new string[]
		{
				"Core",
				"CoreUObject",
				"Engine",
			// ... add other public dependencies that you statically link with here ...
		}
		);


	PrivateDependencyModuleNames.AddRange(
		new string[]
		{
				
			// ... add private dependencies that you statically link with here ...	
		}
		);


	DynamicallyLoadedModuleNames.AddRange(
		new string[]
		{
			// ... add any modules that your module loads dynamically here ...
		}
		);
    }

}
	
		

