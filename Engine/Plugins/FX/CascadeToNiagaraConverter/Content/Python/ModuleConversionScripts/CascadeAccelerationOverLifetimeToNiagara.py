from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeAccelerationOverLifetimeConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleAccelerationOverLifetime

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # find/add the module script for acceleration force
        acceleration_script_asset = ue.AssetData(Paths.script_acceleration_force)
        acceleration_script = niagara_emitter_context.find_or_add_module_script(
            "Acceleration",
            acceleration_script_asset,
            ue.ScriptExecutionCategory.PARTICLE_UPDATE)

        # get all properties from the cascade acceleration module
        acceleration_over_life = ue_fx_utils.get_particle_module_acceleration_over_lifetime_props(cascade_module)

        # make an input to apply the acceleration vector
        acceleration_input = c2n_utils.create_script_input_for_distribution(
            acceleration_over_life)

        # set the acceleration value
        acceleration_script.set_parameter(
            "Acceleration", acceleration_input)
