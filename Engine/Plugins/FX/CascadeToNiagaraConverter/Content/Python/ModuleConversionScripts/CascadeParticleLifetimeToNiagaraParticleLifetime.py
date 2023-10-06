from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeParticleLifetimeConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleLifetime

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()

        # find/add the module script for particle state
        particle_state_script = niagara_emitter_context.find_or_add_module_script(
            "ParticleState",
            ue.AssetData(Paths.script_particle_state),
            ue.ScriptExecutionCategory.PARTICLE_UPDATE)
        
        # get all properties from the cascade particle lifetime module
        lifetime_distribution = ue_fx_utils.get_particle_module_lifetime_props(cascade_module)
        if c2n_utils.distribution_always_equals(lifetime_distribution, 0.0):
            # early exit if lifetime is set to 0; special case for infinite 
            # lifetime
            particle_state_script.set_parameter(
                "Kill Particles When Lifetime Has Elapsed",
                ue_fx_utils.create_script_input_bool(False)
            )
            particle_state_script.set_parameter(
                "Let Infinitely Lived Particles Die When Emitter Deactivates",
                ue_fx_utils.create_script_input_bool(True)
            )
            return
        
        # find/add the module script for init particle
        if niagara_emitter_context.find_renderer("RibbonRenderer") is not None:
            script_name = "InitializeRibbon"
            initialize_script_asset_data = ue.AssetData(
                Paths.script_initialize_ribbon)
        elif niagara_emitter_context.find_renderer("MeshRenderer") is not None:
            script_name = "InitializeParticle"
            initialize_script_asset_data = ue.AssetData(
                Paths.script_initialize_particle)
        else:
            script_name = "InitializeParticle"
            initialize_script_asset_data = ue.AssetData(
                Paths.script_initialize_particle)
            
        initialize_script = niagara_emitter_context.find_or_add_module_script(
            script_name,
            initialize_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)

        # convert the lifetime property
        lifetime_input = c2n_utils.create_script_input_for_distribution(
            lifetime_distribution)

        # set the lifetime
        initialize_script.set_parameter("Lifetime", lifetime_input)
