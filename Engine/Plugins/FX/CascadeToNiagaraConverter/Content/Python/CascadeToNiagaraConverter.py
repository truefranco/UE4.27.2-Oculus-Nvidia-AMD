"""
Main script with static methods for converting cascade systems to niagara
systems.
"""
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import os
import imp
import inspect
import Paths
import Strings
import HelperMethods
import traceback
from EmitterPropertiesConverterInterface import (
    EmitterPropertiesConverterInterface as EmitterPropertiesConverterBase)
from ModuleConverterInterface import (
    ModuleConverterInterface as ModuleConverterBase)
from RendererConverterInterface import (
    RendererConverterInterface as RendererConverterBase)


class SystemCleanupWrapper(object):
    def __init__(self, unreal_object, system_context):
        self.unreal_object = unreal_object
        self.system_context = system_context
    
    def __enter__(self):
        pass
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        # null objects on the system conversion context and its owned emitter
        # conversion contexts to prevent a failure to delete the new system
        # asset
        self.system_context.cleanup()
        # now explicitly delete the python object to clear any references to the
        # system conversion context
        del self.system_context
        # force gc to delete the system conversion context UObject as we will 
        # otherwise leave viewmodels hanging
        ue.SystemLibrary.collect_garbage()
        # if conversion failed, delete the new system asset
        if exc_tb is not None:
            ue.EditorAssetLibrary.delete_asset(
                self.unreal_object.get_path_name()
            )


class EmitterCleanupWrapper(object):
    def __init__(self, emitter_context):
        self.emitter_context = emitter_context
        
    def __enter__(self):
        pass
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.emitter_context.cleanup()

  
class ConverterContextMgr(object):
    def __init__(
        self,
        cascade_module,
        converter,
        emitter_context
    ):
        self.cascade_module = cascade_module
        self.converter = converter
        self.emitter_context = emitter_context

    def __enter__(self):
        pass
   
    def __exit__(self, exc_type, exc_val, exc_tb):
        # if conversion was successful, early exit.
        if exc_tb is None:
            return True

        # else, log what failed during the module conversion.
        # record the module that failed conversion.
        cascade_module_name = c2n_utils.get_module_name(
            self.cascade_module,
            'None')
        result_str = 'Failed converting module \"' + \
                     cascade_module_name + \
                     '\" '

        # record the module converter that failed.
        result_str += 'with converter \"' + \
                      str(self.converter.__name__) + \
                      '\":\n\t'    
        
        # record the traceback.
        result_str += traceback.format_exc()
        
        # log the result.
        self.emitter_context.log(result_str, ue.NiagaraMessageSeverity.ERROR)
        
        # continue execution.
        return True


def convert_cascade_to_niagara(cascade_system, conversion_results):
    """
    Create a new niagara system asset by converting a cascade particle system.

    Args:
        cascade_system (UParticleSystem): Cascade particle system to convert
            to a niagara system.
        conversion_results (UConvertCascadeToNiagaraResults): Status of the 
            conversion operation to be passed back to engine.
    """

    # import all fx module converter modules first.
    emitter_properties_converters = import_fx_converter_classes(
        Paths.properties_converter_dir, EmitterPropertiesConverterBase)
    module_converters = import_fx_converter_classes(
        Paths.module_converter_dir, ModuleConverterBase)
    renderer_converters = import_fx_converter_classes(
        Paths.renderer_converter_dir, RendererConverterBase)

    # create a niagara system.
    factory = ue.NiagaraSystemFactoryNew()
    asset_name = cascade_system.get_name()
    package_path = ue.SystemLibrary.get_path_name(cascade_system)
    long_package_path = ue_fx_utils.get_long_package_path(package_path)
    new_asset_name = asset_name + "_Converted"
    new_package_path = str(long_package_path) + "/" + new_asset_name
    default_suffix = ''
    
    asset_tools = ue.AssetToolsHelpers.get_asset_tools()
    unique_package_path, unique_asset_name = (
        asset_tools.create_unique_asset_name(new_package_path, default_suffix))
    long_unique_package_path = ue_fx_utils.get_long_package_path(
        unique_package_path)

    out_niagara_system = asset_tools.create_asset(
        unique_asset_name,
        long_unique_package_path,
        ue.NiagaraSystem,
        factory)

    # create a niagara system conversion context to begin modifying the 
    # system programmatically.
    niagara_system_context = ue_fx_utils.create_system_conversion_context(
        out_niagara_system)

    # convert from the cascade system to the new niagara system.
    with ue.ScopedSlowTask(
        __get_num_convert_actions(cascade_system)
    ) as slow_task:
        with SystemCleanupWrapper(
            out_niagara_system,
            niagara_system_context
        ):
            slow_task.make_dialog(True)
            __convert_cascade_to_niagara_system(
                cascade_system,
                emitter_properties_converters,
                module_converters,
                renderer_converters,
                niagara_system_context,
                slow_task)
            # mark the conversion as having been cancelled by the user if 
            # appropriate.
            if slow_task.should_cancel():
                conversion_results.cancelled_by_user = True

    # mark the conversion as having completed successfully if end of the script
    # has been reached.
    conversion_results.cancelled_by_python_error = False


def __convert_cascade_to_niagara_system(
    cascade_particle_system,
    emitter_properties_converters,
    module_converters,
    renderer_converters,
    niagara_system_context,
    slow_task
):
    """
    Iterate on the emitters and modules of a cascade system and add equivalent
    emitters and modules to a niagara system.

    Args:
        cascade_particle_system (UParticleSystem): Cascade system to convert to
    a niagara system.
        
        properties_converters (PropertiesConverterInterface[]): List of
            properties converter classes to use for transferring properties of
            cascade systems and emitters to niagara systems and emitters.
        
        module_converters (ModuleConverterInterface[]): List of module
            converter classes to use for converting cascade modules to niagara
            module scripts.
        
        renderer_converters (RendererConverterInterface[]): List of renderer
            converter classes to use for converting cascade renderer modules to
            niagara renderer properties.
        
        niagara_system_context (UNiagaraSystemConversionContext): System
            conversion context used to modify the UNiagaraSystem it points at.
    """
    # convert the cascade system properties.
    #  todo copy cascade system properties

    # decompose the source cascade particle system to emitters and convert
    # each emitter and its modules.
    cascade_emitters = (
        ue_fx_utils.get_cascade_system_emitters(cascade_particle_system))
    context_str = str(cascade_particle_system.get_name() + "| ")
    
    for cascade_emitter in cascade_emitters:
        # early out if the user has requested to cancel.
        if slow_task.should_cancel():
            break

        # create a new empty niagara emitter associated with a new niagara
        # emitter conversion context.
        niagara_emitter_context = niagara_system_context.add_empty_emitter(
            ue_fx_utils.get_cascade_emitter_name(cascade_emitter)
        )
        
        __convert_cascade_to_niagara_emitter(
            cascade_emitter,
            emitter_properties_converters,
            module_converters,
            renderer_converters,
            niagara_emitter_context,
            slow_task,
            context_str)
            
    # finalize the system conversion.
    niagara_system_context.finalize()


def __convert_cascade_to_niagara_emitter(
    cascade_emitter,
    emitter_properties_converters,
    module_converters,
    renderer_converters,
    niagara_emitter_context,
    slow_task,
    system_context_str
):
    # early out if the user has requested to cancel.
    if slow_task.should_cancel():
        return
    
    context_str = str(system_context_str + str(ue_fx_utils.get_cascade_emitter_name(cascade_emitter)) + "| ")
    
    # todo handle lods in niagara paradigm
    cascade_emitter_lod = ue_fx_utils.get_cascade_emitter_lod_level(
        cascade_emitter, 0)

    # copy the emitter enabled state.
    enabled = ue_fx_utils.get_lod_level_is_enabled(cascade_emitter_lod)
    niagara_emitter_context.set_enabled(enabled)

    # get the required module and optional typedata module.
    cascade_typedata_module = ue_fx_utils.get_lod_level_type_data_module(
        cascade_emitter_lod)
    cascade_required_module = ue_fx_utils.get_lod_level_required_module(
        cascade_emitter_lod)
    
    # convert the cascade emitter properties.
    properties_success = False
    for emitter_properties_converter in emitter_properties_converters:
        # default to getting the default property converter but leaving room
        # to configure.
        if (
            emitter_properties_converter.get_name() == 
            Strings.default_emitter_property_converter
        ):
            slow_task.enter_progress_frame(
                1,
                context_str + "Convert emitter properties"
            )
            with ConverterContextMgr(
                cascade_required_module,
                emitter_properties_converter,
                niagara_emitter_context
            ):
                emitter_properties_converter.convert(
                    cascade_emitter,
                    cascade_required_module,
                    niagara_emitter_context)
            
            properties_success = True
            break
    
    if properties_success is False:
        raise NameError("Failed to convert emitter properties: failed to find "
                        "property converter!")

    renderer_success = False
    if cascade_typedata_module is None:
        for renderer_converter in renderer_converters:
            if renderer_converter.get_input_cascade_module() is None:
                slow_task.enter_progress_frame(
                    1,
                    context_str + "Convert renderer: Sprite" 
                )
                with ConverterContextMgr(
                    cascade_required_module,
                    renderer_converter,
                    niagara_emitter_context
                ):
                    renderer_converter.convert(
                        cascade_typedata_module,
                        cascade_required_module,
                        niagara_emitter_context)
                renderer_success = True
                break

    else:
        for renderer_converter in renderer_converters:
            if type(cascade_typedata_module) == renderer_converter.get_input_cascade_module():
                slow_task.enter_progress_frame(
                    1,
                    context_str + "Convert renderer: " + c2n_utils.get_module_name(cascade_typedata_module, "Sprite")
                )
                with ConverterContextMgr(
                    cascade_required_module,
                    renderer_converter,
                    niagara_emitter_context
                ):
                    renderer_converter.convert(
                        cascade_typedata_module,
                        cascade_required_module,
                        niagara_emitter_context)
                renderer_success = True
                break

    if renderer_success is False:
        # log a warning if failed to find a converter for the typedata.
        typedata_module_name = c2n_utils.get_module_name(
            cascade_typedata_module,
            "None (For sprite renderer)")
        
        niagara_emitter_context.log(
            "Skipped converting cascade typedata module \""
            + typedata_module_name
            + "\". \n \t Reason: failed to find converter script for module."
            + "\n \t Note: AnimTrail and Beam renderers are not currently "
            + "supported.",
            ue.NiagaraMessageSeverity.WARNING,
            False)
    
    # convert the cascade modules except for the required module and optional 
    # typedata module.
    cascade_lod_modules = ue_fx_utils.get_lod_level_modules(cascade_emitter_lod)
    
    cascade_spawn_module = ue_fx_utils.get_lod_level_spawn_module(cascade_emitter_lod)
    if cascade_spawn_module is not None:
        cascade_lod_modules.append(cascade_spawn_module)

    for cascade_module in cascade_lod_modules:
        # find the appropriate module or renderer converter script for the 
        # input cascade module and call the convert method.
        module_success = False
        module_name = c2n_utils.get_module_name(cascade_module)
        for module_converter in module_converters:
            if (
                type(cascade_module) == module_converter.get_input_cascade_module()
            ):
                slow_task.enter_progress_frame(
                    1,
                    context_str + "Convert module: " + module_name)

                args = ModuleConverterBase.ModuleConverterArgs(
                    cascade_module,
                    cascade_required_module,
                    cascade_typedata_module,
                    niagara_emitter_context)
                with ConverterContextMgr(
                    cascade_module,
                    module_converter,
                    niagara_emitter_context
                ):
                    module_converter.convert(args)
                    
                niagara_emitter_context.log(
                    "Converted cascade module " + module_name + ".",
                    ue.NiagaraMessageSeverity.INFO,
                    True)
                module_success = True
                break

        if module_success is False:
            # log a warning if failed to find a converter for the module.
            niagara_emitter_context.log(
                "Skipped converting cascade module \""
                + module_name
                + "\". \n \t Reason: failed to find converter script for module.",
                ue.NiagaraMessageSeverity.WARNING,
                False
            )


def import_fx_converter_classes(engine_plugin_relative_dir, interface_class):
    """
    Get all classes from scripts that exist in a specific engine dir with the
    same parent interface class.

    Args:
        engine_plugin_relative_dir (str): Path to the dir to get converter
            classes from, relative to the engine's Content directory.
        interface_class (class): Parent class to check discovered classes
            against; only discovered classes that have a parent that is
            interface_class will be returned.

    Returns:
        class[]: List of classes that have the same parent interface_class.
    """

    engine_plugin_dir = ue.Paths.convert_relative_path_to_full(
        ue.Paths.engine_plugins_dir())

    converter_dir = os.path.join(engine_plugin_dir, engine_plugin_relative_dir)
    converter_names = filter(
        lambda x: (os.path.splitext(x)[1] == '.py'),
        os.listdir(converter_dir))

    converter_modules_and_paths = list()
    # define an explicit string to import source files with to prevent loading
    # all under the same module.
    i = 0
    for converter_name in converter_names:
        i = i+1
        converter_path = os.path.join(converter_dir, converter_name)
        converter_module = imp.load_source(str(i), converter_path)
        converter_modules_and_paths.append((converter_module, converter_path))

    out_classes = list()
    class_names_and_paths = list()
    for converter_module, converter_path in converter_modules_and_paths:
        for name, obj in inspect.getmembers(converter_module, inspect.isclass):
            if HelperMethods.class_has_parent(obj, interface_class):

                # check that the class name will not shadow another class.
                duplicate_path = None
                for visited_name, visited_path in class_names_and_paths:
                    if name == visited_name:
                        duplicate_path = visited_path
                        break

                if duplicate_path is not None:
                    raise NameError("Imported converter with duplicate class name: '"
                    + name + "' from script: '" + converter_path + "'."
                    + "\n Class name: '" + name + "' has already been imported from script: '"
                    + duplicate_path + "'. \n Make the class name unique to prevent a duplicate "
                    + "class import from hiding the previous class import.")

                class_names_and_paths.append((name, converter_path))
                out_classes.append(obj)

    return out_classes


def __get_num_convert_actions(cascade_system):
    """
    Get the number of slow task actions that will be performed when converting 
    the cascade_system (emitter properties, number of modules, etc.)

    Args:
        cascade_system (ue.CascadeSystem): The cascade system being converted.

    Returns:
        num: Number of convert actions to count during the slow task dialog.
    """
    num = 0
    cascade_emitters = (
        ue_fx_utils.get_cascade_system_emitters(cascade_system)
    )
    for cascade_emitter in cascade_emitters:
        # guaranteed to add an action for: typedata module, required module, 
        # and emitter properties.
        num += 3
    
        cascade_emitter_lod = ue_fx_utils.get_cascade_emitter_lod_level(
            cascade_emitter, 0)
        num += len(ue_fx_utils.get_lod_level_modules(cascade_emitter_lod))
        cascade_spawn_module = ue_fx_utils.get_lod_level_spawn_module(
            cascade_emitter_lod)
        if cascade_spawn_module is not None:
            num +=1
    
    # finished tallying emitters, return the total num of actions.
    return num
