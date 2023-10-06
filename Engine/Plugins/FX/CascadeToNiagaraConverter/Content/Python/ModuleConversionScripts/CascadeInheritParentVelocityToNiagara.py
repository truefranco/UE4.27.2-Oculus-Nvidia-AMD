from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeInheritParentVelocityConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleVelocityInheritParent

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties of the cascade inherit parent velocity module.
        (   scale_distribution,
            b_world_space,
            b_apply_owner_scale
        ) = ue_fx_utils.get_particle_module_velocity_inherit_parent_props(cascade_module)
        
        # find/add the inherit velocity module.
        inherit_vel_script = niagara_emitter_context.find_or_add_module_script(
            "InheritVelocity",
            ue.AssetData(Paths.script_inherit_parent_velocity),
            ue.ScriptExecutionCategory.PARTICLE_SPAWN
        )

        #  todo consider apply initial forces as well

        # find/add the module script for solving forces and velocity
        niagara_emitter_context.find_or_add_module_script(
            "SolveForcesAndVelocity",
            ue.AssetData(Paths.script_solve_forces_and_velocity),
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )
        
        # make an input for the velocity scale.
        options = c2n_utils.DistributionConversionOptions()
        options.set_custom_indexer(
            ue_fx_utils.create_script_input_linked_parameter(
                "Emitter.LoopedAge",
                ue.NiagaraScriptInputType.FLOAT
            )
        )
        scale_vel_input = c2n_utils.create_script_input_for_distribution(
            scale_distribution,
            options
        )
        
        #  todo can we skip world space flag
        
        # apply owner scale if required.
        if b_apply_owner_scale:
            mul_vec_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_multiply_vector),
            )
            mul_vec_script.set_parameter("A", scale_vel_input)
            mul_vec_script.set_parameter(
                "B",
                ue_fx_utils.create_script_input_linked_parameter(
                    "Engine.Owner.Scale",
                    ue.NiagaraScriptInputType.VEC3
                )
            )
            scale_vel_input = ue_fx_utils.create_script_input_dynamic(
                mul_vec_script,
                ue.NiagaraScriptInputType.VEC3
            )
        
        # set the velocity scale.
        inherit_vel_script.set_parameter(
            "Velocity Scale",
            scale_vel_input
        )
        
        # disable the velocity limit, cascade does not enforce this.
        inherit_vel_script.set_parameter(
            "Velocity Limit",
            ue_fx_utils.create_script_input_float(0.0),
            True,
            False
        )
