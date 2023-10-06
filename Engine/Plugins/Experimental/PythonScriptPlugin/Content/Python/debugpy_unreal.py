# Copyright Epic Games, Inc. All Rights Reserved.

'''
    Utility script to debug the embedded UnrealEngine Python interpreter using debugpy in VS Code.

    Usage:
        1) import debugpy_unreal into Python within the UE Editor.
        2) If debugpy has not yet been installed, run debugpy_unreal.install_debugpy().
        3) Run debugpy_unreal.start() to begin a debug session, using the same port that you will use to attach your debugger (5678 is the default for VS Code).
        4) Attach your debugger from VS Code (see the "launch.json" configuration below).
        5) Run debugpy_unreal.breakpoint() to instruct the attached debugger to breakpoint on the next statement executed.
           This can be run prior to running any script you want to debug from VS Code, as long as VS Code is still attached and script execution was continued.
    
    VS Code "launch.json" Configuration:
        {
            "name": "UnrealEngine Python",
            "type": "python",
            "request": "attach",
            "connect": {
                "host": "localhost",
                "port": 5678
            },
            "redirectOutput": true
        }
    
    Notes:
        * redirectOutput must be set to true in your debugger configuration to avoid hanging both the debugger and the UE Editor.
        * You may see some errors for "This version of python seems to be incorrectly compiled"; these can be ignored.
'''

import unreal
_PYTHON_INTERPRETER_PATH = unreal.get_interpreter_executable_path()

def install_debugpy():
    '''Attempt to install the debugpy module using pip'''
    import subprocess
    subprocess.check_call([_PYTHON_INTERPRETER_PATH, '-m', 'ensurepip'])
    subprocess.check_call([_PYTHON_INTERPRETER_PATH, '-m', 'pip', 'install', '-q', '--no-warn-script-location', 'debugpy'])

def start(port=5678):
    '''Start a debugging session on the given port and wait for a debugger to attach'''
    try:
        import debugpy
    except ImportError:
        print('Failed to import debugpy! Use debugpy_unreal.install_debugpy() to attempt installation.')

    # Need to set this otherwise debugpy tries to run another UE instance as an adapter
    # This has it run the Python interpreter executable instead
    debugpy.configure(python=_PYTHON_INTERPRETER_PATH)

    debugpy.listen(port)
    print('Waiting for a debugger to attach...')
    debugpy.wait_for_client()
    print('Debugger attached! Use debugpy_unreal.breakpoint() to break into the debugger.')

def breakpoint():
    '''Instruct the attached debugger to breakpoint on the next statement executed'''
    import debugpy
    debugpy.breakpoint()
