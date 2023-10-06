from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeColorScaleConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleColorScaleOverLife

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # make the new module script for color scale over life
        color_scale_script_asset_data = ue.AssetData(Paths.script_color_scale)
        color_scale_script = niagara_emitter_context.find_or_add_module_script(
            "ColorScaleOverLife",
            color_scale_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_UPDATE)

        # get all properties from the cascade color scale over life module
        (   color_scale_over_life_distribution,
            alpha_scale_over_life_distribution,
            b_emitter_time
        ) = ue_fx_utils.get_particle_module_color_scale_over_life_props(cascade_module)

        # convert the color scale and alpha scale over life properties
        options = c2n_utils.DistributionConversionOptions()

        # if sampling curves with emitter age time, replace the curve index
        if b_emitter_time is True:
            emitter_time_input = ue_fx_utils.create_script_input_linked_parameter(
                "Emitter.NormalizedLoopAge",
                ue.NiagaraScriptInputType.FLOAT
            )
            options.set_custom_indexer(emitter_time_input)
        
        color_scale_over_life_input = (
            c2n_utils.create_script_input_for_distribution(
                color_scale_over_life_distribution,
                options
            )
        )

        alpha_scale_over_life_input = (
            c2n_utils.create_script_input_for_distribution(
                alpha_scale_over_life_distribution,
                options
            )
        )

        # set the color and alpha scale
        color_scale_script.set_parameter(
            "Scale RGB",
            color_scale_over_life_input
        )
        color_scale_script.set_parameter(
            "Scale Alpha",
            alpha_scale_over_life_input
        )
