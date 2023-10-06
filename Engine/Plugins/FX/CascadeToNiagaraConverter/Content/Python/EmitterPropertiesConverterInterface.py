class EmitterPropertiesConverterInterface(object):

    @classmethod
    def get_name(cls):
        pass

    @classmethod
    def convert(
        cls,
        cascade_emitter,
        cascade_required_module,
        niagara_emitter_context
    ):
        pass
