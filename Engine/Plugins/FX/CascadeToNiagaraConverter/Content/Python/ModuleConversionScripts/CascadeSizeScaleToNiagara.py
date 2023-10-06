from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeParticleSizeScaleConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleSizeScale

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties of the size scale module
        size_scale_distribution = ue_fx_utils.get_particle_module_size_scale_props(cascade_module)
        
        options = c2n_utils.DistributionConversionOptions()
        # choose the correct parameter and conversion options depending on 
        # renderer
        if niagara_emitter_context.find_renderer("RibbonRenderer") is not None:
            param_name = "Particles.RibbonWidth"
            options.set_target_type_width(ue.NiagaraScriptInputType.FLOAT)
        elif niagara_emitter_context.find_renderer("MeshRenderer") is not None:
            param_name = "Particles.Scale"
            options.set_target_type_width(ue.NiagaraScriptInputType.VEC3)
        else:
            param_name = "Particles.SpriteSize"
            options.set_target_type_width(ue.NiagaraScriptInputType.VEC2)

        # create the scale input
        scale_input = c2n_utils.create_script_input_for_distribution(
            size_scale_distribution,
            options
        )
        
        # set the scale directly
        niagara_emitter_context.set_parameter_directly(
            param_name,
            scale_input,
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )
