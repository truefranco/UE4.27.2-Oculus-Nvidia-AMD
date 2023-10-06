from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils


class CascadeLightConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleLight

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties from the cascade light module
        (   b_use_inverse_squared_falloff,
            b_affects_translucency, 
            b_preview_light_radius,
            spawn_fraction, 
            color_scale_over_life_distribution,
            brightness_over_life_distribution,
            radius_scale_distribution,
            light_exponent_distribution,
            lighting_channels,
            volumetric_scattering_intensity, 
            b_high_quality_lights,
            b_shadow_casting_lights
        ) = ue_fx_utils.get_particle_module_light_props(cascade_module)
        
        def convert_lq_light(niagara_emitter_context):
            pass
            # add a light renderer to the emitter.
            light_renderer_props = ue.NiagaraLightRendererProperties()
            
            # set the inverse squared falloff property.
            light_renderer_props.set_editor_property(
                "bUseInverseSquaredFalloff", b_use_inverse_squared_falloff)
            
            # if necessary, set the spawn fraction amount.
            spawn_message_verbose = True
            if spawn_fraction != 1.0:
                spawn_message_verbose = False
            niagara_emitter_context.log(
                "Cascade light specified a spawn fraction but this mode is not "
                "supported by the asset converter.",
                ue.NiagaraMessageSeverity.WARNING,
                spawn_message_verbose
            )   
            
            # if inverse squared falloff is not used, apply the light exponent.
            if b_use_inverse_squared_falloff is False:
                if (c2n_utils.distribution_always_equals(
                        light_exponent_distribution, 1.0) is False
                ):
                    light_exponent_input = \
                        c2n_utils.create_script_input_for_distribution(
                            light_exponent_distribution)
                    niagara_emitter_context.set_parameter_directly(
                        "Particles.LightExponent",
                        light_exponent_input,
                        ue.ScriptExecutionCategory.PARTICLE_UPDATE
                    )
            
            # set whether to affect translucency.
            light_renderer_props.set_editor_property(
                "bAffectsTranslucency", b_affects_translucency)
            
            # set the light color
            light_color_input = c2n_utils.create_script_input_for_distribution(
                color_scale_over_life_distribution)
            niagara_emitter_context.set_parameter_directly(
                "Particles.LightColor",
                light_color_input,
                ue.ScriptExecutionCategory.PARTICLE_UPDATE
            )
            # explicitly set the binding for light color as it defaults to 
            # particles.color.
            niagara_emitter_context.set_renderer_binding(
                light_renderer_props,
                "ColorBinding",
                "Particles.LightColor",
                ue.NiagaraRendererSourceDataMode.PARTICLES
            )
            
            
            # set the light radius.
            light_radius_input = c2n_utils.create_script_input_for_distribution(
                radius_scale_distribution)
            niagara_emitter_context.set_parameter_directly(
                "Particles.LightRadius",
                light_radius_input,
                ue.ScriptExecutionCategory.PARTICLE_UPDATE
            )
            
            # add the light renderer.
            niagara_emitter_context.add_renderer(
                "LightRenderer", light_renderer_props)

        def convert_hq_light(niagara_emitter_context):
            # add a component renderer to the emitter.
            component_renderer_props = ue.NiagaraComponentRendererProperties()

            # set the component.
            component_renderer_props.set_editor_property(
                "ComponentType", ue.PointLightComponent)

            # if necessary, set the spawn fraction amount.
            spawn_message_verbose = True
            if spawn_fraction != 1.0:
                spawn_message_verbose = False
            niagara_emitter_context.log(
                "Cascade light specified a spawn fraction but this mode is not "
                "supported by the asset converter.",
                ue.NiagaraMessageSeverity.WARNING,
                spawn_message_verbose
            )

            # if inverse squared falloff is not used, apply the light exponent.
            if b_use_inverse_squared_falloff is False:
                if (c2n_utils.distribution_always_equals(
                    light_exponent_distribution, 1.0) is False
                ):
                    light_exponent_input = \
                        c2n_utils.create_script_input_for_distribution(
                            light_exponent_distribution)
                    niagara_emitter_context.set_parameter_directly(
                        "Particles.LightExponent",
                        light_exponent_input,
                        ue.ScriptExecutionCategory.PARTICLE_UPDATE
                    )

            # set whether to affect translucency.
            component_renderer_props.set_editor_property(
                "bAffectsTranslucency", b_affects_translucency)

            # set the light color
            light_color_input = c2n_utils.create_script_input_for_distribution(
                color_scale_over_life_distribution)
            niagara_emitter_context.set_parameter_directly(
                "Particles.LightColor",
                light_color_input,
                ue.ScriptExecutionCategory.PARTICLE_UPDATE
            )
            # explicitly set the binding for light color as it defaults to 
            # particles.color.
            niagara_emitter_context.set_renderer_binding(
                component_renderer_props,
                "ColorBinding",
                "Particles.LightColor",
                ue.NiagaraRendererSourceDataMode.PARTICLES
            )

            # set the light radius.
            light_radius_input = c2n_utils.create_script_input_for_distribution(
                radius_scale_distribution)
            niagara_emitter_context.set_parameter_directly(
                "Particles.LightRadius",
                light_radius_input,
                ue.ScriptExecutionCategory.PARTICLE_UPDATE
            )

            # set the light brightness.
            #  todo implement light brightness
            
            # set the light shadowcasting flag.
            #  todo implement light shadowcasting flag
            
            # set the lighting channels.
            #  todo implement lighting channels.
            
            # add the light component renderer.
            niagara_emitter_context.add_renderer(
                "LightComponentRenderer", component_renderer_props)
        
        if b_high_quality_lights:
            convert_hq_light(niagara_emitter_context)
        else:
            convert_lq_light(niagara_emitter_context)
