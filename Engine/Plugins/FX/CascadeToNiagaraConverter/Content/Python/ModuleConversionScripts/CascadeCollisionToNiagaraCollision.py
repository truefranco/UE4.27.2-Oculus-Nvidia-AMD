from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeCollisionConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleCollision

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()

        # find/add the module script for collision
        collision_script_asset_data = ue.AssetData(Paths.script_collision)
        collision_script = niagara_emitter_context.find_or_add_module_script(
            "Collision",
            collision_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_UPDATE)

        # get all properties from the cascade collision module
        (damping_factor, rotation_damping_factor, max_collisions,
            collision_complete_option, collision_types, b_apply_physics,
            b_ignore_trigger_volumes, particle_mass, dir_scalar,
            b_pawns_do_not_decrement_count,
            b_only_vertical_normals_decrement_count, vertical_fudge_factor,
            delay, b_drop_detail, b_collide_only_if_visible,
            b_ignore_source_actor, max_collision_distance
         ) = ue_fx_utils.get_particle_module_collision_props(cascade_module)
        
        # choose how to handle the collision response option
        if collision_complete_option == ue.ParticleCollisionComplete.EPCC_KILL:
            pass
        elif collision_complete_option == ue.ParticleCollisionComplete.EPCC_FREEZE:
            pass
        elif collision_complete_option == ue.ParticleCollisionComplete.EPCC_FREEZE_MOVEMENT:
            pass
        elif collision_complete_option == ue.ParticleCollisionComplete.EPCC_FREEZE_ROTATION:
            pass
        elif collision_complete_option == ue.ParticleCollisionComplete.EPCC_FREEZE_TRANSLATION:
            pass
        elif collision_complete_option == ue.ParticleCollisionComplete.EPCC_HALT_COLLISIONS:
            pass
        else:
            collision_script.log(
                "Encountered unexpected collision complete option!",
                ue.NiagaraMessageSeverity.ERROR
            )
        return
        
        # apply damping factor
        
        # apply rotation damping factor
        
        # kill particles if they reach max collisions
        #  todo niagara collision module does not support max collisions
        
        # choose the appropriate collision channel
        #  todo niagara collision module does not support channels
        
        # choose to apply physics to world objects
        #  todo niagara collision DI really doesn't have this
        
        """
        collision_script.set_parameter("Control Roll On Collision", None)
        collision_script.set_parameter("Radius Calculation", None)
        collision_script.set_parameter("Method for Calculation", None)
        collision_script.set_parameter("Particle Radius Scale", None)
        collision_script.set_parameter("Restitution", None)
        collision_script.set_parameter("Restitution Coefficient", None)
        collision_script.set_parameter("Randomize Collision Normal", None)
        collision_script.set_parameter("Simple Friction", None)
        collision_script.set_parameter("CPU Friction Blending", None)
        collision_script.set_parameter("Friction", None)
        collision_script.set_parameter("Friction During a Bounce", None)
        collision_script.set_parameter("CPU Trace Vector Length Multiplier", None)
        collision_script.set_parameter("Max CPU Trace Length", None)
        """