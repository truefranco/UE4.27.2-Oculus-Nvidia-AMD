from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeAccelerationDragConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleAccelerationDrag

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # find/add the module script for drag
        drag_script_asset = ue.AssetData(Paths.script_drag)
        drag_script = niagara_emitter_context.find_or_add_module_script(
            "Drag", drag_script_asset, ue.ScriptExecutionCategory.PARTICLE_UPDATE)

        # get all properties from the cascade acceleration module
        acceleration_drag = ue_fx_utils.get_particle_module_acceleration_drag_props(cascade_module)

        # make an input to apply the acceleration drag
        acceleration_drag_input = c2n_utils.create_script_input_for_distribution(acceleration_drag)

        # set the acceleration value
        drag_script.set_parameter("Drag", acceleration_drag_input)

