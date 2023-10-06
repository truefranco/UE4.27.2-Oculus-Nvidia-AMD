from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeTrailSourceConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleTrailSource

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties from the cascade trail source module.
        (   source_method,
            source_name,
            source_strength_distribution,
            b_lock_source_strength,
            source_offset_count,
            source_offset_defaults,
            selection_method,
            b_inherit_rotation
        ) = ue_fx_utils.get_particle_module_trail_source_props(cascade_module)
        
        # note; b_lock_source_strength goes unused by default cascade modules.
        
        if source_method == ue.Trail2SourceMethod.PET2SRCM_DEFAULT:
            pass
        elif source_method == ue.Trail2SourceMethod.PET2SRCM_PARTICLE:
            pass
        elif source_method == ue.Trail2SourceMethod.PET2SRCM_ACTOR:
            pass
        else:
            raise NameError("Encountered unexpected value for "
                            "ETrail2SourceMethod!")
        
        if selection_method == ue.ParticleSourceSelectionMethod.EPSSM_RANDOM:
            pass
        elif selection_method == ue.ParticleSourceSelectionMethod.EPSSM_SEQUENTIAL:
            pass
        else:
            raise NameError("Encountered unexpected value for "
                            "EParticleSourceSelectionMethod!")
        
        # create an event handler add action to add the trail event to the 
        # emitter.
        # set the mode for the add action to "add event and event generator" as
        # the converter will also specify the event generator script to put in
        # the emitter this event points at by the source emitter name.
        event_props = ue.NiagaraEventHandlerAddAction()
        event_props.mode = ue.NiagaraEventHandlerAddMode.ADD_EVENT_AND_EVENT_GENERATOR
        
        # set the properties of the event handler.
        event_props.execution_mode = ue.ScriptExecutionMode.SPAWNED_PARTICLES
        event_props.spawn_number = 1
        event_props.max_events_per_frame = 0
        event_props.source_event_name = "LocationEvent"
        event_props.random_spawn_number = False
        event_props.min_spawn_number = 0
        
        # set the properties of the event generator
        event_generator_props = ue.NiagaraAddEventGeneratorOptions()
        event_generator_props.source_emitter_name = source_name
        event_generator_props.event_generator_script_asset_data = (
            ue.AssetData(Paths.script_generate_location_event)
        )
        
        # add the event add action; this will add an event handler to this 
        # emitter and an event generator to the emitter pointed at by the source
        # emitter name of the event add action's event generator options.
        event_props.add_event_generator_options = event_generator_props
        niagara_emitter_context.add_event_handler(event_props)
        
        # add the event receiver to this emitter.
        receive_location_script = (
            niagara_emitter_context.find_or_add_module_event_script(
                "ReceiveSpawnRibbonEvent",
                ue.AssetData(Paths.script_receive_location_event),
                event_props
            )
        )

        # todo: implement source offset count handling
        b_message_verbose = True
        if source_offset_count != 0:
            b_message_verbose = False
        receive_location_script.log("Skipped converting source_offset_counts "
                                    "from cascade trail source module; "
                                    "converter does not support this mode.",
                                    ue.NiagaraMessageSeverity.WARNING,
                                    b_message_verbose)
        
        if c2n_utils.distribution_always_equals(source_strength_distribution, 1.0) is False: 
            source_strength_input = c2n_utils.create_script_input_for_distribution(
                source_strength_distribution)
            receive_location_script.set_parameter(
                "Inherited Velocity Scale",
                source_strength_input,
                True,
                True)
