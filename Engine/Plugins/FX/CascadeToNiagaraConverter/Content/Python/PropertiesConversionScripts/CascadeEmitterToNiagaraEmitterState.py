from EmitterPropertiesConverterInterface import EmitterPropertiesConverterInterface
import Strings
import unreal as ue
import Paths
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils


class DefaultEmitterPropertyConverter(EmitterPropertiesConverterInterface):

    @classmethod
    def get_name(cls):
        return Strings.default_emitter_property_converter

    @classmethod
    def convert(
        cls,
        cascade_emitter,
        cascade_required_module,
        niagara_emitter_context
    ):
        # get the cascade emitter properties.
        # todo correlate quality level and detail mode to niagara relevance-handling
        cascade_emitter.get_editor_property("EmitterRenderMode")
        cascade_emitter.get_editor_property("SignificanceLevel")
        cascade_emitter.get_editor_property("bUseLegacySpawningBehavior")
        cascade_emitter.get_editor_property("bDisabledLODsKeepEmitterAlive")
        cascade_emitter.get_editor_property("bDisableWhenInsignficant")
        cascade_emitter.get_editor_property("InitialAllocationCount")
        cascade_emitter.get_editor_property("QualityLevelSpawnRateScale")
        cascade_emitter.get_editor_property("DetailModeBitmask")
        
        # get the cascade required module per-renderer properties.
        (emitter_origin, emitter_rotation, b_use_local_space, 
         b_kill_on_deactivate, b_kill_on_complete, b_use_legacy_emitter_time,
         b_emitter_duration_use_range, emitter_duration, emitter_duration_low,
         b_emitter_delay_use_range, b_delay_first_loop_only, emitter_delay,
         emitter_delay_low, b_duration_recalc_each_loop, num_emitter_loops) = \
            ue_fx_utils.get_particle_module_required_per_emitter_props(
                cascade_required_module
            )
        
        # set the niagara emitter config from cascade emitter and required 
        # module properties.
        niagara_emitter = niagara_emitter_context.get_emitter()
        niagara_emitter.set_editor_property("bLocalSpace", b_use_local_space)
        
        # add the emitter state script.
        emitter_state_script_asset = (
            ue.AssetData('/Niagara/Modules/Emitter/EmitterState.EmitterState'))
        emitter_state_script = niagara_emitter_context.find_or_add_module_script(
            "EmitterState",
            emitter_state_script_asset,
            ue.ScriptExecutionCategory.EMITTER_UPDATE)
        
        # set the emitter state script properties from cascade emitter and 
        # required module properties.
        life_cycle_mode_input = ue_fx_utils.create_script_input_enum(
            Paths.enum_niagara_emitter_life_cycle_mode, "Self")
        emitter_state_script.set_parameter(
            "Life Cycle Mode", life_cycle_mode_input)
        
        if b_kill_on_deactivate:
            inactive_response_mode_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_niagara_inactive_mode,
                "Kill (Emitter and Particles Die Immediately)"
            )
        else:
            inactive_response_mode_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_niagara_inactive_mode, 
                "Complete (Let Particles Finish then Kill Emitter)"
            )
        emitter_state_script.set_parameter(
            "Inactive Response", inactive_response_mode_input)

        if num_emitter_loops == 0:
            # special case: 0 loops means loop forever.
            loop_behavior_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_niagara_emitter_state_options, "Infinite")
        elif num_emitter_loops == 1:
            loop_behavior_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_niagara_emitter_state_options, "Once")
        else:
            loop_behavior_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_niagara_emitter_state_options, "Multiple")
        emitter_state_script.set_parameter("Loop Behavior", loop_behavior_input)
        
        if b_emitter_duration_use_range:
            loop_duration_input = c2n_utils.create_script_input_random_range(
                emitter_duration_low,
                emitter_duration
            )
        else:
            loop_duration_input = ue_fx_utils.create_script_input_float(
                emitter_duration)
        emitter_state_script.set_parameter("Loop Duration", loop_duration_input)
        
        recalc_duration_input = ue_fx_utils.create_script_input_bool(
            b_duration_recalc_each_loop)
        emitter_state_script.set_parameter(
            "Recalculate Duration Each Loop", recalc_duration_input)
        
        if b_emitter_delay_use_range:
            emitter_delay_input = c2n_utils.create_script_input_random_range(
                emitter_delay_low, emitter_delay)
        else:
            emitter_delay_input = ue_fx_utils.create_script_input_float(
                emitter_delay)
        emitter_state_script.set_parameter(
            "Loop Delay", emitter_delay_input, True, True)
        
        delay_first_loop_input = ue_fx_utils.create_script_input_bool(
            b_delay_first_loop_only)
        if b_delay_first_loop_only:
            emitter_state_script.set_parameter(
                "Delay First Loop Only", delay_first_loop_input, True, True)
        else:
            emitter_state_script.set_parameter(
                "Delay First Loop Only", delay_first_loop_input, False, False)
