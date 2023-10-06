from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeParticleSizeConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleSize

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        typedata_module = args.get_typedata_module()
        
        # choose the correct niagara module depending on the converted renderer
        if (
            typedata_module is None or  # sprite renderer
            type(typedata_module) == ue.ParticleModuleTypeDataGpu
        ):
            required_module = args.get_required_module()
            cls.__convert_sprite_size(
                cascade_module,
                niagara_emitter_context,
                required_module)
        elif type(typedata_module) == ue.ParticleModuleTypeDataRibbon:
            cls.__convert_ribbon_size(cascade_module, niagara_emitter_context)
        elif type(typedata_module) == ue.ParticleModuleTypeDataMesh:
            cls.__convert_mesh_size(cascade_module, niagara_emitter_context)
        else:
            module_name = c2n_utils.get_module_name(cascade_module, "None")
            typedata_module_name = c2n_utils.get_module_name(
                typedata_module,
                "Sprite")
            raise NameError("Could not convert module \"" +
                            module_name +
                            "\": CascadeParticleSizeConverter does not "
                            "support emitters with renderer of type \"" +
                            typedata_module_name + 
                            "\".")
                

    @classmethod
    def __convert_sprite_size(
        cls,
        cascade_module,
        niagara_emitter_context,
        required_module
    ):
        # find/add the module script for init particle
        initialize_particle_script_asset_data = ue.AssetData(Paths.script_initialize_particle)
        initialize_particle_script = niagara_emitter_context.find_or_add_module_script(
            "InitializeParticle",
            initialize_particle_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)

        # get all properties from the cascade particle size module
        size_distribution = ue_fx_utils.get_particle_module_size_props(cascade_module)

        # convert the size property
        options = c2n_utils.DistributionConversionOptions()
        options.set_target_type_width(ue.NiagaraScriptInputType.VEC2)
        
        # if this is being used to drive a sprite renderer where the facing mode
        # is facing the camera, the only component of the particle size that is
        # needed is the x component.
        screen_alignment = required_module.get_editor_property("ScreenAlignment")
        
        if (
            screen_alignment == ue.ParticleScreenAlignment.PSA_SQUARE or
            screen_alignment == ue.ParticleScreenAlignment.PSA_FACING_CAMERA_POSITION or
            screen_alignment == ue.ParticleScreenAlignment.PSA_FACING_CAMERA_DISTANCE_BLEND
        ):
            options.set_target_vector_component("x")

        size_input = c2n_utils.create_script_input_for_distribution(
                size_distribution, options)

        # set the size
        initialize_particle_script.set_parameter(
            "Sprite Size", size_input, True, True)

    @classmethod
    def __convert_ribbon_size(cls, cascade_module, niagara_emitter_context):
        # find/add the module script for init ribbon
        initialize_ribbon_script_asset_data = ue.AssetData(Paths.script_initialize_ribbon)
        initialize_ribbon_script = niagara_emitter_context.find_or_add_module_script(
            "InitializeRibbon",
            initialize_ribbon_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)
        
        # get all properties from the cascade particle size module
        size_distribution = ue_fx_utils.get_particle_module_size_props(cascade_module)

        # convert the size property; break out the width component of the size vector
        options = c2n_utils.DistributionConversionOptions()
        options.set_target_type_width(ue.NiagaraScriptInputType.FLOAT)
        options.set_target_vector_component("x")
        size_input = c2n_utils.create_script_input_for_distribution(
            size_distribution,
            options
        )

        # set the size
        initialize_ribbon_script.set_parameter("Ribbon Width", size_input)

    @classmethod
    def __convert_mesh_size(cls, cascade_module, niagara_emitter_context):
        # find/add the module script for init particle
        initialize_particle_script_asset_data = ue.AssetData(Paths.script_initialize_particle)
        initialize_particle_script = niagara_emitter_context.find_or_add_module_script(
            "InitializeParticle",
            initialize_particle_script_asset_data,
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)
    
        # get all properties from the cascade particle size module
        size_distribution = ue_fx_utils.get_particle_module_size_props(cascade_module)
    
        # convert the size property
        size_input = c2n_utils.create_script_input_for_distribution(
            size_distribution)
    
        # set the size
        initialize_particle_script.set_parameter("Mesh Scale", size_input, True, True)
