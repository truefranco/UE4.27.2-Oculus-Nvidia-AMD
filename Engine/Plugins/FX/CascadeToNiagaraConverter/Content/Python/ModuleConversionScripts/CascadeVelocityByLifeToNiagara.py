from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeVelocityByLifeConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleVelocityOverLifetime

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get velocity over lifetime properties.
        (   vel_over_life_distribution,
            b_absolute,
            b_world_space,
            b_apply_owner_scale
        ) = ue_fx_utils.get_particle_module_velocity_over_lifetime_props(cascade_module)

        
        # make sure there is a solve forces and velocity module.
        niagara_emitter_context.find_or_add_module_script(
            "SolveForcesAndVelocity",
            ue.AssetData(Paths.script_solve_forces_and_velocity),
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )

        # make input for velocity over life.
        vel_over_life_input = c2n_utils.create_script_input_for_distribution(
            vel_over_life_distribution)
        
        # apply owner scale if necessary.
        if b_apply_owner_scale:
            mul_vec_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_multiply_vector)
            )
            owner_scale_input = ue_fx_utils.create_script_input_linked_parameter(
                "Engine.Owner.Scale",
                ue.NiagaraScriptInputType.VEC3
            )
            
            mul_vec_script.set_parameter("A", vel_over_life_input)
            mul_vec_script.set_parameter("B", owner_scale_input)
            
            vel_over_life_input = ue_fx_utils.create_script_input_dynamic(
                mul_vec_script,
                ue.NiagaraScriptInputType.VEC3
            )

        # choose behavior based on b_absolute.
        # if True, set velocity directly. 
        # if False, scale velocity.
        if b_absolute:
            niagara_emitter_context.set_parameter_directly(
                "Particles.Velocity",
                vel_over_life_input,
                ue.ScriptExecutionCategory.PARTICLE_UPDATE
            )
        else:
            scale_velocity_script = niagara_emitter_context.find_or_add_module_script(
                "ScaleVelocity",
                ue.AssetData(Paths.script_scale_velocity),
                ue.ScriptExecutionCategory.PARTICLE_UPDATE
            )
            scale_velocity_script.set_parameter(
                "Velocity Scale",
                vel_over_life_input
            )
            
            # use appropriate coordinate space.
            if b_world_space:
                coordinate_space_name = "World"
            else:
                coordinate_space_name = "Local"
                
            scale_velocity_script.set_parameter(
                "Coordinate Space",
                ue_fx_utils.create_script_input_enum(
                    Paths.enum_niagara_coordinate_space,
                    coordinate_space_name
                )
            )
