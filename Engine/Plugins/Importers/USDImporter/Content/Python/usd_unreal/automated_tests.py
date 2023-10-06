import unreal
import usd_unreal
import usd_unreal.utils
import usd_unreal.attributes
import usd_unreal.import_level
import usd_unreal.export_sequence
import usd_unreal.export_sequence_class
import usd_unreal.import_level_class
import usd_unreal.diff_usd_files

import importlib
import os
import math

from pxr import Usd, UsdGeom, UsdLux, Sdf, UsdUtils

if usd_unreal.developer_mode == True:
	importlib.reload(usd_unreal)
	importlib.reload(usd_unreal.utils)
	importlib.reload(usd_unreal.attributes)
	importlib.reload(usd_unreal.import_level)
	importlib.reload(usd_unreal.export_sequence)

def compare_usd_files(a, b):
	diff_result = usd_unreal.diff_usd_files.diff_usd_files(a, b)
	if diff_result == "EQUAL":
		return True
	return False

def run_import_level_test():
	absolute_path = os.path.join(unreal.SystemLibrary.get_project_content_directory(), 'Tests/USD/ImportLevelTest.usda')
	unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
	import_task = usd_unreal.import_level.USDImportLevel()
	import_task.allow_asset_import = False
	import_task.generated_asset_base_path = "/Game/Tests/USD/Generated/ImportLevelTest/"
	import_task.import_level(absolute_path)

def run_import_static_meshes():
	absolute_path = os.path.join(unreal.SystemLibrary.get_project_content_directory(), 'Tests/USD/ImportLevelTest.usda')

	options = unreal.UsdStageImportOptions()
	options.import_actors = False
	options.import_geometry = True
	options.import_materials = False
	options.import_skeletal_animations = False
	options.prim_path_folder_structure = False

	import_task = unreal.AssetImportTask()
	import_task.filename = absolute_path
	import_task.destination_path = "/Game/Tests/USD/Generated/"
	import_task.replace_existing = True
	import_task.automated = True
	import_task.options = options
	unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([import_task])

def run():
	run_import_static_meshes()
	run_import_level_test()
	unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
