# The USD 21.05 update introduced a breaking change where most of the attributes of UsdLux light prims were renamed.
# They went from being something like this:
#   def RectLight "RectLight"
#   {
#       color3f color = (1, 1, 1)
#       float colorTemperature = 6500
#       bool enableColorTemperature = 0
#       float exposure = 0
#       float height = 64
#       float intensity = 19.53125
#       token visibility = "inherited"
#       float width = 64
#   }
# To something like this:
#   def RectLight "RectLight"
#   {
#       color3f inputs:color = (1, 1, 1)
#       float inputs:colorTemperature = 6500
#       bool inputs:enableColorTemperature = 0
#       float inputs:exposure = 0
#       float inputs:height = 64
#       float inputs:intensity = 19.53125
#       float inputs:width = 64
#   }
# Essentially just gaining a "inputs:" prefix for some attributes.
#
# This script will open a stage, rename these prims, and save the stage's layers in place. You can use it like this:
#   $ python "path/to/update_lights_to_21_05.py" "C:/path/to/root_layer.usda" [--v] [--r]
#
# WARNING: USD is very complex and its likely impossible to determine precisely whether we should rename an attribute or not 
# (e.g. if the script is called directly on a sublayer that has light attributes authored inside 'over' prims without schema). The script
# will try to err on the side of caution and only rename attributes it can confirm belong to an UsdLux light prim, but it's still
# very much recommended to back up your stage before applying the script!
#
# The recommended way of using this script is to just run it directly from UE's Output Log. For that, follow the guide linked below, and 
# then just run the script from the Output Log like the usage example above.
#   - https://docs.unrealengine.com/en-US/ProductionPipelines/ScriptingAndAutomation/Python/index.html
#
# Note that this script will traverse sublayers and variant sets (when given the --v argument), but will not traverse referenced
# layers or payloads. You can just invoke the script on the referenced/payload usd files as if they were root layers of a stage,
# however.
#
# You can also provide an optional --r argument to revert the attribute name convention to before 21.05, essentially removing the
# "inputs:" prefixes.

from pxr import Usd, Sdf, UsdLux
import argparse


def rename_spec(layer, edit, prim_path, attribute, prefix_before, prefix_after):
    path_before = prim_path.AppendProperty(prefix_before + attribute)
    path_after = prim_path.AppendProperty(prefix_after + attribute)

    # We must check every time, because adding a namespace edit that can't be applied will just cancel the whole batch
    if layer.GetAttributeAtPath(path_before):
        print(f"Trying to rename '{path_before}' to '{path_after}'")
        edit.Add(path_before, path_after)


def rename_specs(layer, edit, prim_path, reverse=False):
    prefix_before = 'inputs:' if reverse else ''
    prefix_after = '' if reverse else 'inputs:'

    # Light
    rename_spec(layer, edit, prim_path, 'intensity', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'exposure', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'diffuse', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'specular', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'normalize', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'color', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'enableColorTemperature', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'colorTemperature', prefix_before, prefix_after)

    # ShapingAPI
    rename_spec(layer, edit, prim_path, 'shaping:focus', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shaping:focusTint', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shaping:cone:angle', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shaping:cone:softness', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shaping:ies:file', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shaping:ies:angleScale', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shaping:ies:normalize', prefix_before, prefix_after)

    # ShadowAPI
    rename_spec(layer, edit, prim_path, 'shadow:enable', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shadow:color', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shadow:distance', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shadow:falloff', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'shadow:falloffGamma', prefix_before, prefix_after)

    # DistantLight
    rename_spec(layer, edit, prim_path, 'angle', prefix_before, prefix_after)

    # DiskLight, SphereLight, CylinderLight
    # Note: treatAsPoint should not have the 'inputs:' prefix so we ignore it
    rename_spec(layer, edit, prim_path, 'radius', prefix_before, prefix_after)

    # RectLight
    rename_spec(layer, edit, prim_path, 'width', prefix_before, prefix_after)
    rename_spec(layer, edit, prim_path, 'height', prefix_before, prefix_after)

    # CylinderLight
    rename_spec(layer, edit, prim_path, 'length', prefix_before, prefix_after)

    # RectLight, DomeLight
    rename_spec(layer, edit, prim_path, 'texture:file', prefix_before, prefix_after)

    # DomeLight
    rename_spec(layer, edit, prim_path, 'texture:format', prefix_before, prefix_after)


def collect_light_prims(prim_path, prim, traverse_variants, light_prim_paths, visited_paths):
    if not prim:
        return

    if prim_path in visited_paths:
        return
    visited_paths.add(prim_path)

    # Traverse manually because we may flip between variants, which would invalidate the stage.Traverse() iterator
    for child in prim.GetChildren():

        # e.g. /Root/Prim/Child
        child_path = prim_path.AppendChild(child.GetName())

        if UsdLux.Light(child):
            light_prim_paths.add(child_path)

        traversed_grandchildren = False
        if traverse_variants:
            varsets = child.GetVariantSets()
            for varset_name in varsets.GetNames():
                varset = varsets.GetVariantSet(varset_name)
                original_selection = varset.GetVariantSelection() if varset.HasAuthoredVariantSelection() else None

                # Switch selections only on the session layer
                with Usd.EditContext(prim.GetStage(), prim.GetStage().GetSessionLayer()):
                    for variant_name in varset.GetVariantNames():
                        varset.SetVariantSelection(variant_name)

                        # e.g. /Root/Prim/Child{VarName=Var}
                        varchild_path = child_path.AppendVariantSelection(varset_name, variant_name)

                        collect_light_prims(varchild_path, child, traverse_variants, light_prim_paths, visited_paths)
                        traversed_grandchildren = True

                        if original_selection:
                            varset.SetVariantSelection(original_selection)
                        else:
                            varset.ClearVariantSelection()

        if not traversed_grandchildren:
            collect_light_prims(child_path, child, traverse_variants, light_prim_paths, visited_paths)


def update_lights_on_stage(stage_root, traverse_variants=False, reverse=False):
    """ Traverses the stage with root layer `stage_root`, updating attributes of light prims to/from USD 21.05.

    The approach here involves traversing the composed stage and collecting paths to prims that are UsdLux lights
    (flipping through variants or not according to the input argument), and later on traverse all the stage's 
    layers and renaming all specs of of light prim attributes to 21.05 (by adding the 'inputs:' prefix) or to
    the schema before 21.05 (by removing the 'inputs:' prefix).

    We traverse the composed stage first to make sure we're modifying exclusively UsdLux light prim attributes,
    avoiding modifications to a Sphere's "radius" attribute, for example.
    """
    stage = Usd.Stage.Open(stage_root, Usd.Stage.LoadAll)
    layers_to_traverse = stage.GetUsedLayers(True)

    # Collect UsdLux prims on the composed stage
    light_prim_paths = set()
    visited_paths = set()
    collect_light_prims(Sdf.Path("/"), stage.GetPseudoRoot(), traverse_variants, light_prim_paths, visited_paths)

    print("Collected light prims:")
    for l in light_prim_paths:
        print(f"\t{l}")

    # Traverse all layers, and rename all relevant attributes of light prims
    visited_paths = set()
    for layer in layers_to_traverse:
        # Batch all rename operations for this layer in a single namespace edit
        edit = Sdf.BatchNamespaceEdit()

        def visit_path(path):
            attr_spec = layer.GetAttributeAtPath(path)
            if attr_spec:
                prim_path = attr_spec.owner.path

                # Only visit each prim once, as we'll handle all UsdLux properties in one go
                if prim_path in visited_paths:
                    return
                visited_paths.add(prim_path)

                if prim_path in light_prim_paths:
                    rename_specs(layer, edit, prim_path, reverse)

        layer.Traverse("/", visit_path)

        if len(edit.edits) == 0:
            print(f"Nothing to rename on layer '{layer.identifier}'")
        else:
            if layer.CanApply(edit):
                layer.Apply(edit)
                print(f"Applied change to layer '{layer.identifier}'")
            else:
                print(f"Failed to apply change to layer '{layer.identifier}'")

    # Save all layers
    for layer in layers_to_traverse:
        if not layer.anonymous:
            layer.Save()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Update light prims to USD 21.05')
    parser.add_argument('root_layer_path', type=str,
                        help='Full path to the root layer of the stage to update e.g. "C:/MyFolder/MyLevel.usda"')
    parser.add_argument('--v', '--traverse_variants', dest='traverse_variants', action='store_true',
                        help='Whether to also flip through every variant in every variant set encountered when looking for light prims')
    parser.add_argument('--r', '--reverse', dest='reverse', action='store_true',
                        help='Optional argument to do the reverse change instead: Rename 21.05 UsdLux light attributes so that they follow the schema from before 21.05')
    args = parser.parse_args()

    update_lights_on_stage(args.root_layer_path, args.traverse_variants, args.reverse)
