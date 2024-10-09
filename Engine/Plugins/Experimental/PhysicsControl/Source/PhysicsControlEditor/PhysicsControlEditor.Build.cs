// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class PhysicsControlEditor : ModuleRules
	{
		public PhysicsControlEditor(ReadOnlyTargetRules Target) : base(Target)
		{

			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);


			PrivateIncludePaths.AddRange(
				new string[]
				{
					// ... add other private include paths required here ...
				}
				);


			PublicDependencyModuleNames.AddRange(
				new string[]
				{
				"Core"
					// ... add other public dependencies that you statically link with here ...
				}
				);


			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"Persona",
				"AnimationEditor",
				"ComponentVisualizers",
				"AnimGraph",
				"WorkspaceMenuStructure",
				"EditorStyle",
				"PhysicsControl",
				"Kismet"
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
}