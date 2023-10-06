from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeSpawnPerUnitConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleSpawnPerUnit

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        (   unit_scalar,
            movement_tolerance,
            spawn_per_unit_distribution, 
            max_frame_distance,
            b_ignore_spawn_rate_when_moving, 
            b_ignore_movement_along_x,
            b_ignore_movement_along_y, 
            b_ignore_movement_along_z,
            b_process_spawn_rate, 
            b_process_spawn_burst
         ) = ue_fx_utils.get_particle_module_spawn_per_unit_props(cascade_module)
    

        # add the spawn per unit module.
        spawn_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.script_spawn_per_unit)
        )
        niagara_emitter_context.add_module_script(
            "SpawnPerUnit",
            spawn_script,
            ue.ScriptExecutionCategory.EMITTER_UPDATE
        )
        
        # set the movement tolerance.
        spawn_script.set_parameter(
            "Movement Tolerance",
            ue_fx_utils.create_script_input_float(movement_tolerance),
            True,
            True
        )
        
        # set the spawn per unit value.
        # NOTE: Cascade specifies spawn per unit as "number of particles per 
        # unit" whereas Niagara specifies as "distance between each spawned 
        # particle".
        options = c2n_utils.DistributionConversionOptions()
        index_input = ue_fx_utils.create_script_input_linked_parameter(
            "Emitter.LoopedAge",
            ue.NiagaraScriptInputType.FLOAT
        )
        options.set_custom_indexer(index_input)
        spawn_per_unit_input = c2n_utils.create_script_input_for_distribution(
            spawn_per_unit_distribution,
            options
        )
        
        div_float_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_divide_float)
        )
        div_float_script.set_parameter("A", spawn_per_unit_input)
        div_float_script.set_parameter(
            "B",
            ue_fx_utils.create_script_input_float(unit_scalar)
        )
        
        spawn_per_unit_input = ue_fx_utils.create_script_input_dynamic(
            div_float_script,
            ue.NiagaraScriptInputType.FLOAT
        )

        div_float_script2 = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_divide_float)
        )
        div_float_script2.set_parameter(
            "A",
            ue_fx_utils.create_script_input_float(1.0)
        )
        div_float_script2.set_parameter("B", spawn_per_unit_input)

        spawn_per_unit_input = ue_fx_utils.create_script_input_dynamic(
            div_float_script2,
            ue.NiagaraScriptInputType.FLOAT
        )
        
        spawn_script.set_parameter("Spawn Per Unit", spawn_per_unit_input)
        
        # set the max movement tolerance.
        if max_frame_distance != 0.0:
            spawn_script.set_parameter(
                "Max Movement Threshold",
                ue_fx_utils.create_script_input_float(max_frame_distance)
            )

        #  todo handle optional spawn rate from the spawn per unit module
        if b_ignore_spawn_rate_when_moving:
            pass
        
        # optionally ignore movement on axes.
        b_need_ignore = False
        axes_mask = ue.Vector(1.0, 1.0, 1.0)
        if b_ignore_movement_along_x:
            b_need_ignore = True
            axes_mask.x = 0.0
        if b_ignore_movement_along_y:
            b_need_ignore = True
            axes_mask.y = 0.0
        if b_ignore_movement_along_z:
            b_need_ignore = True
            axes_mask.z = 0.0
        
        if b_need_ignore:
            mul_vec_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_multiply_vector)
            )
            mul_vec_script.set_parameter(
                "A",
                ue_fx_utils.create_script_input_linked_parameter(
                    "Engine.Owner.Velocity",
                    ue.NiagaraScriptInputType.VEC3
                )
            )
            mul_vec_script.set_parameter(
                "B",
                ue_fx_utils.create_script_input_vector(axes_mask)
            )
            
            velocity_vector_input = ue_fx_utils.create_script_input_dynamic(
                mul_vec_script,
                ue.NiagaraScriptInputType.VEC3
            )
            spawn_script.set_parameter("Velocity Vector", velocity_vector_input)
        
        
        #  todo kill all spawn rate/burst if these are false
        if b_process_spawn_rate:
            pass
        if b_process_spawn_burst:
            pass
