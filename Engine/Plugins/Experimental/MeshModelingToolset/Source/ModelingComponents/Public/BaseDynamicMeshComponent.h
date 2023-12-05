// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/MeshComponent.h"
#include "InteractiveToolObjects.h"
#include "Changes/MeshVertexChange.h"
#include "Changes/MeshChange.h"
#include "Changes/MeshReplacementChange.h"
#include "MeshConversionOptions.h"
#include "UDynamicMesh.h"
#include "DynamicMesh3.h"
#include "TargetInterfaces/DynamicMeshCommitter.h"

#include "BaseDynamicMeshComponent.generated.h"

// predecl
class FDynamicMesh3;
class IMeshDescriptionCommitter;
class IMeshDescriptionProvider;
struct FMeshDescription;
class FMeshVertexChange;
class FBaseDynamicMeshSceneProxy;
class FMeshChange;
/**
 * EMeshRenderAttributeFlags is used to identify different mesh rendering attributes, for things
 * like fast-update functions
 */
enum class EMeshRenderAttributeFlags : uint8
{
	None = 0,
	Positions = 0x1,
	VertexColors = 0x2,
	VertexNormals = 0x4,
	VertexUVs = 0x8,
	SecondaryIndexBuffers = 0x16,
	All = 0xFF
};
ENUM_CLASS_FLAGS(EMeshRenderAttributeFlags);



/**
 * Tangent calculation modes
 */
UENUM()
enum class EDynamicMeshTangentCalcType : uint8
{
	/** Tangents are not used/available, proceed accordingly (eg generate arbitrary orthogonal basis) */
	NoTangents,
	/** Tangents will be automatically calculated on demand. Note that mesh changes due to tangents calculation will *not* be broadcast via MeshChange events! */
	AutoCalculated,
	/** Tangents are externally provided via the FDynamicMesh3 AttributeSet */
	ExternallyCalculated UMETA(DisplayName = "From Dynamic Mesh"),
	/** Tangents mode will be set to the most commonly-useful default -- currently "From Dynamic Mesh" */
	Default = 255
};

/**
 * Color Override Modes
 */
UENUM()
enum class EDynamicMeshComponentColorOverrideMode : uint8
{
	/** No Color Override enabled */
	None,
	/** Vertex Colors are displayed */
	VertexColors,
	/** Polygroup Colors are displayed */
	Polygroups,
	/** Constant Color is displayed */
	Constant
};


/**
 * Color Transform to apply to Vertex Colors when converting from internal DynamicMesh
 * Color attributes (eg Color Overlay stored in FVector4f) to RHI Render Buffers (FColor).
 *
 * Note that UStaticMesh assumes the Source Mesh colors are Linear and always converts to SRGB.
 */
UENUM()
enum class EDynamicMeshVertexColorTransformMode : uint8
{
	/** Do not apply any color-space transform to Vertex Colors */
	NoTransform,
	/** Assume Vertex Colors are in Linear space and transform to SRGB */
	LinearToSRGB,
	/** Assume Vertex Colors are in SRGB space and convert to Linear */
	SRGBToLinear
};

/**
 * UBaseDynamicMeshComponent is a base interface for a UMeshComponent based on a FDynamicMesh.
 * Currently no functionality lives here, only some interface functions are defined that various subclasses implement.
 */
UCLASS(hidecategories = (LOD, Physics, Collision), editinlinenew, ClassGroup = Rendering)
class MODELINGCOMPONENTS_API UBaseDynamicMeshComponent : 
	public UMeshComponent, 
	public IToolFrameworkComponent, 
	public IMeshVertexCommandChangeTarget, 
	public IMeshCommandChangeTarget, 
	public IMeshReplacementCommandChangeTarget
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * initialize the internal mesh from a MeshDescription
	 */
	virtual void InitializeMesh(FMeshDescription* MeshDescription)
	{
		unimplemented();
	}

	/**
	 * initialize the internal mesh from a DynamicMesh
	 */
	virtual void SetMesh(FDynamicMesh3&& MoveMesh)
	{
		unimplemented();
	}

	/**
	 * @return pointer to internal mesh
	 */
	virtual FDynamicMesh3* GetMesh()
	{
		unimplemented();
		return nullptr;
	}

	/**
	 * Allow external code to read the internal mesh.
	 */
	virtual void ProcessMesh(TFunctionRef<void(const FDynamicMesh3&)> ProcessFunc) const
	{
		unimplemented();
	}
	/**
	 * @return the child UDynamicMesh
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	virtual UDynamicMesh* GetDynamicMesh()
	{
		unimplemented();
		return nullptr;
	}

	/**
	 * @return pointer to internal mesh
	 */
	virtual const FDynamicMesh3* GetMesh() const
	{
		unimplemented();
		return nullptr;
	}

	/**
	 * Call this if you update the mesh via GetMesh()
	 * @todo should provide a function that calls a lambda to modify the mesh, and only return const mesh pointer
	 */
	virtual void NotifyMeshUpdated()
	{
		unimplemented();
	}

	/**
	 * Apply a vertex deformation change to the internal mesh  (implements IMeshVertexCommandChangeTarget)
	 */
	virtual void ApplyChange(const FMeshVertexChange* Change, bool bRevert) override
	{
		unimplemented();
	}

	/**
	 * Apply a general mesh change to the internal mesh  (implements IMeshCommandChangeTarget)
	 */
	virtual void ApplyChange(const FMeshChange* Change, bool bRevert) override
	{
		unimplemented();
	}

	/**
	* Apply a full mesh replacement change to the internal mesh  (implements IMeshReplacementCommandChangeTarget)
	*/
	virtual void ApplyChange(const FMeshReplacementChange* Change, bool bRevert) override
	{
		unimplemented();
	}


	//
	// Modification support
	//
public:

	virtual void ApplyTransform(const FTransform3d& Transform, bool bInvert)
	{
		unimplemented();
	}

	/**
	 * Write the internal mesh to a MeshDescription
	 * @param bHaveModifiedTopology if false, we only update the vertex positions in the MeshDescription, otherwise it is Empty()'d and regenerated entirely
	 * @param ConversionOptions struct of additional options for the conversion
	 */
	virtual void Bake(FMeshDescription* MeshDescription, bool bHaveModifiedTopology, const FConversionToMeshDescriptionOptions& ConversionOptions)
	{
		unimplemented();
	}



protected:

	/**
	 * Subclass must implement this to return scene proxy if available, or nullptr
	 */
	virtual FBaseDynamicMeshSceneProxy* GetBaseSceneProxy()
	{
		unimplemented();
		return nullptr;
	}

    /**
	 * Subclass must implement this to notify allocated proxies of updated materials
	 */
	virtual void NotifyMaterialSetUpdated()
	{
		unimplemented();
	}


	//===============================================================================================================
	// Built-in Color Rendering Support. When enabled, Color mode will override any assigned Materials.
	// VertexColor mode displays vertex colors, Polygroup mode displays mesh polygroups via vertex colors,
	// and Constant mode uses ConstantColor as the vertex color. The class-wide DefaultVertexColorMaterial
	// is used as the material that displays the vertex colors, and cannot be overridden per-instance
	// (the OverrideRenderMaterial can be used to do that)
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Mesh Component|Rendering", meta = (DisplayName = "Color Override"))
	EDynamicMeshComponentColorOverrideMode ColorMode = EDynamicMeshComponentColorOverrideMode::None;

	/**
	 * Configure the active Color Override
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component|Rendering")
	virtual void SetColorOverrideMode(EDynamicMeshComponentColorOverrideMode NewMode);

	/**
	 * @return active Color Override mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component|Rendering")
	virtual EDynamicMeshComponentColorOverrideMode GetColorOverrideMode() const { return ColorMode; }

	/**
	 * Constant Color used when Override Color Mode is set to Constant
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Mesh Component|Rendering", meta = (DisplayName = "Constant Color", EditCondition = "ColorMode==EDynamicMeshComponentColorOverrideMode::Constant"))
	FColor ConstantColor = FColor::White;

	/**
	 * Configure the Color used with Constant Color Override Mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component|Rendering")
	virtual void SetConstantOverrideColor(FColor NewColor);

	/**
	 * @return active Color used for Constant Color Override Mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component|Rendering")
	virtual FColor GetConstantOverrideColor() const { return ConstantColor; }

	/**
	 * Color Space Transform that will be applied to the colors stored in the DynamicMesh Attribute Color Overlay when
	 * constructing render buffers.
	 * Default is "No Transform", ie color R/G/B/A will be independently converted from 32-bit float to 8-bit by direct mapping.
	 * LinearToSRGB mode will apply SRGB conversion, ie assumes colors in the Mesh are in Linear space. This will produce the same behavior as UStaticMesh.
	 * SRGBToLinear mode will invert SRGB conversion, ie assumes colors in the Mesh are in SRGB space.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Mesh Component|Rendering", meta = (DisplayName = "Vertex Color Space"))
	EDynamicMeshVertexColorTransformMode ColorSpaceMode = EDynamicMeshVertexColorTransformMode::NoTransform;

	/**
	 * Configure the active Color Space Transform Mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component|Rendering")
	virtual void SetVertexColorSpaceTransformMode(EDynamicMeshVertexColorTransformMode NewMode);

	/**
	 * @return active Color Override mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component|Rendering")
	virtual EDynamicMeshVertexColorTransformMode GetVertexColorSpaceTransformMode() const { return ColorSpaceMode; }



public:

	/**
	 * Configure whether wireframe rendering is enabled or not
	 */
	virtual void SetEnableWireframeRenderPass(bool bEnable) { check(false); }

	/**
	 * @return true if wireframe rendering pass is enabled (default false)
	 */
	virtual bool EnableWireframeRenderPass() const { return false; }


	//
	// Override rendering material support
	//

	/**
	 * Set an active override render material. This should replace all materials during rendering.
	 */
	virtual void SetOverrideRenderMaterial(UMaterialInterface* Material);

	/**
	 * Clear any active override render material
	 */
	virtual void ClearOverrideRenderMaterial();
	
	/**
	 * @return true if an override render material is currently enabled for the given MaterialIndex
	 */
	virtual bool HasOverrideRenderMaterial(int k) const
	{
		return OverrideRenderMaterial != nullptr;
	}

	/**
	 * @return active override render material for the given MaterialIndex
	 */
	virtual UMaterialInterface* GetOverrideRenderMaterial(int MaterialIndex) const
	{
		return OverrideRenderMaterial;
	}




public:

	//
	// secondary buffer support
	//

	/**
	 * Set an active override render material. This should replace all materials during rendering.
	 */
	virtual void SetSecondaryRenderMaterial(UMaterialInterface* Material);

	/**
	 * Clear any active override render material
	 */
	virtual void ClearSecondaryRenderMaterial();

	/**
	 * @return true if an override render material is currently enabled for the given MaterialIndex
	 */
	virtual bool HasSecondaryRenderMaterial() const
	{
		return SecondaryRenderMaterial != nullptr;
	}

	/**
	 * @return active override render material for the given MaterialIndex
	 */
	virtual UMaterialInterface* GetSecondaryRenderMaterial() const
	{
		return SecondaryRenderMaterial;
	}


	/**
	 * Show/Hide the secondary triangle buffers. Does not invalidate SceneProxy.
	 */
	virtual void SetSecondaryBuffersVisibility(bool bVisible);

	/**
	 * @return true if secondary buffers are currently set to be visible
	 */
	virtual bool GetSecondaryBuffersVisibility() const;


protected:

	TArray<UMaterialInterface*> BaseMaterials;

	UMaterialInterface* OverrideRenderMaterial = nullptr;
	UMaterialInterface* SecondaryRenderMaterial = nullptr;

	bool bDrawSecondaryBuffers = true;

	//===============================================================================================================
	// Raytracing support. Must be enabled for various rendering effects. 
	// However, note that in actual "dynamic" contexts (ie where the mesh is changing every frame),
	// enabling Raytracing support has additional renderthread performance costs and does
	// not currently support partial updates in the SceneProxy.
public:

	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	virtual void SetShadowsEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	virtual bool GetShadowsEnabled() const { return CastShadow; }

	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	virtual void SetViewModeOverridesEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	virtual bool GetViewModeOverridesEnabled() const { return bEnableViewModeOverrides; }

	/**
	 * This flag controls whether Editor View Mode Overrides are enabled for this mesh. For example, this controls hidden-line removal on the wireframe
	 * in Wireframe View Mode, and whether the normal map will be disabled in Lighting-Only View Mode, as well as various other things.
	 * Use SetViewModeOverridesEnabled() to control this setting in Blueprints/C++.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Mesh Component|Rendering", meta = (DisplayName = "View Mode Overrides"))
	bool bEnableViewModeOverrides = true;

	/**
	 * Enable/disable Raytracing support on this Mesh, if Raytracing is currently enabled in the Project Settings.
	 * Use SetEnableRaytracing() to configure this flag in Blueprints/C++.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Mesh Component|Rendering")
	bool bEnableRaytracing = true;

	/**
	 * Enable/Disable raytracing support. This is an expensive call as it flushes
	 * the rendering queue and forces an immediate rebuild of the SceneProxy.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	virtual void SetEnableRaytracing(bool bSetEnabled);

	/**
	 * @return true if raytracing support is currently enabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	virtual bool GetEnableRaytracing() const;

protected:
	virtual void OnRenderingStateChanged(bool bForceImmedateRebuild);

public:


	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UMeshComponent Interface.

};

namespace UE {
	namespace DyVIA {

		MODELINGCOMPONENTS_API FDynamicMesh3 GetDynamicMeshViaMeshDescription(
			IMeshDescriptionProvider& MeshDescriptionProvider);

		MODELINGCOMPONENTS_API void CommitDynamicMeshViaMeshDescription(
			FMeshDescription&& CurrentMeshDescription,
			IMeshDescriptionCommitter& MeshDescriptionCommitter,
			const FDynamicMesh3& Mesh,
			const IDynamicMeshCommitter::FDynamicMeshCommitInfo& CommitInfo);

	}
}
