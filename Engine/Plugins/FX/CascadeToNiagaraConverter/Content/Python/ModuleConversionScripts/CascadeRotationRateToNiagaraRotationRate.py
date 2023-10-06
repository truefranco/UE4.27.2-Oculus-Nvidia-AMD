from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeRotationRateConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleRotationRate

    @classmethod
    #  todo handle mesh rotation rate
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties from the cascade rotation module
        rotation = ue_fx_utils.get_particle_module_rotation_rate_props(cascade_module)

        # make an input to apply the rotation rate
        options = c2n_utils.DistributionConversionOptions()
        options.set_target_type_width(ue.NiagaraScriptInputType.FLOAT)
        rotation_input = c2n_utils.create_script_input_for_distribution(
            rotation, options)

        # set the sprite rotation rate parameter directly
        niagara_emitter_context.set_parameter_directly(
            "Particles.SpriteRotationRate",
            rotation_input,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)
        
        # find/add a module script for sprite rotation rate
        rotation_rate_script = niagara_emitter_context.find_or_add_module_script(
            "SpriteRotationRate",
            ue.AssetData(Paths.script_sprite_rotation_rate),
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )
        
        # get the rotation rate from spawn
        rotation_from_spawn_input = ue_fx_utils.create_script_input_linked_parameter(
            "Particles.SpriteRotationRate", ue.NiagaraScriptInputType.FLOAT)
        
        # convert the rotation rate to degrees
        norm_angle_to_degrees_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_normalized_angle_to_degrees))
        norm_angle_to_degrees_script.set_parameter(
            "NormalizedAngle",
            rotation_from_spawn_input)
        rotation_from_spawn_input = ue_fx_utils.create_script_input_dynamic(
            norm_angle_to_degrees_script,
            ue.NiagaraScriptInputType.FLOAT)
        
        rotation_rate_script.set_parameter(
            "Rotation Rate", rotation_from_spawn_input)
