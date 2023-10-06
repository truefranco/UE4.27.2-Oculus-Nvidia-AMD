from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils
import Paths


class CascadeCylinderLocationConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleLocationPrimitiveCylinder

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # find/add the module script for cylinder location
        cylinder_script = niagara_emitter_context.find_or_add_module_script(
            "CylinderLocation",
            ue.AssetData(Paths.script_location_cylinder),
            ue.ScriptExecutionCategory.PARTICLE_SPAWN)

        # get all properties from the cascade cylinder location module
        (   b_radial_velocity,
            start_radius_distribution, 
            start_height_distribution,
            height_axis,
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
        ) = ue_fx_utils.get_particle_module_location_primitive_cylinder_props(cascade_module)

        # apply cylinder radius.
        # index the height and radius values by emitter age as this is evaluated
        # at spawn.
        options = c2n_utils.DistributionConversionOptions()
        options.set_index_by_emitter_age()
        radius_input = c2n_utils.create_script_input_for_distribution(
            start_radius_distribution, options)
        cylinder_script.set_parameter("Cylinder Radius", radius_input)
        
        # apply cylinder height.
        height_input = c2n_utils.create_script_input_for_distribution(
            start_height_distribution, options)
        cylinder_script.set_parameter("Cylinder Height", height_input)
        
        # set the orientation (height) axis.
        if height_axis == ue.CylinderHeightAxis.PMLPC_HEIGHTAXIS_X:
            axis_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_niagara_orientation_axis, "X Axis")
        elif height_axis == ue.CylinderHeightAxis.PMLPC_HEIGHTAXIS_Y:
            axis_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_niagara_orientation_axis, "Y Axis")
        elif height_axis == ue.CylinderHeightAxis.PMLPC_HEIGHTAXIS_Z:
            axis_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_niagara_orientation_axis, "Z Axis")
        else:
            raise NameError("Failed to get valid height axis from cylinder "
                            "location module!")
        cylinder_script.set_parameter("Orientation Axis", axis_input)
        
        # set the appropriate hemispheres depending on x,y,z bounds.
        b_needs_scale = False
        scale = ue.Vector(1.0, 1.0, 1.0)
        if b_positive_x and not b_negative_x:
            cylinder_script.set_parameter(
                "Hemisphere X", ue_fx_utils.create_script_input_bool(True))
        elif b_negative_x and not b_positive_x:
            cylinder_script.set_parameter(
                "Hemisphere X", ue_fx_utils.create_script_input_bool(True))
            b_needs_scale = True
            scale.x = -1.0
        
        if b_positive_y and not b_negative_y:
            cylinder_script.set_parameter(
                "Hemisphere Y", ue_fx_utils.create_script_input_bool(True))
        elif b_negative_y and not b_positive_y:
            cylinder_script.set_parameter(
                "Hemisphere Y", ue_fx_utils.create_script_input_bool(True))
            b_needs_scale = True
            scale.y = -1.0
            
        # note: positive/negative z are not factored into the cascade cylinder 
        # module.
        
        # if negative hemispheres were required, use the non uniform scale to 
        # flip the basis vector appropriately.
        if b_needs_scale:
            scale_input = ue_fx_utils.create_script_input_vector(scale)
            cylinder_script.set_parameter(
                "Non Uniform Scale",
                scale_input,
                True,
                True
            )
        
        # set surface only emission if required.
        if b_surface_only:
            cylinder_script.set_parameter(
                "Surface Only Band Thickness",
                ue_fx_utils.create_script_input_float(0.0),
                True,
                True)
            cylinder_script.set_parameter(
                "Use Endcaps In Surface Only Mode",
                ue_fx_utils.create_script_input_bool(False),
                True,
                True)
        
        # add velocity along the cylinder if required.
        if b_velocity:
            # add a script to add velocity.
            add_velocity_script = niagara_emitter_context.find_or_add_module_script(
                "AddCylinderVelocity",
                ue.AssetData(Paths.script_add_velocity),
                ue.ScriptExecutionCategory.PARTICLE_SPAWN)
            
            velocity_input = ue_fx_utils.create_script_input_linked_parameter(
                "Output.CylinderLocation.CylinderVector",
                ue.NiagaraScriptInputType.VEC3
            )
            
            # if radial velocity is specified, zero the velocity component on 
            # the cylinder up vector.
            if b_radial_velocity:
                if height_axis == ue.CylinderHeightAxis.PMLPC_HEIGHTAXIS_X:
                    vector_mask_input = ue_fx_utils.create_script_input_vector(
                        ue.Vector(0.0, 1.0, 1.0))
                elif height_axis == ue.CylinderHeightAxis.PMLPC_HEIGHTAXIS_Y:
                    vector_mask_input = ue_fx_utils.create_script_input_vector(
                        ue.Vector(1.0, 0.0, 1.0))
                elif height_axis == ue.CylinderHeightAxis.PMLPC_HEIGHTAXIS_Z:
                    vector_mask_input = ue_fx_utils.create_script_input_vector(
                        ue.Vector(1.0, 1.0, 0.0))
                else:
                    raise NameError("Failed to get valid height axis from cylinder "
                                    "location module!")
                
                # mask the configured component of the velocity.
                multiply_vector_script = ue_fx_utils.create_script_context(
                    ue.AssetData(Paths.di_multiply_vector))
                multiply_vector_script.set_parameter("A", velocity_input)
                multiply_vector_script.set_parameter("B", vector_mask_input)
                velocity_input = ue_fx_utils.create_script_input_dynamic(
                    multiply_vector_script,
                    ue.NiagaraScriptInputType.VEC3
                )
                
            # if there is velocity scaling, apply it.
            if c2n_utils.distribution_always_equals(
                velocity_scale_distribution, 0.0) is False:
                
                # make an input to calculate the velocity scale and index the 
                # scale by the emitter age.
                options = c2n_utils.DistributionConversionOptions()
                emitter_age_index = ue_fx_utils.create_script_input_linked_parameter(
                    "Emitter.LoopedAge",
                    ue.NiagaraScriptInputType.FLOAT
                )
                options.set_custom_indexer(emitter_age_index)
                velocity_scale_input = c2n_utils.create_script_input_for_distribution(
                    velocity_scale_distribution,
                    options
                )
                
                # multiply the velocity by the scale.
                multiply_vector_by_float_script = ue_fx_utils.create_script_context(
                    ue.AssetData(Paths.di_multiply_vector_by_float))
                multiply_vector_by_float_script.set_parameter("Vector", velocity_input)
                multiply_vector_by_float_script.set_parameter("Float", velocity_scale_input)
                velocity_input = ue_fx_utils.create_script_input_dynamic(
                    multiply_vector_by_float_script,
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
            cylinder_script.set_parameter("Offset", start_location_input)