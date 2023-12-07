// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

class UTexture;
class UInteractiveToolManager;

class UToolTarget;
class UBaseDynamicMeshComponent;
class UPreviewMesh;


/**
 * Utility functions for Tool implementations to use when doing configuration/setup
 */
namespace ToolSetupUtil
{
	/**
	 * Get the default material for surfaces
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetDefaultMaterial();


	/**
	 * Get the default material to use for objects in an InteractiveTool. Optionally use SourceMaterial if it is valid.
	 * @param SourceMaterial optional material to use if available
	 * @return default material to use for objects in a tool.
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetDefaultMaterial(UInteractiveToolManager* ToolManager, UMaterialInterface* SourceMaterial = nullptr);

	/**
	 * @return configurable vertex color material
	 */
	MODELINGCOMPONENTS_API UMaterialInstanceDynamic* GetVertexColorMaterial(UInteractiveToolManager* ToolManager);


	/**
	 * @return default material to use for "Working"/In-Progress animations
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetDefaultWorkingMaterial(UInteractiveToolManager* ToolManager);


	/**
	 * Get a black-and-white NxN checkerboard material
	 * @param CheckerDensity Number of checks along row/column
	 * @return default material to use for uv checkerboard visualizations
	 */
	MODELINGCOMPONENTS_API UMaterialInstanceDynamic* GetUVCheckerboardMaterial(double CheckerDensity = 20.0);


	/**
	 * @return default material to use for brush volume indicators
	 */
	MODELINGCOMPONENTS_API UMaterialInstanceDynamic* GetDefaultBrushVolumeMaterial(UInteractiveToolManager* ToolManager);


	/**
	 * @return Sculpt Material 1
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetDefaultSculptMaterial(UInteractiveToolManager* ToolManager);


	/** Types of image-based material that we can create */
	enum class ImageMaterialType
	{
		DefaultBasic,
		DefaultSoft,
		TangentNormalFromView
	};

	/**
	 * @return Image-based sculpt material instance, based ImageMaterialType
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetImageBasedSculptMaterial(UInteractiveToolManager* ToolManager, ImageMaterialType Type);

	/**
	 * @return Image-based sculpt material that supports changing the image
	 */
	MODELINGCOMPONENTS_API UMaterialInstanceDynamic* GetCustomImageBasedSculptMaterial(UInteractiveToolManager* ToolManager, UTexture* SetImage);


	/**
	 * @return Selection Material 1
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetSelectionMaterial(UInteractiveToolManager* ToolManager);

	/**
	 * @return Selection Material 1 with custom color and optional depth offset (depth offset moves vertices towards the camera)
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetSelectionMaterial(const FLinearColor& UseColor, UInteractiveToolManager* ToolManager, float PercentDepthOffset = 0.0f);

	/**
	 * @return Simple material with configurable color and opacity.
	 */
	MODELINGCOMPONENTS_API UMaterialInstanceDynamic* GetSimpleCustomMaterial(UInteractiveToolManager* ToolManager, const FLinearColor& Color, float Opacity);

	/**
	 * @param bRoundPoints true for round points, false for square
	 * @return custom material suitable for use with UPointSetComponent
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetDefaultPointComponentMaterial(bool bRoundPoints, UInteractiveToolManager* ToolManager);

	/**
	 * @return custom material suitable for use with ULineSetComponent
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetDefaultLineComponentMaterial(UInteractiveToolManager* ToolManager, bool bDepthTested = true);

	/**
	 * @return a curve asset used for contrast adjustments when using a texture map for displacements.
	 */
	MODELINGCOMPONENTS_API UCurveFloat* GetContrastAdjustmentCurve(UInteractiveToolManager* ToolManager);


	//
	// Rendering Configuration/Setup Functions
	// These utility functions are used to configure rendering settings on Preview Meshes created internally by Modeling Tools.
	// 
	//

	//MODELINGCOMPONENTS_API void ApplyRenderingConfigurationToPreview(UBaseDynamicMeshComponent* Component, TUniquePtr<FPrimitiveComponentTarget> SourceTarget = nullptr);
	MODELINGCOMPONENTS_API void ApplyRenderingConfigurationToPreview(UBaseDynamicMeshComponent* Component, UToolTarget* SourceTarget = nullptr);
	MODELINGCOMPONENTS_API void ApplyRenderingConfigurationToPreview(UPreviewMesh* PreviewMesh, UToolTarget* SourceTarget = nullptr);


	//
	// Rendering Configuration/Setup Functions
	// These utility functions are used to configure rendering settings on Preview Meshes created internally by Modeling Tools.
	// 
	//

	/**
	 * @return Material used when editing AVolume objects using our tools.
	 */
	MODELINGCOMPONENTS_API UMaterialInterface* GetDefaultEditVolumeMaterial();

	/**
	 * @return Simple material with configurable depth offset, color, and opacity. Note that the material
	 *  will have translucent blend mode, which can interact poorly with overlapping translucent
	 *  objects, so use the other overload if you do not need opacity control.
	 */
	MODELINGCOMPONENTS_API UMaterialInstanceDynamic* GetCustomDepthOffsetMaterial(UInteractiveToolManager* ToolManager, const FLinearColor& Color, float PercentDepthOffset, float Opacity);

	/**
	 * @return Simple material with configurable depth offset and color. The material will have opaque blend mode.
	 */
	MODELINGCOMPONENTS_API UMaterialInstanceDynamic* GetCustomDepthOffsetMaterial(UInteractiveToolManager* ToolManager, const FLinearColor& Color, float PercentDepthOffset);

}