import os
import platform
import unreal

if unreal.is_editor() == True and platform.system() != 'Darwin' and unreal.StaticMeshExporterUsd.is_usd_available() == True:
    import usd_unreal.export_sequence_class
