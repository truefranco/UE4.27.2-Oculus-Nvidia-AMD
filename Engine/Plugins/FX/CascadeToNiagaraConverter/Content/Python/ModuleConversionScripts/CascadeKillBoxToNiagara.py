from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeKillBoxConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleKillBox

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties from the cascade kill box module
        (   lower_left_corner_distribution,
            upper_right_corner_distribution,
            b_world_space_coords,
            b_kill_inside,
            b_axis_aligned_and_fixed_size
        ) = ue_fx_utils.get_particle_module_kill_box_props(cascade_module)

        # find/add the niagara kill volume module
        kill_volume_script = niagara_emitter_context.find_or_add_module_script(
            "KillVolumeBox",
            ue.AssetData(Paths.script_kill_particles_in_volume),
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )

        # set the kill volume to be a box shape.
        kill_shape_input = ue_fx_utils.create_script_input_enum(
            Paths.enum_niagara_kill_volume_options,
            "Box"
        )
        kill_volume_script.set_parameter("Kill Shape", kill_shape_input)

        # set the box size; subtract the top right corner position (positive 
        # coordinate) from the lower left position (negative coordinate) to get
        # the box size.
        subtract_vectors_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_subtract_vector))
        
        positive_box_coord_input = c2n_utils.create_script_input_for_distribution(
            upper_right_corner_distribution)
        negative_box_coord_input = c2n_utils.create_script_input_for_distribution(
            lower_left_corner_distribution)
        
        subtract_vectors_script.set_parameter("A", positive_box_coord_input)
        subtract_vectors_script.set_parameter("B", negative_box_coord_input)
        
        box_size_input = ue_fx_utils.create_script_input_dynamic(
            subtract_vectors_script, ue.NiagaraScriptInputType.VEC3)
        
        kill_volume_script.set_parameter("Box Size", box_size_input)

        # explicitly set the world/local space option as the volume origin.
        if b_world_space_coords:
            world_space_origin_input = \
                ue_fx_utils.create_script_input_linked_parameter(
                    "Engine.Owner.Position", ue.NiagaraScriptInputType.VEC3
                )
            kill_volume_script.set_parameter(
                "Volume Origin", world_space_origin_input)
            
        else:
            local_space_origin_input = ue_fx_utils.create_script_input_vector(
                ue.Vector(0.0, 0.0, 0.0))
            kill_volume_script.set_parameter(
                "Volume Origin", local_space_origin_input)

        # set the kill inside/outside option.
        if b_kill_inside:
            invert_volume_input = ue_fx_utils.create_script_input_bool(False)
        else:
            invert_volume_input = ue_fx_utils.create_script_input_bool(True)
        kill_volume_script.set_parameter("Invert Volume", invert_volume_input)
