// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class PhysicsControl : ModuleRules
	{
		public PhysicsControl(ReadOnlyTargetRules Target) : base(Target)
		{
			
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
				"Core"
				
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
				"CoreUObject",
				"PhysicsCore",
				"Engine"
				}
			);

		}
	}
}