from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeSpawnConverter(ModuleConverterInterface):

    call_count = 0

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleSpawn

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        (rate_distribution, rate_scale_distribution, burst_method, burst_list,
         burst_scale_distribution,b_apply_global_spawn_rate_scale,
         b_process_spawn_rate, b_process_spawn_burst) = (
            ue_fx_utils.get_particle_module_spawn_props(cascade_module))
        
        # increment the call count
        cls.call_count += 1

        # convert the spawn rate
        if b_process_spawn_rate is True:
            cls.convert_spawn_rate(
                rate_distribution,
                rate_scale_distribution,
                niagara_emitter_context)

        if b_process_spawn_burst is True:
            cls.convert_spawn_burst(
                burst_scale_distribution,
                burst_method,
                burst_list,
                niagara_emitter_context)

    @classmethod
    def convert_spawn_rate(
        cls,
        rate_distribution,
        rate_scale_distribution,
        niagara_emitter_context
    ):
        # check spawn rate and spawn rate scale values for conditions that 
        # would end up with skipping the code execution
        if c2n_utils.distribution_always_equals(rate_distribution, 0.0):
            return
        rate_input = c2n_utils.create_script_input_for_distribution(
            rate_distribution)

        if c2n_utils.distribution_always_equals(rate_scale_distribution, 0.0):
            return
        rate_scale_input = c2n_utils.create_script_input_for_distribution(
            rate_scale_distribution)

        # make the new module script for spawn rate
        spawn_rate_script = niagara_emitter_context.find_or_add_module_script(
            ("SpawnRate" + str(cls.call_count)),
            ue.AssetData(Paths.script_spawnrate),
            ue.ScriptExecutionCategory.EMITTER_UPDATE)

        # convert whether to apply global spawn rate scale
        spawn_rate_script.log(
            "Skipped converting bApplyGlobalSpawnRateScale.",
            ue.NiagaraMessageSeverity.WARNING)

        # check if spawn rate scale needs to be applied
        if c2n_utils.distribution_always_equals(rate_scale_distribution, 1.0) is False:
            # multiply spawn rate by spawn rate scale
            multiply_float_script_asset = ue.AssetData(Paths.di_multiply_float)
            multiply_float_script = (
                ue_fx_utils.create_script_context(multiply_float_script_asset))
            multiply_float_script.set_parameter("A", rate_input)
            multiply_float_script.set_parameter("B", rate_scale_input)

            # reassign rate_input to the new multiplied rate * rate_scale
            rate_input = ue_fx_utils.create_script_input_dynamic(
                multiply_float_script, ue.NiagaraScriptInputType.FLOAT)

        # set the spawn rate
        spawn_rate_script.set_parameter("SpawnRate", rate_input)

    @classmethod
    def convert_spawn_burst(
        cls,
        burst_scale_distribution,
        burst_method,
        burst_list,
        niagara_emitter_context
    ):
        # check the burst scale is not 0
        if c2n_utils.distribution_always_equals(burst_scale_distribution, 0.0):
            return
        burst_scale_input = c2n_utils.create_script_input_for_distribution(
            burst_scale_distribution)

        # convert the particle burst method
        #  todo how to respond to different burst method?
        if burst_method == ue.ParticleBurstMethod.EPBM_INSTANT:
            pass
        elif burst_method == ue.ParticleBurstMethod.EPBM_INTERPOLATED:
            pass
        else:
            raise NameError("Unexpected ParticleBurstMethod type encountered!")

        # make the new module script asset data for spawn burst
        spawn_burst_asset_data = ue.AssetData(Paths.script_spawnburst)

        # iterate on the particle burst list
        i = 0
        for burst in burst_list:
            i += 1

            # make a new spawn burst script for each burst entry
            spawn_burst_script = niagara_emitter_context.find_or_add_module_script(
                ("SpawnBurst" + str(cls.call_count) + "_" + str(i)),
                spawn_burst_asset_data,
                ue.ScriptExecutionCategory.EMITTER_UPDATE)

            # convert the burst count
            count = burst.get_editor_property("Count")
            count_input = ue_fx_utils.create_script_input_int(count)
            countlow = burst.get_editor_property("CountLow")

            # if countlow is greater than or equal to 0, pick a random 
            # number in range [countlow, count]
            if countlow >= 0:
                random_range_int_script_asset = ue.AssetData(Paths.di_random_range_int)
                random_range_int_script = (
                    ue_fx_utils.create_script_context(random_range_int_script_asset))
                countlow_input = ue_fx_utils.create_script_input_int(countlow)
                random_range_int_script.set_parameter("Minimum", countlow_input)
                random_range_int_script.set_parameter("Maximum", count_input)
                
                # reassign count_input to the random range between count_low and
                # count
                count_input = ue_fx_utils.create_script_input_dynamic(
                    random_range_int_script, ue.NiagaraScriptInputType.INT)

            # if the burst scale is not fixed at 1, multiply it into the final 
            # count
            if (
                c2n_utils.distribution_always_equals(
                    burst_scale_distribution, 1.0) is False
            ):
                # convert the spawn count to a float in order to multiply by the
                # spawn scale float.
                float_from_int_script = ue_fx_utils.create_script_context(
                    ue.AssetData(Paths.di_float_from_int))
                float_from_int_script.set_parameter("Int", count_input)
                count_input = ue_fx_utils.create_script_input_dynamic(
                    float_from_int_script,
                    ue.NiagaraScriptInputType.FLOAT)
                
                # multiply the spawn count by the spawn scale
                multiply_float_script = ue_fx_utils.create_script_context(
                    ue.AssetData(Paths.di_multiply_float))
                multiply_float_script.set_parameter("A", burst_scale_input)
                multiply_float_script.set_parameter("B", count_input)
                count_input = ue_fx_utils.create_script_input_dynamic(
                    multiply_float_script, ue.NiagaraScriptInputType.FLOAT)
                
                # convert back from float to int
                int_from_float_script = ue_fx_utils.create_script_context(
                    ue.AssetData(Paths.di_int_from_float))
                int_from_float_script.set_parameter("Float", count_input)
                count_input = ue_fx_utils.create_script_input_dynamic(
                    int_from_float_script, ue.NiagaraScriptInputType.INT)      
 
            # set the burst spawn count
            spawn_burst_script.set_parameter("Spawn Count", count_input)

            # set the burst time
            time_input = ue_fx_utils.create_script_input_float(
                burst.get_editor_property("Time"))
            spawn_burst_script.set_parameter("Spawn Time", time_input)
