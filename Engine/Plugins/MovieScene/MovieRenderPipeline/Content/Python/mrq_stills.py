import unreal

''' 
Creates a sequence for the given target camera
This script is called by MRQ's StillRenderSetupAutomation.

Args:
    asset_name(str): The name of the sequence to be created
    length_frames(int): The number of frames in the camera cut section.
    package_path(str): The location for the new sequence
    target_camera(unreal.CameraActor): The camera to use.
    
Returns:
    unreal.LevelSequence: The created level sequence
'''
def create_sequence_from_selection(
    asset_name, 
    length_frames = 1, 
    package_path = '/Game/Sequencer/Stills/', 
    target_camera = unreal.Object):

    # Create the sequence asset in the desired location
	sequence = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, 
        package_path, 
        unreal.LevelSequence, 
        unreal.LevelSequenceFactoryNew())

	# Only one frame needed to render stills
	sequence.set_playback_end(1)

	binding = sequence.add_possessable(target_camera)

	try:
		camera = unreal.CameraActor.cast(target_camera)
		camera_cut_track = sequence.add_master_track(unreal.MovieSceneCameraCutTrack)

		# Add a camera cut track for this camera
		# Make sure the camera cut is stretched to the -1 mark
		camera_cut_section = camera_cut_track.add_section()
		camera_cut_section.set_start_frame(-1)
		camera_cut_section.set_end_frame(length_frames)

		camera_binding_id = unreal.MovieSceneObjectBindingID()
		camera_binding_id.set_editor_property("Guid", binding.get_id())
		camera_cut_section.set_editor_property("CameraBindingID", camera_binding_id)
		
		# Add a current focal length track to the cine camera component
		camera_component = target_camera.get_cine_camera_component()
		camera_component_binding = sequence.add_possessable(camera_component)
		camera_component_binding.set_parent(binding)
		focal_length_track = camera_component_binding.add_track(unreal.MovieSceneFloatTrack)
		focal_length_track.set_property_name_and_path('CurrentFocalLength', 'CurrentFocalLength')
		focal_length_section = focal_length_track.add_section()
		focal_length_section.set_start_frame_bounded(0)
		focal_length_section.set_end_frame_bounded(0)	
		
	except TypeError:
		unreal.log_error(f"Trex TypeError {str(sequence)}")

	# Log the output sequence with tag Trex for easy lookup
	unreal.log("Trex" + str(sequence))
	return sequence
