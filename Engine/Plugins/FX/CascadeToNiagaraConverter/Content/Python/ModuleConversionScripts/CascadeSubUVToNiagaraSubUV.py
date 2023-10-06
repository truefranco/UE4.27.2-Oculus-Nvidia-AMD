from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import Paths


class CascadeSubUVConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleSubUV

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # find/add the module script for sub uv animation
        subuv_script_asset = ue.AssetData(Paths.script_subuv_animation)
        subuv_script = niagara_emitter_context.find_or_add_module_script(
            "SubUV",
            subuv_script_asset,
            ue.ScriptExecutionCategory.PARTICLE_UPDATE)

        # get all properties from the cascade sub uv module
        (animation, subuv_index, b_use_real_time) = \
            ue_fx_utils.get_particle_module_sub_uv_props(cascade_module)

        # set the play rate
        if b_use_real_time:
            subuv_script.log(
                "Failed to set \"Use Emitter Time\": Niagara does not support "
                "this mode!",
                ue.NiagaraMessageSeverity.ERROR
            )
            #  todo Divide particle age by world time dilation for the play rate 
            #   input. Not implemented as Niagara does not currently subsume 
            #   world time dilation.
            pass

        # get the max value off of the sub uv distribution as this will be the 
        # number of frames
        success, min_frame_val, max_frame_val = \
            ue_fx_utils.get_distribution_min_max_values(subuv_index)
        
        if success is True:
            max_frame = max_frame_val.x
        else:
            # set a sensible default and log that we couldn't resolve the frame
            # count
            max_frame = 1
            subuv_script.log(
                "Could not determine number of frames in uv sequence!",
                ue.NiagaraMessageSeverity.WARNING)

        # set the number of frames
        max_frame_input = ue_fx_utils.create_script_input_float(max_frame)
        subuv_script.set_parameter("Number Of Frames", max_frame_input)
