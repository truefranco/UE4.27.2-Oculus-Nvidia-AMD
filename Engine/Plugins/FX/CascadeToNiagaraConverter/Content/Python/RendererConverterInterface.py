"""
Interface class for cascade typedata and required module to niagara properties
and renderer converters.
"""


class RendererConverterInterface(object):
    """
    Abstract base class for renderer converters. Extend new RendererConverters
    from this class and place them under the CascadeToNiagaraConverter Plugin's
    Content/Python/RendererConversionScripts directory to have it discovered.
    """

    @classmethod
    def get_input_cascade_module(cls):
        """
        Get the StaticClass() of the target derived Cascade
        UParticleModuleTypeData to input.

        Returns:
            UClass: Return derived type of
            UParticleModuleTypeData::StaticClass() to begin conversion from.
        """
        pass

    @classmethod
    def convert(
        cls, cascade_typedata_module, cascade_required_module,
        niagara_emitter_context
    ):
        """
        Convert the input cascade_typedata_module to niagara renderer
        properties and add the renderer properties to the
        niagara_emitter_context.

        Args:
            cascade_typedata_module (UParticleModuleTypeData): The cascade
                typedata module to convert.
                
            cascade_required_module (UParticleModuleRequired): The cascade
                required module to convert.
                
            niagara_emitter_context (FGuid): Guid handle to the niagara emitter
                to add new renderer properties to.
        """
        pass
