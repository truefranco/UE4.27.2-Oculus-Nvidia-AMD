from RendererConverterInterface import RendererConverterInterface
import unreal as ue
import RendererConversionScripts.NullCascadeTypeDataToNiagaraSpriteRenderer


class CascadeGPUTypeDataConverter(RendererConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleTypeDataGpu

    @classmethod
    def convert(cls, cascade_typedata_module, cascade_required_module,
                niagara_emitter_context):
        
        # set the emitter sim target to gpu
        niagara_emitter_context.get_emitter().set_editor_property(
            "SimTarget", ue.NiagaraSimTarget.GPU_COMPUTE_SIM)
        
        RendererConversionScripts.NullCascadeTypeDataToNiagaraSpriteRenderer.SpriteConverter.convert(
            cascade_typedata_module,
            cascade_required_module,
            niagara_emitter_context
        )
