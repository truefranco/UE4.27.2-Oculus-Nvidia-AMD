from RendererConverterInterface import RendererConverterInterface
import unreal as ue
from unreal import FXConverterUtilitiesLibrary as ue_fx_utils
import CascadeToNiagaraHelperMethods as c2n_utils


class SpriteConverter(RendererConverterInterface):

    @classmethod
    def get_input_cascade_module(cls):
        return None

    @classmethod
    def convert(cls, cascade_typedata_module, cascade_required_module,
                niagara_emitter_context):

        # create a new sprite renderer properties and get the properties from
        # the cascade required module
        sprite_renderer_props = ue.NiagaraSpriteRendererProperties()
        
        (material_interface, screen_alignment, subimages_horizontal, 
         subimages_vertical, sort_mode, interpolation_method, b_remove_hmd_roll, 
         min_facing_camera_blend_distance, max_facing_camera_blend_distance,
         material_cutout_texture, bounding_mode, opacity_source_mode, 
         normal_facing_mode, alpha_threshold) = \
            ue_fx_utils.get_particle_module_required_per_renderer_props(
                cascade_required_module
            )

        sprite_renderer_props.set_editor_property(
            "Material", material_interface)

        sprite_alignment, sprite_facingmode = (
            c2n_utils.get_alignment_and_facingmode_for_screenalignment(screen_alignment))
        sprite_renderer_props.set_editor_property("Alignment", sprite_alignment)
        sprite_renderer_props.set_editor_property("FacingMode", sprite_facingmode)

        # skip setting PivotInUVSpace as the default value (0.5, 0.5) is desired.
        # niagara_renderer_props.set_editor_property("PivotInUVSpace")

        # get the niagara sort mode and binding from the cascade equivalent sort mode
        niagara_sort_mode, sort_binding = (
            c2n_utils.get_niagara_sortmode_and_binding_for_sortmode(sort_mode))
        sprite_renderer_props.set_editor_property("SortMode", niagara_sort_mode)
        if sort_binding is not None:
            niagara_emitter_context.set_renderer_binding(
                sprite_renderer_props,
                "CustomSortingBinding",
                sort_binding,
                ue.NiagaraRendererSourceDataMode.PARTICLES)

        # set the niagara subimage size as the composition of the cascade horizontal
        # and vertical subimages
        subimage_size = ue.Vector2D(subimages_horizontal, subimages_vertical)
        sprite_renderer_props.set_editor_property("SubImageSize", subimage_size)

        # get the niagara subimage blend enabled state and binding from the cascade
        # equivalent interpolation method
        subimage_blend, subimage_binding = (
            c2n_utils.get_enable_subimage_blend_and_binding_for_interpolation_method(
                interpolation_method))
        sprite_renderer_props.set_editor_property("bSubImageBlend", subimage_blend)
        if subimage_binding is not None:
            #  todo implement UV subimage modes
            niagara_emitter_context.log("Tried to set subimage binding for "
                                        "sprite renderer but custom subimage "
                                        "binding conversion is not supported.",
                                        ue.NiagaraMessageSeverity.WARNING)
            # sprite_renderer_props.set_editor_property(
            #    "SubImageIndexBinding", subimage_binding)

        sprite_renderer_props.set_editor_property(
            "bRemoveHMDRollInVR", b_remove_hmd_roll)

        # skip setting SortOnlyWhenTranslucent as the default value True is desired.
        # sprite_renderer_props.set_editor_property("SortOnlyWhenTranslucent")

        sprite_renderer_props.set_editor_property(
            "MinFacingCameraBlendDistance", min_facing_camera_blend_distance)

        sprite_renderer_props.set_editor_property(
            "MaxFacingCameraBlendDistance", max_facing_camera_blend_distance)

        if material_cutout_texture is not None:
            sprite_renderer_props.set_editor_property(
                "bUseMaterialCutoutTexture", False)
            sprite_renderer_props.set_editor_property(
                "CutoutTexture", material_cutout_texture)

        sprite_renderer_props.set_editor_property(
            "BoundingMode", bounding_mode)

        sprite_renderer_props.set_editor_property(
            "OpacitySourceMode", opacity_source_mode)
        
        if normal_facing_mode != ue.EmitterNormalsMode.ENM_CAMERA_FACING:
            niagara_emitter_context.log(
                "Failed to copy sprite renderer \"Emitter Normal Mode\": "
                "Niagara sprite renderer does not support this. ",
                ue.NiagaraMessageSeverity.ERROR
            )
        
        # set the opacity alpha threshold
        sprite_renderer_props.set_editor_property(
            "AlphaThreshold", alpha_threshold)

        # add the renderer properties
        niagara_emitter_context.add_renderer(
            "SpriteRenderer", sprite_renderer_props)
