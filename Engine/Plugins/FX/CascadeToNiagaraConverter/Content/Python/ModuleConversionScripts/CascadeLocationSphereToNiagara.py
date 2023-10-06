from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeSphereLocationConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleLocationPrimitiveSphere

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # make the new module script for sphere location
        sphere_location_script = niagara_emitter_context.find_or_add_module_script(
            "SphereLocation",
            ue.AssetData(Paths.script_location_sphere),
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)

        # get all properties from the cascade sphere location module
        (   radius,
            b_positive_x,
            b_positive_y,
            b_positive_z,
            b_negative_x, 
            b_negative_y,
            b_negative_z,
            b_surface_only,
            b_velocity, 
            velocity_scale_distribution,
            start_location_distribution
        ) = ue_fx_utils.get_particle_module_location_primitive_sphere_props(cascade_module)
        

        # make an input for radius
        radius_input = (
            c2n_utils.create_script_input_for_distribution(radius))

        # set the radius
        sphere_location_script.set_parameter("Sphere Radius", radius_input)
        
        # set the sphere hemispheres if required.
        b_needs_scale = False
        scale = ue.Vector(1.0, 1.0, 1.0)
        
        if b_positive_x and not b_negative_x:
            sphere_location_script.set_parameter(
                "Hemisphere X",
                ue_fx_utils.create_script_input_bool(True)
            )
        elif b_negative_x and not b_positive_x:
            sphere_location_script.set_parameter(
                "Hemisphere X",
                ue_fx_utils.create_script_input_bool(True)
            )
            b_needs_scale = True
            scale.x = -1.0
        elif not b_positive_x and not b_negative_x:
            b_needs_scale = True
            scale.x = 0.0
            
        if b_positive_y and not b_negative_y:
            sphere_location_script.set_parameter(
                "Hemisphere Y",
                ue_fx_utils.create_script_input_bool(True)
            )
        elif b_negative_y and not b_positive_y:
            sphere_location_script.set_parameter(
                "Hemisphere Y",
                ue_fx_utils.create_script_input_bool(True)
            )
            b_needs_scale = True
            scale.y = -1.0
        elif not b_positive_y and not b_negative_y:
            b_needs_scale = True
            scale.y = 0.0
            
        if b_positive_z and not b_negative_z:
            sphere_location_script.set_parameter(
                "Hemisphere Z",
                ue_fx_utils.create_script_input_bool(True)
            )
        elif b_negative_z and not b_positive_z:
            sphere_location_script.set_parameter(
                "Hemisphere Z",
                ue_fx_utils.create_script_input_bool(True)
            )
            b_needs_scale = True
            scale.z = -1.0
        elif not b_positive_z and not b_negative_z:
            b_needs_scale = True
            scale.z = 0.0
            
        # if non uniform scale is required due to negative hemispheres, apply
        # it.
        if b_needs_scale:
            sphere_location_script.set_parameter(
                "Non Uniform Scale",
                ue_fx_utils.create_script_input_vector(scale),
                True,
                True
            )

        # set surface only emission if required.
        if b_surface_only:
            sphere_location_script.set_parameter(
                "Surface Only Band Thickness",
                ue_fx_utils.create_script_input_float(0.0),
                True,
                True
            )

        # add velocity along the sphere if required.
        if b_velocity:
            # add a script to add velocity.
            add_velocity_script = niagara_emitter_context.find_or_add_module_script(
                "AddSphereVelocity",
                ue.AssetData(Paths.script_add_velocity),
                ue.ScriptExecutionCategory.PARTICLE_SPAWN)

            velocity_input = ue_fx_utils.create_script_input_linked_parameter(
                "Output.SphereLocation.SphereVector",
                ue.NiagaraScriptInputType.VEC3
            )

            # if there is velocity scaling, apply it.
            if c2n_utils.distribution_always_equals(
                velocity_scale_distribution,
                0.0
            ) is False:
                # make an input to calculate the velocity scale and index the 
                # scale by the emitter age.
                options = c2n_utils.DistributionConversionOptions()
                options.set_index_by_emitter_age()
                velocity_scale_input = c2n_utils.create_script_input_for_distribution(
                    velocity_scale_distribution,
                    options
                )

                # multiply the velocity by the scale.
                multiply_vector_script = ue_fx_utils.create_script_context(
                    ue.AssetData(Paths.di_multiply_vector_by_float))
                multiply_vector_script.set_parameter("Vector", velocity_input)
                multiply_vector_script.set_parameter("Float", velocity_scale_input)
                velocity_input = ue_fx_utils.create_script_input_dynamic(
                    multiply_vector_script,
                    ue.NiagaraScriptInputType.VEC3
                )

            # apply the velocity.
            add_velocity_script.set_parameter("Velocity", velocity_input)

            # make sure we have a solve forces and velocity script.
            solve_forces_velocity_script_asset_data = ue.AssetData(
                Paths.script_solve_forces_and_velocity)
            niagara_emitter_context.find_or_add_module_script(
                "SolveForcesAndVelocity",
                solve_forces_velocity_script_asset_data,
                ue.ScriptExecutionCategory.PARTICLE_UPDATE)

        # offset the location if required.
        if c2n_utils.distribution_always_equals(
            start_location_distribution, 0.0) is False:
            # create an input to set the offset and index the value by emitter
            # age.
            options = c2n_utils.DistributionConversionOptions()
            emitter_age_index = ue_fx_utils.create_script_input_linked_parameter(
                "Emitter.LoopedAge",
                ue.NiagaraScriptInputType.FLOAT
            )
            options.set_custom_indexer(emitter_age_index)
            start_location_input = c2n_utils.create_script_input_for_distribution(
                start_location_distribution,
                options
            )

            # set the start location.
            sphere_location_script.set_parameter(
                "Offset",
                start_location_input
            )
