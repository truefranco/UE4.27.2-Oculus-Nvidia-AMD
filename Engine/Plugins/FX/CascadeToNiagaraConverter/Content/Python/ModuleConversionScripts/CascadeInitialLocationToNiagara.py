from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeInitialLocationConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleLocation

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # choose the correct niagara module depending on the converted renderer
        if niagara_emitter_context.find_renderer("RibbonRenderer") is not None:
            script_name = "InitializeRibbon"
            initialize_script_asset_data = ue.AssetData(
                Paths.script_initialize_ribbon)
        else:
            script_name = "InitializeParticle"
            initialize_script_asset_data = ue.AssetData(
                Paths.script_initialize_particle)

        # find/add the module script for init'ing the location.
        initialize_script = niagara_emitter_context.find_or_add_module_script(
            script_name,
            initialize_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN
        )

        # get all properties from the cascade initial location module
        (start_location_distribution, distribute_over_n_points,
         distribute_threshold) = (
            ue_fx_utils.get_particle_module_location_props(cascade_module)
        )

        #  todo implement "choose only n particles" dynamic input
        
        # if distribute over n points is not 0 or 1, special case handle the 
        # start location distribution to be over a equispaced range.
        if distribute_over_n_points != 0.0 and distribute_over_n_points != 1.0:
            range_n_input = c2n_utils.create_script_input_random_range(
                0.0, distribute_over_n_points)
            n_input = ue_fx_utils.create_script_input_int(
                distribute_over_n_points)
            
            div_float_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_divide_float))
            div_float_script.set_parameter("A", range_n_input)
            div_float_script.set_parameter("B", n_input)
            indexer_input = ue_fx_utils.create_script_input_dynamic(
                div_float_script, ue.NiagaraScriptInputType.FLOAT)
            
            options = c2n_utils.DistributionConversionOptions()
            options.set_custom_indexer(indexer_input)
            position_input = c2n_utils.create_script_input_for_distribution(
                start_location_distribution, 
                options
            )
        else:
            indexer_input = ue_fx_utils.create_script_input_linked_parameter(
                "Emitter.LoopedAge", ue.NiagaraScriptInputType.FLOAT)
            options = c2n_utils.DistributionConversionOptions()
            options.set_custom_indexer(indexer_input)
            position_input = c2n_utils.create_script_input_for_distribution(
                start_location_distribution)

        # set the position.
        initialize_script.set_parameter("Position", position_input)
