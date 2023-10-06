from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeRotationConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleRotation

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # find/add the module script for init particle
        initialize_particle_script_asset_data = ue.AssetData(Paths.script_initialize_particle)
        initialize_particle_script = niagara_emitter_context.find_or_add_module_script(
            "InitializeParticle",
            initialize_particle_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)
        
        # get all properties from the cascade rotation module
        rotation = ue_fx_utils.get_particle_module_rotation_props(cascade_module)

        # make an input to apply the rotation
        rotation_input = c2n_utils.create_script_input_for_distribution(rotation)
        
        # convert the rotation to degrees
        norm_angle_to_degrees_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_normalized_angle_to_degrees))
        norm_angle_to_degrees_script.set_parameter(
            "NormalizedAngle",
            rotation_input)
        rotation_input = ue_fx_utils.create_script_input_dynamic(
            norm_angle_to_degrees_script,
            ue.NiagaraScriptInputType.FLOAT)

        # set the rotation value
        initialize_particle_script.set_parameter("Sprite Rotation", rotation_input, True, True)
