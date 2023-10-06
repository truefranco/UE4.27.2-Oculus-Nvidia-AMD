from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeParameterDynamicConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleParameterDynamic

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        def get_velocity_channel_input(vector_channel_name):
            # make the float from vector dynamic input
            float_from_vector_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_float_from_vector))
            
            # bind the vector to particle velocity
            velocity_input = ue_fx_utils.create_script_input_linked_parameter(
                "Particles.Velocity", ue.NiagaraScriptInputType.VEC3)
            float_from_vector_script.set_parameter("Vector", velocity_input)
            
            # choose the desired component of velocity.
            vector3_channel_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_vec3_channel,
                vector_channel_name
            )
            float_from_vector_script.set_parameter("Channel", vector3_channel_input)
            
            # return the input
            return ue_fx_utils.create_script_input_dynamic(
                float_from_vector_script, ue.NiagaraScriptInputType.FLOAT)
        
        def __set_dynamic_param_internal(
            in_spawn_script,
            in_update_script,
            b_in_spawn,
            b_in_update,
            in_param_name,
            in_param_input
        ):
            """
            Wrapper around setting a parameter on the dynamic parameter script 
            to handle independently enabling/disabling inputs in spawn/update if
            the source cascade module defines some inputs as evaluating in spawn
            and the rest in update.
            
            Args:
                in_spawn_script (UNiagaraScriptConversionContext): The dynamic 
                    parameter script evaluated in spawn.
                in_update_script (UNiagaraScriptConversionContext): The dynamic 
                    parameter script evaluated in update.
                b_in_spawn (bool): Whether the parameter should be set on the 
                    spawn script.
                b_in_update (bool): Whether the parameter should be set on the 
                    update script.
                in_param_name (str): The name of the parameter to be set.
                in_param_input (UNiagaraScriptConversionContextInput): The input 
                    to set on the parameter.
            """
            def __set_dynamic_param_for_script(script, b_set):
                # if the script does not exist, early out.
                if script is None:
                    return
                
                if b_set is True:
                    script.set_parameter(in_param_name, in_param_input)
                else:
                    # set an empty value and disable the parameter static 
                    # switch.
                    script.set_parameter(
                        in_param_name,
                        ue_fx_utils.create_script_input_float(0.0),
                        True,
                        False)
                    
            __set_dynamic_param_for_script(in_spawn_script, b_in_spawn)
            __set_dynamic_param_for_script(in_update_script, b_in_update)
                
        # get all properties from the cascade parameter dynamic module
        (   dynamic_params,
            b_uses_velocity
        ) = ue_fx_utils.get_particle_module_parameter_dynamic_props(cascade_module)

        # make scripts for spawn and/or update
        spawn_script = None
        update_script = None
        for dynamic_param in dynamic_params:
            if spawn_script is not None and update_script is not None:
                break
            if spawn_script is None and dynamic_param.spawn_time_only is True:
                spawn_script = niagara_emitter_context.find_or_add_module_script(
                    "DynamicParametersSpawn",
                    ue.AssetData(Paths.script_dynamic_parameters),
                    ue.ScriptExecutionCategory.PARTICLE_SPAWN)
            elif update_script is None and dynamic_param.spawn_time_only is False:
                update_script = niagara_emitter_context.find_or_add_module_script(
                    "DynamicParametersUpdate",
                    ue.AssetData(Paths.script_dynamic_parameters),
                    ue.ScriptExecutionCategory.PARTICLE_UPDATE)
            
        # track the dynamic parameter index for getting the appropriate module
        # input name
        target_index = 0

        # iterate for each dynamic parameter
        for dynamic_param in dynamic_params:
            target_index += 1
            if target_index == 1:
                target_param_name = "Index 0 Param 1"
            elif target_index == 2:
                target_param_name = "Index 0 Param 2"
            elif target_index == 3:
                target_param_name = "Index 0 Param 3"
            elif target_index == 4:
                target_param_name = "Index 0 Param 4"
            else:
                raise NameError("Cascade dynamic parameter script had more than"
                                "4 dynamic parameters!")

            # choose to set on spawn or update
            if dynamic_param.spawn_time_only is True:
                b_spawn = True
                b_update = False
            else:
                b_spawn = False
                b_update = True
            
            options = c2n_utils.DistributionConversionOptions()
            if dynamic_param.use_emitter_time is True:
                options.set_index_by_emitter_age()
            
            dynamic_param_value = dynamic_param.get_editor_property("ParamValue")
            dynamic_param_distribution = dynamic_param_value.get_editor_property(
                "Distribution")
            dynamic_param_method = dynamic_param.value_method

            # make the lambda to call for setting parameters. This will handle 
            # independently setting the parameter in spawn and update scripts.
            def set_dynamic_param(b_in_spawn, b_in_update, in_param_input):
                __set_dynamic_param_internal(
                    spawn_script,
                    update_script,
                    b_in_spawn,
                    b_in_update,
                    target_param_name,
                    in_param_input)
            
            # choose what value to set based on the dynamic param value method.
            if dynamic_param_method == ue.EmitterDynamicParameterValue.EDPV_AUTO_SET:
                # auto set must means ignore, pass this entry
                set_dynamic_param(
                    False,
                    False,
                    ue_fx_utils.create_script_input_float(0.0))
            
            elif dynamic_param_method == ue.EmitterDynamicParameterValue.EDPV_USER_SET:
                # user set means evaluate the distribution
                param_value_input = c2n_utils.create_script_input_for_distribution(
                    dynamic_param_distribution, options)
                set_dynamic_param(b_spawn, b_update, param_value_input)
            
            elif dynamic_param_method == ue.EmitterDynamicParameterValue.EDPV_VELOCITY_X:
                velocity_x_input = get_velocity_channel_input("X")
                
                if dynamic_param.scale_velocity_by_param_value:
                    multiply_float_script = ue_fx_utils.create_script_context(
                        ue.AssetData(Paths.di_multiply_float))
                    multiply_float_script.set_parameter("A", velocity_x_input)
                    param_value_input = c2n_utils.create_script_input_for_distribution(
                        dynamic_param_distribution, options)
                    multiply_float_script.set_parameter("B", param_value_input)
                    velocity_x_input = ue_fx_utils.create_script_input_dynamic(
                        multiply_float_script, ue.NiagaraScriptInputType.FLOAT)
                    
                set_dynamic_param(b_spawn, b_update, velocity_x_input)
                
            elif dynamic_param_method == ue.EmitterDynamicParameterValue.EDPV_VELOCITY_Y:
                velocity_y_input = get_velocity_channel_input("Y")
                
                if dynamic_param.scale_velocity_by_param_value:
                    multiply_float_script = ue_fx_utils.create_script_context(
                        ue.AssetData(Paths.di_multiply_float))
                    multiply_float_script.set_parameter("A", velocity_y_input)
                    param_value_input = c2n_utils.create_script_input_for_distribution(
                        dynamic_param_distribution, options)
                    multiply_float_script.set_parameter("B", param_value_input)
                    velocity_y_input = ue_fx_utils.create_script_input_dynamic(
                        multiply_float_script, ue.NiagaraScriptInputType.FLOAT)
                    
                set_dynamic_param(b_spawn, b_update, velocity_y_input)
                
            elif dynamic_param_method == ue.EmitterDynamicParameterValue.EDPV_VELOCITY_Z:
                velocity_z_input = get_velocity_channel_input("Z")
                
                if dynamic_param.scale_velocity_by_param_value:
                    multiply_float_script = ue_fx_utils.create_script_context(
                        ue.AssetData(Paths.di_multiply_float))
                    multiply_float_script.set_parameter("A", velocity_z_input)
                    param_value_input = c2n_utils.create_script_input_for_distribution(
                        dynamic_param_distribution, options)
                    multiply_float_script.set_parameter("B", param_value_input)
                    velocity_z_input = ue_fx_utils.create_script_input_dynamic(
                        multiply_float_script, ue.NiagaraScriptInputType.FLOAT)
                
                set_dynamic_param(b_spawn, b_update, velocity_z_input)

            elif dynamic_param_method == ue.EmitterDynamicParameterValue.EDPV_VELOCITY_MAG:
                vector_mag_script = ue_fx_utils.create_script_context(
                    ue.AssetData(Paths.di_vector_length_safe))
                velocity_input = ue_fx_utils.create_script_input_linked_parameter(
                    "Particles.Velocity", ue.NiagaraScriptInputType.VEC3)
                vector_mag_script.set_parameter("Velocity", velocity_input)
                vector_mag_input = ue_fx_utils.create_script_input_dynamic(
                    vector_mag_script, ue.NiagaraScriptInputType.FLOAT)
                
                set_dynamic_param(b_spawn, b_update, vector_mag_input)
                
            else:
                if b_spawn:
                    spawn_script.log(
                        "Encountered unknown emitter dynamic parameter value "
                        "type!",
                        ue.NiagaraMessageSeverity.ERROR)
                elif b_update:
                    update_script.log(
                        "Encountered unknown emitter dynamic parameter value "
                        "type!",
                        ue.NiagaraMessageSeverity.ERROR)
