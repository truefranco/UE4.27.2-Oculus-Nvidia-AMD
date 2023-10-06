from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths

class CascadeMeshRotationRateConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleMeshRotationRate

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties of the mesh rotation rate module.
        start_rotation_rate_distribution = (
            ue_fx_utils.get_particle_module_mesh_rotation_rate_props(cascade_module)
        )
        
        # add the update mesh orientation script.
        mesh_orient_script = niagara_emitter_context.find_or_add_module_script(
            "UpdateMeshOrientation",
            ue.AssetData(Paths.script_update_mesh_orientation),
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )
        
        # evaluate the rotation vector in spawn, and set it on the script in 
        # update.
        options = c2n_utils.DistributionConversionOptions()
        emitter_age_index_input = ue_fx_utils.create_script_input_linked_parameter(
            "Emitter.LoopedAge",
            ue.NiagaraScriptInputType.FLOAT
        )
        options.set_custom_indexer(emitter_age_index_input)
        rotation_vec_input = c2n_utils.create_script_input_for_distribution(
            start_rotation_rate_distribution,
            options
        )
        
        niagara_emitter_context.set_parameter_directly(
            "Particles.SpawnRotationVector",
            rotation_vec_input,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN
        )
        
        spawn_rotation_vec_input = ue_fx_utils.create_script_input_linked_parameter(
            "Particles.SpawnRotationVector",
            ue.NiagaraScriptInputType.VEC3
        )
        
        mesh_orient_script.set_parameter(
            "Rotation Vector",
            spawn_rotation_vec_input
        )
