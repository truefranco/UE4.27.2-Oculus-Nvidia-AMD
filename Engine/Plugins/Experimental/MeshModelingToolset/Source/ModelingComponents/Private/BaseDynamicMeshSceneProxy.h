// Copyright Epic Games, Inc. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "VertexFactory.h"
#include "LocalVertexFactory.h"
#include "PrimitiveViewRelevance.h"
#include "BaseDynamicMeshComponent.h"


/**
 * FMeshRenderBufferSet stores a set of RenderBuffers for a mesh
 */
class FMeshRenderBufferSet
{
public:
	/** Number of triangles in this renderbuffer set. Note that triangles may be split between IndexBuffer and SecondaryIndexBuffer. */
	int TriangleCount = 0;

	/** The buffer containing vertex data. */
	FStaticMeshVertexBuffer StaticMeshVertexBuffer;
	/** The buffer containing the position vertex data. */
	FPositionVertexBuffer PositionVertexBuffer;
	/** The buffer containing the vertex color data. */
	FColorVertexBuffer ColorVertexBuffer;

	/** triangle indices */
	FDynamicMeshIndexBuffer32 IndexBuffer;

	/** vertex factory */
	FLocalVertexFactory VertexFactory;

	/** Material to draw this mesh with */
	UMaterialInterface* Material = nullptr;

	/**
	 * Optional list of triangles stored in this buffer. Storing this allows us
	 * to rebuild the buffers if vertex data changes.
	 */
	TOptional<TArray<int>> Triangles;

	/**
	 * If secondary index buffer is enabled, we populate this index buffer with additional triangles indexing into the same vertex buffers
	 */
	bool bEnableSecondaryIndexBuffer = false;

	/**
	 * partition or subset of IndexBuffer that indexes into same vertex buffers
	 */
	FDynamicMeshIndexBuffer32 SecondaryIndexBuffer;


	/**
	 * In situations where we want to *update* the existing Vertex or Index buffers, we need to synchronize
	 * access between the Game and Render threads. We use this lock to do that.
	 */
	FCriticalSection BuffersLock;


	FMeshRenderBufferSet(ERHIFeatureLevel::Type FeatureLevelType)
		: VertexFactory(FeatureLevelType, "FMeshRenderBufferSet")
	{

	}


	virtual ~FMeshRenderBufferSet()
	{
		check(IsInRenderingThread());

		if (TriangleCount > 0)
		{
			PositionVertexBuffer.ReleaseResource();
			StaticMeshVertexBuffer.ReleaseResource();
			ColorVertexBuffer.ReleaseResource();
			VertexFactory.ReleaseResource();
			if (IndexBuffer.IsInitialized())
			{
				IndexBuffer.ReleaseResource();
			}
			if (SecondaryIndexBuffer.IsInitialized())
			{
				SecondaryIndexBuffer.ReleaseResource();
			}
		}
	}


	/**
	 * Upload initialized mesh buffers. 
	 * @warning This can only be called on the Rendering Thread.
	 */
	void Upload()
	{
		check(IsInRenderingThread());

		if (TriangleCount == 0)
		{
			return;
		}

		InitOrUpdateResource(&this->PositionVertexBuffer);
		InitOrUpdateResource(&this->StaticMeshVertexBuffer);
		InitOrUpdateResource(&this->ColorVertexBuffer);

		FLocalVertexFactory::FDataType Data;
		this->PositionVertexBuffer.BindPositionVertexBuffer(&this->VertexFactory, Data);
		this->StaticMeshVertexBuffer.BindTangentVertexBuffer(&this->VertexFactory, Data);
		this->StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(&this->VertexFactory, Data);
		// currently no lightmaps support
		//this->StaticMeshVertexBuffer.BindLightMapVertexBuffer(&this->VertexFactory, Data, LightMapIndex);
		this->ColorVertexBuffer.BindColorVertexBuffer(&this->VertexFactory, Data);
		this->VertexFactory.SetData(Data);

		InitOrUpdateResource(&this->VertexFactory);
		PositionVertexBuffer.InitResource();
		StaticMeshVertexBuffer.InitResource();
		ColorVertexBuffer.InitResource();
		VertexFactory.InitResource();

		if (IndexBuffer.Indices.Num() > 0)
		{
			IndexBuffer.InitResource();
		}
		if (bEnableSecondaryIndexBuffer && SecondaryIndexBuffer.Indices.Num() > 0)
		{
			SecondaryIndexBuffer.InitResource();
		}
	}


	void UploadIndexBufferUpdate()
	{
		check(IsInRenderingThread());
		if (IndexBuffer.Indices.Num() > 0)
		{
			InitOrUpdateResource(&IndexBuffer);
		}
		if (bEnableSecondaryIndexBuffer && SecondaryIndexBuffer.Indices.Num() > 0)
		{
			InitOrUpdateResource(&SecondaryIndexBuffer);
		}
	}


	void UploadVertexUpdate(bool bPositions, bool bMeshAttribs, bool bColors)
	{
		check(IsInRenderingThread());

		if (TriangleCount == 0)
		{
			return;
		}

		if (bPositions)
		{
			InitOrUpdateResource(&this->PositionVertexBuffer);
		}
		if (bMeshAttribs)
		{
			InitOrUpdateResource(&this->StaticMeshVertexBuffer);
		}
		if (bColors)
		{
			InitOrUpdateResource(&this->ColorVertexBuffer);
		}

		FLocalVertexFactory::FDataType Data;
		this->PositionVertexBuffer.BindPositionVertexBuffer(&this->VertexFactory, Data);
		this->StaticMeshVertexBuffer.BindTangentVertexBuffer(&this->VertexFactory, Data);
		this->StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(&this->VertexFactory, Data);
		// currently no lightmaps support
		//this->StaticMeshVertexBuffer.BindLightMapVertexBuffer(&this->VertexFactory, Data, LightMapIndex);
		this->ColorVertexBuffer.BindColorVertexBuffer(&this->VertexFactory, Data);
		this->VertexFactory.SetData(Data);

		InitOrUpdateResource(&this->VertexFactory);
	}


	void TransferVertexUpdateToGPU(bool bPositions, bool bNormals, bool bTexCoords, bool bColors)
	{
		check(IsInRenderingThread());
		if (TriangleCount == 0)
		{
			return;
		}

		if (bPositions)
		{
			auto& VertexBuffer = this->PositionVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
			RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
		}
		if (bNormals)
		{
			auto& VertexBuffer = this->StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTangentSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTangentData(), VertexBuffer.GetTangentSize());
			RHIUnlockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
		}
		if (bColors)
		{
			auto& VertexBuffer = this->ColorVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
			RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
		}
		if (bTexCoords)
		{
			auto& VertexBuffer = this->StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTexCoordSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTexCoordData(), VertexBuffer.GetTexCoordSize());
			RHIUnlockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI);
		}
	}






	/**
	 * Initializes a render resource, or update it if already initialized.
	 * @wearning This function can only be called on the Render Thread
	 */
	void InitOrUpdateResource(FRenderResource* Resource)
	{
		check(IsInRenderingThread());

		if (!Resource->IsInitialized())
		{
			Resource->InitResource();
		}
		else
		{
			Resource->UpdateRHI();
		}
	}




protected:
	friend class FBaseDynamicMeshSceneProxy;

	/**
	 * Enqueue a command on the Render Thread to destroy the passed in buffer set.
	 * At this point the buffer set should be considered invalid.
	 */
	static void DestroyRenderBufferSet(FMeshRenderBufferSet* BufferSet)
	{
		if (BufferSet->TriangleCount == 0)
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND(FMeshRenderBufferSetDestroy)(
			[BufferSet](FRHICommandListImmediate& RHICmdList)
		{
			delete BufferSet;
		});
	}


};




/**
 * FBaseDynamicMeshSceneProxy is an abstract base class for a Render Proxy
 * for a UBaseDynamicMeshComponent, where the assumption is that mesh data
 * will be stored in FMeshRenderBufferSet instances
 */
class FBaseDynamicMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	UBaseDynamicMeshComponent* ParentBaseComponent;

	/**
	 * Constant color assigned to vertices if no other vertex color is specified
	 */
	FColor ConstantVertexColor = FColor::White;

	/**
	 * If true, vertex colors on the FDynamicMesh will be ignored
	 */
	bool bIgnoreVertexColors = false;

	/**
	 * If true, a per-triangle color is used to set vertex colors
	 */
	bool bUsePerTriangleColor = false;

	/**
	 * Per-triangle color function. Only called if bUsePerTriangleColor=true
	 */
	TFunction<FColor(const FDynamicMesh3*, int)> PerTriangleColorFunc = nullptr;

	/**
	 * If true, VertexColorRemappingFunc is called on Vertex Colors provided from Mesh to remap them to a different color
	 */
	bool bApplyVertexColorRemapping = false;

	/**
	 * Vertex color remapping function. Only called if bApplyVertexColorRemapping == true, for mesh vertex colors
	 */
	TUniqueFunction<void(FVector4f&)> VertexColorRemappingFunc = nullptr;

	/**
	 * Color Space Transform/Conversion applied to Vertex Colors provided from Mesh Color Overlay Attribute
	 * Color Space Conversion is applied after any Vertex Color Remapping.
	 */
	EDynamicMeshVertexColorTransformMode ColorSpaceTransformMode = EDynamicMeshVertexColorTransformMode::NoTransform;

	/**
	* If true, a facet normals are used instead of mesh normals
	*/
	bool bUsePerTriangleNormals = false;
	/**
	 * If true, populate secondary buffers using SecondaryTriFilterFunc
	 */
	bool bUseSecondaryTriBuffers = false;

	/**
	 * Filter predicate for secondary triangle index buffer. Only called if bUseSecondaryTriBuffers=true
	 */
	TUniqueFunction<bool(const FDynamicMesh3*, int32)> SecondaryTriFilterFunc = nullptr;

protected:
	// Set of currently-allocated RenderBuffers. We own these pointers and must clean them up.
	// Must guard access with AllocatedSetsLock!!
	TSet<FMeshRenderBufferSet*> AllocatedBufferSets;

	// use to control access to AllocatedBufferSets 
	FCriticalSection AllocatedSetsLock;

public:
	FBaseDynamicMeshSceneProxy(UBaseDynamicMeshComponent* Component)
		: FPrimitiveSceneProxy(Component),
		ParentBaseComponent(Component)
	{
	}


	virtual ~FBaseDynamicMeshSceneProxy()
	{
		// we are assuming in code below that this is always called from the rendering thread
		check(IsInRenderingThread());

		// destroy all existing renderbuffers
		for (FMeshRenderBufferSet* BufferSet : AllocatedBufferSets)
		{
			FMeshRenderBufferSet::DestroyRenderBufferSet(BufferSet);
		}
	}


	//
	// FBaseDynamicMeshSceneProxy API - subclasses must implement these functions
	//


	/**
	 * Return set of active renderbuffers. Must be implemented by subclass.
	 * This is the set of render buffers that will be drawn by GetDynamicMeshElements
	 */
	virtual void GetActiveRenderBufferSets(TArray<FMeshRenderBufferSet*>& Buffers) const = 0;

	
	//
	// RenderBuffer management
	//


	/**
	 * Allocates a set of render buffers. FPrimitiveSceneProxy will keep track of these
	 * buffers and destroy them on destruction.
	 */
	virtual FMeshRenderBufferSet* AllocateNewRenderBufferSet()
	{
		// should we hang onto these and destroy them in constructor? leaving to subclass seems risky?
		FMeshRenderBufferSet* RenderBufferSet = new FMeshRenderBufferSet(GetScene().GetFeatureLevel());

		RenderBufferSet->Material = UMaterial::GetDefaultMaterial(MD_Surface);

		AllocatedSetsLock.Lock();
		AllocatedBufferSets.Add(RenderBufferSet);
		AllocatedSetsLock.Unlock();

		return RenderBufferSet;
	}

	/**
	 * Explicitly release a set of RenderBuffers
	 */
	virtual void ReleaseRenderBufferSet(FMeshRenderBufferSet* BufferSet)
	{
		AllocatedSetsLock.Lock();
		check(AllocatedBufferSets.Contains(BufferSet));
		AllocatedBufferSets.Remove(BufferSet);
		AllocatedSetsLock.Unlock();

		FMeshRenderBufferSet::DestroyRenderBufferSet(BufferSet);
	}

	/**
	 * Initialize rendering buffers from given attribute overlays.
	 * Creates three vertices per triangle, IE no shared vertices in buffers.
	 */
	template<typename TriangleEnumerable>
	void InitializeBuffersFromOverlays(
		FMeshRenderBufferSet* RenderBuffers,
		const FDynamicMesh3* Mesh,
		int NumTriangles, TriangleEnumerable Enumerable,
		const FDynamicMeshUVOverlay* UVOverlay,
		const FDynamicMeshNormalOverlay* NormalOverlay,
		const FDynamicMeshColorOverlay* ColorOverlay,
		TFunctionRef<void(int, int, int, const FVector3f&, FVector3f&, FVector3f&)> TangentsFunc,
		bool bTrackTriangles = false)
	{
		TArray<const FDynamicMeshUVOverlay*> UVOverlays;
		UVOverlays.Add(UVOverlay);
		InitializeBuffersFromOverlays(RenderBuffers, Mesh, NumTriangles, Enumerable,
			UVOverlays, NormalOverlay, ColorOverlay, TangentsFunc, bTrackTriangles);
	}

	/**
	 * Initialize rendering buffers from given attribute overlays.
	 * Creates three vertices per triangle, IE no shared vertices in buffers.
	 */
	template<typename TriangleEnumerable, typename UVOverlayListAllocator>
	void InitializeBuffersFromOverlays(
		FMeshRenderBufferSet* RenderBuffers,
		const FDynamicMesh3* Mesh,
		int NumTriangles, TriangleEnumerable Enumerable,
		const TArray<const FDynamicMeshUVOverlay*, UVOverlayListAllocator>& UVOverlays,
		const FDynamicMeshNormalOverlay* NormalOverlay,
		const FDynamicMeshColorOverlay* ColorOverlay,
		TFunctionRef<void(int, int, int, const FVector3f&, FVector3f&, FVector3f&)> TangentsFunc,
		bool bTrackTriangles = false)
	{
		//SCOPE_CYCLE_COUNTER(STAT_SculptToolOctree_InitializeBufferFromOverlay);

		RenderBuffers->TriangleCount = NumTriangles;
		if (NumTriangles == 0)
		{
			return;
		}

		bool bHaveColors = Mesh->HasVertexColors() && (bIgnoreVertexColors == false);

		int NumVertices = NumTriangles * 3;
		int NumUVOverlays = UVOverlays.Num();
		int NumTexCoords = FMath::Max(1, NumUVOverlays);		// no! zero!
		TArray<FIndex3i, TFixedAllocator<MAX_STATIC_TEXCOORDS>> UVTriangles;
		UVTriangles.SetNum(NumTexCoords);

		{
			RenderBuffers->PositionVertexBuffer.Init(NumVertices);
			RenderBuffers->StaticMeshVertexBuffer.Init(NumVertices, NumTexCoords);
			RenderBuffers->ColorVertexBuffer.Init(NumVertices);
			RenderBuffers->IndexBuffer.Indices.AddUninitialized(NumTriangles * 3);
		}

		// build triangle list if requested, or if we are using secondary buffers in which case we need it to filter later
		bool bBuildTriangleList = bTrackTriangles || bUseSecondaryTriBuffers;
		if (bBuildTriangleList)
		{
			RenderBuffers->Triangles = TArray<int32>();
		}

		int TriIdx = 0, VertIdx = 0;
		FVector3f TangentX, TangentY;
		for (int TriangleID : Enumerable)
		{
			FIndex3i Tri = Mesh->GetTriangle(TriangleID);
			for (int32 k = 0; k < NumTexCoords; ++k)
			{
				UVTriangles[k] = (k < NumUVOverlays && UVOverlays[k] != nullptr) ? UVOverlays[k]->GetTriangle(TriangleID) : FIndex3i::Invalid();
			}
			FIndex3i TriNormal = (NormalOverlay != nullptr) ? NormalOverlay->GetTriangle(TriangleID) : FIndex3i::Zero();
			FIndex3i TriColor = (ColorOverlay != nullptr) ? ColorOverlay->GetTriangle(TriangleID) : FIndex3i::Zero();

			FColor UniformTriColor = ConstantVertexColor;
			if (bUsePerTriangleColor && PerTriangleColorFunc != nullptr)
			{
				UniformTriColor = PerTriangleColorFunc(Mesh, TriangleID);
				bHaveColors = false;
			}

			for (int j = 0; j < 3; ++j)
			{
				RenderBuffers->PositionVertexBuffer.VertexPosition(VertIdx) = (FVector)Mesh->GetVertex(Tri[j]);

				FVector3f Normal;
				if (bUsePerTriangleNormals)
				{
					Normal = (FVector3f)Mesh->GetTriNormal(TriangleID);
				}
				else
				{
					Normal = (NormalOverlay != nullptr && TriNormal[j] != FDynamicMesh3::InvalidID) ?
						NormalOverlay->GetElement(TriNormal[j]) : Mesh->GetVertexNormal(Tri[j]);
				}

				// get tangents
				TangentsFunc(Tri[j], TriangleID, j, Normal, TangentX, TangentY);

				RenderBuffers->StaticMeshVertexBuffer.SetVertexTangents(VertIdx, (FVector)TangentX, (FVector)TangentY, (FVector)Normal);

				for (int32 k = 0; k < NumTexCoords; ++k)
				{
					FVector2f UV = (UVTriangles[k][j] != FDynamicMesh3::InvalidID) ?
						UVOverlays[k]->GetElement(UVTriangles[k][j]) : FVector2f::Zero();
					RenderBuffers->StaticMeshVertexBuffer.SetVertexUV(VertIdx, k, (FVector2D&)UV);
				}

				FColor VertexFColor = (bHaveColors && TriColor[j] != FDynamicMesh3::InvalidID) ?
					GetOverlayColorAsFColor(ColorOverlay, TriColor[j]) : UniformTriColor;

				RenderBuffers->ColorVertexBuffer.VertexColor(VertIdx) = VertexFColor;

				RenderBuffers->IndexBuffer.Indices[TriIdx++] = VertIdx;		// currently TriIdx == VertIdx so we don't really need both...
				VertIdx++;
			}

			if (bBuildTriangleList)
			{
				RenderBuffers->Triangles->Add(TriangleID);
			}
		}

		// split triangles into secondary buffer (at bit redudant since we just built IndexBuffer, but we may optionally duplicate triangles in the future
		if (bUseSecondaryTriBuffers)
		{
			RenderBuffers->bEnableSecondaryIndexBuffer = true;
			UpdateSecondaryTriangleBuffer(RenderBuffers, Mesh, false);
		}
	}

	FColor GetOverlayColorAsFColor(
		const FDynamicMeshColorOverlay* ColorOverlay,
		int32 ElementID)
	{
		checkSlow(ColorOverlay);
		FVector4f UseColor = ColorOverlay->GetElement(ElementID);

		if (bApplyVertexColorRemapping)
		{
			VertexColorRemappingFunc(UseColor);
		}

		if (ColorSpaceTransformMode == EDynamicMeshVertexColorTransformMode::SRGBToLinear)
		{
			// is there a better way to do this? 
			FColor QuantizedSRGBColor = ((FLinearColor)UseColor).ToFColor(false);
			return FLinearColor(QuantizedSRGBColor).ToFColor(false);
		}
		else
		{
			bool bConvertToSRGB = (ColorSpaceTransformMode == EDynamicMeshVertexColorTransformMode::LinearToSRGB);
			return ((FLinearColor)UseColor).ToFColor(bConvertToSRGB);
		}
	}

	/**
	 * Filter the triangles in a FMeshRenderBufferSet into the SecondaryIndexBuffer.
	 * Requires that RenderBuffers->Triangles has been initialized.
	 * @param bDuplicate if set, then primary IndexBuffer is unmodified and SecondaryIndexBuffer contains duplicates. Otherwise triangles are sorted via predicate into either primary or secondary.
	 */
	void UpdateSecondaryTriangleBuffer(
		FMeshRenderBufferSet* RenderBuffers,
		const FDynamicMesh3* Mesh,
		bool bDuplicate)
	{
		check(bUseSecondaryTriBuffers == true);
		check(RenderBuffers->Triangles.IsSet());

		const TArray<int32>& TriangleIDs = RenderBuffers->Triangles.GetValue();
		int NumTris = TriangleIDs.Num();
		TArray<uint32>& Indices = RenderBuffers->IndexBuffer.Indices;
		TArray<uint32>& SecondaryIndices = RenderBuffers->SecondaryIndexBuffer.Indices;

		RenderBuffers->SecondaryIndexBuffer.Indices.Reset();
		if (bDuplicate == false)
		{
			RenderBuffers->IndexBuffer.Indices.Reset();
		}
		for ( int k = 0; k < NumTris; ++k)
		{
			int TriangleID = TriangleIDs[k];
			bool bInclude = SecondaryTriFilterFunc(Mesh, TriangleID);
			if (bInclude)
			{
				SecondaryIndices.Add(3*k);
				SecondaryIndices.Add(3*k + 1);
				SecondaryIndices.Add(3*k + 2);
			} 
			else if (bDuplicate == false)
			{
				Indices.Add(3*k);
				Indices.Add(3*k + 1);
				Indices.Add(3*k + 2);
			}
		}
	}


	/**
	 * FastUpdateTriangleBuffers re-sorts the existing set of triangles in a FMeshRenderBufferSet
	 * into primary and secondary index buffers. Note that UploadIndexBufferUpdate() must be called
	 * after this function!
	 */
	void FastUpdateIndexBuffers(
		FMeshRenderBufferSet* RenderBuffers, 
		const FDynamicMesh3* Mesh)
	{
		if (RenderBuffers->TriangleCount == 0)
		{
			return;
		}
		check(RenderBuffers->Triangles.IsSet() && RenderBuffers->Triangles->Num() > 0);

		//bool bDuplicate = false;		// flag for future use, in case we want to draw all triangles in primary and duplicates in secondary...
		RenderBuffers->IndexBuffer.Indices.Reset();
		RenderBuffers->SecondaryIndexBuffer.Indices.Reset();
		
		TArray<uint32>& Indices = RenderBuffers->IndexBuffer.Indices;
		TArray<uint32>& SecondaryIndices = RenderBuffers->SecondaryIndexBuffer.Indices;
		const TArray<int32>& TriangleIDs = RenderBuffers->Triangles.GetValue();

		int NumTris = TriangleIDs.Num();
		for (int k = 0; k < NumTris; ++k)
		{
			int TriangleID = TriangleIDs[k];
			bool bInclude = SecondaryTriFilterFunc(Mesh, TriangleID);
			if (bInclude)
			{
				SecondaryIndices.Add(3 * k);
				SecondaryIndices.Add(3 * k + 1);
				SecondaryIndices.Add(3 * k + 2);
			}
			else // if (bDuplicate == false)
			{
				Indices.Add(3 * k);
				Indices.Add(3 * k + 1);
				Indices.Add(3 * k + 2);
			}
		}
	}

	/**
	 * RecomputeRenderBufferTriangleIndexSets re-sorts the existing set of triangles in a FMeshRenderBufferSet
	 * into primary and secondary index buffers. Note that UploadIndexBufferUpdate() must be called
	 * after this function!
	 */
	void RecomputeRenderBufferTriangleIndexSets(
		FMeshRenderBufferSet* RenderBuffers,
		const FDynamicMesh3* Mesh)
	{
		if (RenderBuffers->TriangleCount == 0)
		{
			return;
		}
		if (ensure(RenderBuffers->Triangles.IsSet() && RenderBuffers->Triangles->Num() > 0) == false)
		{
			return;
		}

		//bool bDuplicate = false;		// flag for future use, in case we want to draw all triangles in primary and duplicates in secondary...
		RenderBuffers->IndexBuffer.Indices.Reset();
		RenderBuffers->SecondaryIndexBuffer.Indices.Reset();

		TArray<uint32>& Indices = RenderBuffers->IndexBuffer.Indices;
		TArray<uint32>& SecondaryIndices = RenderBuffers->SecondaryIndexBuffer.Indices;
		const TArray<int32>& TriangleIDs = RenderBuffers->Triangles.GetValue();

		int NumTris = TriangleIDs.Num();
		for (int k = 0; k < NumTris; ++k)
		{
			int TriangleID = TriangleIDs[k];
			bool bInclude = SecondaryTriFilterFunc(Mesh, TriangleID);
			if (bInclude)
			{
				SecondaryIndices.Add(3 * k);
				SecondaryIndices.Add(3 * k + 1);
				SecondaryIndices.Add(3 * k + 2);
			}
			else // if (bDuplicate == false)
			{
				Indices.Add(3 * k);
				Indices.Add(3 * k + 1);
				Indices.Add(3 * k + 2);
			}
		}
	}

	/**
	 * Update vertex positions/normals/colors of an existing set of render buffers.
	 * Assumes that buffers were created with unshared vertices, ie three vertices per triangle, eg by InitializeBuffersFromOverlays()
	 */
	template<typename TriangleEnumerable>
	void UpdateVertexBuffersFromOverlays(
		FMeshRenderBufferSet* RenderBuffers,
		const FDynamicMesh3* Mesh,
		int NumTriangles, TriangleEnumerable Enumerable,
		const FDynamicMeshNormalOverlay* NormalOverlay,
		const FDynamicMeshColorOverlay* ColorOverlay,
		TFunctionRef<void(int, int, int, const FVector3f&, FVector3f&, FVector3f&)> TangentsFunc,
		bool bUpdatePositions = true,
		bool bUpdateNormals = false,
		bool bUpdateColors = false)
	{
		if (RenderBuffers->TriangleCount == 0)
		{
			return;
		}

		bool bHaveColors = (ColorOverlay != nullptr) && (bIgnoreVertexColors == false);

		int NumVertices = NumTriangles * 3;
		if ((bUpdatePositions && ensure(RenderBuffers->PositionVertexBuffer.GetNumVertices() == NumVertices) == false)
			|| (bUpdateNormals && ensure(RenderBuffers->StaticMeshVertexBuffer.GetNumVertices() == NumVertices) == false)
			|| (bUpdateColors && ensure(RenderBuffers->ColorVertexBuffer.GetNumVertices() == NumVertices) == false))
		{
			return;
		}

		int VertIdx = 0;
		FVector3f TangentX, TangentY;
		for (int TriangleID : Enumerable)
		{
			FIndex3i Tri = Mesh->GetTriangle(TriangleID);

			FIndex3i TriNormal = (bUpdateNormals && NormalOverlay != nullptr) ? NormalOverlay->GetTriangle(TriangleID) : FIndex3i::Zero();
			FIndex3i TriColor = (bUpdateColors && ColorOverlay != nullptr) ? ColorOverlay->GetTriangle(TriangleID) : FIndex3i::Zero();

			FColor UniformTriColor = ConstantVertexColor;
			if (bUpdateColors && bUsePerTriangleColor && PerTriangleColorFunc != nullptr)
			{
				UniformTriColor = PerTriangleColorFunc(Mesh, TriangleID);
				bHaveColors = false;
			}

			for (int j = 0; j < 3; ++j)
			{
				if (bUpdatePositions)
				{
					RenderBuffers->PositionVertexBuffer.VertexPosition(VertIdx) = (FVector)Mesh->GetVertex(Tri[j]);
				}

				if (bUpdateNormals)
				{
					// get normal and tangent
					FVector3f Normal;
					if (bUsePerTriangleNormals)
					{
						Normal = (FVector3f)Mesh->GetTriNormal(TriangleID);
					}
					else
					{
						Normal = (NormalOverlay != nullptr && TriNormal[j] != FDynamicMesh3::InvalidID) ?
							NormalOverlay->GetElement(TriNormal[j]) : Mesh->GetVertexNormal(Tri[j]);
					}

					TangentsFunc(Tri[j], TriangleID, j, Normal, TangentX, TangentY);

					RenderBuffers->StaticMeshVertexBuffer.SetVertexTangents(VertIdx, (FVector)TangentX, (FVector)TangentY, (FVector)Normal);
				}

				if (bUpdateColors)
				{
					FColor VertexFColor = (bHaveColors && TriColor[j] != FDynamicMesh3::InvalidID) ?
						GetOverlayColorAsFColor(ColorOverlay, TriColor[j]) : UniformTriColor;
					RenderBuffers->ColorVertexBuffer.VertexColor(VertIdx) = VertexFColor;
				}

				VertIdx++;
			}
		}
	}



	/**
	 * Update vertex uvs of an existing set of render buffers.
	 * Assumes that buffers were created with unshared vertices, ie three vertices per triangle, eg by InitializeBuffersFromOverlays()
	 */
	template<typename TriangleEnumerable, typename UVOverlayListAllocator>
	void UpdateVertexUVBufferFromOverlays(
		FMeshRenderBufferSet* RenderBuffers,
		const FDynamicMesh3* Mesh,
		int32 NumTriangles, TriangleEnumerable Enumerable,
		const TArray<const FDynamicMeshUVOverlay*, UVOverlayListAllocator>& UVOverlays)
	{
		// We align the update to the way we set UV's in InitializeBuffersFromOverlays.

		if (RenderBuffers->TriangleCount == 0)
		{
			return;
		}
		int NumVertices = NumTriangles * 3;
		if (ensure(RenderBuffers->StaticMeshVertexBuffer.GetNumVertices() == NumVertices) == false)
		{
			return;
		}

		int NumUVOverlays = UVOverlays.Num();
		int NumTexCoords = RenderBuffers->StaticMeshVertexBuffer.GetNumTexCoords();
		if (!ensure(NumUVOverlays <= NumTexCoords))
		{
			return;
		}

		// Temporarily stores the UV element indices for all UV channels of a single triangle
		TArray<FIndex3i, TFixedAllocator<MAX_STATIC_TEXCOORDS>> UVTriangles;
		UVTriangles.SetNum(NumTexCoords);

		int VertIdx = 0;
		for (int TriangleID : Enumerable)
		{
			for (int32 k = 0; k < NumTexCoords; ++k)
			{
				UVTriangles[k] = (k < NumUVOverlays && UVOverlays[k] != nullptr) ? UVOverlays[k]->GetTriangle(TriangleID) : FIndex3i::Invalid();
			}

			for (int j = 0; j < 3; ++j)
			{
				for (int32 k = 0; k < NumTexCoords; ++k)
				{
					FVector2f UV = (UVTriangles[k][j] != FDynamicMesh3::InvalidID) ?
						UVOverlays[k]->GetElement(UVTriangles[k][j]) : FVector2f::Zero();
					RenderBuffers->StaticMeshVertexBuffer.SetVertexUV(VertIdx, k, (FVector2D&)UV);
				}

				++VertIdx;
			}
		}
	}




	/**
	 * @return number of active materials
	 */
	virtual int32 GetNumMaterials() const
	{
		return ParentBaseComponent->GetNumMaterials();
	}

	/**
	 * Safe GetMaterial function that will never return nullptr
	 */
	virtual UMaterialInterface* GetMaterial(int32 k) const
	{
		UMaterialInterface* Material = ParentBaseComponent->GetMaterial(k);
		return (Material != nullptr) ? Material : UMaterial::GetDefaultMaterial(MD_Surface);
	}


	/**
	 * This needs to be called if the set of active materials changes, otherwise
	 * the check in FPrimitiveSceneProxy::VerifyUsedMaterial() will fail if an override
	 * material is set, if materials change, etc, etc
	 */
	virtual void UpdatedReferencedMaterials()
	{
		// copied from FPrimitiveSceneProxy::FPrimitiveSceneProxy()
#if WITH_EDITOR
		TArray<UMaterialInterface*> Materials;
		ParentBaseComponent->GetUsedMaterials(Materials, true);
		ENQUEUE_RENDER_COMMAND(FMeshRenderBufferSetDestroy)(
			[this, Materials](FRHICommandListImmediate& RHICmdList)
		{
			this->SetUsedMaterialForVerification(Materials);
		});
#endif
	}


	//
	// FBaseDynamicMeshSceneProxy implementation
	//


	/**
	 * Render set of active RenderBuffers returned by GetActiveRenderBufferSets
	 */
	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views, 
		const FSceneViewFamily& ViewFamily, 
		uint32 VisibilityMap, 
		FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_OctreeDynamicMeshSceneProxy_GetDynamicMeshElements);

		const bool bWireframe = (AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe)
			|| ParentBaseComponent->GetEnableWireframeRenderPass();
		
		// set up wireframe material. Probably bad to reference GEngine here...also this material is very bad?
		FMaterialRenderProxy* WireframeMaterialProxy = nullptr;
		if (bWireframe)
		{
			    FLinearColor UseWireframeColor = (IsSelected() && (ParentBaseComponent->GetEnableWireframeRenderPass() == false 
					|| (AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe))) ?
				GEngine->GetSelectedMaterialColor() : ParentBaseComponent->WireframeColor;

				FColoredMaterialRenderProxy* WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
					UseWireframeColor
			);
				Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
				WireframeMaterialProxy = WireframeMaterialInstance;
			}

		ESceneDepthPriorityGroup DepthPriority = SDPG_World;

		TArray<FMeshRenderBufferSet*> Buffers;
		GetActiveRenderBufferSets(Buffers);

		FMaterialRenderProxy* SecondaryMaterialProxy =
			ParentBaseComponent->HasSecondaryRenderMaterial() ? ParentBaseComponent->GetSecondaryRenderMaterial()->GetRenderProxy() : nullptr;
		bool bDrawSecondaryBuffers = ParentBaseComponent->GetSecondaryBuffersVisibility();

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];

				bool bHasPrecomputedVolumetricLightmap;
				FMatrix PreviousLocalToWorld;
				int32 SingleCaptureIndex;
				bool bOutputVelocity;
				GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

				// Draw the mesh.
				for (FMeshRenderBufferSet* BufferSet : Buffers)
				{
					UMaterialInterface* UseMaterial = BufferSet->Material;
					if (ParentBaseComponent->HasOverrideRenderMaterial(0))
					{
						UseMaterial = ParentBaseComponent->GetOverrideRenderMaterial(0);
					}
					FMaterialRenderProxy* MaterialProxy = UseMaterial->GetRenderProxy();

					if (BufferSet->TriangleCount == 0)
					{
						continue;
					}

					// lock buffers so that they aren't modified while we are submitting them
					FScopeLock BuffersLock(&BufferSet->BuffersLock);

					// do we need separate one of these for each MeshRenderBufferSet?
					FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
					DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, DrawsVelocity(), bOutputVelocity);

					if (BufferSet->IndexBuffer.Indices.Num() > 0)
					{
						// Unlike most meshes, which just use the wireframe material in wireframe mode, we draw the wireframe on top of the normal material if needed,
						// as this is easier to interpret. However, we do not do this in ortho viewports, where it frequently causes the our edit gizmo to be hidden 
						// beneath the material. So, only draw the base material if we are in perspective mode, or we're in ortho but not in wireframe.
						if (View->IsPerspectiveProjection() || !(AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe))
						{
							DrawBatch(Collector, *BufferSet, BufferSet->IndexBuffer, MaterialProxy, false, DepthPriority, ViewIndex, DynamicPrimitiveUniformBuffer);
						}
						if (bWireframe)
						{
							DrawBatch(Collector, *BufferSet, BufferSet->IndexBuffer, WireframeMaterialProxy, true, DepthPriority, ViewIndex, DynamicPrimitiveUniformBuffer);
						}
					}

					// draw secondary buffer if we have it, falling back to base material if we don't have the Secondary material
					FMaterialRenderProxy* UseSecondaryMaterialProxy = (SecondaryMaterialProxy != nullptr) ? SecondaryMaterialProxy : MaterialProxy;
					if (bDrawSecondaryBuffers && BufferSet->SecondaryIndexBuffer.Indices.Num() > 0 && UseSecondaryMaterialProxy != nullptr)
					{
						DrawBatch(Collector, *BufferSet, BufferSet->SecondaryIndexBuffer, UseSecondaryMaterialProxy, false, DepthPriority, ViewIndex, DynamicPrimitiveUniformBuffer);
						if (bWireframe)
						{
							DrawBatch(Collector, *BufferSet, BufferSet->SecondaryIndexBuffer, UseSecondaryMaterialProxy, true, DepthPriority, ViewIndex, DynamicPrimitiveUniformBuffer);
						}
					}
				}
			}
		}
	}



	/**
	 * Draw a single-frame FMeshBatch for a FMeshRenderBufferSet
	 */
	virtual void DrawBatch(FMeshElementCollector& Collector,
		const FMeshRenderBufferSet& RenderBuffers,
		const FDynamicMeshIndexBuffer32& IndexBuffer,
		FMaterialRenderProxy* UseMaterial,
		bool bWireframe,
		ESceneDepthPriorityGroup DepthPriority,
		int ViewIndex,
		FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer) const
	{
		FMeshBatch& Mesh = Collector.AllocateMesh();
		FMeshBatchElement& BatchElement = Mesh.Elements[0];
		BatchElement.IndexBuffer = &IndexBuffer;
		Mesh.bWireframe = bWireframe;
		Mesh.VertexFactory = &RenderBuffers.VertexFactory;
		Mesh.MaterialRenderProxy = UseMaterial;

		BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

		BatchElement.FirstIndex = 0;
		BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = RenderBuffers.PositionVertexBuffer.GetNumVertices() - 1;
		Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
		Mesh.Type = PT_TriangleList;
		Mesh.DepthPriorityGroup = DepthPriority;
		Mesh.bCanApplyViewModeOverrides = false;
		Collector.AddMesh(ViewIndex, Mesh);
	}

	/**
	 * Set whether or not to validate mesh batch materials against the component materials.
	 */
	void SetVerifyUsedMaterials(const bool bState)
	{
		bVerifyUsedMaterials = bState;
	}


};