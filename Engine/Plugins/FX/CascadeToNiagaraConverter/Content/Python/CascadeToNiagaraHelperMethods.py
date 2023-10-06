"""
Helper methods for converting cascade types to niagara types.
"""
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import Paths
import copy


class DistributionConversionOptions:
    """
    Options to be used with create_script_input_for_distribution methods.
    """
    def __init__(self):
        self.__target_type_width = None
        self.__custom_indexer = None
        self.__target_vector_component = None
        self.__b_evaluate_locked_axes = True
        self.__b_optimize_curve_evaluation = True
        self.__specific_curve_key_indices_list = []
        self.__b_evaluate_spawn_only = False
        
    def set_target_type_width(self, type_width):
        """
        Sets the target script input type to make the converted cascade
            distribution obey by trimming or adding zeroed channels. Supports
            float, vec2 and vec3.
        
        Args:
            type_width (ue.NiagaraScriptInputType): The optional target script
                input type to make the incoming cascade distribution obey by 
                trimming or adding zeroed channels. Supports float, vec2 and 
                vec3.
        """
        if isinstance(type_width, ue.NiagaraScriptInputType) is False:
            raise NameError("Tried to set target_type_width that was not a "
                            "ue.NiagaraScriptInputType!")
        elif(
            type_width == ue.NiagaraScriptInputType.FLOAT or
            type_width == ue.NiagaraScriptInputType.VEC2 or
            type_width == ue.NiagaraScriptInputType.VEC3
        ) is False:
            raise NameError("Tried to set target_type_width that was not "
                            "ue.NiagaraScriptInputType of FLOAT, VEC2 or VEC3!")
        self.__target_type_width = type_width
        
    def get_target_type_width(self):
        """
        Get the target type width of the distribution to convert.

        Returns:
            target_type_width (NiagaraScriptInputType): Script input type 
                representing the number of channels of the converted 
                distribution. Guaranteed to be float, vec2 or vec3.
        """
        return self.__target_type_width
    
    def set_custom_indexer(self, indexer):
        """
        Sets the custom indexer to evaluate a curve, lerp or other value range
            by. By default the custom indexer is None, and the converter will
            default the indexing of value ranges by the particle normalized age.

        Args:
            indexer (ue.NiagaraScriptConversionContextInput): The input to drive
                converted curve or lerp script range evaluation by. The indexer
                must be of type float.
        """
        
        # allow using None to clear the custom indexer.
        if indexer is None:
            self.__custom_indexer = indexer
            return
        
        # otherwise ensure the custom indexer is a 
        # NiagaraScriptConversionContextInput with NiagaraScriptInputType float.
        elif isinstance(indexer, ue.NiagaraScriptConversionContextInput):
            if indexer.input_type == ue.NiagaraScriptInputType.FLOAT:
                self.__custom_indexer = indexer
                return
            else:
                raise NameError("Tried to set custom_indexer that was not of "
                                "type FLOAT!")
        else:
            raise NameError("Tried to set custom_indexer that was not a "
                            "ue.NiagaraScriptConversionContextInput or None!")
    
    def get_custom_indexer(self):
        """
        Get the custom indexer of the distribution to convert.

        Returns:
            custom_indexer (NiagaraScriptConversionContextInput): Conversion 
                context input of type float that specifies how to index a curve
                if the converted distribution is a curve type.
        """
        return self.__custom_indexer
    
    def set_index_by_emitter_age(self):
        """
        Set the custom indexer of the distribution to convert to use 
            Emitter.LoopedAge.
        """
        self.set_custom_indexer(
            ue_fx_utils.create_script_input_linked_parameter(
                "Emitter.LoopedAge",
                ue.NiagaraScriptInputType.FLOAT
            )
        )
    
    def set_target_vector_component(self, vector_component):
        """
        Sets the target vector component to convert from a cascade distribution
            of type vector. By default the target vector component is None; in
            this case conversion of a cascade vector distribution will create a 
            niagara script input that is a also a vector. If the target vector 
            component is set to a value "x", "y" or "z" then conversion of a 
            cascade vector distribution will create a niagara script input of 
            type float which is just the specified component of the vector.

        Args:
            vector_component (str): The target component of the cascade vector
                distribution to single out during conversion.
        """
        if (
            vector_component == "x" or
            vector_component == "y" or
            vector_component == "z"
        ) is False:
            raise NameError("Tried to set target_vector_component that was not "
                            "\"x\", \"y\", or \"z\"!")
        self.__target_vector_component = vector_component
        
    def get_target_vector_component(self):
        """
        Get the target vector component of the distribution to convert.

        Returns:
            target_vector_component (str): String specifying the component of a 
                vector distribution to get the value of and to ignore the other
                components. Guaranteed to be "x", "y" or "z".
        """
        return self.__target_vector_component
    
    def set_b_evaluate_locked_axes(self, b_evaluate):
        """
        Sets whether to evaluate the locked axes of the distribution to convert 
            (if it has any). For example, if axes XY are locked, then the 
            distribution converter will get the x component of the source
            distribution, and use it to drive the x and y components of the
            returned NiagaraScriptConversionContextInput. 

        Args:
            b_evaluate (bool): Whether to evaluate locked axes of the 
                distribution to convert.
            
        Notes:
            The create_script_input_for_distribution method 
                uses this flag to check for reentrancy when converting; For 
                example if create_script_input_for_distribution is 
                called with b_evaluate_locked_axes set to True and the 
                distribution has the xy axes locked, the method will recursively
                call create_script_input_for_distribution with 
                DistributionConversionOptions with b_evaluate_locked_axes set
                to False. This is to allow converting the same distribution 
                twice in order to get just the values of two axes (in this 
                specific example, the x and z axes), and combine the two 
                generated NiagaraScriptConversionContextInputs of type float 
                into a composite NiagaraScriptConversionContextInput of type
                vector.
        """
        self.__b_evaluate_locked_axes = b_evaluate
        
    def get_b_evaluate_locked_axes(self):
        """
        Get whether to evaluate locked axes of the distribution to convert.

        Returns:
            b_evaluate_locked_axes (bool): Bool specifying whether the 
                conversion should obey the locked axes flags of the source 
                distribution (if it has any).
        
        Notes:
            The b_evaluate_locked_axes flag is ignored if the distribution 
                conversion options specify a target vector component.
        """
        return self.__b_evaluate_locked_axes

    def set_b_optimize_curve_evaluation(self, b_optimize):
        """
        Sets the flag for whether to allow optimizing converted curves by 
            changing the generated NiagaraScriptConversionContextInput from a 
            curve evaluation to an equivalent analytical equation.

        Args:
            b_optimize (bool): Whether to allow optimizing curve evaluation to 
                analytical evaluation.
        """
        self.__b_optimize_curve_evaluation = b_optimize

    def get_b_optimize_curve_evaluation(self):
        """
        Gets the flag for whether to allow optimizing converted curves by 
            changing the generated NiagaraScriptConversionContextInput from a 
            curve evaluation to an equivalent analytical equation.

        Returns:
            b_optimize_curve_evaluation (bool)
        """
        return self.__b_optimize_curve_evaluation

    def set_specific_curve_key_indices_list(self, key_indices_list):
        """
        Sets a list of vectors representing key indices to specifically choose 
            when converting a curve distribution. For example, if 
            specific_curve_key_indices_list  is set to [(1, 2, 2)] and the curve
            distribution is of type const vector, the generated 
            NiagaraScriptConversionContextInput will be a vector curve with the 
            components swizzled to y, z, z instead of x, y, z.

        Args:
            key_indices_list (list): List of vectors representing component 
                swizzle of curves to convert.

        Notes:
            For multi-vector distributions, i.e. uniform vector curve, 
                specific_curve_key_indices_list may have an entry for each 
                vector.
                For example, 
                set_specific_curve_key_indices_list([0, 0, 2], [1, 2, 2])
                is valid for uniform vector curve distributions.
        """
        self.__specific_curve_key_indices_list = key_indices_list

    def get_specific_curve_key_indices_list(self):
        """
        Gets a list of vectors representing key indices to specifically choose 
            when converting a curve distribution.

        Returns:
            specific_curve_key_indices_list
        """
        return self.__specific_curve_key_indices_list

    def set_b_evaluate_spawn_only(self, b_spawn_only):
        self.__b_evaluate_spawn_only = b_spawn_only

    def get_b_evaluate_spawn_only(self):
        return self.__b_evaluate_spawn_only

    def report_status(self):
        """
        Logs the status of this distribution to the ue4 output log.
        """
        ue.log("Distribution conversion options status for: " + str(self))
        ue.log("\t distribution target type width: " + str(self.__target_type_width))
        ue.log("\t distribution custom indexer: " + str(self.__custom_indexer))
        ue.log("\t distribution target vector component: " + str(self.__target_vector_component))
        ue.log("\t distribution evaluate locked axes: " + str(self.__b_evaluate_locked_axes))
        ue.log("\t distribution optimize curve evaluation: " + str(self.__b_optimize_curve_evaluation))
        ue.log("\t distribution specific indices: " + str(self.__specific_curve_key_indices_list))


def create_script_input_for_distribution(
    distribution,
    in_options=DistributionConversionOptions()
):
    """
    Create a niagara script input with the same behavior as the input cascade
    rate distribution.

    Args:
        distribution (UDistribution): The distribution to convert
            to a UNiagaraClipboardFunctionInput script_input and an optional
            UNiagaraScriptConversionContext dynamic input script. Depending on the
            UDistribution type, the returned dynamic input script may be None, for
            example a UDistributionFloatConst will not generate a dynamic input script
            as the generated script_input does not require a dynamic input script to
            drive a constant value in a niagara script.
        in_options (DistributionConversionOptions): The options for how to 
            perform the conversion of the distribution option. Refer to the 
            docstring for DistributionConversionOptions for behavior.

    Returns:
        script_input (UNiagaraScriptConversionContextInput): script conversion
            context input with same behavior as the input 
            cascade_rate_distribution.

    Raises:
        NameError: If an unknown UDistribution type is encountered.

    """
    
    # state of options is mutated in this method and the changes in this scope 
    # should not return; make a copy of the options.
    options = copy.copy(in_options)

    def __convert_distribution_locked_axes(distribution, options, locked_axes):
        options_2component = DistributionConversionOptions()
        options_2component.set_target_type_width(
            ue.NiagaraScriptInputType.FLOAT)
        options_2component.set_b_evaluate_locked_axes(False)
        options_2component.set_custom_indexer(options.get_custom_indexer())
        options_1component = DistributionConversionOptions()
        options_1component.set_target_type_width(
            ue.NiagaraScriptInputType.FLOAT)
        options_1component.set_b_evaluate_locked_axes(False)
        options_1component.set_custom_indexer(options.get_custom_indexer())

        split_vec_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_split_vector)
        )

        if locked_axes == ue.DistributionVectorLockFlags.EDVLF_XY:
            options_2component.set_target_vector_component("x")
            options_1component.set_target_vector_component("z")
            split_vec_switch_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_cascade_niagara_two_vec_channels,
                "XY"
            )
        elif locked_axes == ue.DistributionVectorLockFlags.EDVLF_YZ:
            options_2component.set_target_vector_component("y")
            options_1component.set_target_vector_component("x")
            split_vec_switch_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_cascade_niagara_two_vec_channels,
                "YZ"
            )
        elif locked_axes == ue.DistributionVectorLockFlags.EDVLF_XZ:
            options_2component.set_target_vector_component("x")
            options_1component.set_target_vector_component("y")
            split_vec_switch_input = ue_fx_utils.create_script_input_enum(
                Paths.enum_cascade_niagara_two_vec_channels,
                "XZ"
            )
        else:
            raise NameError("Locked axes were not xy, yz or xz!")

        input_2component = create_script_input_for_distribution(
            distribution, options_2component
        )
        input_1component = create_script_input_for_distribution(
            distribution, options_1component
        )
        split_vec_script.set_parameter(
            "Double Component Vector Channels",
            split_vec_switch_input
        )
        split_vec_script.set_parameter(
            "Double Component",
            input_2component)
        split_vec_script.set_parameter(
            "Single Component",
            input_1component)
        return ue_fx_utils.create_script_input_dynamic(
            split_vec_script,
            ue.NiagaraScriptInputType.VEC3
        )
    
    # begin conversion by getting the distribution's type and value type.
    distribution_type, distribution_value_type = (
        ue_fx_utils.get_distribution_type(distribution))
    
    # special case handling. If there are locked axes then the distribution 
    # potentially needs to be converted more than once to handle where 2 axes
    # are diverging from one distribution value.
    # skip evaluating locked axes if there is a target vector component; the
    # locked axes do not matter in this situation.
    if (
        distribution_value_type == ue.DistributionValueType.VECTOR and
        options.get_target_vector_component() is None and 
        options.get_b_evaluate_locked_axes()
    ):
        locked_axes_list = ue_fx_utils.get_distribution_locked_axes(distribution)
        if len(locked_axes_list) == 0:
            pass
        elif len(locked_axes_list) == 1:
            locked_axes_a = locked_axes_list[0]
            if locked_axes_a == ue.DistributionVectorLockFlags.EDVLF_NONE:
                pass
            elif (
                locked_axes_a == ue.DistributionVectorLockFlags.EDVLF_XY or
                locked_axes_a == ue.DistributionVectorLockFlags.EDVLF_YZ or
                locked_axes_a == ue.DistributionVectorLockFlags.EDVLF_XZ
            ):
                return __convert_distribution_locked_axes(
                    distribution,
                    options,
                    locked_axes_a
                )
            elif locked_axes_a == ue.DistributionVectorLockFlags.EDVLF_XYZ:
                if options.get_target_type_width() is None:
                    options.set_target_type_width(ue.NiagaraScriptInputType.VEC3)
                options.set_target_vector_component("x")
            else:
                raise NameError("Encountered unknown lock axis flag!")
        
        elif len(locked_axes_list) == 2:
            locked_axes_a = locked_axes_list[0]
            locked_axes_b = locked_axes_list[1]
            
            def get_key_indices_for_locked_axes(locked_axes):
                """
                helper to get the vector swizzle indices for a value of
                    DistributionVectorLockFlags enum.
                Args:
                    locked_axes (DistributionVectorLockFlags)
                Returns:
                    key_indices (vector): vector of indices 0-2 representing 
                        a swizzle of a vector's components.
                """
                if locked_axes == ue.DistributionVectorLockFlags.EDVLF_NONE:
                    return [0, 1, 2]
                elif locked_axes == ue.DistributionVectorLockFlags.EDVLF_XY:
                    return [0, 0, 2]
                elif locked_axes == ue.DistributionVectorLockFlags.EDVLF_YZ:
                    return [0, 1, 1]
                elif locked_axes == ue.DistributionVectorLockFlags.EDVLF_XZ:
                    return [0, 1, 0]
                elif locked_axes == ue.DistributionVectorLockFlags.EDVLF_XYZ:
                    return [0, 0, 0]
                else:
                    raise NameError("Locked axes were not xy, yz, xz, or xyz!")
            
            if locked_axes_a == locked_axes_b:
                # if the locked axes are the same for both vectors then convert
                # as normal.
                return __convert_distribution_locked_axes(
                    distribution,
                    options,
                    locked_axes_a
                )
            else:
                key_indices_list = []
                key_indices_list.append(
                    get_key_indices_for_locked_axes(locked_axes_a)
                )
                key_indices_list.append(
                    get_key_indices_for_locked_axes(locked_axes_b)
                )
                options.set_specific_curve_key_indices_list(key_indices_list)
    
        else:
            raise NameError("Locked axes list len was greater than 2 but there "
                            "is no supported type of distribution with more "
                            "than two vector components!")

    # if there are no locked axes, proceed to convert based on distribution type
    # and distribution value type.
    if distribution_type == ue.DistributionType.CONST:
        if distribution_value_type == ue.DistributionValueType.FLOAT:
            value = ue_fx_utils.get_float_distribution_const_values(
                distribution)
            return create_script_input_constant(value, options)

        elif distribution_value_type == ue.DistributionValueType.VECTOR:
            value = ue_fx_utils.get_vector_distribution_const_values(
                distribution)
            return create_script_input_constant(value, options)

    elif distribution_type == ue.DistributionType.UNIFORM:
        if distribution_value_type == ue.DistributionValueType.FLOAT:
            value_min, value_max = (
                ue_fx_utils.get_float_distribution_uniform_values(distribution))
            return create_script_input_random_range(
                value_min, value_max, options)
        
        elif distribution_value_type == ue.DistributionValueType.VECTOR:
            value_min, value_max = (
                ue_fx_utils.get_vector_distribution_uniform_values(distribution))
            return create_script_input_random_range(
                value_min, value_max, options)

    elif (
        distribution_type == ue.DistributionType.CONST_CURVE or
        distribution_type == ue.DistributionType.UNIFORM_CURVE
    ):
        return convert_cascade_curve_distribution(
            distribution,
            options
        )

    elif distribution_type == ue.DistributionType.PARAMETER:
        if distribution_value_type == ue.DistributionValueType.FLOAT:
            script_input_type = ue.NiagaraScriptInputType.FLOAT
        elif distribution_value_type == ue.DistributionValueType.VECTOR:
            script_input_type = ue.NiagaraScriptInputType.VEC3
        else:
            raise NameError("Encountered distribution value type that was not "
                            "float or vector!")
            
        name, min_input, max_input, min_output, max_output = (
            ue_fx_utils.get_float_distribution_parameter_values(distribution))
        return ue_fx_utils.create_script_input_linked_parameter(
            name,
            script_input_type
        )


    else:
        raise NameError("Unknown distribution type encountered!")
    raise NameError("Failed to handle distribution type!")


def convert_cascade_curve_distribution(
    curve_distribution,
    options,
):
    """
    Create a niagara script input and an optional script with the same behavior
    as the input cascade curve.

    Args:
        curve_distribution (ue.UDistribution): The curve distribution to convert
            to a UNiagaraClipboardFunctionInput script_input and an optional
            UNiagaraScriptConversionContext dynamic input script.
       options (DistributionConversionOptions): The options for how to perform
            the conversion of the cascade curve. Refer to the docstring for
            CurveDistributionConversionOptions for behavior.

    Returns:
        script_input (UNiagaraScriptConversionContextInput): script conversion
            context input with same behavior as the input curve.

    Raises:
        NameError: If an unknown child of UNiagaraDataInterfaceCurveBase is
            encountered.

    Notes:
    If the options argument has b_optimize_curve_evaluation set True and 
        the input cascade_curve has less than three points and meets certain 
        optimization criteria, the output UNiagaraScriptConversionContextInput 
        may be simplified from evaluating a curve directly to an analytical
        solution.

    """
    
    def make_curve_di(cascade_curve, keys_from_curve_getter, key_indices):
        numkeys = len(key_indices)
        getkeys = lambda key_index : keys_from_curve_getter(cascade_curve, key_index)
        
        if numkeys == 1:
            return ue_fx_utils.create_float_curve_di(getkeys(key_indices[0]))
        elif numkeys == 2:
            return ue_fx_utils.create_vec2_curve_di(
                getkeys(key_indices[0]),
                getkeys(key_indices[1])
            )
        elif numkeys == 3:
            return ue_fx_utils.create_vec3_curve_di(
                getkeys(key_indices[0]),
                getkeys(key_indices[1]),
                getkeys(key_indices[2])
            )
        else:
            raise NameError("Unhandled number of key components encountered!")

    distribution_type, distribution_value_type = (
        ue_fx_utils.get_distribution_type(curve_distribution))
    b_optimize = options.get_b_optimize_curve_evaluation()
    
    if distribution_type == ue.DistributionType.CONST_CURVE:
        if distribution_value_type == ue.DistributionValueType.FLOAT:
            cascade_curve = ue_fx_utils.get_float_distribution_const_curve_values(
                curve_distribution)
        elif distribution_value_type == ue.DistributionValueType.VECTOR:
            cascade_curve = ue_fx_utils.get_vector_distribution_const_curve_values(
                curve_distribution)
        else:
            raise NameError("Unhandled distribution value type encountered!")
    elif distribution_type == ue.DistributionType.UNIFORM_CURVE:
        if distribution_value_type == ue.DistributionValueType.FLOAT:
            cascade_curve = ue_fx_utils.get_float_distribution_uniform_curve_values(
                curve_distribution)
        elif distribution_value_type == ue.DistributionValueType.VECTOR:
            cascade_curve = ue_fx_utils.get_vector_distribution_uniform_curve_values(
                curve_distribution)
        else:
            raise NameError("Unhandled distribution value type encountered!")
    else:
        raise NameError("Curve distribution argument was not a const curve or "
                        "uniform curve!")

    points = cascade_curve.get_editor_property("Points")
    numpoints = len(points)
    if numpoints == 0:
        # no points, put in a zero value.
        if distribution_value_type == ue.DistributionValueType.FLOAT:
            value = 0.0
        else:  # distribution_value_type == ue.DistributionValueType.VECTOR:
            value = ue.Vector(0.0, 0.0, 0.0)
        return create_script_input_constant(value, options)
            
    elif numpoints == 1 and b_optimize:
        # only one point, just express as a const.
        value = points[0].get_editor_property("OutVal")
        return create_script_input_constant(value, options)
        
    elif (
        #  todo re-enable this path
        False and
        b_optimize and
        numpoints == 2 and
        points[0].get_editor_property("InterpMode") == ue.InterpCurveMode.CIM_LINEAR and
        points[1].get_editor_property("InterpMode") == ue.InterpCurveMode.CIM_LINEAR and
        points[0].get_editor_property("InVal") == 0.0 and
        points[1].get_editor_property("InVal") == 1.0
    ):
        # only two points with linear interp, express as a lerp.
        value1 = points[0].get_editor_property("OutVal")
        value2 = points[1].get_editor_property("OutVal")
        
        if distribution_type == ue.DistributionType.CONST_CURVE:
            return create_script_input_lerp(value1, value2, options)
        
        elif distribution_type == ue.DistributionType.UNIFORM_CURVE:
            target_type_width = options.get_target_type_width()
            custom_indexer = options.get_custom_indexer()
            target_vector_component = options.get_target_vector_component()
            
            if (
                distribution_value_type == ue.DistributionValueType.FLOAT and
                isinstance(value1, ue.Vector2D) and
                isinstance(value2, ue.Vector2D)
            ):
                lerp_input_x, lerp_script_x = __create_script_input_lerp(
                    value1.x, value2.x, None, custom_indexer, None)
                lerp_input_y, lerp_script_y = __create_script_input_lerp(
                    value1.y, value2.y, None, custom_indexer, None)
                return create_script_input_random_range(
                    lerp_input_x,
                    lerp_input_y,
                    options
                )

            elif (
                distribution_value_type == ue.DistributionValueType.VECTOR and
                isinstance(value1, ue.TwoVectors) and
                isinstance(value2, ue.TwoVectors)
            ):
                lerp_input_v1, lerp_script_v1 = __create_script_input_lerp(
                    value1.v1,
                    value2.v1,
                    None,
                    custom_indexer,
                    target_vector_component
                )
                lerp_input_v2, lerp_script_v2 = __create_script_input_lerp(
                    value1.v2,
                    value2.v2,
                    None,
                    custom_indexer,
                    target_vector_component
                )
                return create_script_input_random_range(
                    lerp_input_v1,
                    lerp_input_v2,
                    options
                )

    else:
        # converting a full curve, make a curve DI and set the rich curve on it
        # from the interp curve of the distribution
        target_type_width = options.get_target_type_width()
        custom_indexer = options.get_custom_indexer()
        target_vector_component = options.get_target_vector_component()

        # if specific key indices have been set, set the swizzle for the key 
        # indices so that the curves are correctly generated.
        specific_key_indices_list = options.get_specific_curve_key_indices_list()
        num_key_indices_entries = len(specific_key_indices_list)
        
        if num_key_indices_entries > 0 and specific_key_indices_list[0] is not None:
            key_idx_a_x = specific_key_indices_list[0].x
            key_idx_a_y = specific_key_indices_list[0].y
            key_idx_a_z = specific_key_indices_list[0].z
        else:
            key_idx_a_x = 0
            key_idx_a_y = 1
            key_idx_a_z = 2
            
        if num_key_indices_entries > 1 and specific_key_indices_list[1] is not None:
            key_idx_b_x = specific_key_indices_list[1].x + 3
            key_idx_b_y = specific_key_indices_list[1].y + 3
            key_idx_b_z = specific_key_indices_list[1].z + 3
        else:
            key_idx_b_x = 3
            key_idx_b_y = 4
            key_idx_b_z = 5

        # if a target vector component is set, choose the component index for
        # the keys to copy from the cascade interp curve to niagara rich key 
        # curve.
        if target_vector_component is None:
            key_component_idx_a = None
            key_component_idx_b = None
        elif target_vector_component == "x":
            key_component_idx_a = key_idx_a_x
            key_component_idx_b = key_idx_b_x
        elif target_vector_component == "y":
            key_component_idx_a = key_idx_a_y
            key_component_idx_b = key_idx_b_y
        elif target_vector_component == "z":
            key_component_idx_a = key_idx_a_z
            key_component_idx_b = key_idx_b_z
        else:
            raise NameError("Encountered target vector component that was "
                            "not None, \"x\", \"y\", or \"z\"!")
        
        if isinstance(cascade_curve, ue.InterpCurveFloat):
            if target_vector_component is not None:
                raise NameError("Tried to specify target vector component for "
                                "const float curve!")
            keys = ue_fx_utils.keys_from_interp_curve_float(cascade_curve)
            curve_di = ue_fx_utils.create_float_curve_di(keys)
            
            curve_input = create_curve_input(curve_di, custom_indexer)

            return expand_value_width(
                curve_input,
                target_type_width
            )

        elif isinstance(cascade_curve, ue.InterpCurveVector):
            if target_vector_component is None:
                if target_type_width is None:
                    key_indices = [key_idx_a_x, key_idx_a_y, key_idx_a_z]
                elif target_type_width == ue.NiagaraScriptInputType.FLOAT:
                    key_indices = [key_idx_a_x]
                elif target_type_width == ue.NiagaraScriptInputType.VEC2:
                    key_indices = [key_idx_a_x, key_idx_a_y]
                elif target_type_width == ue.NiagaraScriptInputType.VEC3:
                    key_indices = [key_idx_a_x, key_idx_a_y, key_idx_a_z]
                else:
                    raise NameError("Target type width was not None, float, "
                                    "vec2 or vec3!")
                
                curve_di = make_curve_di(
                    cascade_curve,
                    ue_fx_utils.keys_from_interp_curve_vector,
                    key_indices
                )
                curve_input = create_curve_input(curve_di, custom_indexer)
                return expand_value_width(
                    curve_input,
                    target_type_width
                )
            
            else:
                keys = ue_fx_utils.keys_from_interp_curve_vector(
                    cascade_curve, key_component_idx_a)
                curve_di = ue_fx_utils.create_float_curve_di(keys)
                curve_input = create_curve_input(curve_di, custom_indexer)
                
                return expand_value_width(
                    curve_input,
                    target_type_width
                )
            
        elif isinstance(cascade_curve, ue.InterpCurveVector2D):
            if target_vector_component is not None:
                raise NameError("Tried to specify target vector component for "
                                "uniform range float curve!")
            keys_x = ue_fx_utils.keys_from_interp_curve_vector2d(
                cascade_curve, 0)
            keys_y = ue_fx_utils.keys_from_interp_curve_vector2d(
                cascade_curve, 1)
            curve_di_x = ue_fx_utils.create_float_curve_di(keys_x)
            curve_di_y = ue_fx_utils.create_float_curve_di(keys_y)
            
            curve_x_input = create_curve_input(curve_di_x, custom_indexer)
            curve_y_input = create_curve_input(curve_di_y, custom_indexer)

            random_range_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_random_range_float))
            random_range_script.set_parameter("Minimum", curve_x_input)
            random_range_script.set_parameter("Maximum", curve_y_input)
            random_range_input = ue_fx_utils.create_script_input_dynamic(
                random_range_script, ue.NiagaraScriptInputType.FLOAT)
            
            return expand_value_width(
                random_range_input,
                target_type_width
            )
            
        elif isinstance(cascade_curve, ue.InterpCurveTwoVectors):
            if target_vector_component is None:
                if target_type_width is None:
                    key_indices_a = [key_idx_a_x, key_idx_a_y, key_idx_a_z]
                    key_indices_b = [key_idx_b_x, key_idx_b_y, key_idx_b_z]
                elif target_type_width == ue.NiagaraScriptInputType.FLOAT:
                    key_indices_a = [key_idx_a_x]
                    key_indices_b = [key_idx_b_x]
                elif target_type_width == ue.NiagaraScriptInputType.VEC2:
                    key_indices_a = [key_idx_a_x, key_idx_a_y]
                    key_indices_b = [key_idx_b_x, key_idx_b_y]
                elif target_type_width == ue.NiagaraScriptInputType.VEC3:
                    key_indices_a = [key_idx_a_x, key_idx_a_y, key_idx_a_z]
                    key_indices_b = [key_idx_b_x, key_idx_b_y, key_idx_b_z]
                else:
                    raise NameError("Target type width was not None, float, "
                                    "vec2 or vec3!")
                
                curve_di_a = make_curve_di(
                    cascade_curve,
                    ue_fx_utils.keys_from_interp_curve_two_vectors,
                    key_indices_a
                )
                curve_di_b = make_curve_di(
                    cascade_curve,
                    ue_fx_utils.keys_from_interp_curve_two_vectors,
                    key_indices_b
                )

                curve_a_input = create_curve_input(curve_di_a, custom_indexer)
                curve_b_input = create_curve_input(curve_di_b, custom_indexer)

                random_range_script = ue_fx_utils.create_script_context(
                    ue.AssetData(Paths.di_random_range_vector))
                random_range_script.set_parameter("Minimum", curve_a_input)
                random_range_script.set_parameter("Maximum", curve_b_input)
                random_range_input = ue_fx_utils.create_script_input_dynamic(
                    random_range_script, ue.NiagaraScriptInputType.VEC3)

                return expand_value_width(
                    random_range_input,
                    target_type_width
                )
                
            else:
                keys_a = ue_fx_utils.keys_from_interp_curve_two_vectors(
                    cascade_curve, key_component_idx_a)
                keys_b = ue_fx_utils.keys_from_interp_curve_two_vectors(
                    cascade_curve, key_component_idx_b)
                curve_di_a = ue_fx_utils.create_float_curve_di(keys_a)
                curve_di_b = ue_fx_utils.create_float_curve_di(keys_b)
                
                curve_a_input = create_curve_input(curve_di_a, custom_indexer)
                curve_b_input = create_curve_input(curve_di_b, custom_indexer)

                random_range_script = ue_fx_utils.create_script_context(
                    ue.AssetData(Paths.di_random_range_float))
                random_range_script.set_parameter("Minimum", curve_a_input)
                random_range_script.set_parameter("Maximum", curve_b_input)
                random_range_input = ue_fx_utils.create_script_input_dynamic(
                    random_range_script, ue.NiagaraScriptInputType.FLOAT)

                return expand_value_width(
                    random_range_input,
                    target_type_width
                )

        else:
            raise NameError("Points for FInterpCurve from UDistribution were "
                            "not floats, vec2, vec3 or twovectors!")


def create_script_input_random_range(
    value_min,
    value_max,
    options=DistributionConversionOptions()
):
    target_type_width = options.get_target_type_width()
    target_vector_component = options.get_target_vector_component()
    b_evaluate_at_spawn = options.get_b_evaluate_spawn_only()
    if value_min == value_max:
        return __create_script_input_constant(
            value_max,
            target_type_width,
            target_vector_component
        )
    
    return __create_script_input_random_range(
        value_min,
        value_max,
        target_type_width,
        target_vector_component,
        b_evaluate_at_spawn
    )


def __create_script_input_random_range(
    value_min,
    value_max,
    target_type_width,
    target_vector_component,
    b_evaluate_at_spawn
):
    if isinstance(value_min, type(value_max)) is False:
        raise NameError("value_min and value_max are different types!")

    if isinstance(value_min, ue.NiagaraScriptConversionContextInput):
        input_min = value_min
        input_max = value_max
    elif target_vector_component is not None:
        # order of operations: if an explicit target type width and target 
        # vector component is set, make the inputs without specifying the 
        # target type width and then expand to the target type width before 
        # finishing. Otherwise, immediately obey the target type width for the 
        # inputs.
        input_min = __create_script_input_constant(
            value_min, None, target_vector_component)
        input_max = __create_script_input_constant(
            value_max, None, target_vector_component)
    else:
        input_min = __create_script_input_constant(
            value_min, target_type_width, target_vector_component)
        input_max = __create_script_input_constant(
            value_max, target_type_width, target_vector_component)

    script_input_type = input_min.input_type
    if script_input_type == ue.NiagaraScriptInputType.FLOAT:
        random_range_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_random_range_float))

    elif script_input_type == ue.NiagaraScriptInputType.VEC2:
        random_range_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_random_range_vec2))

    elif script_input_type == ue.NiagaraScriptInputType.VEC3:
        random_range_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_random_range_vector))

    else:
        raise NameError("Script input type was not float, vec2 or vec3!")

    random_range_script.set_parameter("Minimum", input_min)
    random_range_script.set_parameter("Maximum", input_max)
    
    if b_evaluate_at_spawn:
        eval_type_name = "Spawn Only"
    else:
        eval_type_name = "Every Frame"
    random_range_script.set_parameter(
        "Evaluation Type",
        ue_fx_utils.create_script_input_enum(
            Paths.enum_niagara_randomness_evaluation,
            eval_type_name
        )
    )

    random_range_input = ue_fx_utils.create_script_input_dynamic(
        random_range_script, script_input_type)
    return expand_value_width(
        random_range_input,
        target_type_width
    )


def create_script_input_lerp(
    value_a,
    value_b,
    options=DistributionConversionOptions()
):
    target_type_width = options.get_target_type_width()
    custom_indexer = options.get_custom_indexer()
    target_vector_component = options.get_target_vector_component()
    return __create_script_input_lerp(
        value_a,
        value_b,
        target_type_width,
        custom_indexer,
        target_vector_component
    )


def __create_script_input_lerp(
    value_a,
    value_b,
    target_type_width,
    custom_indexer,
    target_vector_component
):
    if isinstance(value_a, type(value_b)) is False:
        raise NameError("value_a and value_b are different types!")

    if isinstance(value_a, ue.NiagaraScriptConversionContextInput):
        input_a = value_a
        input_b = value_b
    elif target_vector_component is not None:
        # order of operations: if an explicit target type width and target 
        # vector component is set, make the inputs without specifying the 
        # target type width and then expand to the target type width before 
        # finishing. Otherwise, immediately obey the target type width for the 
        # inputs.
        input_a = __create_script_input_constant(
            value_a, None, target_vector_component)
        input_b = __create_script_input_constant(
            value_b, None, target_vector_component)
    else:
        input_a = __create_script_input_constant(
            value_a, target_type_width, target_vector_component)
        input_b = __create_script_input_constant(
            value_b, target_type_width, target_vector_component)

    script_input_type = input_a.input_type
    if script_input_type == ue.NiagaraScriptInputType.FLOAT:
        lerp_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_lerp_float))

    elif script_input_type == ue.NiagaraScriptInputType.VEC2:
        lerp_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_lerp_vec2))

    elif script_input_type == ue.NiagaraScriptInputType.VEC3:
        lerp_script = ue_fx_utils.create_script_context(
            ue.AssetData(Paths.di_lerp_vector))

    else:
        raise NameError("script input type was not float, vec2 or vec3!")

    lerp_script.set_parameter("A", input_a)
    lerp_script.set_parameter("B", input_b)
    if custom_indexer is not None:
        lerp_script.set_parameter("Alpha", custom_indexer)
    else:
        input_alpha = ue_fx_utils.create_script_input_linked_parameter(
            "Particles.NormalizedAge", ue.NiagaraScriptInputType.FLOAT)
        lerp_script.set_parameter("Alpha", input_alpha)

    lerp_input = ue_fx_utils.create_script_input_dynamic(
        lerp_script, script_input_type)
    return expand_value_width(lerp_input, target_type_width)


def create_script_input_constant(
    value,
    options=DistributionConversionOptions()
):
    target_type_width = options.get_target_type_width()
    target_vector_component = options.get_target_vector_component()
    return __create_script_input_constant(
        value,
        target_type_width,
        target_vector_component
    )


def __create_script_input_constant(
    value,
    target_type_width,
    target_vector_component
):
    swizzled_value = get_value_swizzle(
        value,
        target_type_width,
        target_vector_component
    )

    if isinstance(swizzled_value, float):
        script_input = ue_fx_utils.create_script_input_float(swizzled_value)
    elif isinstance(swizzled_value, ue.Vector2D):
        script_input = ue_fx_utils.create_script_input_vec2(swizzled_value)
    elif isinstance(swizzled_value, ue.Vector):
        script_input = ue_fx_utils.create_script_input_vector(swizzled_value)
    else:
        raise NameError("Swizzled value was not a float, vec2 or vec3!")

    return script_input


def create_curve_input(curve_di, custom_indexer):
    """
    Create a niagara script input and an optional script with the same behavior
    as the input cascade curve data interface.

    Args:
        curve_di (ue.UNiagaraDataInterfaceCurveBase): The curve to convert
            to a UNiagaraClipboardFunctionInput script_input and an optional
            UNiagaraScriptConversionContext dynamic input script.
        custom_indexer (ue.NiagaraScriptConversionContextInput): The input to
            drive converted curve or lerp script range evaluation by.

    Returns:
        (UNiagaraScriptConversionContextInput): Script input with the same 
        behavior as the input curve data interface.

    Raises:
        NameError: If an unknown child of UNiagaraDataInterfaceCurveBase is
            encountered.

    """
    if curve_di is None:
        raise NameError("Did not receive valid Niagara curve DI to create input!")
    
    curve_input = ue_fx_utils.create_script_input_di(curve_di)
    if isinstance(curve_di, ue.NiagaraDataInterfaceCurve):
        float_from_curve_script_asset = ue.AssetData(
            Paths.di_float_from_curve)
        curve_script_context = ue_fx_utils.create_script_context(
            float_from_curve_script_asset)
        curve_script_context.set_parameter("FloatCurve", curve_input)
        if custom_indexer is not None:
            curve_script_context.set_parameter("CurveIndex", custom_indexer)
        return ue_fx_utils.create_script_input_dynamic(
            curve_script_context, 
            ue.NiagaraScriptInputType.FLOAT
        )

    elif isinstance(curve_di, ue.NiagaraDataInterfaceVector2DCurve):
        vec2_from_curve_script_asset = ue.AssetData(
            Paths.di_vec2_from_curve)
        curve_script_context = ue_fx_utils.create_script_context(
            vec2_from_curve_script_asset)
        curve_script_context.set_parameter("Vector2Curve", curve_input)
        if custom_indexer is not None:
            curve_script_context.set_parameter("CurveIndex", custom_indexer)
        return ue_fx_utils.create_script_input_dynamic(
            curve_script_context,
            ue.NiagaraScriptInputType.VEC2
        )

    elif isinstance(curve_di, ue.NiagaraDataInterfaceVectorCurve):
        vector_from_curve_script_asset = ue.AssetData(
            Paths.di_vector_from_curve)
        curve_script_context = ue_fx_utils.create_script_context(
            vector_from_curve_script_asset)
        curve_script_context.set_parameter("VectorCurve", curve_input)
        if custom_indexer is not None:
            curve_script_context.set_parameter("CurveIndex", custom_indexer)
        return ue_fx_utils.create_script_input_dynamic(
            curve_script_context,
            ue.NiagaraScriptInputType.VEC3
        )
        
    else:
        raise NameError("Encountered unhandled curve datainterface type!")


def get_alignment_and_facingmode_for_screenalignment(cascade_screen_alignment):
    """
    Get niagara compatible sprite alignment and sprite facing mode from cascade
    type screen alignment.

    Args:
        cascade_screen_alignment (EParticleScreenAlignment): Cascade
            description for how a particle should orient to the view.

    Returns:
        (ENiagaraSpriteAlignment, ENiagaraSpriteFacingMode): Tuple of niagara
            description for how a particle sprite should be aligned, and how the
            sprite should face the view.

    Raises:
        NameError: If an unknown cascade_screen_alignment is encountered.
    """
    if cascade_screen_alignment == ue.ParticleScreenAlignment.PSA_FACING_CAMERA_POSITION:
        return ue.NiagaraSpriteAlignment.UNALIGNED, ue.NiagaraSpriteFacingMode.FACE_CAMERA

    elif cascade_screen_alignment == ue.ParticleScreenAlignment.PSA_SQUARE:
        return ue.NiagaraSpriteAlignment.UNALIGNED, ue.NiagaraSpriteFacingMode.FACE_CAMERA_PLANE

    elif cascade_screen_alignment == ue.ParticleScreenAlignment.PSA_RECTANGLE:
        return ue.NiagaraSpriteAlignment.UNALIGNED, ue.NiagaraSpriteFacingMode.FACE_CAMERA_POSITION

    elif cascade_screen_alignment == ue.ParticleScreenAlignment.PSA_VELOCITY:
        return ue.NiagaraSpriteAlignment.VELOCITY_ALIGNED, ue.NiagaraSpriteFacingMode.FACE_CAMERA

    elif cascade_screen_alignment == ue.ParticleScreenAlignment.PSA_AWAY_FROM_CENTER:
        return ue.NiagaraSpriteAlignment.UNALIGNED, ue.NiagaraSpriteFacingMode.FACE_CAMERA

    elif cascade_screen_alignment == ue.ParticleScreenAlignment.PSA_TYPE_SPECIFIC:
        return ue.NiagaraSpriteAlignment.CUSTOM_ALIGNMENT, ue.NiagaraSpriteFacingMode.CUSTOM_FACING_VECTOR

    elif cascade_screen_alignment == ue.ParticleScreenAlignment.PSA_FACING_CAMERA_DISTANCE_BLEND:
        return ue.NiagaraSpriteAlignment.UNALIGNED, ue.NiagaraSpriteFacingMode.FACE_CAMERA_DISTANCE_BLEND

    else:
        raise NameError("Unknown cascade_screen_alignment encountered!")


def get_niagara_sortmode_and_binding_for_sortmode(cascade_sort_mode):
    """
    Get niagara compatible sort mode and variable binding for the sort mode
    from cascade type sort mode.

    Args:
        cascade_sort_mode (EParticleSortMode): Cascade description for how
            particles in an emitter should be sorted.

    Returns:
        (ENiagaraSortMode, FString): Tuple of niagara compatible sort mode and
            variable name string to bind to the sort mode.

    Raises:
        NameError: If an unknown cascade_sort_mode is encountered.
    """
    if cascade_sort_mode == ue.ParticleSortMode.PSORTMODE_NONE:
        return ue.NiagaraSortMode.NONE, None

    elif cascade_sort_mode == ue.ParticleSortMode.PSORTMODE_VIEW_PROJ_DEPTH:
        return ue.NiagaraSortMode.VIEW_DEPTH, None

    elif cascade_sort_mode == ue.ParticleSortMode.PSORTMODE_DISTANCE_TO_VIEW:
        return ue.NiagaraSortMode.VIEW_DISTANCE, None

    elif cascade_sort_mode == ue.ParticleSortMode.PSORTMODE_AGE_OLDEST_FIRST:
        return ue.NiagaraSortMode.CUSTOM_ASCENDING, "Particles.Age"

    elif cascade_sort_mode == ue.ParticleSortMode.PSORTMODE_AGE_NEWEST_FIRST:
        return ue.NiagaraSortMode.CUSTOM_DECENDING, "Particles.Age"

    else:
        raise NameError("Unknown cascade_sort_mode encountered!")


#  todo set sensible defaults
def get_enable_subimage_blend_and_binding_for_interpolation_method(
        cascade_interpolation_method):
    """
    Get niagara compatible bool if subimage blend should be enabled and
    parameter name string to bind to the interpolation method from cascade
    interpolation method.

    Args:
        cascade_interpolation_method (EParticleSubUVInterpMethod): Cascade
            description for how sub uvs should be interpolated.

    Returns:
        (bool, FString): Tuple of bool to enable sub uv interpolation and
            parameter name string to bind to the niagara interpolation method.

    Raises:
        NameError: If an unknown cascade_interpolation_method is
            encountered.
    """
    if cascade_interpolation_method == ue.ParticleSubUVInterpMethod.PSUVIM_NONE:
        return False, None

    elif cascade_interpolation_method == ue.ParticleSubUVInterpMethod.PSUVIM_LINEAR:
        return False, None

    elif cascade_interpolation_method == ue.ParticleSubUVInterpMethod.PSUVIM_LINEAR_BLEND:
        return True, None

    elif cascade_interpolation_method == ue.ParticleSubUVInterpMethod.PSUVIM_RANDOM:
        return False, None

    elif cascade_interpolation_method == ue.ParticleSubUVInterpMethod.PSUVIM_RANDOM_BLEND:
        return True, None

    else:
        raise NameError("Unknown cascade_interpolation_method encountered!")


def distribution_is_const(
    distribution
):
    """
    Determine if the evaluated values of the input distribution are constant.

    Args:
        distribution (UDistribution): The cascade parameter distribution to
            evaluate.

    Returns:
        (bool): Whether the value range of the input distribution is constant.

    Raises:
        NameError: If an unknown cascade_interpolation_method is
            encountered.
    """
    distribution_type, distribution_value_type = (
        ue_fx_utils.get_distribution_type(distribution)
    )
    if distribution_type == ue.DistributionType.CONST:
        return True
    
    elif (
        distribution_type == ue.DistributionType.UNIFORM or
        distribution_type == ue.DistributionType.CONST_CURVE or
        distribution_type == ue.DistributionType.UNIFORM_CURVE
    ):
        success, minvalue, maxvalue = ue_fx_utils.get_distribution_min_max_values(
            distribution
        )
        if success is False:
            return False
        elif minvalue == maxvalue:
            return True
        else:
            return False
        
    elif distribution_type == ue.DistributionType.PARAMETER:
        return False
    
    else:
        raise NameError("Unknown distribution type encountered!")


def distribution_always_equals(
    distribution,
    value,
    target_vector_component=None
):
    """
    Determine if the evaluated values of the input distribution are always equal
    to the input value.

    Args:
        distribution (UDistribution): The cascade parameter distribution to
            evaluate.
        value (float or ue.Vector): The target value to test the distribution
            value range against.

    Returns:
        (bool): Whether the value range of the input distribution is always
            equal to the input value.

    Raises:
        NameError: If an unknown cascade_interpolation_method is
            encountered or invalid value type is input.
    """
    success, minvalue, maxvalue = ue_fx_utils.get_distribution_min_max_values(
        distribution)
    if success is False:
        return False
    elif target_vector_component is None and minvalue != maxvalue:
        return False
    
    distribution_type, distribution_value_type = (
        ue_fx_utils.get_distribution_type(distribution))
    
    if target_vector_component is not None:
        if distribution_value_type != ue.DistributionValueType.VECTOR:
            raise NameError("Tried to check target vector component of "
                            "distribution that was not a vector!")
        elif isinstance(value, float) is False:
            raise NameError("Tried to check target vector component but value "
                            "to compare was not a float!")
        
        if target_vector_component == "x":
            return value == minvalue.x and value == maxvalue.x
        elif target_vector_component == "y":
            return value == minvalue.y and value == maxvalue.y
        elif target_vector_component == "z":
            return value == minvalue.z and value == maxvalue.z
        else:
            raise NameError("Tried to check target vector component that was "
                            "not x, y or z!")
    
    if distribution_value_type == ue.DistributionValueType.FLOAT:
        if isinstance(value, float):
            return value == minvalue.x
        elif isinstance(value, ue.Vector):
            return value == minvalue
        else:
            raise NameError("Value type was not float or vec3!")
    elif distribution_value_type == ue.DistributionValueType.VECTOR:
        if isinstance(value, float):
            value_vec3 = ue.Vector(value, value, value)
            return value_vec3 == minvalue
        elif isinstance(value, ue.Vector):
            return value == minvalue
        else:
            raise NameError("Value type was not float or vec3!")
    else:
        raise NameError("Encountered distribution value type that was not "
                        "float or vector!")


def get_value_swizzle(value, target_type_width, target_vector_component):
    if target_type_width is None:
        if isinstance(value, float):
            if target_vector_component is None:
                return value
            elif target_vector_component == "x":
                return value
            elif target_vector_component == "y":
                raise NameError("Tried to get y component of float "
                                "distribution!")
            elif target_vector_component == "z":
                raise NameError("Tried to get z component of float "
                                "distribution!")
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        elif isinstance(value, ue.Vector2D):
            if target_vector_component is None:
                return value
            elif target_vector_component == "x":
                return value.x
            elif target_vector_component == "y":
                return value.y
            elif target_vector_component == "z":
                raise NameError("Tried to get z component of vec2 "
                                "distribution!")
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        elif isinstance(value, ue.Vector):
            if target_vector_component is None:
                return value
            elif target_vector_component == "x":
                return value.x
            elif target_vector_component == "y":
                return value.y
            elif target_vector_component == "z":
                return value.z
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        else:
            raise NameError("Input value was not a float, vec2 or vec3!")

    elif target_type_width is ue.NiagaraScriptInputType.FLOAT:
        if isinstance(value, float):
            if target_vector_component is None:
                return value
            elif target_vector_component == "x":
                return value
            elif target_vector_component == "y":
                raise NameError("Tried to get y component of float "
                                "distribution!")
            elif target_vector_component == "z":
                raise NameError("Tried to get z component of float "
                                "distribution!")
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        elif isinstance(value, ue.Vector2D):
            if target_vector_component is None:
                return value.x
            elif target_vector_component == "x":
                return value.x
            elif target_vector_component == "y":
                return value.y
            elif target_vector_component == "z":
                raise NameError("Tried to get z component of vec2 "
                                "distribution!")
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        elif isinstance(value, ue.Vector):
            if target_vector_component is None:
                return value.x
            elif target_vector_component == "x":
                return value.x
            elif target_vector_component == "y":
                return value.y
            elif target_vector_component == "z":
                return value.z
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        else:
            raise NameError("Input value was not a float, vec2 or vec3!")

    elif target_type_width is ue.NiagaraScriptInputType.VEC2:
        if isinstance(value, float):
            if target_vector_component is None:
                return ue.Vector2D(value, 0.0)
            elif target_vector_component == "x":
                return ue.Vector2D(value, value)
            elif target_vector_component == "y":
                raise NameError("Tried to get y component of float "
                                "distribution!")
            elif target_vector_component == "z":
                raise NameError("Tried to get z component of float "
                                "distribution!")
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        elif isinstance(value, ue.Vector2D):
            if target_vector_component is None:
                return value
            elif target_vector_component == "x":
                return ue.Vector2D(value.x, value.x)
            elif target_vector_component == "y":
                return ue.Vector2D(value.y, value.y)
            elif target_vector_component == "z":
                raise NameError("Tried to get z component of vec2 "
                                "distribution!")
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        elif isinstance(value, ue.Vector):
            if target_vector_component is None:
                return ue.Vector2D(value.x, value.y)
            elif target_vector_component == "x":
                return ue.Vector2D(value.x, value.x)
            elif target_vector_component == "y":
                return ue.Vector2D(value.y, value.y)
            elif target_vector_component == "z":
                return ue.Vector2D(value.z, value.z)
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        else:
            raise NameError("Input value was not a float, vec2 or vec3!")

    elif target_type_width is ue.NiagaraScriptInputType.VEC3:
        if isinstance(value, float):
            if target_vector_component is None:
                return ue.Vector(value, 0.0, 0.0)
            elif target_vector_component == "x":
                return ue.Vector(value, value, value)
            elif target_vector_component == "y":
                raise NameError("Tried to get y component of float "
                                "distribution!")
            elif target_vector_component == "z":
                raise NameError("Tried to get z component of float "
                                "distribution!")
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        elif isinstance(value, ue.Vector2D):
            if target_vector_component is None:
                return ue.Vector(value.x, value.y, 0.0)
            elif target_vector_component == "x":
                return ue.Vector(value.x, value.x, value.x)
            elif target_vector_component == "y":
                return ue.Vector(value.y, value.y, value.y)
            elif target_vector_component == "z":
                raise NameError("Tried to get z component of vec2 "
                                "distribution!")
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        elif isinstance(value, ue.Vector):
            if target_vector_component is None:
                return value
            elif target_vector_component == "x":
                return ue.Vector(value.x, value.x, value.x)
            elif target_vector_component == "y":
                return ue.Vector(value.y, value.y, value.y)
            elif target_vector_component == "z":
                return ue.Vector(value.z, value.z, value.z)
            else:
                raise NameError("Encountered target vector component that was "
                                "not \"x\", \"y\", or \"z\"!")

        else:
            raise NameError("Input value was not a float, vec2 or vec3!")
    else:
        raise NameError("target_type_width was not None, float, vec2 or vec3!")


def expand_value_width(value_input, target_type_width):
    if isinstance(value_input, ue.NiagaraScriptConversionContextInput) is False:
        raise NameError("Expected conversion context input!")

    if target_type_width is None:
        return value_input

    if value_input.input_type == ue.NiagaraScriptInputType.FLOAT:
        if target_type_width == ue.NiagaraScriptInputType.FLOAT:
            return value_input
        elif target_type_width == ue.NiagaraScriptInputType.VEC2:
            vec2_from_float_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_vec2_from_float))
            vec2_from_float_script.set_parameter("Value", value_input)
            return ue_fx_utils.create_script_input_dynamic(
                vec2_from_float_script,
                ue.NiagaraScriptInputType.VEC2
            )
        elif target_type_width == ue.NiagaraScriptInputType.VEC3:
            vec3_from_float_script = ue_fx_utils.create_script_context(
                ue.AssetData(Paths.di_vector_from_float))
            vec3_from_float_script.set_parameter("Value", value_input)
            return  ue_fx_utils.create_script_input_dynamic(
                vec3_from_float_script,
                ue.NiagaraScriptInputType.VEC3
            )
        else:
            raise NameError("Target type width was not float, vec2 or vec3!")

    elif value_input.input_type == ue.NiagaraScriptInputType.VEC2:
        if target_type_width == ue.NiagaraScriptInputType.FLOAT:
            raise NameError("Tried to trim vec2 value to float!")
        elif target_type_width == ue.NiagaraScriptInputType.VEC2:
            return value_input
        elif target_type_width == ue.NiagaraScriptInputType.VEC3:
            raise NameError("Mode not implemented! Cannot expand vec2 "
                            "to vec3!")
        else:
            raise NameError("Target type width was not float, vec2 or vec3!")

    elif value_input.input_type == ue.NiagaraScriptInputType.VEC3:
        if target_type_width == ue.NiagaraScriptInputType.FLOAT:
            raise NameError("Tried to trim vec3 value to float!")
        elif target_type_width == ue.NiagaraScriptInputType.VEC2:
            raise NameError("Tried to trim vec3 value to vec2!")
        elif target_type_width == ue.NiagaraScriptInputType.VEC3:
            return value_input
        else:
            raise NameError("Target type width was not float, vec2 or vec3!")

    else:
        raise NameError("Value input type was not float, vec2 or vec3!")


def get_module_name(module, fallback_name='None'):
    """
    Get a name string for a module. Handles where the module class is unknown 
    if it has not been exported as a python type.
    
    Args:
        module (UParticleModule): Module to get a legible name string from.
        fallback_name (str): Fallback name to return if the module argument is 
            None.

    Returns:
        (str): Name string for the input module.
    """
    if module is None:
        return fallback_name
    
    module_type = type(module)
    if (
        module_type == ue.ParticleModule or
        module_type == ue.ParticleModuleTypeDataBase
    ):
        # this module is not exported and the scripting is not aware of its 
        # class name, get the object name instead.
        return module.get_name()
    else:
        return type(module).__name__
