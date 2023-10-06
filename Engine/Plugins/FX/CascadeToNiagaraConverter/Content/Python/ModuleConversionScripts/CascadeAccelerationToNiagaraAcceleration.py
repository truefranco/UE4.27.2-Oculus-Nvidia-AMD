from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeAccelerationConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleAcceleration

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
        acceleration_distribution, b_apply_owner_scale = (
            ue_fx_utils.get_particle_module_acceleration_props(cascade_module))

        # log that we are skipping apply owner scale for now
        if b_apply_owner_scale:
            acceleration_script.log(
                "Skipped converting b_apply_owner_scale of cascade acceleration "
                "module; niagara equivalent module does not support this mode!",
                ue.NiagaraMessageSeverity.WARNING,
            )
        else:
            pass

        # make an input to apply the acceleration vector
        acceleration_input = c2n_utils.create_script_input_for_distribution(
            acceleration_distribution
        )

        # cache the acceleration value in the spawn script
        niagara_emitter_context.set_parameter_directly(
            "Particles.Acceleration",
            acceleration_input,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)

        # set the acceleration value
        cached_acceleration_input = ue_fx_utils.create_script_input_linked_parameter(
            "Particles.Acceleration", ue.NiagaraScriptInputType.VEC3)
        acceleration_script.set_parameter(
            "Acceleration", cached_acceleration_input)
