from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeDirectLocationConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleLocationDirect

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties from the cascade direct location module
        (   location_distribution,
            location_offset_distribution,
            scale_factor_distribution
        ) = ue_fx_utils.get_particle_module_location_direct_props(cascade_module)

        # create an input for the location.
        location_input = c2n_utils.create_script_input_for_distribution(
            location_distribution)
        
        # if the location offset is non zero, add it to the location.
        if (
            c2n_utils.distribution_always_equals(
                location_offset_distribution, 0.0)
        ) is False:
            add_vector_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_add_vector))
            location_offset_input = c2n_utils.create_script_input_for_distribution(
                location_offset_distribution)
            add_vector_script.set_parameter("A", location_input)
            add_vector_script.set_parameter("B", location_offset_input)
            location_input = ue_fx_utils.create_script_input_dynamic(
                add_vector_script, ue.NiagaraScriptInputType.VEC3)

        # if the scale factor distribution is not equal to 1, multiply the 
        # location input.
        if (
            c2n_utils.distribution_always_equals(
                scale_factor_distribution, 1.0)
        ) is False:
            multiply_vector_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_multiply_vector))
            scale_factor_input = c2n_utils.create_script_input_for_distribution(
                scale_factor_distribution)
            multiply_vector_script.set_parameter("A", location_input)
            multiply_vector_script.set_parameter("B", scale_factor_input)
            location_input = ue_fx_utils.create_script_input_dynamic(
                multiply_vector_script, ue.NiagaraScriptInputType.VEC3)

        # set the position.
        niagara_emitter_context.set_parameter_directly(
            "Particles.Position",
            location_input,
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )
