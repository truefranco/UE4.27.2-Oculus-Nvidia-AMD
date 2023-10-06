from ModuleConverterInterface import ModuleConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import Paths


class CascadeLocationBoneSocketConverter(ModuleConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return ue.ParticleModuleLocationBoneSocket

    @classmethod
    def convert(cls, args):
        cascade_module = args.get_cascade_module()
        niagara_emitter_context = args.get_niagara_emitter_context()
        
        # get all properties for the bone socket location module.
        (   source_type,
            universal_offset,
            source_locations,
            selection_method, 
            b_update_positions_each_frame,
            b_orient_mesh_emitters, 
            b_inherit_bone_velocity,
            inherit_velocity_scale, 
            skel_mesh_actor_param_name,
            num_pre_selected_indices, 
            editor_skel_mesh
        ) = ue_fx_utils.get_particle_module_location_bone_socket_props(cascade_module)

        # choose whether to update each frame.
        if b_update_positions_each_frame:
            exec_category = ue.ScriptExecutionCategory.PARTICLE_UPDATE
        else:
            exec_category = ue.ScriptExecutionCategory.PARTICLE_SPAWN
        
        # add the skeletal mesh location module to the emitter.
        skel_loc_script = niagara_emitter_context.find_or_add_module_script(
            "SkeletalMeshLocation",
            ue.AssetData(Paths.script_skeletal_mesh_location),
            exec_category
        )
        
        # create a skeletal mesh DI to configure.
        skel_di = ue.NiagaraDataInterfaceSkeletalMesh()
        
        # set the sampling type.
        b_sample_bones = False
        b_sample_sockets = False
        if source_type == ue.LocationBoneSocketSource.BONESOCKETSOURCE_BONES:
            list_name = "FilteredBones"
            sample_mode_name = "Bone Sampling Mode"
            sample_mode_enum = Paths.enum_niagara_bone_sampling_mode
            b_sample_bones = True
            skel_loc_script.set_parameter(
                "Mesh Sampling Type",
                ue_fx_utils.create_script_input_enum(
                    Paths.enum_niagara_skel_sampling_mode_full,
                    "Skeleton (Bones)"
                )
            )
        elif source_type == ue.LocationBoneSocketSource.BONESOCKETSOURCE_SOCKETS:
            list_name = "FilteredSockets"
            sample_mode_name = "Socket Sampling Mode"
            sample_mode_enum = Paths.enum_niagara_socket_sampling_mode
            b_sample_sockets = True
            skel_loc_script.set_parameter(
                "Mesh Sampling Type",
                ue_fx_utils.create_script_input_enum(
                    Paths.enum_niagara_skel_sampling_mode_full,
                    "Skeleton (Sockets)"
                )
            )
        else:
            raise NameError("Encountered invalid location bone socket source "
                            "when converting cascade location bone socket "
                            "module!")
        
        # set the sampled position offset.
        skel_loc_script.set_parameter(
            "Sampled Position Offset",
            ue_fx_utils.create_script_input_vector(universal_offset)
        )
        
        # copy the source locations to the skel mesh sampling DI.
        if len(source_locations) == 0:
            b_sampling_all = True
        else:
            b_sampling_all = False
            location_names = []
            location_offsets = []
            b_logged_offset_warning = False
            for source_location in source_locations:
                location_names.append(source_location.get_editor_property("BoneSocketName"))
                location_offset = source_location.get_editor_property("Offset")
                if b_logged_offset_warning is False and location_offset != ue.Vector(0.0, 0.0, 0.0):
                    skel_loc_script.log(
                        "Failed to copy per bone/socket offset: Niagara skeletal "
                        "mesh location module does not support this mode.",
                        ue.NiagaraMessageSeverity.WARNING
                    )
                    b_logged_offset_warning = True
            
            skel_di.set_editor_property(list_name, location_names)
        
        # set the selection method.
        if selection_method == ue.LocationBoneSocketSelectionMethod.BONESOCKETSEL_SEQUENTIAL:
            sample_mode_prefix_str = "Direct "
        elif selection_method == ue.LocationBoneSocketSelectionMethod.BONESOCKETSEL_RANDOM:
            sample_mode_prefix_str = "Random "
        else:
            raise NameError("Encountered invalid location bone socket "
                            "selection method when converting cascade location "
                            "bone socket module!")

        if b_sample_bones:
            if b_sampling_all:
                sample_mode_postfix_str = "(All Bones)"
            else:
                sample_mode_postfix_str = "(Filtered Bones)"
        elif b_sample_sockets:
            sample_mode_postfix_str = "(Filtered Sockets)"
        else:
            raise NameError("Encountered invalid selection when converting "
                            "cascade location bone socket module; mode was "
                            "not sampling bones or sockets!")
        
        skel_loc_script.set_parameter(
            sample_mode_name,
            ue_fx_utils.create_script_input_enum(
                sample_mode_enum,
                sample_mode_prefix_str + sample_mode_postfix_str
            )
        )
        
        # choose whether to orient mesh emitters.
        # NOTE: this setting does nothing for this module.
        # if b_orient_mesh_emitters:
        
        # choose whether to inherit bone velocity.
        if b_inherit_bone_velocity:
            skel_loc_script.set_parameter(
                "Inherit Velocity (Scale)",
                ue_fx_utils.create_script_input_float(inherit_velocity_scale),
                True,
                True
            )
        else:
            skel_loc_script.set_parameter(
                "Inherit Velocity (Scale)",
                ue_fx_utils.create_script_input_float(1.0),
                True,
                False
            )
        
        # set the source skel mesh name
        # NOTE: niagara does not support skel mesh name lookup.
        # skel_mesh_actor_param_name
        
        # skip num_pre_selected_indices; Niagara does not support an equivalent
        # mode.
        
        # set the preview mesh.
        skel_di.set_editor_property("PreviewMesh", editor_skel_mesh)
        
        # set the di.
        skel_di_input = ue_fx_utils.create_script_input_di(skel_di)
        skel_loc_script.set_parameter("Skeletal Mesh", skel_di_input)
