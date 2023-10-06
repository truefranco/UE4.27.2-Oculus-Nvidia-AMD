"""
Simple launch script for converting a cascade system to a niagara system.

Args:
    cascade_system_asset_path (FString): The path to the cascade system asset
        to convert to a new niagara system.
"""
import sys
import unreal as ue
import CascadeToNiagaraConverter

if len(sys.argv) != 3:
    raise NameError("Expected 2 arguments; Path to cascade particle system "
                    "asset and path to conversion results object, but "
                    "received " + str(len(sys.argv)) + "!")

cascade_system = ue.load_asset(sys.argv[1])
conversion_results = ue.load_object(None, sys.argv[2])
CascadeToNiagaraConverter.convert_cascade_to_niagara(
    cascade_system,
    conversion_results
)
