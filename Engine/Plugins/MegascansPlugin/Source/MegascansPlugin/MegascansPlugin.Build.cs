// Copyright Epic Games, Inc. All Rights Reserved.
namespace UnrealBuildTool.Rules
{
	public class MegascansPlugin : ModuleRules
	{
		public MegascansPlugin(ReadOnlyTargetRules Target)
			: base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"AssetRegistry",
					"ContentBrowser",
					"Core",
					"CoreUObject",
					"EditorStyle",
					"Engine",
					"LevelEditor",
					"PythonScriptPlugin",
					"Settings",
					"Slate",
					"SlateCore",
                    "Sockets",
                    "Networking",
                    "EditorScriptingUtilities",
					"Json",
					"JsonUtilities",
                    "UnrealEd",
                    "MaterialEditor",
					"AlembicImporter",
					"AlembicLibrary",
					"FoliageEdit",
                    "Foliage",
					"HTTP",
					"SQLiteCore",
					"SQLiteSupport",

				}
			);
		}
	}
}