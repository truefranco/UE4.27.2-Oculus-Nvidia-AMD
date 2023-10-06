# Copyright Epic Games, Inc. All Rights Reserved.
import unreal
import random 

#
# USAGE:
#   - Requires the "Python Editor Script Plugin" to be enabled in your project.
#
#   Open the Python interactive console and use:
#       import MoviePipelineMiscExamples
#       MoviePipelineMiscExamples.ExampleResolveOutputPath()
#   or:
#       MoviePipelineMiscExamples.ExampleResolveVersionNumber()

'''
    Summary:
        This example shows how you can partially determine what filename or output
        folder a Queue may render to, ie: it lets you resolve the Output Directory
        into an actual filepath taking into account shot names, etc.
        
        For things not determined by the job, they are pulled from the Params struct
        which must be filled out.
'''
def ExampleResolveOutputPath():
    subsystem = unreal.get_editor_subsystem(unreal.MoviePipelineQueueSubsystem)
    pipelineQueue = subsystem.get_queue()
    if(len(pipelineQueue.get_jobs()) == 0):
        unreal.log_error("Open the Window > Movie Render Queue and add at least one job to use this example.")
        return
        
    # Get the output settings
    job = pipelineQueue.get_jobs()[0]
    outputSetting = job.get_configuration().find_or_add_setting_by_class(unreal.MoviePipelineOutputSetting)
    outputSetting.output_directory.set_editor_property("path", "C:/TestFolder")
    outputSetting.file_name_format = "{shot_name}.{frame_number}"
    
    combinedPath = unreal.Paths.combine([outputSetting.output_directory.path, outputSetting.file_name_format])
    
    # Create the params struct
    params = unreal.MoviePipelineFilenameResolveParams()
    params.frame_number = 55
    params.frame_number_shot = 23
    params.frame_number_rel = 12
    params.frame_number_shot_rel = 3
    params.camera_name_override = job.shot_info[0].inner_name
    params.shot_name_override = job.shot_info[0].outer_name
    params.zero_pad_frame_number_count = 3
    params.force_relative_frame_numbers = False
    params.file_name_format_overrides["ext"] = "jpg"
    params.initialization_time = unreal.MathLibrary.utc_now()
    params.initialization_version = 3
    params.job = job
    params.shot_override = job.shot_info[0]
    
    (resolvedPath, mergedFormatArgs) = unreal.MoviePipelineLibrary.resolve_filename_format_arguments(combinedPath, params)
    
    unreal.log("Resolved Path: " + resolvedPath)
    

'''
    Summary:
        This example shows how you can partially determine what filename or output
        folder a Queue may render to, ie: it lets you resolve the Output Directory
        into an actual filepath taking into account shot names, etc.
        
        For things not determined by the job, they are pulled from the Params struct
        which must be filled out.
'''
def ExampleResolveVersionNumber():
    subsystem = unreal.get_editor_subsystem(unreal.MoviePipelineQueueSubsystem)
    pipelineQueue = subsystem.get_queue()
    if(len(pipelineQueue.get_jobs()) == 0):
        unreal.log_error("Open the Window > Movie Render Queue and add at least one job to use this example.")
        return
        
    # Get the output settings
    job = pipelineQueue.get_jobs()[0]
    outputSetting = job.get_configuration().find_or_add_setting_by_class(unreal.MoviePipelineOutputSetting)
    outputSetting.auto_version = True
    outputSetting.file_name_format = "{version}/{shot_name}.{frame_number}"
    
    combinedPath = unreal.Paths.combine([outputSetting.output_directory.path, outputSetting.file_name_format])
    
    # Create the params struct
    params = unreal.MoviePipelineFilenameResolveParams()
    params.frame_number = 55
    params.frame_number_shot = 23
    params.frame_number_rel = 12
    params.frame_number_shot_rel = 3
    params.camera_name_override = job.shot_info[0].inner_name # Only need these if shot_override isn't specified, but listed here for examples sake
    params.shot_name_override = job.shot_info[0].outer_name
    params.zero_pad_frame_number_count = 3
    params.force_relative_frame_numbers = False
    params.file_name_format_overrides["ext"] = "jpg"
    params.initialization_time = unreal.MathLibrary.utc_now()
    params.job = job
    params.shot_override = job.shot_info[0]
    
    unreal.log("Looking for a folder with version formatting (vXXX) at path: " + combinedPath)
    versionNumberFoundOnDisk = unreal.MoviePipelineLibrary.resolve_version_number(params)
    if versionNumberFoundOnDisk == 0:
        unreal.log("Either didn't find a folder with pattern 'v000' or no folder found on disk, version number is 0.")
    else:
        unreal.log("Found a version folder, new version would be: " + str(versionNumberFoundOnDisk))