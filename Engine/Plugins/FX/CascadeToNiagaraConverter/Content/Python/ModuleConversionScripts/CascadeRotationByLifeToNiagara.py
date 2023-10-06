from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeRotationByLifeConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleRotationOverLifetime

    @classmethod
    #  todo handle mesh rotation rate
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties from the cascade rotation over life module
        rotation_over_life_distribution, b_scale = \
            ue_fx_utils.get_particle_module_rotation_over_lifetime_props(cascade_module)

        # make an input to apply the rotation
        rotation_input = c2n_utils.create_script_input_for_distribution(
            rotation_over_life_distribution)

        # choose how to apply the rotation.
        # from ParticleModuleRotationOverLifetime.h:
        # If true, the particle rotation is multiplied by the value retrieved 
        # from RotationOverLife.
        # If false, the particle rotation is incremented by the value retrieved 
        # from RotationOverLife.
        
        cur_rotation_input = ue_fx_utils.create_script_input_linked_parameter(
            "Particles.SpriteRotation",
            ue.NiagaraScriptInputType.FLOAT
        )
        if b_scale:
            combine_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_multiply_float)
            )
        else:
            combine_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_add_floats)
            )
        combine_script.set_parameter("A", cur_rotation_input)
        combine_script.set_parameter("B", rotation_input)
        final_rotation_input = ue_fx_utils.create_script_input_dynamic(
            combine_script,
            ue.NiagaraScriptInputType.FLOAT
        )
        
        # convert the combined rotation to degrees
        norm_angle_to_degrees_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_normalized_angle_to_degrees))
        norm_angle_to_degrees_script.set_parameter(
            "NormalizedAngle",
            final_rotation_input)
        final_rotation_input = ue_fx_utils.create_script_input_dynamic(
            norm_angle_to_degrees_script,
            ue.NiagaraScriptInputType.FLOAT)
        
        # set the sprite rotation parameter directly
        niagara_emitter_context.set_parameter_directly(
            "Particles.SpriteRotation",
            final_rotation_input,
            ue.ScriptExecutionCategory.PARTICLE_UPDATE)
