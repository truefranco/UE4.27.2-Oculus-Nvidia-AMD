from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeOrbitConverter(ModuleConverterInterface):

    # mark when the first link mode orbit module is visited.
    b_first_link_visited = False

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleOrbit

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties of the orbit module.
        (   chain_mode,
            offset_amount_distribution,
            offset_options, 
             rotation_distribution,
            rotation_options, 
            rotation_rate_distribution,
            rotation_rate_options
        ) = ue_fx_utils.get_particle_module_orbit_props(cascade_module)
        
        # add a converter specific orbit script to the emitter.
        orbit_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.script_orbit),
        )
        niagara_emitter_context.add_module_script(
            "Orbit",
            orbit_script,
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )
        
        # make sure there is also a solver script.
        niagara_emitter_context.find_or_add_module_script(
            "SolveOrbit",
            ue.AssetData(Paths.script_solve_orbit),
            ue.ScriptExecutionCategory.PARTICLE_UPDATE
        )
        
        # point the renderers position binding to the output position of the 
        # orbit solver script.
        renderers = niagara_emitter_context.get_all_renderers()
        for renderer in renderers:
            niagara_emitter_context.set_renderer_binding(
                renderer,
                "PositionBinding",
                "Particles.OrbitOffsetPosition",
                ue.NiagaraRendererSourceDataMode.PARTICLES
            )
        
        # choose behavior based on the orbit chain mode.
        b_check_first_orbit_link = False
        if chain_mode == ue.OrbitChainMode.EO_CHAIN_MODE_ADD:
            enum_value_name = "Add"
        elif chain_mode == ue.OrbitChainMode.EO_CHAIN_MODE_LINK:
            enum_value_name = "Link"
            b_check_first_orbit_link = True
        elif chain_mode == ue.OrbitChainMode.EO_CHAIN_MODE_SCALE:
            enum_value_name = "Scale"
        else:
            raise NameError("Encountered invalid chain mode when converting "
                            "cascade orbit module!")
        orbit_script.set_parameter(
            "Chain Mode",
            ue_fx_utils.create_script_input_enum(
                Paths.enum_cascade_niagara_orbit_mode,
                enum_value_name
            )
        )
        
        # set the static switch if this is the first link mode orbit module.
        if cls.b_first_link_visited is False and b_check_first_orbit_link:
            cls.b_first_link_visited = True
            niagara_emitter_context.set_parameter(
                "First Chain Mode Link",
                ue_fx_utils.create_script_input_bool(True)
            )

        def set_orbit_param(distribution, options, param_name):
            convert_options = c2n_utils.DistributionConversionOptions()
            if options.get_editor_property("bProcessDuringSpawn"):
                convert_options.set_b_evaluate_spawn_only(True)
            elif options.get_editor_property("bProcessDuringUpdate"):
                convert_options.set_b_evaluate_spawn_only(False)
            else:
                raise NameError("Orbit was not set to evaluate during spawn or "
                                "update!")

            if options.get_editor_property("bUseEmitterTime"):
                convert_options.set_index_by_emitter_age()

            input = c2n_utils.create_script_input_for_distribution(
                distribution,
                convert_options
            )
            orbit_script.set_parameter(param_name, input)

        # set the offset.
        set_orbit_param(offset_amount_distribution, offset_options, "Offset")
        # set the rotation.
        set_orbit_param(
            rotation_distribution,
            rotation_options,
            "Rotation"
        )
        # set the rotation rate.
        set_orbit_param(
            rotation_rate_distribution,
            rotation_rate_options,
            "Rotation Rate"
        )

