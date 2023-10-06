from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeLineAttractorConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleAttractorLine

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties from the cascade line attractor module
        (start_point, end_point, range_distribution, strength_distribution) = (
            ue_fx_utils.get_particle_module_attractor_line_props(cascade_module)
        )
        
        # find/add the niagara line attractor module
        line_attractor_script = niagara_emitter_context.find_or_add_module_script(
            "LineAttractor",
            ue.AssetData(Paths.script_line_attractor),
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )
        
        # find/add the niagara solve forces and velocity module to resolve the 
        # dependency.
        niagara_emitter_context.find_or_add_module_script(
            "SolveForcesAndVelocity",
            ue.AssetData(Paths.script_solve_forces_and_velocity),
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )
        
        # set the start and end points
        start_point_input = ue_fx_utils.create_script_input_vector(start_point)
        end_point_input = ue_fx_utils.create_script_input_vector(end_point)
        line_attractor_script.set_parameter("Line Start", start_point_input)
        line_attractor_script.set_parameter("Line End", end_point_input)
        
        # set the attraction strength
        strength_input = c2n_utils.create_script_input_for_distribution(
            strength_distribution)
        line_attractor_script.set_parameter(
            "Attraction Strength", strength_input)
        
        # set the attraction range
        range_input = c2n_utils.create_script_input_for_distribution(
            range_distribution)
        line_attractor_script.set_parameter(
            "Attraction Falloff", range_input, True, True)
