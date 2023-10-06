from collections import defaultdict
from pxr import Usd, UsdUtils, UsdGeom, Sdf, UsdLux, Gf, UsdSkel, UsdShade, Tf
import os
import re
import sys
import unreal
import math
from pathlib import Path
from usd_unreal.exporting_utils import *
from usd_unreal.generic_utils import *
from usd_unreal.constants import *


def collect_actors(context):
    """ Collects a list of actors to export according to `context`'s options

    :param context: UsdExportContext object describing the export
    :returns: Set of unreal.Actor objects that were collected
    """
    actors = None
    if context.options.selection_only:
        # Go through LayersSubsystem as EditorLevelLibrary has too aggressive filtering
        layers_subsystem = unreal.LayersSubsystem()
        actors = set(layers_subsystem.get_selected_actors())
    else:
        actors = unreal.UsdConversionLibrary.get_actors_to_convert(context.world)
    actors = set([a for a in actors if a])

    # Each sublevel has an individual (mostly unused) world, that actually has the name of the sublevel
    # on the UE editor. The levels themselves are all named "Persistent Level", so we can't filter based on them
    if len(context.options.levels_to_ignore) > 0:
        persistent_level_allowed = "Persistent Level" not in context.options.levels_to_ignore

        filtered_actors = set()
        for actor in actors:
            # If the actor is in a sublevel, this will return the persistent world instead
            persistent_world = actor.get_world()

            # If the actor is in a sublevel, this will be the (mostly unused) world that actually owns the sublevel
            actual_world = actor.get_outer().get_outer()

            actor_in_persistent_level = actual_world == persistent_world
            sublevel_name = actual_world.get_name()

            # Have to make sure we only allow it via sublevel name if the actor is not on the persistent level,
            # because if it is then the level name will be the name of the ULevel asset (and not actually "Persistent Level")
            # and we'd incorrectly let it through
            if (persistent_level_allowed and actor_in_persistent_level) or (not actor_in_persistent_level and sublevel_name not in context.options.levels_to_ignore):
                filtered_actors.add(actor)

        actors = set(filtered_actors)

    actors = [a for a in actors if should_export_actor(a)]
    return actors


def collect_materials(actors, static_meshes, skeletal_meshes):
    """ Collects all unreal.MaterialInterface objects that are used by meshes and as overrides in unreal.MeshComponents

    :param actors: Iterable of unreal.Actor objects to traverse looking for unreal.MeshComponents material overrides
    :param static_meshes: Iterable of unreal.StaticMesh objects to collect used materials from
    :param skeletal_meshes: Iterable of unreal.SkeletalMesh objects to collect used materials from
    :returns: Set of unreal.MaterialInterface objects that were collected
    """
    materials = set()

    # Collect materials used as component overrides
    visited_components = set()

    def traverse_components(component):
        if not component or component in visited_components:
            return
        visited_components.add(component)

        actor = component.get_owner()
        if not should_export_actor(actor):
            return

        if isinstance(component, unreal.MeshComponent):
            for mat in component.get_editor_property('override_materials'):
                if mat:
                    materials.add(mat)

        if isinstance(component, unreal.SceneComponent):
            for child in component.get_children_components(include_all_descendants=False):
                traverse_components(child)

    for actor in actors:
        traverse_components(actor.get_editor_property("root_component"))

    # Collect materials used by static meshes
    for mesh in static_meshes:
        for static_material in mesh.get_editor_property('static_materials'):
            mat = static_material.get_editor_property('material_interface')
            if mat:
                materials.add(mat)

    # Collect materials used by skeletal meshes
    for mesh in skeletal_meshes:
        for skeletal_material in mesh.get_editor_property('materials'):
            mat = skeletal_material.get_editor_property('material_interface')
            if mat:
                materials.add(mat)

    return materials


def collect_static_meshes(actors):
    """ Collects all static meshes that are used by unreal.StaticMeshComponents of actors

    :param actors: Iterable of unreal.Actor
    :returns: Set of unreal.StaticMesh objects
    """
    meshes = set()

    for actor in actors:
        for comp in actor.get_components_by_class(unreal.StaticMeshComponent.static_class()):
            # Really we want to check bIsVisualizationComponent, so that we can skip exporting sprites
            # and camera meshes and directional light arrows and so on, but that's not exposed to blueprint
            # so this is the best we can do
            if comp.is_editor_only:
                continue

            mesh = comp.get_editor_property('static_mesh')
            if mesh:
                meshes.add(mesh)

        if isinstance(actor, unreal.InstancedFoliageActor):
            for foliage_type in actor.get_used_foliage_types():
                source = foliage_type.get_source()
                if isinstance(source, unreal.StaticMesh):
                    meshes.add(source)

    return meshes


def collect_skeletal_meshes(actors):
    """ Collects all skeletal meshes that are used by unreal.SkinnedMeshComponents of actors

    :param actors: Iterable of unreal.Actor
    :returns: Set of unreal.SkeletalMesh objects
    """
    meshes = set()

    for actor in actors:
        for comp in actor.get_components_by_class(unreal.SkinnedMeshComponent.static_class()):
            mesh = comp.get_editor_property('skeletal_mesh')
            if mesh:
                meshes.add(mesh)

    return meshes


def assign_static_mesh_component_assets(component, prim, exported_assets):
    """ Assigns the reference, exported static mesh asset for this static mesh component 

    :param component: unreal.StaticMeshComponent to export
    :param prim: Usd.Prim that contains the UsdGeom.Mesh schema
    :param exported_assets: Maps from unreal.Object to exported filename (e.g. "C:/MyFolder/Mesh.usda")
                            Used to retrieve the exported filename of the unreal.StaticMesh used by the component, if any
    :returns: None
    """
    geom_mesh = UsdGeom.Mesh(prim)
    if not geom_mesh:
        return

    mesh = component.get_editor_property('static_mesh')
    if not mesh:
        return

    asset_file_path = exported_assets[mesh]
    if asset_file_path is None:
        unreal.log_warning(f"Failed to find an exported usd file to use for mesh asset '{mesh.get_name()}'")
        return

    add_relative_reference(prim, asset_file_path)


def assign_hism_component_assets(component, prim, exported_assets):
    """ Assigns the reference, exported static mesh asset for this hierarchical instanced static mesh component 

    :param component: unreal.HierarchicalInstancedStaticMeshComponent to export
    :param prim: Usd.Prim that contains the UsdGeom.PointInstancer schema
    :param exported_assets: Maps from unreal.Object to exported filename (e.g. "C:/MyFolder/Mesh.usda")
                            Used to retrieve the exported filename of the unreal.StaticMesh used by the component, if any
    :returns: None
    """
    point_instancer = UsdGeom.PointInstancer(prim)
    if not point_instancer:
        return

    mesh = component.get_editor_property('static_mesh')
    if not mesh:
        return

    asset_file_path = exported_assets[mesh]
    if asset_file_path is None:
        unreal.log_warning(f"Failed to find an exported usd file to use for mesh asset '{mesh.get_name()}'")
        return

    stage = prim.GetStage()
    if not stage:
        return

    # Create prototypes Scope prim
    prototypes = stage.DefinePrim(prim.GetPath().AppendPath('Prototypes'), 'Scope')

    # Create child proxy prims to reference the exported meshes
    # Add proxy prims to prototypes relationship in order
    rel = point_instancer.CreatePrototypesRel()

    exported_filepath = exported_assets[mesh]
    filename = Path(exported_filepath).stem
    proxy_prim_name = Tf.MakeValidIdentifier(filename)
    proxy_prim_name = get_unique_name(set(), proxy_prim_name)

    # Don't use 'Mesh' if we have LODs because it makes it difficult to parse back the prototypes if
    # they're nested Meshes
    proxy_prim_schema = 'Mesh' if unreal.EditorStaticMeshLibrary.get_lod_count(mesh) < 2 else ''

    proxy_prim = stage.DefinePrim(prototypes.GetPath().AppendPath(proxy_prim_name), proxy_prim_schema)
    add_relative_reference(proxy_prim, exported_filepath)

    rel.AddTarget(proxy_prim.GetPath())

    # Set material overrides if we have any
    mat_overrides = component.get_editor_property('override_materials')
    apply_static_mesh_material_overrides(mat_overrides, proxy_prim, mesh)


def assign_skinned_mesh_component_assets(component, prim, exported_assets):
    """ Assigns the reference, exported skeletal mesh asset for this skeletal mesh component

    :param component: unreal.SkinnedMeshComponent to export
    :param prim: Usd.Prim that contains the UsdSkel.Root schema
    :param exported_assets: Maps from unreal.Object to exported filename (e.g. "C:/MyFolder/Mesh.usda")
                            Used to retrieve the exported filename of the unreal.SkeletalMesh used by the component, if any
    :returns: None
    """
    skel_root = UsdSkel.Root(prim)
    if not skel_root:
        return

    mesh = component.get_editor_property('skeletal_mesh')
    if not mesh:
        return

    asset_file_path = exported_assets[mesh]
    if asset_file_path is None:
        unreal.log_warning(f"Failed to find an exported usd file to use for mesh asset '{mesh.get_name()}'")
        return

    add_relative_reference(prim, asset_file_path)


def assign_instanced_foliage_actor_assets(actor, prim, exported_assets):
    """ Assigns the reference, exported static mesh assets for this unreal.InstancedFoliageActor

    :param actor: unreal.InstancedFoliageActor to convert
    :param prim: Usd.Prim with the UsdGeom.PointInstancer schema
    :param exported_assets: Maps from unreal.Object to exported filename (e.g. "C:/MyFolder/Mesh.usda")
                            Used to retrieve the exported filename of the unreal.StaticMeshes used by actor, if any
    :returns: None
    """
    point_instancer = UsdGeom.PointInstancer(prim)
    if not point_instancer:
        return

    if not isinstance(actor, unreal.InstancedFoliageActor):
        return

    stage = prim.GetStage()
    if not stage:
        return

    # Get a packed, sorted list of foliage types that correspond to meshes we exported
    types = actor.get_used_foliage_types()
    exported_types = []
    exported_type_sources = []
    for foliage_type in types:
        source = foliage_type.get_source()
        if isinstance(source, unreal.StaticMesh) and source in exported_assets:
            exported_types.append(foliage_type)
            exported_type_sources.append(source)

    # Create prototypes Scope prim
    prototypes = stage.DefinePrim(prim.GetPath().AppendPath('Prototypes'), 'Scope')

    # Create child proxy prims to reference the exported meshes
    # Add proxy prims to prototypes relationship in order
    rel = point_instancer.CreatePrototypesRel()
    unique_child_prims = set()
    for foliage_type, source in zip(exported_types, exported_type_sources):
        exported_filepath = exported_assets[source]

        filename = Path(exported_filepath).stem
        proxy_prim_name = Tf.MakeValidIdentifier(filename)
        proxy_prim_name = get_unique_name(unique_child_prims, proxy_prim_name)
        unique_child_prims.add(proxy_prim_name)

        # Don't use 'Mesh' if we have LODs because it makes it difficult to parse back the prototypes if
        # they're nested Meshes
        proxy_prim_schema = 'Mesh' if unreal.EditorStaticMeshLibrary.get_lod_count(source) < 2 else ''

        proxy_prim = stage.DefinePrim(prototypes.GetPath().AppendPath(proxy_prim_name), proxy_prim_schema)
        add_relative_reference(proxy_prim, exported_filepath)

        rel.AddTarget(proxy_prim.GetPath())

        # Set material overrides if we have any
        if isinstance(foliage_type, unreal.FoliageType_InstancedStaticMesh):
            mat_overrides = foliage_type.get_editor_property('override_materials')
            apply_static_mesh_material_overrides(mat_overrides, proxy_prim, source)


def assign_material_prim(prim, material_prim):
    """ Assigns material_prim as a material binding to prim, traversing LODs instead, if available

    :param prim: Usd.Prim to receive the material binding. It optionally contains a LOD variant set
    :param material_prim: Usd.Prim with the UsdShade.Material schema that will be bound as a material
    :returns: None
    """
    stage = prim.GetStage()

    var_set = prim.GetVariantSet(LOD_VARIANT_SET_TOKEN)
    lods = var_set.GetVariantNames()
    if len(lods) > 0:
        original_selection = var_set.GetVariantSelection() if var_set.HasAuthoredVariantSelection() else None

        # Switch into each of the LOD variants the prim has, and recurse into the child prims
        for variant in lods:
            with Usd.EditContext(stage, stage.GetSessionLayer()):
                var_set.SetVariantSelection(variant)

            with var_set.GetVariantEditContext():
                for child in prim.GetChildren():
                    if UsdGeom.Mesh(child):
                        shade_api = UsdShade.MaterialBindingAPI(child)
                        shade_api.Bind(UsdShade.Material(material_prim))

        # Restore the variant selection to what it originally was
        with Usd.EditContext(stage, stage.GetSessionLayer()):
            if original_selection:
                var_set.SetVariantSelection(original_selection)
            else:
                var_set.ClearVariantSelection()
    else:
        shade_api = UsdShade.MaterialBindingAPI(prim)
        shade_api.Bind(UsdShade.Material(material_prim))


def replace_unreal_materials_with_baked(stage, material_override_layer, baked_materials, is_asset_layer, use_payload, remove_unreal_materials):
    """ Traverses the stage, and replaces all usages of an unrealMaterial with a material binding to the baked material.

    To accomplish this this function will define on the stage prims for each baked material within a "Materials" Scope
    prim, and have those prims reference the assets where the materials were baked to, according to
    context.baked_materials

    :param stage: Usd.Stage to replace the references in
    :param material_override_layer: Sdf.Layer to author the Material prims that reference the baked material assets
    :param baked_materials: Maps from unreal.MaterialInterface's path name to the filename of the baked USD asset
                            e.g. '/Game/Materials/Red.Red': 'C:/MyFolder/Assets/Red.usda'
    :param is_asset_layer: True if the stage's root layer represent an exported asset. 
                           False if it describes the a level or scene
    :param use_payload: True if we're exporting with payload files
    :param is_asset_layer: Whether to remove the 'unrealMaterial' attribute after binding the corresponding baked material. 
    :returns: None
    """
    # UsedLayers here instead of layer stack because we may be exporting using payloads, and payload layers
    # don't show up on the layer stack list but do show up on the UsedLayers list
    layers_to_traverse = stage.GetUsedLayers()
    stage_mat_scope = None

    class MaterialScopePrim:
        def __init__(self, scope_stage, parent_prim):
            path = parent_prim.GetPrimPath().AppendPath(Sdf.Path('Materials'))

            self.prim = scope_stage.DefinePrim(path, 'Scope')
            self.used_prim_names = set()
            self.baked_filename_to_mat_prim = {}

    def traverse_for_material_replacement(stage_to_traverse, prim, mat_prim_scope, outer_var_set):
        """ Recursively traverses the stage, doing the material assignment replacements.

        This handles Mesh prims as well as GeomSubset prims.
        Note how we receive the stage as an argument instead of capturing it from the outer scope:
        This ensures the inner function doesn't hold a reference to the stage

        :param stage_to_traverse: Usd.Stage to traverse
        :param prim: Usd.Prim to visit
        :param mat_scope_prim: MaterialScopePrim object that will contain the material proxy prims defined from this call
        :returns: None
        """
        # Recurse into children before doing anything as we may need to parse LODs
        var_set = prim.GetVariantSet(LOD_VARIANT_SET_TOKEN)
        lods = var_set.GetVariantNames()
        if len(lods) > 0:
            original_selection = var_set.GetVariantSelection() if var_set.HasAuthoredVariantSelection() else None

            # Prims within variant sets can't have relationships to prims outside the scope of the prim that
            # contains the variant set itself. This means we'll need a new material scope prim if we're stepping
            # into a variant within an asset layer, so that any material proxy prims we author are contained within it.
            # Note that we only do this for asset layers: If we're parsing the root layer, any LOD variant sets we can step into
            # are brought in via references to asset files, and we know that referenced subtree only has relationships to
            # things within that same subtree (which will be entirely brought in to the root layer). This means we can
            # just keep inner_mat_prim_scope as None and default to using the layer's mat scope prim if we need one
            inner_mat_prim_scope = MaterialScopePrim(stage_to_traverse, prim) if is_asset_layer else None

            # Switch into each of the LOD variants the prim has, and recurse into the child prims
            for variant in lods:
                with Usd.EditContext(stage_to_traverse, stage_to_traverse.GetSessionLayer()):
                    var_set.SetVariantSelection(variant)
                for child in prim.GetChildren():
                    traverse_for_material_replacement(stage_to_traverse, child, inner_mat_prim_scope, var_set)

            # Restore the variant selection to what it originally was
            with Usd.EditContext(stage_to_traverse, stage_to_traverse.GetSessionLayer()):
                if original_selection:
                    var_set.SetVariantSelection(original_selection)
                else:
                    var_set.ClearVariantSelection()
        else:
            for child in prim.GetChildren():
                traverse_for_material_replacement(stage_to_traverse, child, mat_prim_scope, outer_var_set)

        attr = prim.GetAttribute(UNREAL_MATERIAL_TOKEN)
        if not attr:
            return

        material_path_name = attr.Get()
        baked_filename = baked_materials.get(material_path_name)

        # If we have a valid unrealMaterial but just haven't baked it, leave unrealMaterials alone and abort
        if len(material_path_name) > 0 and baked_filename is None:
            return

        attr_path = attr.GetPath()

        # Find out if we need to remove/author material bindings within an actual variant or outside of it, as an over.
        # We don't do this when using payloads because our override prims aren't inside the actual LOD variants: They just
        # directly override a mesh called e.g. 'LOD3' as if it's a child prim, so that the override automatically only
        # does anything when we happen to have the variant that enables the LOD3 Mesh
        author_inside_variants = outer_var_set and is_asset_layer and not use_payload

        if author_inside_variants:
            var_prim_path = outer_var_set.GetPrim().GetPath()
            if attr_path.HasPrefix(var_prim_path):
                # This builds a path like '/MyMesh{LOD=LOD0}LOD0.unrealMaterial',
                # or '/MyMesh{LOD=LOD0}LOD0/Section1.unrealMaterial'. This is required because we'll query the layer
                # for a spec path below, and this path must contain the variant selection in it, which the path returned
                # from attr.GetPath() doesn't contain
                var_prim_path_with_var = var_prim_path.AppendVariantSelection(outer_var_set.GetName(), outer_var_set.GetVariantSelection())
                attr_path = attr_path.ReplacePrefix(var_prim_path, var_prim_path_with_var)

        # We always want to replace the actual attribute in whatever layer it was authored, and not just override with
        # a stronger opinion. So search through all sublayers to find the ones with unrealMaterial attribute specs
        for layer in layers_to_traverse:
            attr_spec = layer.GetAttributeAtPath(attr_path)
            if not attr_spec:
                continue

            with Usd.EditContext(stage_to_traverse, layer):
                if remove_unreal_materials:
                    if author_inside_variants:
                        with outer_var_set.GetVariantEditContext():
                            prim.RemoveProperty(UNREAL_MATERIAL_TOKEN)
                    else:
                        prim.RemoveProperty(UNREAL_MATERIAL_TOKEN)

                # It was just an empty unrealMaterial attribute, so just cancel now as our baked_filename
                # can't possibly be useful
                if len(material_path_name) == 0:
                    return

                # Get the proxy prim for the material within this layer
                # (or create one outside the variant edit context)
                mat_prim = None
                with Usd.EditContext(stage_to_traverse, material_override_layer):
                    # On-demand create a *single* material scope prim for the stage, if we're not inside a variant set
                    if not mat_prim_scope:
                        nonlocal stage_mat_scope
                        if not stage_mat_scope:
                            # If a prim from a stage references another layer, USD's composition will effectively
                            # paste the default prim of the referenced layer over the referencing prim. Because of
                            # this, the subprims within the hierarchy of that default prim can't ever have
                            # relationships to other prims outside that of that same hierarchy, as those prims
                            # will not be present on the referencing stage at all. This is why we author our stage
                            # materials scope under the default prim, and not the pseudoroot
                            stage_mat_scope = MaterialScopePrim(stage_to_traverse, stage_to_traverse.GetDefaultPrim())
                        mat_prim_scope = stage_mat_scope

                    try:
                        mat_prim = mat_prim_scope.baked_filename_to_mat_prim[baked_filename]
                    except KeyError:
                        mat_name = Tf.MakeValidIdentifier(Path(material_path_name).stem)
                        mat_prim_name = get_unique_name(mat_prim_scope.used_prim_names, mat_name)
                        mat_prim_scope.used_prim_names.add(mat_name)

                        mat_prim = stage_to_traverse.DefinePrim(mat_prim_scope.prim.GetPath().AppendPath(mat_prim_name), 'Material')
                        add_relative_reference(mat_prim, baked_filename)
                        mat_prim_scope.baked_filename_to_mat_prim[baked_filename] = mat_prim

                # Reference the mat proxy prim as a material binding
                if author_inside_variants:
                    with outer_var_set.GetVariantEditContext():
                        shade_api = UsdShade.MaterialBindingAPI(prim)
                        shade_api.Bind(UsdShade.Material(mat_prim))
                else:
                    shade_api = UsdShade.MaterialBindingAPI(prim)
                    shade_api.Bind(UsdShade.Material(mat_prim))

    root = stage.GetPseudoRoot()
    traverse_for_material_replacement(stage, root, None, None)


def convert_component(context, converter, component, visited_components=set(), parent_prim_path=""):
    """ Exports a component onto the stage, creating the required prim of the corresponding schema as necessary

    :param context: UsdExportContext object describing the export
    :param component: unreal.SceneComponent to export
    :param visited_components: Set of components to skip during traversal. Exported components are added to it
    :param parent_prim_path: Prim path the parent component was exported to (e.g. "/Root/Parent"). 
                             Purely an optimization: Can be left empty, and will be discovered automatically
    :returns: None
    """
    actor = component.get_owner()
    if not should_export_actor(actor):
        return

    # We use this as a proxy for bIsVisualizationComponent
    if component.is_editor_only:
        return

    prim_path = get_prim_path_for_component(
        component,
        context.exported_prim_paths,
        parent_prim_path,
        context.options.export_actor_folders
    )

    unreal.log(f"Exporting component '{component.get_name()}' onto prim '{prim_path}'")

    prim = context.stage.GetPrimAtPath(prim_path)
    if not prim:
        target_schema_name = get_schema_name_for_component(component)
        unreal.log(f"Creating new prim at '{prim_path}' with schema '{target_schema_name}'")
        prim = context.stage.DefinePrim(prim_path, target_schema_name)

    # Add attributes to prim depending on component type
    if isinstance(component, unreal.HierarchicalInstancedStaticMeshComponent):
        # Create a separate new child of `prim` that can receive our PointInstancer component schema instead.
        # We do this because we can have any component tree in UE, but in USD the recommendation is that you don't
        # place drawable prims inside PointInstancers, and that DCCs don't traverse PointInstancers looking for drawable
        # prims, so that they can work as a way to "hide" their prototypes
        child_prim_path = get_unique_name(context.exported_prim_paths, prim_path + "/HISMInstance")
        context.exported_prim_paths.add(child_prim_path)

        unreal.log(f"Creating new prim at '{child_prim_path}' with schema 'PointInstancer'")
        child_prim = context.stage.DefinePrim(child_prim_path, "PointInstancer")

        assign_hism_component_assets(component, child_prim, context.exported_assets)
        converter.convert_hism_component(component, child_prim_path)

    elif isinstance(component, unreal.StaticMeshComponent):
        assign_static_mesh_component_assets(component, prim, context.exported_assets)
        converter.convert_mesh_component(component, prim_path)

    elif isinstance(component, unreal.SkinnedMeshComponent):
        assign_skinned_mesh_component_assets(component, prim, context.exported_assets)
        converter.convert_mesh_component(component, prim_path)

    elif isinstance(component, unreal.CineCameraComponent):

        # If we're the main camera component of an ACineCameraActor, then write that out on our parent prim
        # so that if we ever import this file back into UE we can try to reconstruct the ACineCameraActor 
        # with the same root and camera components, instead of creating new ones
        owner_actor = component.get_owner()
        if isinstance(owner_actor, unreal.CineCameraActor):
            main_camera_component = owner_actor.get_editor_property("camera_component")
            if component == main_camera_component:
                parent_prim = prim.GetParent()
                if parent_prim:
                    attr = parent_prim.CreateAttribute('unrealCameraPrimName', Sdf.ValueTypeNames.Token)
                    attr.Set(prim.GetName())

        converter.convert_cine_camera_component(component, prim_path)

    elif isinstance(component, unreal.LightComponentBase):
        converter.convert_light_component(component, prim_path)

        if isinstance(component, unreal.SkyLightComponent):
            converter.convert_sky_light_component(component, prim_path)

        if isinstance(component, unreal.DirectionalLightComponent):
            converter.convert_directional_light_component(component, prim_path)

        if isinstance(component, unreal.RectLightComponent):
            converter.convert_rect_light_component(component, prim_path)

        if isinstance(component, unreal.PointLightComponent):
            converter.convert_point_light_component(component, prim_path)

            if isinstance(component, unreal.SpotLightComponent):
                converter.convert_spot_light_component(component, prim_path)

    if isinstance(component, unreal.SceneComponent):
        converter.convert_scene_component(component, prim_path)

        owner_actor = component.get_owner()

        # We have to export the instanced foliage actor in one go, because it will contain one component
        # for each foliage type, and we don't want to end up with one PointInstancer prim for each
        if isinstance(owner_actor, unreal.InstancedFoliageActor):
            assign_instanced_foliage_actor_assets(actor, prim, context.exported_assets)
            converter.convert_instanced_foliage_actor(actor, prim_path)
        elif isinstance(owner_actor, unreal.LandscapeProxy):
            success, mesh_path = export_landscape(context, owner_actor)
            if success:
                add_relative_reference(prim, mesh_path)
            else:
                unreal.log_warning(f"Failed to export landscape '{owner_actor.get_name()}' to filepath '{mesh_path}'")
        else:
            # Recurse to children
            for child in component.get_children_components(include_all_descendants=False):
                if child in visited_components:
                    continue
                visited_components.add(child)

                convert_component(context, converter, child, visited_components, prim_path)


def export_actors(context, actors):
    """ Will export the `actors`' component hierarchies as prims on the context's stage

    :param context: UsdExportContext object describing the export
    :param actors: Collection of unreal.Actor objects to iterate over and export
    :returns: None
    """
    unreal.log(f"Exporting components from {len(actors)} actors")

    visited_components = set()

    with UsdConversionContext(context.root_layer_path) as converter:

        # Component traversal ensures we parse parent components before child components,
        # but since we get our actors in random order we need to manually ensure we parse
        # parent actors before child actors. Otherwise we may parse a child, end up having USD create
        # all the parent prims with default Xform schemas to make the child path work, and then
        # not being able to convert a parent prim to a Mesh schema (or some other) because a
        # default prim already exists with that name, and it's a different schema.
        def attach_depth(a):
            depth = 0
            parent = a.get_attach_parent_actor()
            while parent:
                depth += 1
                parent = parent.get_attach_parent_actor()
            return depth
        actor_list = list(actors)
        actor_list.sort(key=attach_depth)

        for actor in actor_list:
            comp = actor.get_editor_property("root_component")
            if not comp or comp in visited_components:
                continue
            visited_components.add(comp)

            # If this actor is in a sublevel, make sure the prims are authored in the matching sub-layer
            with ScopedObjectEditTarget(actor, context):
                this_edit_target = context.stage.GetEditTarget().GetLayer().realPath
                converter.set_edit_target(unreal.FilePath(this_edit_target))

                convert_component(context, converter, comp, visited_components)


def setup_material_override_layer(context):
    """ Creates a dedicated layer for material overrides, and adds it as a sublayer to all layers in the layer stack.

    We use a dedicated layer to author material overrides because if a prim is defined in a stage with multiple layers,
    USD will only actually `def` the prim on the weakest layer, and author `over`s on stronger layers. We don't want
    this behavior for our material prims, because then we wouldn't be able to open each layer as an independent stage.
    We may want to be able to do that, as UE levels are fully independent from eachother.

    By keeping all material overrides in a separate layer that all other layers have as sublayer, we can open each one
    independently, or the whole stage as a group, and the material assignments are correct.

    :param context: UsdExportContext object describing the export
    :returns: Sdf.Layer that should contain the material override prims 
    """

    root_layer_folder = os.path.dirname(context.root_layer_path)
    mat_layer_path = os.path.join(root_layer_folder, "MaterialOverrides" + context.scene_file_extension)

    # Create as a stage so that it owns the reference to its root layer, and is predictably closed when it runs out of
    # scope. If we open the layer directly a strong reference to it will linger and prevent it from being collected
    mat_layer_stage = Usd.Stage.CreateNew(mat_layer_path)

    for layer in reversed(context.stage.GetLayerStack(includeSessionLayers=False)):
        unreal.UsdConversionLibrary.insert_sub_layer(layer.realPath, mat_layer_path)

    return mat_layer_stage.GetRootLayer()


def export_level(context, actors):
    """ Exports the actors and components of the level to the main output root layer, and potentially sublayers

    This function creates the root layer for the export, and then iterates the component attachment hierarchy of the
    current level, creating a prim for each exportable component. 

    :param context: UsdExportContext object describing the export
    :returns: None
    """
    unreal.log(f"Creating new stage with root layer '{context.root_layer_path}'")

    with unreal.ScopedSlowTask(3, f"Exporting main root layer") as slow_task:
        slow_task.make_dialog(True)

        slow_task.enter_progress_frame(1)

        # Setup stage
        context.stage = Usd.Stage.CreateNew(context.root_layer_path)
        root_prim = context.stage.DefinePrim('/' + ROOT_PRIM_NAME, 'Xform')
        context.stage.SetDefaultPrim(root_prim)
        root_layer = context.stage.GetRootLayer()

        # Set stage metadata
        UsdGeom.SetStageUpAxis(context.stage, UsdGeom.Tokens.z if context.options.stage_options.up_axis == unreal.UsdUpAxis.Z_AXIS else UsdGeom.Tokens.y)
        UsdGeom.SetStageMetersPerUnit(context.stage, context.options.stage_options.meters_per_unit)
        context.stage.SetStartTimeCode(context.options.start_time_code)
        context.stage.SetEndTimeCode(context.options.end_time_code)

        # Prepare sublayers for export, if applicable
        if context.options.export_sublayers:
            levels = set([a.get_outer() for a in actors])
            context.level_to_sublayer = create_a_sublayer_for_each_level(context, levels)
            if context.options.bake_materials:
                context.materials_sublayer = setup_material_override_layer(context)

        # Export actors
        export_actors(context, actors)

        # Post-processing
        slow_task.enter_progress_frame(1)
        if context.options.bake_materials:
            override_layer = context.materials_sublayer if context.materials_sublayer else root_layer
            replace_unreal_materials_with_baked(
                context.stage,
                override_layer,
                context.baked_materials,
                is_asset_layer=False,
                use_payload=context.options.use_payload,
                remove_unreal_materials=context.options.remove_unreal_materials
            )

            # Abandon the material overrides sublayer if we don't have any material overrides
            if context.materials_sublayer and context.materials_sublayer.empty:
                # Cache this path because the materials sublayer may get suddenly dropped as we erase all sublayer references
                materials_sublayer_path = context.materials_sublayer.realPath
                sublayers = context.stage.GetLayerStack(includeSessionLayers=False)
                sublayers.remove(context.materials_sublayer)

                for sublayer in sublayers:
                    relative_path = unreal.UsdConversionLibrary.make_path_relative_to_layer(sublayer.realPath, materials_sublayer_path)
                    sublayer.subLayerPaths.remove(relative_path)
                os.remove(materials_sublayer_path)

        # Write file
        slow_task.enter_progress_frame(1)
        unreal.log(f"Saving root layer '{context.root_layer_path}'")
        if context.options.export_sublayers:
            # Use the layer stack directly so we also get a material sublayer if we made one
            for sublayer in context.stage.GetLayerStack(includeSessionLayers=False):
                sublayer.Save()
        context.stage.GetRootLayer().Save()


def export_material(context, material):
    """ Exports a single unreal.MaterialInterface

    :param context: UsdExportContext object describing the export
    :param material: unreal.MaterialInterface object to export
    :returns: (Bool, String) containing True if the export was successful, and the output filename that was used 
    """
    material_file = get_filename_to_export_to(os.path.dirname(context.root_layer_path), material, context.scene_file_extension)

    # If we try to bake a material instance without a valid parent, the material baking module will hard crash the editor
    if isinstance(material, unreal.MaterialInstance):
        parent = material.get_editor_property('parent')
        if not parent:
            unreal.log_warning(f"Failed to export material '{material.get_name()}' to filepath '{material_file}': Material instance has no parent!")
            return (False, material_file)

    options = unreal.MaterialExporterUSDOptions()
    options.textures_dir = unreal.DirectoryPath(os.path.join(os.path.dirname(material_file), 'Textures'))
    options.default_texture_size = context.options.bake_resolution
    options.properties = [
        # Only these channels are supported:
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_ANISOTROPY),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_BASE_COLOR),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_EMISSIVE_COLOR),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_METALLIC),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_NORMAL),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_OPACITY),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_OPACITY_MASK),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_ROUGHNESS),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_SPECULAR),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_SUBSURFACE_COLOR),
        unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_TANGENT)
    ]

    task = unreal.AssetExportTask()
    task.object = material
    task.filename = material_file
    task.selected = False
    task.replace_identical = True
    task.prompt = False
    task.options = options
    task.automated = True

    unreal.log(f"Exporting material '{material.get_name()}' to filepath '{material_file}'")
    success = unreal.Exporter.run_asset_export_task(task)

    if not success:
        unreal.log_warning(f"Failed to export material '{material.get_name()}' to filepath '{material_file}'")
        return (False, material_file)

    try:
        # Add USD info to exported asset
        stage = Usd.Stage.Open(material_file)
        usd_prim = stage.GetDefaultPrim()
        model = Usd.ModelAPI(usd_prim)
        model.SetAssetIdentifier(material_file)
        model.SetAssetName(material.get_name())
        stage.Save()
    except BaseException as e:
        if len(material_file) > 220:
            unreal.log_error(f"USD failed to open a stage with a very long filepath: Try to use a destination folder with a shorter file path")
        unreal.log_error(e)

    return (True, material_file)


def export_materials(context, materials):
    """ Exports a single Static/Skeletal mesh

    :param context: UsdExportContext object describing the export
    :param materials: Collection of unreal.MaterialInterface objects to export
    :returns: None
    """
    if not context.options.bake_materials:
        return

    num_materials = len(materials)
    unreal.log(f"Baking {len(materials)} materials")

    with unreal.ScopedSlowTask(num_materials, 'Baking materials') as slow_task:
        slow_task.make_dialog(True)

        for material in materials:
            if slow_task.should_cancel():
                break
            slow_task.enter_progress_frame(1)

            success, filename = export_material(context, material)
            if success:
                context.exported_assets[material] = filename
                context.baked_materials[material.get_path_name()] = filename


def export_mesh(context, mesh, mesh_type):
    """ Exports a single Static/Skeletal mesh

    :param context: UsdExportContext object describing the export
    :param mesh: unreal.SkeletalMesh or unreal.StaticMesh object to export
    :param mesh_type: String 'static' or 'skeletal', according to the types of `meshes`
    :returns: (Bool, String) containing True if the export was successful, and the output filename that was used 
    """
    mesh_file = get_filename_to_export_to(os.path.dirname(context.root_layer_path), mesh, context.scene_file_extension)

    options = None
    if mesh_type == 'static':
        options = unreal.StaticMeshExporterUSDOptions()
        options.stage_options = context.options.stage_options
        options.use_payload = context.options.use_payload
        options.payload_format = context.options.payload_format
    elif mesh_type == 'skeletal':
        options = unreal.SkeletalMeshExporterUSDOptions()
        options.inner.stage_options = context.options.stage_options
        options.inner.use_payload = context.options.use_payload
        options.inner.payload_format = context.options.payload_format

    task = unreal.AssetExportTask()
    task.object = mesh
    task.filename = mesh_file
    task.selected = False
    task.replace_identical = True
    task.prompt = False
    task.automated = True
    task.options = options

    unreal.log(f"Exporting {mesh_type} mesh '{mesh.get_name()}' to filepath '{mesh_file}'")
    success = unreal.Exporter.run_asset_export_task(task)

    if not success:
        unreal.log_warning(f"Failed to export {mesh_type} mesh '{mesh.get_name()}' to filepath '{mesh_file}'")
        return (False, mesh_file)

    # Add USD info to exported asset
    try:
        stage = Usd.Stage.Open(mesh_file)
        usd_prim = stage.GetDefaultPrim()
        model = Usd.ModelAPI(usd_prim)
        model.SetAssetIdentifier(mesh_file)
        model.SetAssetName(mesh.get_name())

        # Take over material baking for the mesh, so that it's easier to share
        # baked materials between mesh exports
        if context.options.bake_materials:
            replace_unreal_materials_with_baked(
                stage,
                stage.GetRootLayer(),
                context.baked_materials,
                is_asset_layer=True,
                use_payload=context.options.use_payload,
                remove_unreal_materials=context.options.remove_unreal_materials
            )

        stage.Save()
    except BaseException as e:
        if len(mesh_file) > 220:
            unreal.log_error(f"USD failed to open a stage with a very long filepath: Try to use a destination folder with a shorter file path")
        unreal.log_error(e)

    return (True, mesh_file)


def export_meshes(context, meshes, mesh_type):
    """ Exports a collection of Static/Skeletal meshes 

    :param context: UsdExportContext object describing the export
    :param meshes: Homogeneous collection of unreal.SkeletalMesh or unreal.StaticMesh
    :param mesh_type: String 'static' or 'skeletal', according to the types of `meshes`
    :returns: None
    """
    num_meshes = len(meshes)
    unreal.log(f"Exporting {num_meshes} {mesh_type} meshes")

    with unreal.ScopedSlowTask(num_meshes, f"Exporting {mesh_type} meshes") as slow_task:
        slow_task.make_dialog(True)

        for mesh in meshes:
            if slow_task.should_cancel():
                break
            slow_task.enter_progress_frame(1)

            success, path = export_mesh(context, mesh, mesh_type)
            if success:
                context.exported_assets[mesh] = path


def export_landscape(context, landscape_actor):
    """ Exports a landscape actor as separate mesh and material USD files referencing eachother, and returns the path to the mesh file

    :param context: UsdExportContext object describing the export
    :param landscape_actor: unreal.LandscapeProxy actor to export
    :returns: (Bool, String) containing True if the export was successful, and the output mesh filename that was used 
    """
    folder_path = get_actor_asset_folder_to_export_to(os.path.dirname(context.root_layer_path), landscape_actor)
    actor_name = sanitize_filepath(landscape_actor.get_name())

    textures_folder = os.path.join(folder_path, "Textures")
    asset_path = os.path.join(folder_path, actor_name + "_asset" + context.scene_file_extension)
    mesh_path = os.path.join(folder_path, actor_name + ("_payload." + context.options.payload_format if context.options.use_payload else "_mesh" + context.scene_file_extension))
    mat_path = os.path.join(folder_path, actor_name + "_material" + context.scene_file_extension)

    # Use the UsdUtils stage cache because we'll use the C++ wrapper function to add a payload,
    # and we want to fetch the same stage
    cache = UsdUtils.StageCache.Get()
    with Usd.StageCacheContext(cache):
        # Create stage for the material
        mat_stage = Usd.Stage.CreateNew(mat_path)
        mat_prim = mat_stage.DefinePrim('/' + Tf.MakeValidIdentifier(actor_name), 'Material')
        mat_stage.SetDefaultPrim(mat_prim)

        # Set material stage metadata
        UsdGeom.SetStageUpAxis(mat_stage, UsdGeom.Tokens.z if context.options.stage_options.up_axis == unreal.UsdUpAxis.Z_AXIS else UsdGeom.Tokens.y)
        UsdGeom.SetStageMetersPerUnit(mat_stage, context.options.stage_options.meters_per_unit)
        mat_stage.SetStartTimeCode(context.options.start_time_code)
        mat_stage.SetEndTimeCode(context.options.end_time_code)
        model = Usd.ModelAPI(mat_prim)
        model.SetAssetIdentifier(mat_path)
        model.SetAssetName(actor_name)

        # Convert material data
        properties_to_bake = [
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_BASE_COLOR),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_METALLIC),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_SPECULAR),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_ROUGHNESS),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_NORMAL),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_TANGENT),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_EMISSIVE_COLOR),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_OPACITY),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_OPACITY_MASK),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_ANISOTROPY),
            unreal.PropertyEntry(property_=unreal.MaterialProperty.MP_SUBSURFACE_COLOR),
        ]
        with UsdConversionContext(mat_path) as converter:
            converter.convert_landscape_proxy_actor_material(
                landscape_actor,
                str(mat_prim.GetPath()),
                properties_to_bake,
                context.options.landscape_bake_resolution,
                unreal.DirectoryPath(textures_folder)
            )

        # Create stage for the mesh
        mesh_stage = Usd.Stage.CreateNew(mesh_path)
        num_lods = max(abs(context.options.highest_landscape_lod - context.options.lowest_landscape_lod + 1), 1)
        mesh_prim = mesh_stage.DefinePrim('/' + Tf.MakeValidIdentifier(actor_name), 'Mesh' if num_lods == 1 else "")
        mesh_stage.SetDefaultPrim(mesh_prim)

        # Set mesh stage metadata
        UsdGeom.SetStageUpAxis(mesh_stage, UsdGeom.Tokens.z if context.options.stage_options.up_axis == unreal.UsdUpAxis.Z_AXIS else UsdGeom.Tokens.y)
        UsdGeom.SetStageMetersPerUnit(mesh_stage, context.options.stage_options.meters_per_unit)
        mesh_stage.SetStartTimeCode(context.options.start_time_code)
        mesh_stage.SetEndTimeCode(context.options.end_time_code)
        model = Usd.ModelAPI(mesh_prim)
        model.SetAssetIdentifier(mesh_path)
        model.SetAssetName(actor_name)

        # Convert mesh data
        with UsdConversionContext(mesh_path) as converter:
            converter.convert_landscape_proxy_actor_mesh(
                landscape_actor,
                str(mesh_prim.GetPath()),
                context.options.lowest_landscape_lod,
                context.options.highest_landscape_lod
            )

        # Create a separate "asset" file for the landscape
        asset_stage = None
        if context.options.use_payload:
            # Create stage for the mesh
            asset_stage = Usd.Stage.CreateNew(asset_path)
            num_lods = max(abs(context.options.highest_landscape_lod - context.options.lowest_landscape_lod + 1), 1)
            asset_prim = asset_stage.DefinePrim('/' + Tf.MakeValidIdentifier(actor_name), 'Mesh' if num_lods == 1 else "")
            asset_stage.SetDefaultPrim(asset_prim)

            # Set assset stage metadata
            UsdGeom.SetStageUpAxis(asset_stage, UsdGeom.Tokens.z if context.options.stage_options.up_axis == unreal.UsdUpAxis.Z_AXIS else UsdGeom.Tokens.y)
            UsdGeom.SetStageMetersPerUnit(asset_stage, context.options.stage_options.meters_per_unit)
            asset_stage.SetStartTimeCode(context.options.start_time_code)
            asset_stage.SetEndTimeCode(context.options.end_time_code)
            model = Usd.ModelAPI(asset_prim)
            model.SetAssetIdentifier(asset_path)
            model.SetAssetName(actor_name)

            # Refer to the mesh prim as a payload
            unreal.UsdConversionLibrary.add_payload(asset_path, str(asset_prim.GetPath()), mesh_path)

            # Create a material proxy prim and scope on the asset stage
            mat_scope_prim = asset_stage.DefinePrim(mesh_prim.GetPath().AppendChild('Materials'), 'Scope')
            mat_proxy_prim = asset_stage.DefinePrim(mat_scope_prim.GetPath().AppendChild('Material'), 'Material')
            add_relative_reference(mat_proxy_prim, mat_path)

            # Assign material proxy prim to any and all LODs/mesh on the asset stage
            assign_material_prim(asset_prim, mat_proxy_prim)

        # Just export mesh and material files
        else:
            # Create a material proxy prim and scope on the mesh stage
            mat_scope_prim = mesh_stage.DefinePrim(mesh_prim.GetPath().AppendChild('Materials'), 'Scope')
            mat_proxy_prim = mesh_stage.DefinePrim(mat_scope_prim.GetPath().AppendChild('Material'), 'Material')
            add_relative_reference(mat_proxy_prim, mat_path)

            # Assign material proxy prim to any and all LODs/mesh on the mesh stage
            assign_material_prim(mesh_prim, mat_proxy_prim)

        # Write data to files
        mesh_stage.Save()
        mat_stage.Save()
        if asset_stage:
            asset_stage.Save()

        # Remove these stages from the cache or else they will sit there forever
        cache.Erase(mesh_stage)
        cache.Erase(mat_stage)
        if asset_stage:
            cache.Erase(asset_stage)

    return (True, asset_path if context.options.use_payload else mesh_path)


def export(context):
    """ Exports the current level according to the received export context

    :param context: UsdExportContext object describing the export
    :returns: None
    """
    if not context.world or not isinstance(context.world, unreal.World):
        unreal.log_error("UsdExportContext's 'world' member must point to a valid unreal.World object!")
        return

    context.root_layer_path = sanitize_filepath(context.root_layer_path)

    if not context.scene_file_extension:
        extension = os.path.splitext(context.root_layer_path)[1]
        context.scene_file_extension = extension if extension else ".usda"
    if not context.options.payload_format:
        context.options.payload_format = context.scene_file_extension

    unreal.log(f"Starting export to root layer: '{context.root_layer_path}'")

    with unreal.ScopedSlowTask(5, f"Exporting level to '{context.root_layer_path}'") as slow_task:
        slow_task.make_dialog(True)

        # Collect items to export
        slow_task.enter_progress_frame(1)
        actors = collect_actors(context)
        static_meshes = collect_static_meshes(actors)
        skeletal_meshes = collect_skeletal_meshes(actors)
        materials = collect_materials(actors, static_meshes, skeletal_meshes)

        # Export assets
        slow_task.enter_progress_frame(1)
        export_materials(context, materials)

        slow_task.enter_progress_frame(1)
        export_meshes(context, static_meshes, 'static')

        slow_task.enter_progress_frame(1)
        export_meshes(context, skeletal_meshes, 'skeletal')

        # Export actors
        slow_task.enter_progress_frame(1)
        export_level(context, actors)


def export_with_cdo_options():
    options_cdo = unreal.get_default_object(unreal.LevelExporterUSDOptions)

    context = UsdExportContext()
    context.root_layer_path = options_cdo.current_task.filename
    context.world = options_cdo.current_task.object
    context.options = options_cdo

    export(context)
