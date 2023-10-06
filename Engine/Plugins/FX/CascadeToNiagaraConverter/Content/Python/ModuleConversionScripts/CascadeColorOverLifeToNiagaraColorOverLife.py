from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeColorOverLifeConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleColorOverLife

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # make the new module script for color over life
        color_script_asset_data = ue.AssetData(Paths.script_color)
        color_script = niagara_emitter_context.find_or_add_module_script(
            "ColorOverLife",
            color_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_UPDATE)

        # get all properties from the cascade color over life module
        color_over_life_distribution, alpha_over_life_distribution, b_clamp_alpha = (
            ue_fx_utils.get_particle_module_color_over_life_props(cascade_module))
        
        # convert the color over life property
        color_over_life_input = c2n_utils.create_script_input_for_distribution(
            color_over_life_distribution)
        
        # convert the alpha over life property
        alpha_over_life_input = c2n_utils.create_script_input_for_distribution(
                alpha_over_life_distribution)
        
        # convert the clamp alpha property
        if b_clamp_alpha is True:
            clamp_float_script_asset = ue.AssetData(Paths.di_clamp_float)
            clamp_float_script = ue_fx_utils.create_script_context(
                clamp_float_script_asset)
            min_input = ue_fx_utils.create_script_input_float(0)
            max_input = ue_fx_utils.create_script_input_float(1)
            clamp_float_script.set_parameter("Min", min_input)
            clamp_float_script.set_parameter("Max", max_input)
            
            # reassign the alpha over life input to the new clamped alpha over
            # life input so that it is applied to the top level color script
            clamp_float_script.set_parameter("Float", alpha_over_life_input)
            alpha_over_life_input = ue_fx_utils.create_script_input_dynamic(
                clamp_float_script, ue.NiagaraScriptInputType.FLOAT)
        
        # combine color over life and alpha over life into linear color
        break_color_script_asset = ue.AssetData(Paths.di_color_from_vec_and_float)
        break_color_script = ue_fx_utils.create_script_context(
            break_color_script_asset)
        break_color_script.set_parameter("Vector (RGB)", color_over_life_input)
        break_color_script.set_parameter("Float (Alpha)", alpha_over_life_input)
        
        # set the color
        break_color_script_input = ue_fx_utils.create_script_input_dynamic(
            break_color_script, ue.NiagaraScriptInputType.LINEAR_COLOR)
        color_script.set_parameter("Color", break_color_script_input)
