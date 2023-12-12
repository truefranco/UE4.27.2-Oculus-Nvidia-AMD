// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseDynamicMeshComponent.h"
#include "MeshConversionOptions.h"
#include "Drawing/MeshRenderDecomposition.h"
#include "MeshTangents.h"
#include "TransformTypes.h"
#include "UDynamicMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "SimpleDynamicMeshComponent.generated.h"

// predecl
struct FMeshDescription;

/** internal FPrimitiveSceneProxy defined in SimpleDynamicMeshSceneProxy.h */
class FSimpleDynamicMeshSceneProxy;
class FBaseDynamicMeshSceneProxy;

/**
 * Interface for a render mesh processor. Use this to process the Mesh stored in UDynamicMeshComponent before
 * sending it off for rendering.
 * NOTE: This is called whenever the Mesh is updated and before rendering, so performance matters.
 */
class IRenderMeshPostProcessor
{
public:
	virtual ~IRenderMeshPostProcessor() = default;

	virtual void ProcessMesh(const FDynamicMesh3& Mesh, FDynamicMesh3& OutRenderMesh) = 0;
};

/** Render data update hint */
UENUM()
enum class EDynamicMeshComponentRenderUpdateMode
{
	/** Do not update render data */
	NoUpdate = 0,
	/** Invalidate overlay of internal component, rebuilding all render data */
	FullUpdate = 1,
	/** Attempt to do partial update of render data if possible */
	FastUpdate = 2
};


/** 
 * USimpleDynamicMeshComponent is a mesh component similar to UProceduralMeshComponent,
 * except it bases the renderable geometry off an internal FDynamicMesh3 instance.
 * 
 * There is some support for undo/redo on the component (@todo is this the right place?)
 * 
 * This component draws wireframe-on-shaded when Wireframe is enabled, or when bExplicitShowWireframe = true
 *
 */
UCLASS(hidecategories = (LOD, Physics, Collision), editinlinenew, ClassGroup = Rendering)
class MODELINGCOMPONENTS_API USimpleDynamicMeshComponent : public UBaseDynamicMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * initialize the internal mesh from a MeshDescription
	 */
	virtual void InitializeMesh(FMeshDescription* MeshDescription) override;

	/**
	 * @return pointer to internal mesh
	 */
	virtual FDynamicMesh3* GetMesh() override { return MeshObject->GetMeshPtr(); }

	/**
	 * @return pointer to internal mesh
	 */
	virtual const FDynamicMesh3* GetMesh() const override { return MeshObject->GetMeshPtr(); }


	/**
	 * @return the child UDynamicMesh
	 */
	 //UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	virtual UDynamicMesh* GetDynamicMesh() override { return MeshObject; }

	/**
	 * Replace the current UDynamicMesh with a new one, and transfer ownership of NewMesh to this Component.
	 * This can be used to (eg) assign a UDynamicMesh created with NewObject in the Transient Package to this Component.
	 * @warning If NewMesh is owned/Outer'd to another DynamicMeshComponent, a GLEO error may occur if that Component is serialized.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	void SetDynamicMesh(UDynamicMesh* NewMesh);

	/**
	 * initialize the internal mesh from a DynamicMesh
	 */
	virtual void SetMesh(FDynamicMesh3&& MoveMesh) override;

	/**
	 * Allow external code to read the internal mesh.
	 */
	virtual void ProcessMesh(TFunctionRef<void(const FDynamicMesh3&)> ProcessFunc) const;

	/**
	 * Allow external code to to edit the internal mesh.
	 */
	virtual void EditMesh(TFunctionRef<void(FDynamicMesh3&)> EditFunc,
		EDynamicMeshComponentRenderUpdateMode UpdateMode = EDynamicMeshComponentRenderUpdateMode::FullUpdate);

public:
	/**
	 * @return the current internal mesh, which is replaced with an empty mesh
	 */
	TUniquePtr<FDynamicMesh3> ExtractMesh(bool bNotifyUpdate);

	/**
	 * Copy externally-calculated tangents into the internal tangets buffer.
	 * @param bFastUpdateIfPossible if true, will try to do a fast normals/tangets update of the SceneProxy, instead of full invalidatiohn
	 */
	void UpdateTangents(const FMeshTangentsf* ExternalTangents, bool bFastUpdateIfPossible);

	/**
	 * Copy externally-calculated tangents into the internal tangets buffer.
	 * @param bFastUpdateIfPossible if true, will try to do a fast normals/tangets update of the SceneProxy, instead of full invalidatiohn
	 */
	void UpdateTangents(const FMeshTangentsd* ExternalTangents, bool bFastUpdateIfPossible);


	/**
	 * @return pointer to internal tangents object. 
	 * @warning calling this with TangentsType = AutoCalculated will result in possibly-expensive Tangents calculation
	 * @warning this is only currently safe to call on the Game Thread!!
	 */
	const FMeshTangentsf* GetTangents();




	/**
	 * Write the internal mesh to a MeshDescription
	 * @param bHaveModifiedTopology if false, we only update the vertex positions in the MeshDescription, otherwise it is Empty()'d and regenerated entirely
	 * @param ConversionOptions struct of additional options for the conversion
	 */
	virtual void Bake(FMeshDescription* MeshDescription, bool bHaveModifiedTopology, const FConversionToMeshDescriptionOptions& ConversionOptions) override;

	/**
	* Write the internal mesh to a MeshDescription with default conversion options
	* @param bHaveModifiedTopology if false, we only update the vertex positions in the MeshDescription, otherwise it is Empty()'d and regenerated entirely
	*/
	void Bake(FMeshDescription* MeshDescription, bool bHaveModifiedTopology)
	{
		FConversionToMeshDescriptionOptions ConversionOptions;
		Bake(MeshDescription, bHaveModifiedTopology, ConversionOptions);
	}

	/**
	 * Apply transform to internal mesh. Updates Octree and RenderProxy if available.
	 * @param bInvert if true, inverse tranform is applied instead of forward transform
	 */
	virtual void ApplyTransform(const FTransform3d& Transform, bool bInvert) override;

protected:
	/**
	 * Internal FDynamicMesh is stored inside a UDynamicMesh container, which allows it to be
	 * used from BP, shared with other UObjects, and so on
	 */
	UPROPERTY(Instanced)
	UDynamicMesh* MeshObject;
	//
	// change tracking/etc
	//
public:
	/**
	 * Call this if you update the mesh via GetMesh(). This will destroy the existing RenderProxy and create a new one.
	 * @todo should provide a function that calls a lambda to modify the mesh, and only return const mesh pointer
	 */
	virtual void NotifyMeshUpdated() override;

	/**
	 * Call this instead of NotifyMeshUpdated() if you have only updated the vertex colors (or triangle color function).
	 * This function will update the existing RenderProxy buffers if possible
	 */
	void FastNotifyColorsUpdated();

	/**
	 * Call this instead of NotifyMeshUpdated() if you have only updated the vertex positions (and possibly some attributes).
	 * This function will update the existing RenderProxy buffers if possible
	 */
	void FastNotifyPositionsUpdated(bool bNormals = false, bool bColors = false, bool bUVs = false);

	/**
	 * Call this instead of NotifyMeshUpdated() if you have only updated the vertex attributes (but not positions).
	 * This function will update the existing RenderProxy buffers if possible, rather than create new ones.
	 */
	void FastNotifyVertexAttributesUpdated(bool bNormals, bool bColors, bool bUVs);

	/**
	 * Call this instead of NotifyMeshUpdated() if you have only updated the vertex positions/attributes
	 * This function will update the existing RenderProxy buffers if possible, rather than create new ones.
	 */
	void FastNotifyVertexAttributesUpdated(EMeshRenderAttributeFlags UpdatedAttributes);

	/**
	 * Call this instead of NotifyMeshUpdated() if you have only updated the vertex uvs.
	 * This function will update the existing RenderProxy buffers if possible
	 */
	void FastNotifyUVsUpdated();

	/**
	 * Call this instead of NotifyMeshUpdated() if you have only updated secondary triangle sorting.
	 * This function will update the existing buffers if possible, without rebuilding entire RenderProxy.
	 */
	void FastNotifySecondaryTrianglesChanged();

	/**
	 * This function updates vertex positions/attributes of existing SceneProxy render buffers if possible, for the given triangles.
	 * If a FMeshRenderDecomposition has not been explicitly set, call is forwarded to FastNotifyVertexAttributesUpdated()
	 */
	void FastNotifyTriangleVerticesUpdated(const TArray<int32>& Triangles, EMeshRenderAttributeFlags UpdatedAttributes);

	/**
	 * If a Decomposition is set on this Component, and everything is currently valid (proxy/etc), precompute the set of
	 * buffers that will be modified, as well as the bounds of the modified region. These are both computed in parallel.
	 * Use FastNotifyTriangleVerticesUpdated_ApplyPrecompute() with the returned future to apply this precomputation.
	 * @return a future that will (eventually) return true if the precompute is OK, and (immediately) false if it is not
	 */
    TFuture<bool> FastNotifyTriangleVerticesUpdated_TryPrecompute(const TArray<int32>& Triangles, TArray<int32>& UpdateSetsOut, FAxisAlignedBox3d& BoundsOut);

	/**
	 * This function updates vertex positions/attributes of existing SceneProxy render buffers if possible, for the given triangles.
	 * The assumption is that FastNotifyTriangleVerticesUpdated_TryPrecompute() was used to get the Precompute future, this function
	 * will Wait() until it is done and then use the UpdateSets and UpdateSetBounds that were computed (must be the same variables
	 * passed to FastNotifyTriangleVerticesUpdated_TryPrecompute).
	 * If the Precompute future returns false, then we forward the call to FastNotifyTriangleVerticesUpdated(), which will do more work.
	 */
	void FastNotifyTriangleVerticesUpdated_ApplyPrecompute(const TArray<int32>& Triangles, EMeshRenderAttributeFlags UpdatedAttributes,
		TFuture<bool>& Precompute, const TArray<int32>& UpdateSets, const FAxisAlignedBox3d& UpdateSetBounds);

	/**
	 * This function updates vertex positions/attributes of existing SceneProxy render buffers if possible, for the given triangles.
	 * If a FMeshRenderDecomposition has not been explicitly set, call is forwarded to FastNotifyVertexAttributesUpdated()
	 */
	void FastNotifyTriangleVerticesUpdated(const TSet<int32>& Triangles, EMeshRenderAttributeFlags UpdatedAttributes);

	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component|Rendering", DisplayName = "Notify Mesh Updated")
	virtual void NotifyMeshModified();

	/**
	 * Notify the Component that vertex attribute values of it's DynamicMesh have been modified externally. This will result in
	 * Rendering vertex buffers being updated. This update path is more efficient than doing a full Notify Mesh Updated.
	 *
	 * @warning it is invalid to call this function if (1) the mesh triangulation has also been changed, (2) triangle MaterialIDs have been changed,
	 * or (3) any attribute overlay (normal, color, UV) topology has been modified, ie split-vertices have been added/removed.
	 * Behavior of this function is undefined in these cases and may crash. If you are unsure, use Notify Mesh Updated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component|Rendering", DisplayName = "Notify Vertex Attributes Updated")
	virtual void NotifyMeshVertexAttributesModified(
		bool bPositions = true,
		bool bNormals = true,
		bool bUVs = true,
		bool bColors = true);

	/** If false, we don't completely invalidate the RenderProxy when ApplyChange() is called (assumption is it will be handled elsewhere) */
	UPROPERTY()
	bool bInvalidateProxyOnChange = true;

	/**
	 * Apply a vertex deformation change to the internal mesh
	 */
	virtual void ApplyChange(const FMeshVertexChange* Change, bool bRevert) override;

	/**
	 * Apply a general mesh change to the internal mesh
	 */
	virtual void ApplyChange(const FMeshChange* Change, bool bRevert) override;

	/**
	* Apply a general mesh replacement change to the internal mesh
	*/
	virtual void ApplyChange(const FMeshReplacementChange* Change, bool bRevert) override;


	/**
	 * This delegate fires when a FCommandChange is applied to this component, so that
	 * parent objects know the mesh has changed.
	 */
	FSimpleMulticastDelegate OnMeshChanged;


	/**
	 * This delegate fires when ApplyChange(FMeshVertexChange) executes
	 */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FMeshVerticesModified, USimpleDynamicMeshComponent*, const FMeshVertexChange*, bool);
	FMeshVerticesModified OnMeshVerticesChanged;


protected:
	
	    /** Handle for OnMeshObjectChanged which is registered with MeshObject::OnMeshChanged delegate */
		FDelegateHandle MeshObjectChangedHandle;

		/** Called whenever internal MeshObject is modified, fires OnMeshChanged and OnMeshVerticesChanged above */
		void OnMeshObjectChanged(UDynamicMesh* ChangedMeshObject, FDynamicMeshChangeInfo ChangeInfo);

public:

	/** Set an active triangle color function if one exists, and update the mesh */
	virtual void SetTriangleColorFunction(TUniqueFunction<FColor(const FDynamicMesh3*, int)> TriangleColorFuncIn,
		EDynamicMeshComponentRenderUpdateMode UpdateMode = EDynamicMeshComponentRenderUpdateMode::FastUpdate);
	/** Clear an active triangle color function if one exists, and update the mesh */
	virtual void ClearTriangleColorFunction(EDynamicMeshComponentRenderUpdateMode UpdateMode = EDynamicMeshComponentRenderUpdateMode::FastUpdate);

	/** @return true if a triangle color function is configured */
	virtual bool HasTriangleColorFunction();
protected:
	/** If this function is set, we will use these colors instead of vertex colors */
	TUniqueFunction<FColor(const FDynamicMesh3*, int)> TriangleColorFunc = nullptr;

	/** This function is passed via lambda to the RenderProxy to be able to access TriangleColorFunc */
	FColor GetTriangleColor(const FDynamicMesh3* Mesh, int TriangleID);

	/** This function is passed via lambda to the RenderProxy when BaseDynamicMeshComponent::ColorMode == Polygroups */
	FColor GetGroupColor(const FDynamicMesh3* Mesh, int TriangleID) const;
public:

	/**
	 * Configure whether wireframe rendering is enabled or not
	 */
	virtual void SetEnableWireframeRenderPass(bool bEnable) override { bExplicitShowWireframe = bEnable; }

	/**
	 * @return true if wireframe rendering pass is enabled
	 */
	virtual bool EnableWireframeRenderPass() const override { return bExplicitShowWireframe; }


	/**
	 * If Secondary triangle buffers are enabled, then we will filter triangles that pass the given predicate
	 * function into a second index buffer. These triangles will be drawn with the Secondary render material
	 * that is set in the BaseDynamicMeshComponent. Calling this function invalidates the SceneProxy.
	 */
	virtual void EnableSecondaryTriangleBuffers(TUniqueFunction<bool(const FDynamicMesh3*, int32)> SecondaryTriFilterFunc);

	/**
	 * Disable secondary triangle buffers. This invalidates the SceneProxy.
	 */
	virtual void DisableSecondaryTriangleBuffers();

	/**
	 * Configure a decomposition of the mesh, which will result in separate render buffers for each decomposition triangle group.
	 * Invalidates existing SceneProxy.
	 */
	virtual void SetExternalDecomposition(TUniquePtr<FMeshRenderDecomposition> Decomposition);

private:
		// Default what the 'Default' tangent type should map to
		static constexpr EDynamicMeshTangentCalcType UseDefaultTangentsType = EDynamicMeshTangentCalcType::ExternallyCalculated;

public:
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	void SetTangentsType(EDynamicMeshTangentCalcType NewTangentsType);

	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	EDynamicMeshTangentCalcType GetTangentsType() const
	{
		return TangentsType == EDynamicMeshTangentCalcType::Default ? UseDefaultTangentsType : TangentsType;
	}

private:
		// pure version of GetTangentsType, so it can be used as a getter below (getters must be BlueprintPure)
		UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintInternalUseOnly, Category = "Dynamic Mesh Component")
		EDynamicMeshTangentCalcType GetTangentsTypePure() const
		{
			return GetTangentsType();
		}

public:

	/** This function marks the auto tangents as dirty, they will be recomputed before they are used again */
	virtual void InvalidateAutoCalculatedTangents();

	/** @return AutoCalculated Tangent Set, which may require that they be recomputed, or nullptr if not enabled/available */
	const FMeshTangentsf* GetAutoCalculatedTangents();

protected:
	/** Tangent source defines whether we use the provided tangents on the Dynamic Mesh, autogenerate tangents, or do not use tangents */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetTangentsType, BlueprintGetter = GetTangentsTypePure, Category = "Dynamic Mesh Component|Rendering")
	EDynamicMeshTangentCalcType TangentsType = EDynamicMeshTangentCalcType::Default;
	/** true if AutoCalculatedTangents has been computed for current mesh */
	bool bAutoCalculatedTangentsValid = false;

	/** Set of per-triangle-vertex tangents computed for the current mesh. Only valid if bAutoCalculatedTangentsValid == true */
	FMeshTangentsf AutoCalculatedTangents;

	void UpdateAutoCalculatedTangents();

	/**
	 * If the render proxy is invalidated (eg by MarkRenderStateDirty()), it will be destroyed at the end of
	 * the frame, but the base SceneProxy pointer is not nulled out immediately. As a result if we call various
	 * partial-update functions after invalidating the proxy, they may be operating on an invalid proxy.
	 * So we have to keep track of proxy-valid state ourselves.
	 */
	bool bProxyValid = false;

	/**
	 * If true, the render proxy will verify that the mesh batch materials match the contents from
	 * the component GetUsedMaterials(). Used material verification is prone to races when changing
	 * materials on this component in quick succession (for example, SetOverrideRenderMaterial). This
	 * parameter is provided to allow clients to opt out of used material verification for these
	 * use cases.
	 */
	bool bProxyVerifyUsedMaterials = true;

	//===============================================================================================================
		// IRenderMeshPostProcessor Support. If a RenderMesh Postprocessor is configured, then instead of directly 
		// passing the internal mesh to the RenderProxy, IRenderMeshPostProcessor::PostProcess is applied to populate
		// the internal RenderMesh which is passed instead. This allows things like Displacement or Subdivision to be 
		// done on-the-fly at the rendering level (which is potentially more efficient).
		//
public:
	/**
	 * Add a render mesh processor, to be called before the mesh is sent for rendering.
	 */
	virtual void SetRenderMeshPostProcessor(TUniquePtr<IRenderMeshPostProcessor> Processor);

	/**
	 * The SceneProxy should call these functions to get the post-processed RenderMesh. (See IRenderMeshPostProcessor.)
	 */
	virtual FDynamicMesh3* GetRenderMesh();

	/**
	 * The SceneProxy should call these functions to get the post-processed RenderMesh. (See IRenderMeshPostProcessor.)
	 */
	virtual const FDynamicMesh3* GetRenderMesh() const;
	/**
	 * Set new list of Materials for the Mesh. Dynamic Mesh Component does not have
	 * Slot Names, so the size of the Material Set should be the same as the number of
	 * different Material IDs on the mesh MaterialID attribute
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	void ConfigureMaterialSet(const TArray<UMaterialInterface*>& NewMaterialSet);


public:

	// do not use this
	UPROPERTY()
	bool bDrawOnTop = false;

	// do not use this
	void SetDrawOnTop(bool bSet);


protected:

	/** Once async physics cook is done, create needed state */
	virtual void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);


	/**
	 * This is called to tell our RenderProxy about modifications to the material set.
	 * We need to pass this on for things like material validation in the Editor.
	 */
	virtual void NotifyMaterialSetUpdated();

	FAxisAlignedBox3d LocalBounds;

	void UpdateLocalBounds();

	/** Simplified collision representation for the mesh component  */
	UPROPERTY(EditAnywhere, Category = BodySetup, meta = (DisplayName = "Primitives", NoResetToDefault))
	struct FKAggregateGeom AggGeom;

	virtual UBodySetup* CreateBodySetupHelper();

	/** @return Set new BodySetup for this Component. */
	virtual void SetBodySetup(UBodySetup* NewSetup);

private:
	

	
	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.

	TUniquePtr<IRenderMeshPostProcessor> RenderMeshPostProcessor;
	TUniquePtr<FDynamicMesh3> RenderMesh;
	void InitializeNewMesh();

	bool bTangentsValid = false;
	FMeshTangentsf Tangents;

	//===============================================================================================================
	// Support for Vertex Color remapping/filtering. This allows external code to modulate the existing 
	// Vertex Colors on the rendered mesh. The remapping is only applied to FVector4f Color Overlay attribute buffers.
	// The lambda that is passed is held for the lifetime of the Component and
	// must remain valid. If the Vertex Colors are modified, FastNotifyColorsUpdated() can be used to do the 
	// minimal vertex buffer updates necessary in the RenderProxy

	/** Queue for async body setups that are being cooked */
	UPROPERTY(transient)
	TArray<UBodySetup*> AsyncBodySetupQueue;

	UPROPERTY(Instanced)
	UBodySetup* MeshBodySetup;

	virtual void InvalidatePhysicsData();
	virtual void RebuildPhysicsData();

	bool bTransientDeferCollisionUpdates = false;
	bool bCollisionUpdatePending = false;
public:

	DECLARE_MULTICAST_DELEGATE_TwoParams(FComponentChildrenChangedDelegate, USceneComponent*, bool);

	/**
	 * The OnChildAttached() and OnChildDetached() implementations (from USceneComponent API) broadcast this delegate. This
	 * allows Actors that have UDynamicMeshComponent's to respond to changes in their Component hierarchy.
	 */
	FComponentChildrenChangedDelegate OnChildAttachmentModified;

	/**
	 *	Controls whether the physics cooking should be done off the game thread.
	 *  This should be used when collision geometry doesn't have to be immediately up to date (For example streaming in far away objects)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamic Mesh Component|Collision")
	bool bUseAsyncCooking = false;

	/** @return current BodySetup for this Component, or nullptr if it does not exist */
	virtual const UBodySetup* GetBodySetup() const { return MeshBodySetup; }
	/** @return BodySetup for this Component. A new BodySetup will be created if one does not exist. */
	virtual UBodySetup* GetBodySetup() override;

	/** If true, updates to the mesh will not result in immediate collision regeneration. Useful when the mesh will be modified multiple times before collision is needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Mesh Component|Collision");
	bool bDeferCollisionUpdates = false;

	/**
	 * Force an update of the Collision/Physics data for this Component.
	 * @param bOnlyIfPending only update if a collision update is pending, ie the underlying DynamicMesh changed and bDeferCollisionUpdates is enabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	virtual void UpdateCollision(bool bOnlyIfPending = true);

	/**
	 * calls SetComplexAsSimpleCollisionEnabled(true, true)
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	void EnableComplexAsSimpleCollision();

	/**
	 * If bEnabled=true, sets bEnableComplexCollision=true and CollisionType=CTF_UseComplexAsSimple
	 * If bEnabled=true, sets bEnableComplexCollision=false and CollisionType=CTF_UseDefault
	 * @param bImmediateUpdate if true, UpdateCollision(true) is called
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Component")
	void SetComplexAsSimpleCollisionEnabled(bool bEnabled, bool bImmediateUpdate = true);

	/**
	 * If true, current mesh will be used as Complex Collision source mesh.
	 * This is independent of the CollisionType setting, ie, even if Complex collision is enabled, if this is false, then the Complex Collision mesh will be empty
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Mesh Component|Collision");
	bool bEnableComplexCollision = false;


	/** Type of Collision Geometry to use for this Mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Mesh Component|Collision")
	TEnumAsByte<enum ECollisionTraceFlag> CollisionType = ECollisionTraceFlag::CTF_UseSimpleAsComplex;

	/**
	 * Update the simple collision shapes associated with this mesh component
	 *
	 * @param AggGeom				New simple collision shapes to be used
	 * @param bUpdateCollision		Whether to automatically call UpdateCollision() -- if false, manually call it to register the change with the physics system
	 */
	virtual void SetSimpleCollisionShapes(const struct FKAggregateGeom& AggGeom, bool bUpdateCollision);

	const FKAggregateGeom& GetSimpleCollisionShapes() const
	{
		return AggGeom;
	}

protected:
	void ResetProxy();

	virtual FBaseDynamicMeshSceneProxy* GetBaseSceneProxy() override { return (FBaseDynamicMeshSceneProxy*)GetCurrentSceneProxy(); }
	/**
	 * @return current render proxy, if valid, otherwise nullptr
	 */
	FSimpleDynamicMeshSceneProxy* GetCurrentSceneProxy();

	/** Set an active VertexColor Remapping function if one exists, and update the mesh */
	virtual void SetVertexColorRemappingFunction(TUniqueFunction<void(FVector4f&)> ColorMapFuncIn,
		EDynamicMeshComponentRenderUpdateMode UpdateMode = EDynamicMeshComponentRenderUpdateMode::FastUpdate);

	/** Clear an active VertexColor Remapping function if one exists, and update the mesh */
	virtual void ClearVertexColorRemappingFunction(EDynamicMeshComponentRenderUpdateMode UpdateMode = EDynamicMeshComponentRenderUpdateMode::FastUpdate);

	/** @return true if a VertexColor Remapping function is configured */
	virtual bool HasVertexColorRemappingFunction();


	/** If this function is set, DynamicMesh Attribute Color Overlay colors will be passed through this function before sending to render buffers */
	TUniqueFunction<void(FVector4f&)> VertexColorMappingFunc = nullptr;

	/** This function is passed via lambda to the RenderProxy to be able to access VertexColorMappingFunc */
	void RemapVertexColor(FVector4f& VertexColorInOut);
	virtual void OnChildAttached(USceneComponent* ChildComponent) override;
	virtual void OnChildDetached(USceneComponent* ChildComponent) override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
	virtual void BeginDestroy() override;

public:
	TUniqueFunction<bool(const FDynamicMesh3*, int32)> SecondaryTriFilterFunc = nullptr;

	TUniquePtr<FMeshRenderDecomposition> Decomposition;

public:
	/** Set whether or not to validate mesh batch materials against the component materials. */
	void SetSceneProxyVerifyUsedMaterials(bool bState);

};