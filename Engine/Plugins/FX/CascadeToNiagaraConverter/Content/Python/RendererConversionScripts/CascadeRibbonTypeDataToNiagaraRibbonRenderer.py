from RendererConverterInterface import RendererConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils


class CascadeRibbonTypeDataConverter(RendererConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleTypeDataRibbon

    @classmethod
    def convert(cls, cascade_typedata_module, cascade_required_module,
                niagara_emitter_context):
        
        # create a new ribbon renderer properties and get the properties from
        # the cascade typedata and required modules
        ribbon_renderer_props = ue.NiagaraRibbonRendererProperties()

        (max_tessellation_between_particles, sheets_per_trail, max_trail_count,
         max_particle_in_trail_count, b_dead_trails_on_deactivate,
         b_clip_source_segment, b_enable_previous_tangent_recalculation,
         b_tangent_recalculation_every_Frame, b_spawn_initial_particle,
         render_axis, tangent_spawning_scalar, b_render_geometry,
         b_render_spawn_points, b_render_tangents, b_render_tessellation,
         tiling_distance, distance_tessellation_step_size,
         b_enable_tangent_diff_interp_scale, tangent_tessellation_scalar) = (
            ue_fx_utils.get_particle_module_type_data_ribbon_props(
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
        
        ribbon_renderer_props.set_editor_property("Material", material_interface)
        
        #  todo evaluate whether we will ever need custom material param binds
        # ribbon_renderer_props.set_editor_property("MaterialUserParamBinding")
        
        # the niagara ribbon facing mode is always custom to emulate cascade
        # behavior
        ribbon_renderer_props.set_editor_property(
            "FacingMode", ue.NiagaraRibbonFacingMode.CUSTOM)
        
        # default ribbon particle sort ordering to front-to-back
        ribbon_renderer_props.set_editor_property(
            "DrawDirection", ue.NiagaraRibbonDrawDirection.FRONT_TO_BACK)
        
        #  todo evaluate what curve tension is equivalent to in cascade paradigm
        # ribbon_renderer_props.set_editor_property("CurveTension")
        
        # the niagara ribbon tessellation mode is always custom to emulate
        # cascade behavior
        ribbon_renderer_props.set_editor_property(
            "TessellationMode", ue.NiagaraRibbonTessellationMode.CUSTOM)

        #  todo is this cascade alike?
        # ribbon_renderer_props.set_editor_property("TessellationFactor", distance_tessellation_step_size)
        # ribbon_renderer_props.set_editor_property("bUseConstantFactor")

        #  todo sanity check this parameterization is correct
        ribbon_renderer_props.set_editor_property(
            "TessellationAngle", 180.0/tangent_tessellation_scalar) 
        
        ribbon_renderer_props.set_editor_property(
            "bScreenSpaceTessellation", False)
        
        # add the renderer properties
        niagara_emitter_context.add_renderer("RibbonRenderer", ribbon_renderer_props)
