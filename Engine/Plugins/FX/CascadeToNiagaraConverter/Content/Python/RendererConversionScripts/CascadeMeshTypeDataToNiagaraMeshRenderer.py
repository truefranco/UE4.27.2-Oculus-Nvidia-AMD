from RendererConverterInterface import RendererConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils


class CascadeMeshTypeDataConverter(RendererConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleTypeDataMesh

    @classmethod
    def convert(cls, cascade_typedata_module, cascade_required_module,
                niagara_emitter_context):
        # create a new mesh renderer properties and get the properties from
        # the cascade typedata and required modules
        mesh_renderer_props = ue.NiagaraMeshRendererProperties()

        (mesh, lod_size_scale, b_use_static_mesh_lods, b_cast_shadows,
         b_do_collisions, mesh_alignment, b_override_material,
         b_override_default_motion_blur_settings, b_enable_motion_blur,
         roll_pitch_yaw_range_distribution, axis_lock_option, b_camera_facing,
         camera_facing_up_axis_option_deprecated, camera_facing_option,
         b_apply_particle_rotation_as_spin,
         b_facing_camera_direction_rather_than_position,
         b_collision_consider_particle_size) = (
            ue_fx_utils.get_particle_module_type_data_mesh_props(
                cascade_typedata_module
            ))

        (material_interface, screen_alignment, subimages_horizontal, 
         subimages_vertical, sort_mode, interpolation_method, b_remove_hmd_roll, 
         min_facing_camera_blend_distance, max_facing_camera_blend_distance,
         material_cutout_texture, bounding_mode, opacity_source_mode, 
         emitter_normals_mode, alpha_threshold) = (
            ue_fx_utils.get_particle_module_required_per_renderer_props(
                cascade_required_module
            ))

        mesh_entry =  ue.NiagaraMeshRendererMeshProperties()
        mesh_entry.set_editor_property("Mesh", mesh)
        meshes = ue.Array(ue.NiagaraMeshRendererMeshProperties)
        meshes.append(mesh_entry)
        mesh_renderer_props.set_editor_property("Meshes", meshes)

        # set the override material if necessary
        if b_override_material:
            mesh_renderer_props.set_editor_property("bOverrideMaterials", True)
            
            override_mat = ue.NiagaraMeshMaterialOverride()
            override_mat.set_editor_property("ExplicitMat", material_interface)
            override_mats = ue.Array(ue.NiagaraMeshMaterialOverride)
            override_mats.append(override_mat)
            mesh_renderer_props.set_editor_property("OverrideMaterials", override_mats)

        # get the niagara sort mode and binding from the cascade equivalent sort mode
        niagara_sort_mode, sort_binding = (
            c2n_utils.get_niagara_sortmode_and_binding_for_sortmode(sort_mode))
        mesh_renderer_props.set_editor_property("SortMode", niagara_sort_mode)
        
        # default to only sorting translucent particles
        mesh_renderer_props.set_editor_property("bSortOnlyWhenTranslucent", True)

        # set the niagara subimage size as the composition of the cascade
        # horizontal and vertical subimages
        subimage_size = ue.Vector2D(subimages_horizontal, subimages_vertical)
        mesh_renderer_props.set_editor_property("SubImageSize", subimage_size)

        # get the niagara subimage blend enabled state and binding from the cascade
        # equivalent interpolation method
        subimage_blend, subimage_binding = (
            c2n_utils.get_enable_subimage_blend_and_binding_for_interpolation_method(
                interpolation_method))
        mesh_renderer_props.set_editor_property("bSubImageBlend", subimage_blend)
        
        # mesh_renderer_props.set_editor_property("FacingMode", )
        # mesh_renderer_props.set_editor_property("bLockedAxisEnable", )
        # mesh_renderer_props.set_editor_property("LockedAxis", )  # todo equivalent blueprint types
        # mesh_renderer_props.set_editor_property("LockedAxisSpace", )
        mesh_renderer_props.set_editor_property("bEnableFrustumCulling", False)
        mesh_renderer_props.set_editor_property("bEnableCameraDistanceCulling", False)
        # mesh_renderer_props.set_editor_property("MinCameraDistance", )
        # mesh_renderer_props.set_editor_property("MaxCameraDistance", )
        # mesh_renderer_props.set_editor_property("RendererVisibility", )

        #  todo generate collision module if enabled on cascade props

        # add the renderer properties
        niagara_emitter_context.add_renderer("MeshRenderer", mesh_renderer_props)
