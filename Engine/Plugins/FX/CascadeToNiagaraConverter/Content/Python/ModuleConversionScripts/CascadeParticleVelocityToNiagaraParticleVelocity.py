from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeParticleVelocityConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleVelocity

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()

        # get all properties from the cascade particle velocity module
        (   start_velocity_distribution,
            start_velocity_radial_distribution,
            b_world_space,
            b_apply_owner_scale
        ) = ue_fx_utils.get_particle_module_velocity_props(cascade_module)

        b_need_solver = False
        if c2n_utils.distribution_always_equals(start_velocity_distribution, 0.0) is False:
            b_need_solver = True

            # find/add the module script for adding velocity in a cone
            add_velocity_script = niagara_emitter_context.find_or_add_module_script(
                "AddVelocityInCone",
                ue.AssetData(Paths.script_add_velocity),
                ue.ScriptExecutionCategory.PARTICLE_SPAWN)

            # convert the distribution properties
            start_velocity_input = c2n_utils.create_script_input_for_distribution(
                start_velocity_distribution)

            # set the start velocity
            add_velocity_script.set_parameter("Velocity", start_velocity_input)
        
        if c2n_utils.distribution_always_equals(start_velocity_radial_distribution, 0.0) is False:
            b_need_solver = True
            
            # find/add the module script for adding force from a point
            add_velocity_point_script = niagara_emitter_context.find_or_add_module_script(
                "PointForce",
                ue.AssetData(Paths.script_add_velocity_from_point),
                ue.ScriptExecutionCategory.PARTICLE_SPAWN)
            
            # convert the distribution properties
            start_velocity_radial_input = c2n_utils.create_script_input_for_distribution(
                start_velocity_radial_distribution)

            # set the point velocity
            add_velocity_point_script.set_parameter("Velocity Strength", start_velocity_radial_input)
        
        if b_need_solver:
            # find/add the module script for solving forces and velocity
            niagara_emitter_context.find_or_add_module_script(
                "SolveForcesAndVelocity",
                ue.AssetData(Paths.script_solve_forces_and_velocity),
                ue.ScriptExecutionCategory.PARTICLE_UPDATE)
