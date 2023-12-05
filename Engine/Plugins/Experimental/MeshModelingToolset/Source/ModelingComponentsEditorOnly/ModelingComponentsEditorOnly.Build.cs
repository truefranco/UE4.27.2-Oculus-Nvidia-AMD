// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ModelingComponentsEditorOnly : ModuleRules
{
	public ModelingComponentsEditorOnly(ReadOnlyTargetRules Target) : base(Target)
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
                "InteractiveToolsFramework",
                "MeshDescription",
				"StaticMeshDescription",
				"GeometricObjects",
				"GeometryAlgorithms",
				"DynamicMesh",
				"MeshConversion",
				"ModelingOperators",
				"ModelingComponents"
				// ... add other public dependencies that you statically link with here ...
			}
            );
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"InputCore",
                "RenderCore",
				"PhysicsCore",
				"Slate",
                "RHI",
				"AssetTools",
				"UnrealEd", 
				
			}
			);

		// OpenSubdiv has Windows, Mac and Unix support
		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Windows) || Target.Platform == UnrealTargetPlatform.Mac || Target.IsInPlatformGroup(UnrealPlatformGroup.Unix))
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"OpenSubdiv",
				}
				);
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
