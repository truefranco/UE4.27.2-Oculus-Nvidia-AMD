"""
Interface class for module converters.
"""


class ModuleConverterInterface(object):
    """
    Abstract base class for module converters. Extend new ModuleConverters from
    this class and place them under the CascadeToNiagaraConverter Plugin's
    Content/Python/ModuleConversionScripts directory to have it discovered.
    """

    class ModuleConverterArgs:
        """
        Args struct for converter interface methods.
        """

        def __init__(
            self,
            cascade_module,
            required_module,
            typedata_module,
            niagara_emitter_context
        ):
            """
            Arguments for module converter method.
            
            Args:
                cascade_module (UParticleModule): The main module to convert.
                required_module (UParticleModuleRequired): The required module 
                    which all cascade emitters have.
                niagara_emitter_context (UNiagaraEmitterConversionContext): The 
                    niagara emitter to be modified.
            """
            self.__cascade_module = cascade_module
            self.__required_module = required_module
            self.__typedata_module = typedata_module
            self.__niagara_emitter_context = niagara_emitter_context

        def get_cascade_module(self):
            """
            Get the main module to convert.

            Returns:
                cascade_module (UParticleModule): The main module to convert.
            """
            return self.__cascade_module
        
        def get_required_module(self):
            """
            Get the required module which all cascade emitters have.

            Returns:
                required_module (UParticleModuleRequired): The required module 
                    which all cascade emitters have.
            """
            return self.__required_module
        
        def get_typedata_module(self):
            """
            Get the typedata module of the emitter being converted. 
            
            Returns:
                typedata_module (UParticleModuleTypeDataBase): The typedata 
                    module of the emitter being converted.
                    
            Notes:
                If the emitter being converted does not have a typedata module, 
                    this will return None.
            """
            return self.__typedata_module
        
        def get_niagara_emitter_context(self):
            """
            Get the niagara emitter to be modified.

            Returns:
                cascade_module (UParticleModule): The niagara emitter to be 
                    modified.
            """
            return self.__niagara_emitter_context


    @classmethod
    def get_input_cascade_module(cls):
        """
        Get the StaticClass() of the target derived Cascade UParticleModule
        to input.

        Returns:
            UClass: Return derived type of UParticleModule::StaticClass() to
                begin conversion from.
        """
        pass

    @classmethod
    def convert(cls, args):
        """
        Convert the input cascade_module to new niagara scripts (modules,
        functions, dynamic inputs) and add the new scripts to the
        niagara_emitter_context.

        Args:
            args (ModuleConverterArgs) Args struct containing the module to be 
                converted and the target niagara emitter to modify.
        """
        pass
