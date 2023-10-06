from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeColorConverter(ModuleConverterInterface):
    color_script_dir = '/Niagara/Modules/Update/Color/Color.Color'

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleColor

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # make the new module script for initial color
        color_script_asset_data = ue.AssetData(cls.color_script_dir)
        color_script = niagara_emitter_context.find_or_add_module_script(
            "InitialColor",
            color_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)

        # get all properties from the cascade color module
        initial_color, initial_alpha, b_clamp_alpha = (
            ue_fx_utils.get_particle_module_color_props(cascade_module))

        # make inputs for the initial values
        initial_color_input = c2n_utils.create_script_input_for_distribution(
            initial_color)
        initial_alpha_input = c2n_utils.create_script_input_for_distribution(
            initial_alpha)

        # clamp alpha if required
        if b_clamp_alpha is True:
            clamp_float_script_asset = ue.AssetData(Paths.di_clamp_float)
            clamp_float_script = ue_fx_utils.create_script_context(
                clamp_float_script_asset)
            min_input = ue_fx_utils.create_script_input_float(0)
            max_input = ue_fx_utils.create_script_input_float(1)
            clamp_float_script.set_parameter("Min", min_input)
            clamp_float_script.set_parameter("Max", max_input)

            # reassign the initial alpha input to the new clamped alpha input so
            # that it is applied to the top level color script.
            clamp_float_script.set_parameter("Float", initial_alpha_input)
            initial_alpha_input = ue_fx_utils.create_script_input_dynamic(
                clamp_float_script, ue.NiagaraScriptInputType.FLOAT)

        # combine initial color and alpha into linear color
        break_color_script_asset = ue.AssetData(Paths.di_color_from_vec_and_float)
        break_color_script = ue_fx_utils.create_script_context(
            break_color_script_asset)
        break_color_script.set_parameter("Vector (RGB)", initial_color_input)
        break_color_script.set_parameter("Float (Alpha)", initial_alpha_input)

        # set the color
        break_color_script_input = ue_fx_utils.create_script_input_dynamic(
            break_color_script, ue.NiagaraScriptInputType.LINEAR_COLOR)
        color_script.set_parameter("Color", break_color_script_input)
