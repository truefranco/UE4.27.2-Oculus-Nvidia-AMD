import re
import os
import math
import unreal
from pxr import Usd, UsdGeom, Sdf, UsdUtils, Tf
from usd_unreal.constants import *
from usd_unreal.generic_utils import get_unique_name


class UsdExportContext:
    def __init__(self):
        self.options = unreal.LevelExporterUSDOptions()

        self.world = None                   # UWorld object that is actually being exported (likely the editor world, but can be others when exporting Level assets)
        self.stage = None                   # Opened Usd.Stage for the main scene

        self.root_layer_path = ""           # Full, absolute filepath of the main scene root layer to export
        self.scene_file_extension = ""      # ".usda", ".usd" or ".usdc". Extension to use for scene files, asset files, material override files, etc.

        self.level_to_sublayer = {}         # Maps from unreal.Level to corresponding Sdf.Layer
        self.materials_sublayer = None      # Dedicated layer to author material overrides in

        self.exported_prim_paths = set()    # Set of already used prim paths e.g. "/Root/MyPrim"
        self.exported_assets = {}           # Map from unreal.Object to exported asset path e.g. "C:/MyFolder/Assets/Cube.usda"
        self.baked_materials = {}           # Map from unreal.Object.get_path_name() to exported baked filepath e.g. "C:/MyFolder/Assets/Red.usda"


def sanitize_filepath(full_path):
    drive, path = os.path.splitdrive(full_path)
    path = re.sub(r'[^\(\)\w\s\-/\\\.]', '_', path)
    return os.path.join(drive, path)


def get_prim_path_for_component(component, used_prim_paths, parent_prim_path="", use_folders=False):
    """ Finds a valid, full and unique prim path for a prim corresponding to `component`

    For a level with an actor named `Actor`, with a root component `SceneComponent0` and an attach child
    static mesh component `mesh`, this will emit the path "/<ROOT_PRIM_NAME>/Actor/SceneComponent0/mesh"
    when `mesh` is the `component` argument.

    If provided a parent_prim_path (like "/<ROOT_PRIM_NAME>/Actor/SceneComponent0"), it will just append
    a name to that path. Otherwise it will traverse up the attachment hierarchy to discover the full path.

    :param component: unreal.SceneComponent object to fetch a prim path for
    :param used_prim_paths: Collection of prim paths we're not allowed to use e.g. {"/Root/OtherPrim"}. 
                            This function will append the generated prim path to this collection before returning
    :param parent_prim_path: Path of the parent prim to optimize traversal with e.g. "/Root"
    :returns: Valid, full and unique prim path str e.g. "/Root/Parent/ComponentName"
    """
    name = component.get_name()

    # Use actor name if we're a root component, and also keep track of folder path in that case too
    actor = component.get_owner()
    folder_path_str = ""
    if actor:
        if actor.get_editor_property('root_component') == component:
            name = actor.get_actor_label()
            if use_folders:
                folder_path = actor.get_folder_path()
                if not folder_path.is_none():
                    folder_path_str = str(folder_path)

    # Make name valid for USD
    name = Tf.MakeValidIdentifier(name)

    # If we have a folder path, sanitize each folder name separately or else we lose the slashes in case of nested folders
    # Defining a prim with slashes in its name like this will lead to parent prims being constructed for each segment, if necessary
    if folder_path_str:
        segments = [Tf.MakeValidIdentifier(seg) for seg in folder_path_str.split('/')]
        segments.append(name)
        name = "/".join(segments)

    # Find out our parent prim path if we need to
    if parent_prim_path == "":
        parent_comp = component.get_attach_parent()
        if parent_comp is None:  # Root component of top-level actor -> Top-level prim
            parent_prim_path = "/" + ROOT_PRIM_NAME
        else:
            parent_prim_path = get_prim_path_for_component(
                parent_comp,
                used_prim_paths,
                use_folders=use_folders
            )
    path = parent_prim_path + "/" + name

    path = get_unique_name(used_prim_paths, path)
    used_prim_paths.add(path)

    return path


def add_relative_reference(prim, layer_filepath):
    """ Adds a reference on `prim` to layer `layer_filepath`, but using a relative filepath

    :param prim: Usd.Prim to receive the reference
    :layer_filepath: String, absolute filepath of a layer to add as reference
    :returns: None
    """
    prim_layer_path = prim.GetStage().GetRootLayer().realPath
    relative_layer_path = unreal.UsdConversionLibrary.make_path_relative_to_layer(prim_layer_path, layer_filepath)
    prim.GetReferences().AddReference(relative_layer_path)


def get_schema_name_for_component(component):
    """ Uses a priority list to figure out what is the best schema for a prim matching `component`

    Matches UsdUtils::GetComponentTypeForPrim() from the USDImporter C++ source.

    :param component: unreal.SceneComponent object or derived
    :returns: String containing the intended schema for a prim matching `component` e.g. "UsdGeomMesh"
    """

    owner_actor = component.get_owner()
    if isinstance(owner_actor, unreal.InstancedFoliageActor):
        return 'PointInstancer'
    elif isinstance(owner_actor, unreal.LandscapeProxy):
        return 'Mesh'

    if isinstance(component, unreal.SkinnedMeshComponent):
        return 'SkelRoot'
    elif isinstance(component, unreal.HierarchicalInstancedStaticMeshComponent):
        # The original HISM component becomes just a regular Xform prim, so that we can handle
        # its children correctly. We'll manually create a new child PointInstancer prim to it 
        # however, and convert the HISM data onto that prim.
        # c.f. convert_component()
        return 'Xform'
    elif isinstance(component, unreal.StaticMeshComponent):
        return 'Mesh'
    elif isinstance(component, unreal.CineCameraComponent):
        return 'Camera'
    elif isinstance(component, unreal.DirectionalLightComponent):
        return 'DistantLight'
    elif isinstance(component, unreal.RectLightComponent):
        return 'RectLight'
    elif isinstance(component, unreal.PointLightComponent):  # SpotLightComponent derives PointLightComponent
        return 'SphereLight'
    elif isinstance(component, unreal.SkyLightComponent):
        return 'DomeLight'
    elif isinstance(component, unreal.SceneComponent):
        return 'Xform'
    return ""


def should_export_actor(actor):
    """ Heuristic used to decide whether the received unreal.Actor should be exported or not

    :param actor: unreal.Actor to check 
    :returns: True if the actor should be exported
    """
    actor_classes_to_ignore = set([
        unreal.AbstractNavData,
        unreal.AtmosphericFog,
        unreal.Brush,
        unreal.DefaultPhysicsVolume,
        unreal.GameModeBase,
        unreal.GameNetworkManager,
        unreal.GameplayDebuggerCategoryReplicator,
        unreal.GameplayDebuggerPlayerManager,
        unreal.GameSession,
        unreal.GameStateBase,
        unreal.HUD,
        unreal.LevelSequenceActor,
        unreal.ParticleEventManager,
        unreal.PlayerCameraManager,
        unreal.PlayerController,
        unreal.PlayerStart,
        unreal.PlayerState,
        unreal.SphereReflectionCapture,
        unreal.USDLevelInfo,
        unreal.UsdStageActor,
        unreal.WorldSettings
    ])
    actor_class_names_to_ignore = set(['BP_Sky_Sphere_C'])

    # The editor world's persistent level always has a foliage actor, but it may be empty/unused
    if isinstance(actor, unreal.InstancedFoliageActor):
        foliage_types = actor.get_used_foliage_types()
        for foliage_type in foliage_types:
            transforms = actor.get_instance_transforms(foliage_type, actor.get_outer())
            if len(transforms) > 0:
                return True
        return False

    # This is a tag added to all actors spawned by the UsdStageActor
    if actor.actor_has_tag("SequencerActor"):
        return False

    for unreal_class in actor_classes_to_ignore:
        if isinstance(actor, unreal_class):
            return False

    unreal_class_name = actor.get_class().get_name()
    if unreal_class_name in actor_class_names_to_ignore:
        return False

    return True


def get_filename_to_export_to(folder, asset, extension):
    """ Returns the full usda filename to use for exported USD assets for a given unreal.Object

    :param folder: String path of folder to contain all exported assets (e.g. "C:/MyFolder")
    :param asset: unreal.Object to export
    :returns: Full path to an usda file that should be used as export target for `asset` (e.g. "C:/MyFolder/Game/Colors/Red.usda")
    """
    # '/Game/SomeFolder/MyMesh.MyMesh'
    path_name = asset.get_path_name()

    # '/Game/SomeFolder/MyMesh'
    path_name = path_name[:path_name.rfind('.')]

    # 'Game/SomeFolder/MyMesh' (or else path.join gets confused)
    if path_name.startswith('/'):
        path_name = path_name[1:]

    # 'C:/MyFolder/ExportedScene/Assets/Game/SomeFolder/MyMesh.usda'
    return sanitize_filepath(os.path.normpath(os.path.join(folder, 'Assets', path_name + extension)))


def get_actor_asset_folder_to_export_to(folder, actor):
    """ Returns the full usda filename to use for exported USD assets for a given unreal.Actor

    :param folder: String path of folder to contain all exported assets for the scene (e.g. "C:/MyFolder")
    :param actor: unreal.Actor 
    :returns: Full path to an usda file that should be used as export target for `asset` (e.g. "C:/MyFolder/Game/Colors/Red.usda")
    """
    # '/Game/NewMap.NewMap:PersistentLevel'
    path_name = actor.get_outer().get_path_name()

    # '/Game/NewMap'
    path_name = path_name[:path_name.rfind('.')]

    # 'Game/NewMap' (or else path.join gets confused)
    if path_name.startswith('/'):
        path_name = path_name[1:]

    # 'C:/MyFolder/ExportedScene/Assets/Game/NewMap/Actor'
    return sanitize_filepath(os.path.normpath(os.path.join(folder, 'Assets', path_name, actor.get_name())))


def apply_static_mesh_material_overrides(overrides, proxy_prim, mesh):
    """ Applies the received material overrides onto the proxy_prim, assuming it is a proxy that references the `mesh`'s file

    This is mainly used to author Over prims with the unrealMaterial attribute pointing at the
    desired material override whenever we're exporting static mesh components.

    :param overrides: Iterable of unreal.MaterialInterface objects that describe the overrides. Can contain interspersed Nones
    :param proxy_prim: Prim that references the `Mesh` schema, and will receive the unrealMaterial attribute overrides
    :param mesh: unreal.StaticMesh mesh object whose materials we're overriding
    :returns: None
    """
    if not isinstance(mesh, unreal.StaticMesh) or not proxy_prim:
        return

    stage = proxy_prim.GetStage()

    num_lods = mesh.get_num_lods()
    has_lods = num_lods > 1

    for mat_index, override in enumerate(overrides):
        if not override:
            continue

        for lod_index in range(num_lods):

            num_sections = mesh.get_num_sections(lod_index)
            has_subsets = num_sections > 1

            for section_index in range(num_sections):
                used_index = unreal.EditorStaticMeshLibrary.get_lod_material_slot(mesh, lod_index, section_index)

                if used_index == mat_index:
                    over_path = proxy_prim.GetPath()

                    # If we have only 1 LOD, the asset's DefaultPrim will be the Mesh prim directly.
                    # If we have multiple, the default prim won't have any schema, but will contain separate
                    # Mesh prims for each LOD named "LOD0", "LOD1", etc., switched via a "LOD" variant set
                    if has_lods:
                        over_path = over_path.AppendPath("LOD" + str(lod_index))

                    # If our LOD has only one section, its material assignment will be authored directly on the Mesh prim.
                    # If it has more than one material slot, we'll author UsdGeomSubset for each LOD Section, and author the material
                    # assignment there
                    if has_subsets:
                        over_path = over_path.AppendPath("Section" + str(section_index))

                    over = stage.OverridePrim(over_path)
                    attr = over.CreateAttribute(UNREAL_MATERIAL_TOKEN, Sdf.ValueTypeNames.String)
                    attr.Set(override.get_path_name())


def create_a_sublayer_for_each_level(context, levels):
    """ Creates a brand new usd file for each given level in `levels`

    :param context: UsdExportContext object describing the export
    :param levels: Iterable of unreal.Level objects to create sublayers for
    :returns: Dict relating the layer created for each level e.g. {unreal.Level : Sdf.Layer}
    """
    root_layer_folder = os.path.dirname(context.root_layer_path)

    level_to_sublayer = {}

    # We will reuse an existing layer if we find one on disk, but we will never
    # reuse the same sublayer for multiple different levels. We use this set to keep track of that
    used_sublayer_paths = set()

    for level in levels:
        # Skip creating one for the persistent world, as that will be the root layer itself
        if level.get_outer() == level.get_world():
            continue

        level_name = Tf.MakeValidIdentifier(level.get_outer().get_name())
        sublayer_path = os.path.join(root_layer_folder, level_name)
        sublayer_path = get_unique_name(used_sublayer_paths, sublayer_path)
        used_sublayer_paths.add(sublayer_path)

        sublayer_path += context.scene_file_extension
        sublayer_path = sanitize_filepath(sublayer_path)

        # Create as a stage so that it owns the reference to its root layer, and is predictably closed when it runs out of
        # scope. If we open the layer directly a strong reference to it will linger and prevent it from being collected
        sublayer_stage = Usd.Stage.CreateNew(sublayer_path)
        root_prim = sublayer_stage.DefinePrim('/' + ROOT_PRIM_NAME, 'Xform')
        sublayer_stage.SetDefaultPrim(root_prim)
        UsdGeom.SetStageUpAxis(sublayer_stage, UsdGeom.Tokens.z if context.options.stage_options.up_axis == unreal.UsdUpAxis.Z_AXIS else UsdGeom.Tokens.y)
        UsdGeom.SetStageMetersPerUnit(sublayer_stage, context.options.stage_options.meters_per_unit)
        context.stage.SetStartTimeCode(context.options.start_time_code)
        context.stage.SetEndTimeCode(context.options.end_time_code)

        unreal.UsdConversionLibrary.insert_sub_layer(context.stage.GetRootLayer().realPath, sublayer_path)
        level_to_sublayer[level] = sublayer_stage.GetRootLayer()

    return level_to_sublayer


class ScopedObjectEditTarget:
    """ RAII-style setting and resetting of edit target based on the ideal layer for an actor.

    Similar to Usd.UsdEditContext, but the idea is that we want to switch the edit target to a 
    sublayer, if we're going to define prims that correspond to an actor that is in a sublevel. 

    Will only actively switch the edit target if context.options.export_sublayers is True.

    Usage:
    with ScopedObjectEditTarget(mesh_actor, context):
        prim = context.stage.DefinePrim(path, 'Mesh')  # Mesh ends up on a sublayer, if mesh_component is in a sublevel

    """

    def __init__(self, actor, context):
        # Get the layer to be set as edit target
        level = actor.get_outer()
        layer_to_edit = context.stage.GetRootLayer()
        if context.options.export_sublayers:
            try:
                layer_to_edit = context.level_to_sublayer[level]
            except KeyError:
                pass

        self.stage = context.stage
        self.edit_target = layer_to_edit
        self.original_edit_target = self.stage.GetEditTarget()

    def __enter__(self):
        if self.edit_target:
            self.stage.SetEditTarget(self.edit_target)
        return self

    def __exit__(self, type, value, traceback):
        if self.original_edit_target:
            self.stage.SetEditTarget(self.original_edit_target)


class UsdConversionContext:
    """ RAII-style wrapper around unreal.UsdConversionContext, so that Cleanup is always called.

    Usage:
    with UsdConversionContext("C:/MyFolder/RootLayer.usda") as converter:
        converter.ConvertMeshComponent(component, "/MyMeshPrim")
    """

    def __init__(self, root_layer_path):
        self.root_layer_path = root_layer_path
        self.context = None

    def __enter__(self):
        self.context = unreal.UsdConversionContext()
        self.context.set_stage_root_layer(unreal.FilePath(self.root_layer_path))
        return self.context

    def __exit__(self, type, value, traceback):
        self.context.cleanup()
