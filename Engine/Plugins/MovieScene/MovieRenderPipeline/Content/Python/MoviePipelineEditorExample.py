# Copyright Epic Games, Inc. All Rights Reserved.
import unreal
import random 

# This example is an implementation of isolated Python script which is responsible
# for taking a level sequence and rendering it with some fixed settings on the current
# map in the editor. A more ideal implementation would be to implement an Executor which
# can replace the behavior of the Render (Local) / Render (Remote) buttons in the UI, but
# standalone python scripts are simpler. 
#
# See MoviePipelineExampleRuntimeExecutor.py for an example on how to implement an Executor class, 
# though you will need to add the following to the _post_init function.
#   projectSettings = unreal.get_default_object(unreal.MovieRenderPipelineProjectSettings)
#   projectSettings.default_remote_executor = self.get_class()
# This overrides the behavior of the Render (Remote) to automatically instantiate and run your executor
# as Python classes cannot be selected via the Project Preferences dropdown at this time.
# 
# Standalone Python scripts like this need to use global Python functions for callbacks, otherwise
# garbage collection will remove it and cause the callback to silently fail, leaving users confused as
# to why they didn't get the callback.
#
# USAGE:
#   - Requires the "Python Editor Script Plugin" to be enabled in your project.
#
#   Open Window > Movie Render Queue 
#       Add a level sequence to the Queue to be rendered
#
#   Open the Python interactive console and use:
#       import MoviePipelineEditorExample
#       MoviePipelineEditorExample.RenderQueue_ViaDuplication()
#    OR:
#       MoviePipelineEditorExample.RenderQueue_InPlace()
#
# See individual functions below for specifics on differences.

'''
    Python Globals to keep the UObjects alive through garbage collection.
    Must be deleted by hand on completion.
'''
# The light that we spawn to manipulate between jobs
SpawnedPointLightActor = None

# The duplicated queue we edit. You don't need to duplicate the queue,
# but we do it in this example to show a slightly more advanced use case.
NewQueue = None

# The executor we are using to execute the actual jobs. This also isn't needed
# unless you want to use a queue that is different than the one in the UI.
NewExecutor = None

'''
    Summary:
        This function is called after the executor has finished
    Params:
        success - True if all jobs completed successfully.
'''
def OnQueueFinishedCallback(executor, success):
    global SpawnedPointLightActor
    global NewExecutor
    global NewQueue
    
    unreal.log("Render completed. Success: " + str(success))
    global SpawnedPointLightActor 
    unreal.EditorLevelLibrary.destroy_actor(SpawnedPointLightActor)
    
    # Delete our reference too so we don't keep it alive.
    if SpawnedPointLightActor != None:
        del SpawnedPointLightActor 
    if NewExecutor != None:
        del NewExecutor
    if NewQueue != None:
        del NewQueue
    
    
'''
    Summary:
        This function is called after each individual job in the queue is finished.
        At this point, PIE has been stopped so edits you make will be applied to the
        editor world. This can be useful if you want to swap an actor in the level
        and render the next job with the different actor, such as creating turn tables.
'''
def OnIndividualJobFinishedCallback(params):
    # You can make any edits you want to the editor world here, and the world
    # will be duplicated when the next render happens. Make sure you undo your
    # edits in OnQueueFinishedCallback if you don't want to leak state changes
    # into the editor world.
    unreal.log("Individual job completed. Changing light color.")
    global SpawnedPointLightActor
    SpawnedPointLightActor.set_light_color((random.uniform(0, 1), random.uniform(0, 1), random.uniform(0, 1), 1))
    

def OnIndividualShotFinishedCallback(params):
    # When using a per-shot callback there's only one shot_data in the array
    unreal.log("Individual shot finished. Shot: " + params.shot_data[0].shot.outer_name)
    
    for shot in params.shot_data:
        for passIdentifier in shot.render_pass_data:
            unreal.log("render pass: " + passIdentifier.name)
            for file in shot.render_pass_data[passIdentifier].file_paths:
                unreal.log("file: " + file)
    
    # We can also resolve these to more 'friendly' for command line tools.
    # This is only tested with relatively simple configurations ie:
    # {shot_name}/{frame_number} or {sequence_name}.{frame_number}
    outputSetting = params.job.get_configuration().find_or_add_setting_by_class(unreal.MoviePipelineOutputSetting)

    resolveParams = unreal.MoviePipelineFilenameResolveParams()
    resolveParams.camera_name_override = params.shot_data[0].shot.inner_name
    resolveParams.shot_name_override = params.shot_data[0].shot.outer_name
    resolveParams.zero_pad_frame_number_count = outputSetting.zero_pad_frame_numbers
    resolveParams.force_relative_frame_numbers = False
    resolveParams.file_name_format_overrides["ext"] = "jpg"
    resolveParams.file_name_format_overrides["frame_number"] = "%4d"
    resolveParams.initialization_time = unreal.MoviePipelineLibrary.get_job_initialization_time(params.pipeline)
    resolveParams.job = params.job
    resolveParams.shot_override = params.shot_data[0].shot
    
    combinedPath = unreal.Paths.combine([outputSetting.output_directory.path, outputSetting.file_name_format])
    (resolvedPath, mergedFormatArgs) = unreal.MoviePipelineLibrary.resolve_filename_format_arguments(combinedPath, resolveParams)
    
    unreal.log("Simplified path for CLI tools: " + resolvedPath)
    
'''
    Summary:
        This example takes the queue that you have in the UI, duplicates the queue,
        changes some settings on each job and then renders that job. Between each
        render a light (which is spawned in at (0, 0, 50)) is modified to show an
        example of how to change something between each render.
        
        This shows how you would render with custom jobs/queue without disturbing
        what is already in the UI. If you wish to just execute with what is in the
        UI see the second example.
'''
def RenderQueue_ViaDuplication():
    # We are going to spawn a light into the world at the (0,0,0) point. If you have more than
    # one job in the queue, we will change the color of the light after each job to show how to
    # make variations of your render if desired.
    subsystem = unreal.get_editor_subsystem(unreal.MoviePipelineQueueSubsystem)
    pipelineQueue = subsystem.get_queue()
    if(len(pipelineQueue.get_jobs()) == 0):
        unreal.log_error("Open the Window > Movie Render Queue and add at least one job to use this example.")
        return
        
    # We're going to duplicate that queue (so that we don't modify the UI), this is also how you would make
    # a queue from scratch.
    global NewQueue
    NewQueue = unreal.MoviePipelineQueue()
    NewQueue.copy_from(pipelineQueue)
    
    # Ensure there's at least two jobs so we can show off the editor world change.
    if(len(NewQueue.get_jobs()) == 1):
        NewQueue.duplicate_job(NewQueue.get_jobs()[0])
        # If you wanted to make a job from scratch you would instead use:
        # DupJob = NewQueue.allocate_new_job(unreal.MoviePipelineExecutorJob)
        # and then fill out the settings of the new job by hand (see below)
        unreal.log("Duplicated first job to make a second one to show off multi-job editor world changes.")
        
    # For each job we're going to adjust the settings just to show you how to add and adjust settings.
    for job in NewQueue.get_jobs():
        # This is already set (because we duplicated the main queue) but this is how you set what sequence is rendered for this job
        # job.sequence = unreal.SoftObjectPath(levelSequencePath)
        job.author = "Example Python Script"
        # etc... see help(unreal.MoviePipelineExecutorJob) for more info.
        
        # Now we can configure the job. Calling find_or_add_setting_by_class is how you add new settings or find the existing one.
        outputSetting = job.get_configuration().find_or_add_setting_by_class(unreal.MoviePipelineOutputSetting)
        outputSetting.output_resolution = unreal.IntPoint(1280, 720)
        outputSetting.file_name_format = "{sequence_name}.{frame_number}"
        outputSetting.flush_disk_writes_per_shot = True # Required for the OnIndividualShotFinishedCallback to get called.
        
        # Ensure there is something to render. A job should have a render pass and a file output. 
        renderPass = job.get_configuration().find_or_add_setting_by_class(unreal.MoviePipelineDeferredPassBase)
        renderPass.disable_multisample_effects = True
        
        # Ensure there's a file output. Not all settings have adjustable settings, but anything in the UI should be in Python too.
        job.get_configuration().find_or_add_setting_by_class(unreal.MoviePipelineImageSequenceOutput_PNG)
    
    # spawn a light into the editor world. We store the path to this so we can manipulate it in the per-job-finished callback.
    global SpawnedPointLightActor
    SpawnedPointLightActor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PointLight, (0, 0, 100))
    SpawnedPointLightActor.set_brightness(5000)
    SpawnedPointLightActor.set_light_color((random.uniform(0, 1), random.uniform(0, 1), random.uniform(0, 1), 1))
    
    # Now that we've configured a queue, we can execute it with the PIE executor, which is the standard for
    # pressing Render (Local). 
    global NewExecutor
    NewExecutor = unreal.MoviePipelinePIEExecutor()
    NewExecutor.on_executor_finished_delegate.add_callable_unique(OnQueueFinishedCallback)
    NewExecutor.on_individual_job_work_finished_delegate.add_callable_unique(OnIndividualJobFinishedCallback) # Only available on PIE Executor
    NewExecutor.on_individual_shot_work_finished_delegate.add_callable_unique(OnIndividualShotFinishedCallback) # Only available on PIE executor
    NewExecutor.execute(NewQueue)

'''
    Summary:
        This example takes the queue that you have in the UI and renders the contents
        without changing any settings on each job. Between each render a light (which 
        is spawned in at (0, 0, 50)) is modified to show an example of how to change 
        something between each render.
        
        This example would be if you wanted to use a custom script to validate something
        in the user interface or otherwise have your script 'correct' the data the user 
        entered via the UI and have the changes stick. 
'''
def RenderQueue_InPlace():
    # We are going to spawn a light into the world at the (0,0,0) point. If you have more than
    # one job in the queue, we will change the color of the light after each job to show how to
    # make variations of your render if desired.
    subsystem = unreal.get_editor_subsystem(unreal.MoviePipelineQueueSubsystem)
    pipelineQueue = subsystem.get_queue()
    if(len(pipelineQueue.get_jobs()) == 0):
        unreal.log_error("Open the Window > Movie Render Queue and add at least one job to use this example.")
        return
            
    # Ensure there's at least two jobs so we can show off the editor world change.
    if(len(pipelineQueue.get_jobs()) == 1):
        pipelineQueue.duplicate_job(pipelineQueue.get_jobs()[0])
        unreal.log("Duplicated first job to make a second one to show off multi-job editor world changes.")
        
    for job in pipelineQueue.get_jobs():
        unreal.log("Validating job " + str(job))
            
    # spawn a light into the editor world. We store the path to this so we can manipulate it in the per-job-finished callback.
    global SpawnedPointLightActor
    SpawnedPointLightActor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PointLight, (0, 0, 100))
    SpawnedPointLightActor.set_brightness(5000)
    SpawnedPointLightActor.set_light_color((random.uniform(0, 1), random.uniform(0, 1), random.uniform(0, 1), 1))
    
    # This renders the queue that the subsystem belongs with the PIE executor, mimicking Render (Local)
    global NewExecutor
    NewExecutor = subsystem.render_queue_with_executor(unreal.MoviePipelinePIEExecutor)
    NewExecutor.on_executor_finished_delegate.add_callable_unique(OnQueueFinishedCallback)
    NewExecutor.on_individual_job_work_finished_delegate.add_callable_unique(OnIndividualJobFinishedCallback) # Only available on PIE Executor
    # Not implemented in this example because we don't set the appropriate output setting
    # NewExecutor.on_individual_shot_work_finished_delegate.add_callable_unique(OnIndividualShotFinishedCallback) 