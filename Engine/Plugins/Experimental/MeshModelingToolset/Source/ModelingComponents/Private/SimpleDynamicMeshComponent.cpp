// Copyright Epic Games, Inc. All Rights Reserved. 

#include "SimpleDynamicMeshComponent.h"
#include "RenderingThread.h"
#include "RenderResource.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "VertexFactory.h"
#include "MaterialShared.h"
#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"
#include "LocalVertexFactory.h"
#include "SceneManagement.h"
#include "DynamicMeshBuilder.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "StaticMeshResources.h"
#include "StaticMeshAttributes.h"
#include "Async/Async.h"
#include "Util/ColorConstants.h"
#include "Engine/CollisionProfile.h"

#include "DynamicMeshAttributeSet.h"
#include "MeshNormals.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "DynamicMeshToMeshDescription.h"

#include "Changes/MeshVertexChange.h"
#include "Changes/MeshChange.h"
#include "DynamicMeshChangeTracker.h"
#include "MeshTransforms.h"



// default proxy for this component
#include "SimpleDynamicMeshSceneProxy.h"

namespace
{
	// probably should be something defined for the whole tool framework...
#if WITH_EDITOR
	static EAsyncExecution DynamicMeshComponentAsyncExecTarget = EAsyncExecution::LargeThreadPool;
#else
	static EAsyncExecution DynamicMeshComponentAsyncExecTarget = EAsyncExecution::ThreadPool;
#endif
}

namespace UELocal
{
	static EMeshRenderAttributeFlags ConvertChangeFlagsToUpdateFlags(EDynamicMeshAttributeChangeFlags ChangeFlags)
	{
		EMeshRenderAttributeFlags UpdateFlags = EMeshRenderAttributeFlags::None;
		if ((ChangeFlags & EDynamicMeshAttributeChangeFlags::VertexPositions) != EDynamicMeshAttributeChangeFlags::Unknown)
		{
			UpdateFlags |= EMeshRenderAttributeFlags::Positions;
		}
		if ((ChangeFlags & EDynamicMeshAttributeChangeFlags::NormalsTangents) != EDynamicMeshAttributeChangeFlags::Unknown)
		{
			UpdateFlags |= EMeshRenderAttributeFlags::VertexNormals;
		}
		if ((ChangeFlags & EDynamicMeshAttributeChangeFlags::VertexColors) != EDynamicMeshAttributeChangeFlags::Unknown)
		{
			UpdateFlags |= EMeshRenderAttributeFlags::VertexColors;
		}
		if ((ChangeFlags & EDynamicMeshAttributeChangeFlags::UVs) != EDynamicMeshAttributeChangeFlags::Unknown)
		{
			UpdateFlags |= EMeshRenderAttributeFlags::VertexUVs;
		}
		return UpdateFlags;
	}

}


USimpleDynamicMeshComponent::USimpleDynamicMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	MeshObject = CreateDefaultSubobject<UDynamicMesh>(TEXT("DynamicMesh"));

	MeshObjectChangedHandle = MeshObject->OnMeshChanged().AddUObject(this, &USimpleDynamicMeshComponent::OnMeshObjectChanged);

	//InitializeNewMesh();
}



void USimpleDynamicMeshComponent::InitializeMesh(FMeshDescription* MeshDescription)
{
	FMeshDescriptionToDynamicMesh Converter;
	MeshObject->InitializeMesh();
	Converter.Convert(MeshDescription, *GetMesh());
	if (TangentsType == EDynamicMeshTangentCalcType::ExternallyCalculated)
	{
		Converter.CopyTangents(MeshDescription, GetMesh(), &Tangents);
	}

	NotifyMeshUpdated();
}


void USimpleDynamicMeshComponent::UpdateTangents(const FMeshTangentsf* ExternalTangents, bool bFastUpdateIfPossible)
{
	Tangents.CopyTriVertexTangents(*ExternalTangents);
	if (bFastUpdateIfPossible)
	{
		FastNotifyVertexAttributesUpdated(EMeshRenderAttributeFlags::VertexNormals);
	}
	else
	{
		NotifyMeshUpdated();
	}
}

void USimpleDynamicMeshComponent::UpdateTangents(const FMeshTangentsd* ExternalTangents, bool bFastUpdateIfPossible)
{
	Tangents.CopyTriVertexTangents(*ExternalTangents);
	if (bFastUpdateIfPossible)
	{
		FastNotifyVertexAttributesUpdated(EMeshRenderAttributeFlags::VertexNormals);
	}
	else
	{
		NotifyMeshUpdated();
	}
}


TUniquePtr<FDynamicMesh3> USimpleDynamicMeshComponent::ExtractMesh(bool bNotifyUpdate)
{
	TUniquePtr<FDynamicMesh3> CurMesh = MakeUnique<FDynamicMesh3>(*GetMesh());
	InitializeNewMesh();
	if (bNotifyUpdate)
	{
		NotifyMeshUpdated();
	}
	return CurMesh;
}


void USimpleDynamicMeshComponent::InitializeNewMesh()
{
	MeshObject->InitializeMesh();
	Tangents.SetMesh(GetMesh());
}


void USimpleDynamicMeshComponent::ApplyTransform(const FTransform3d& Transform, bool bInvert)
{
	if (bInvert)
	{
		MeshTransforms::ApplyTransformInverse(*GetMesh(), Transform);
	}
	else
	{
		MeshTransforms::ApplyTransform(*GetMesh(), Transform);
	}

	NotifyMeshUpdated();
}


void USimpleDynamicMeshComponent::Bake(FMeshDescription* MeshDescription, bool bHaveModifiedTopology, const FConversionToMeshDescriptionOptions& ConversionOptions)
{
	FDynamicMeshToMeshDescription Converter(ConversionOptions);
	if (bHaveModifiedTopology == false && Converter.HaveMatchingElementCounts(GetMesh(), MeshDescription))
	{
		if (ConversionOptions.bUpdatePositions)
		{
			Converter.Update(GetMesh(), *MeshDescription, ConversionOptions.bUpdateNormals, ConversionOptions.bUpdateUVs);
		}
		else
		{
			Converter.UpdateAttributes(GetMesh(), *MeshDescription, ConversionOptions.bUpdateNormals, ConversionOptions.bUpdateUVs);
		}
	}
	else
	{
		Converter.Convert(GetMesh(), *MeshDescription);

		//UE_LOG(LogTemp, Warning, TEXT("MeshDescription has %d instances"), MeshDescription->VertexInstances().Num());
	}
}






const FMeshTangentsf* USimpleDynamicMeshComponent::GetTangents()
{
	if (TangentsType == EDynamicMeshTangentCalcType::NoTangents)
	{
		return nullptr;
	}
	
	// in this mode we assume the tangents are valid
	if (TangentsType == EDynamicMeshTangentCalcType::ExternallyCalculated)
	{
		// if you hit this, you did not request ExternallyCalculated tangents before initializing this PreviewMesh
		GetAutoCalculatedTangents();
		Tangents = AutoCalculatedTangents;
		check(Tangents.GetTangents().Num() > 0)
	}

	return &Tangents;
}



void USimpleDynamicMeshComponent::SetDrawOnTop(bool bSet)
{
	bDrawOnTop = bSet;
	bUseEditorCompositing = bSet;
}

void USimpleDynamicMeshComponent::UpdateLocalBounds()
{
	LocalBounds = GetMesh()->GetBounds(true);
	if (LocalBounds.MaxDim() <= 0)
	{
		// If bbox is empty, set a very small bbox to avoid log spam/etc in other engine systems.
		// The check used is generally IsNearlyZero(), which defaults to KINDA_SMALL_NUMBER, so set 
		// a slightly larger box here to be above that threshold
		LocalBounds = FAxisAlignedBox3d(FVector3d::Zero(), (double)(KINDA_SMALL_NUMBER + SMALL_NUMBER));
	}
}

void USimpleDynamicMeshComponent::ResetProxy()
{
	bProxyValid = false;
	InvalidateAutoCalculatedTangents();

	// Need to recreate scene proxy to send it over
	MarkRenderStateDirty();
	UpdateLocalBounds();
	UpdateBounds();

	// this seems speculative, ie we may not actually have a mesh update, but we currently ResetProxy() in lots
	// of places where that is what it means
	GetDynamicMesh()->PostRealtimeUpdate();
}

void USimpleDynamicMeshComponent::NotifyMeshUpdated()
{
	if (RenderMeshPostProcessor)
	{
		RenderMeshPostProcessor->ProcessMesh(*GetMesh(), *RenderMesh);
	}

	ResetProxy();
}

void USimpleDynamicMeshComponent::NotifyMeshModified()
{
	NotifyMeshUpdated();
}


void USimpleDynamicMeshComponent::FastNotifyColorsUpdated()
{
	FSimpleDynamicMeshSceneProxy* Proxy = GetCurrentSceneProxy();
	if (Proxy != nullptr)
	{
		if (TriangleColorFunc != nullptr &&  Proxy->bUsePerTriangleColor == false )
		{
			Proxy->bUsePerTriangleColor = true;
			Proxy->PerTriangleColorFunc = [this](const FDynamicMesh3* MeshIn, int TriangleID) { return GetTriangleColor(MeshIn, TriangleID); };
		} 
		else if (TriangleColorFunc == nullptr && Proxy->bUsePerTriangleColor == true)
		{
			Proxy->bUsePerTriangleColor = false;
			Proxy->PerTriangleColorFunc = nullptr;
		}

		Proxy->FastUpdateVertices(false, false, true, false);
		//MarkRenderDynamicDataDirty();
	}
	else
	{
		NotifyMeshUpdated();
	}
}



void USimpleDynamicMeshComponent::FastNotifyPositionsUpdated(bool bNormals, bool bColors, bool bUVs)
{
	if (GetCurrentSceneProxy() != nullptr)
	{
		// calculate bounds while we are updating vertices
		TFuture<void> UpdateBoundsCalc;
		UpdateBoundsCalc = Async(DynamicMeshComponentAsyncExecTarget, [this]()
			{
				UpdateLocalBounds();
			});

		GetCurrentSceneProxy()->FastUpdateVertices(true, bNormals, bColors, bUVs);
		//MarkRenderDynamicDataDirty();
		MarkRenderTransformDirty();
		UpdateBoundsCalc.Wait();
		UpdateBounds();
	}
	else
	{
		NotifyMeshUpdated();
	}
}


void USimpleDynamicMeshComponent::FastNotifyVertexAttributesUpdated(bool bNormals, bool bColors, bool bUVs)
{
	check(bNormals || bColors || bUVs);
	if (GetCurrentSceneProxy() != nullptr)
	{
		GetCurrentSceneProxy()->FastUpdateVertices(false, bNormals, bColors, bUVs);
		//MarkRenderDynamicDataDirty();
		//MarkRenderTransformDirty();
	}
	else
	{
		NotifyMeshUpdated();
	}
}


void USimpleDynamicMeshComponent::FastNotifyVertexAttributesUpdated(EMeshRenderAttributeFlags UpdatedAttributes)
{
	check(UpdatedAttributes != EMeshRenderAttributeFlags::None);
	if (GetCurrentSceneProxy() != nullptr)
	{
		bool bPositions = (UpdatedAttributes & EMeshRenderAttributeFlags::Positions) != EMeshRenderAttributeFlags::None;
		GetCurrentSceneProxy()->FastUpdateVertices(bPositions,
			(UpdatedAttributes & EMeshRenderAttributeFlags::VertexNormals) != EMeshRenderAttributeFlags::None,
			(UpdatedAttributes & EMeshRenderAttributeFlags::VertexColors) != EMeshRenderAttributeFlags::None,
			(UpdatedAttributes & EMeshRenderAttributeFlags::VertexUVs) != EMeshRenderAttributeFlags::None);
		// calculate bounds while we are updating vertices
		TFuture<void> UpdateBoundsCalc;
		if (bPositions)
		{
			UpdateBoundsCalc = Async(DynamicMeshComponentAsyncExecTarget, [this]()
				{
					UpdateLocalBounds();
				});
		}

		if (bPositions)
		{
			MarkRenderTransformDirty();
			UpdateBoundsCalc.Wait();
			UpdateBounds();
		}
	}
	else
	{
		NotifyMeshUpdated();
	}
}


void USimpleDynamicMeshComponent::FastNotifyUVsUpdated()
{
	FastNotifyVertexAttributesUpdated(EMeshRenderAttributeFlags::VertexUVs);
}



void USimpleDynamicMeshComponent::FastNotifySecondaryTrianglesChanged()
{
	if (GetCurrentSceneProxy() != nullptr)
	{
		GetCurrentSceneProxy()->FastUpdateAllIndexBuffers();
	}
	else
	{
		NotifyMeshUpdated();
	}
}


void USimpleDynamicMeshComponent::FastNotifyTriangleVerticesUpdated(const TArray<int32>& Triangles, EMeshRenderAttributeFlags UpdatedAttributes)
{
	if (GetCurrentSceneProxy() == nullptr)
	{
		NotifyMeshUpdated();
	}
	else if ( ! Decomposition )
	{
		FastNotifyVertexAttributesUpdated(UpdatedAttributes);
	}
	else
	{
		TArray<int32> UpdatedSets;
		for (int32 tid : Triangles)
		{
			int32 SetID = Decomposition->GetGroupForTriangle(tid);
			UpdatedSets.AddUnique(SetID);
		}

		bool bPositions = (UpdatedAttributes & EMeshRenderAttributeFlags::Positions) != EMeshRenderAttributeFlags::None;
		GetCurrentSceneProxy()->FastUpdateVertices(UpdatedSets, bPositions,
			(UpdatedAttributes & EMeshRenderAttributeFlags::VertexNormals) != EMeshRenderAttributeFlags::None,
			(UpdatedAttributes & EMeshRenderAttributeFlags::VertexColors) != EMeshRenderAttributeFlags::None,
			(UpdatedAttributes & EMeshRenderAttributeFlags::VertexUVs) != EMeshRenderAttributeFlags::None);
		// calculate bounds while we are updating vertices
		TFuture<void> UpdateBoundsCalc;
		if (bPositions)
		{
			UpdateBoundsCalc = Async(DynamicMeshComponentAsyncExecTarget, [this]()
				{
					UpdateLocalBounds();
				});
		}

		if (bPositions)
		{
			MarkRenderTransformDirty();
			UpdateBoundsCalc.Wait();
			UpdateBounds();
		}
	}
}



void USimpleDynamicMeshComponent::FastNotifyTriangleVerticesUpdated(const TSet<int32>& Triangles, EMeshRenderAttributeFlags UpdatedAttributes)
{
	if (GetCurrentSceneProxy() == nullptr)
	{
		NotifyMeshUpdated();
	}
	else if (!Decomposition)
	{
		FastNotifyVertexAttributesUpdated(UpdatedAttributes);
	}
	else
	{
		TArray<int32> UpdatedSets;
		for (int32 tid : Triangles)
		{
			int32 SetID = Decomposition->GetGroupForTriangle(tid);
			UpdatedSets.AddUnique(SetID);
		}

		int32 TotalTris = 0;
		for (int32 SetID : UpdatedSets)
		{
			TotalTris += Decomposition->GetGroup(SetID).Triangles.Num();
		}

		//UE_LOG(LogTemp, Warning, TEXT("Updating %d groups with %d tris"), UpdatedSets.Num(), TotalTris);

		bool bPositions = (UpdatedAttributes & EMeshRenderAttributeFlags::Positions) != EMeshRenderAttributeFlags::None;

		// calculate bounds while we are updating vertices
		TFuture<void> UpdateBoundsCalc;
		if (bPositions)
		{
			UpdateBoundsCalc = Async(DynamicMeshComponentAsyncExecTarget, [this]()
				{
					TRACE_CPUPROFILER_EVENT_SCOPE(SimpleDynamicMeshComponent_FastVertexUpdate_AsyncBoundsUpdate);
					UpdateLocalBounds();
				});
		}

		GetCurrentSceneProxy()->FastUpdateVertices(UpdatedSets, bPositions,
			(UpdatedAttributes & EMeshRenderAttributeFlags::VertexNormals) != EMeshRenderAttributeFlags::None,
			(UpdatedAttributes & EMeshRenderAttributeFlags::VertexColors) != EMeshRenderAttributeFlags::None,
			(UpdatedAttributes & EMeshRenderAttributeFlags::VertexUVs) != EMeshRenderAttributeFlags::None);

		if (bPositions)
		{
			MarkRenderTransformDirty();
			UpdateBoundsCalc.Wait();
			//LocalBounds = Mesh->GetCachedBounds();
			UpdateBounds();
		}
	}
}




FPrimitiveSceneProxy* USimpleDynamicMeshComponent::CreateSceneProxy()
{
	ensure(GetCurrentSceneProxy() == nullptr);

	FSimpleDynamicMeshSceneProxy* NewProxy = nullptr;
	if (GetMesh()->TriangleCount() > 0)
	{
		NewProxy = new FSimpleDynamicMeshSceneProxy(this);

		if (TriangleColorFunc)
		{
			NewProxy->bUsePerTriangleColor = true;
			NewProxy->PerTriangleColorFunc = [this](const FDynamicMesh3* MeshIn, int TriangleID) { return GetTriangleColor(MeshIn, TriangleID); };
		}
		else if (GetColorOverrideMode() == EDynamicMeshComponentColorOverrideMode::Polygroups)
		{
			NewProxy->bUsePerTriangleColor = true;
			NewProxy->PerTriangleColorFunc = [this](const FDynamicMesh3* MeshIn, int TriangleID) { return GetGroupColor(MeshIn, TriangleID); };
		}

		if (HasVertexColorRemappingFunction())
		{
			NewProxy->bApplyVertexColorRemapping = true;
			NewProxy->VertexColorRemappingFunc = [this](FVector4f& Color) { RemapVertexColor(Color); };
		}

		if (SecondaryTriFilterFunc)
		{
			NewProxy->bUseSecondaryTriBuffers = true;
			NewProxy->SecondaryTriFilterFunc = [this](const FDynamicMesh3* MeshIn, int32 TriangleID)
				{
					return (SecondaryTriFilterFunc) ? SecondaryTriFilterFunc(MeshIn, TriangleID) : false;
				};
		}

		if (Decomposition)
		{
			NewProxy->InitializeFromDecomposition(Decomposition);
		}
		else
		{
			NewProxy->Initialize();
		}

		NewProxy->SetVerifyUsedMaterials(bProxyVerifyUsedMaterials);
	}

	bProxyValid = true;
	return NewProxy;
}


void USimpleDynamicMeshComponent::NotifyMaterialSetUpdated()
{
	if (GetCurrentSceneProxy() != nullptr)
	{
		GetCurrentSceneProxy()->UpdatedReferencedMaterials();
	}
}




void USimpleDynamicMeshComponent::EnableSecondaryTriangleBuffers(TUniqueFunction<bool(const FDynamicMesh3*, int32)> SecondaryTriFilterFuncIn)
{
	SecondaryTriFilterFunc = MoveTemp(SecondaryTriFilterFuncIn);
	NotifyMeshUpdated();
}

void USimpleDynamicMeshComponent::DisableSecondaryTriangleBuffers()
{
	SecondaryTriFilterFunc = nullptr;
	NotifyMeshUpdated();
}


void USimpleDynamicMeshComponent::SetExternalDecomposition(TUniquePtr<FMeshRenderDecomposition> DecompositionIn)
{
	ensure(DecompositionIn->Num() > 0);
	Decomposition = MoveTemp(DecompositionIn);
	NotifyMeshUpdated();
}



FColor USimpleDynamicMeshComponent::GetTriangleColor(const FDynamicMesh3* MeshIn, int TriangleID)
{
	if (TriangleColorFunc)
	{
		return TriangleColorFunc(MeshIn, TriangleID);
	}
	else
	{
		return (TriangleID % 2 == 0) ? FColor::Red : FColor::White;
	}
}



FBoxSphereBounds USimpleDynamicMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	// can get a tighter box by calculating in world space, but we care more about performance
	FBox LocalBoundingBox = (FBox)LocalBounds;
	FBoxSphereBounds Ret(LocalBoundingBox.TransformBy(LocalToWorld));
	Ret.BoxExtent *= BoundsScale;
	Ret.SphereRadius *= BoundsScale;
	return Ret;
}


void USimpleDynamicMeshComponent::ApplyChange(const FMeshVertexChange* Change, bool bRevert)
{
	// will fire UDynamicMesh::MeshChangedEvent, which will call OnMeshObjectChanged() below to invalidate proxy, fire change events, etc
	MeshObject->ApplyChange(Change, bRevert);
}




void USimpleDynamicMeshComponent::ApplyChange(const FMeshChange* Change, bool bRevert)
{
	// will fire UDynamicMesh::MeshChangedEvent, which will call OnMeshObjectChanged() below to invalidate proxy, fire change events, etc
	MeshObject->ApplyChange(Change, bRevert);
}


void USimpleDynamicMeshComponent::ApplyChange(const FMeshReplacementChange* Change, bool bRevert)
{
	// will fire UDynamicMesh::MeshChangedEvent, which will call OnMeshObjectChanged() below to invalidate proxy, fire change events, etc
	MeshObject->ApplyChange(Change, bRevert);
}

void USimpleDynamicMeshComponent::SetTangentsType(EDynamicMeshTangentCalcType NewTangentsType)
{
	if (NewTangentsType != TangentsType)
	{
		TangentsType = NewTangentsType;
		InvalidateAutoCalculatedTangents();
	}
}

void USimpleDynamicMeshComponent::InvalidateAutoCalculatedTangents()
{
	bAutoCalculatedTangentsValid = false;
}

const FMeshTangentsf* USimpleDynamicMeshComponent::GetAutoCalculatedTangents()
{
	if (GetTangentsType() == EDynamicMeshTangentCalcType::AutoCalculated && GetDynamicMesh()->GetMeshRef().HasAttributes())
	{
		UpdateAutoCalculatedTangents();
		return (bAutoCalculatedTangentsValid) ? &AutoCalculatedTangents : nullptr;
	}
	return nullptr;
}

void USimpleDynamicMeshComponent::UpdateAutoCalculatedTangents()
{
	if (GetTangentsType() == EDynamicMeshTangentCalcType::AutoCalculated && bAutoCalculatedTangentsValid == false)
	{
		GetDynamicMesh()->ProcessMesh([&](const FDynamicMesh3& Mesh)
			{
				if (Mesh.HasAttributes())
				{
					const FDynamicMeshUVOverlay* UVOverlay = Mesh.Attributes()->PrimaryUV();
					const FDynamicMeshNormalOverlay* NormalOverlay = Mesh.Attributes()->PrimaryNormals();
					if (UVOverlay && NormalOverlay)
					{
						AutoCalculatedTangents.SetMesh(&Mesh);
						AutoCalculatedTangents.ComputeTriVertexTangents(NormalOverlay, UVOverlay, FComputeTangentsOptions());
						AutoCalculatedTangents.SetMesh(nullptr);
					}
				}
			});

		bAutoCalculatedTangentsValid = true;
	}
}

void USimpleDynamicMeshComponent::SetMesh(FDynamicMesh3&& MoveMesh)
{
	MeshObject->SetMesh(MoveTemp(MoveMesh));
}


void USimpleDynamicMeshComponent::ProcessMesh(
	TFunctionRef<void(const FDynamicMesh3&)> ProcessFunc) const
{
	MeshObject->ProcessMesh(ProcessFunc);
}

void USimpleDynamicMeshComponent::EditMesh(TFunctionRef<void(FDynamicMesh3&)> EditFunc,
	EDynamicMeshComponentRenderUpdateMode UpdateMode)
{
	MeshObject->EditMesh(EditFunc);
	if (UpdateMode != EDynamicMeshComponentRenderUpdateMode::NoUpdate)
	{
		NotifyMeshUpdated();
	}
}

void USimpleDynamicMeshComponent::SetDynamicMesh(UDynamicMesh* NewMesh)
{
	if (ensure(NewMesh) == false)
	{
		return;
	}

	if (ensure(MeshObject))
	{
		MeshObject->OnMeshChanged().Remove(MeshObjectChangedHandle);
	}

	// set Outer of NewMesh to be this Component, ie transfer ownership. This is done via "renaming", which is
	// a bit odd, so the flags prevent some standard "renaming" behaviors from happening
	NewMesh->Rename(nullptr, this, REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
	MeshObject = NewMesh;
	MeshObjectChangedHandle = MeshObject->OnMeshChanged().AddUObject(this, &USimpleDynamicMeshComponent::OnMeshObjectChanged);

	NotifyMeshUpdated();
	OnMeshChanged.Broadcast();
}

void USimpleDynamicMeshComponent::OnMeshObjectChanged(UDynamicMesh* ChangedMeshObject, FDynamicMeshChangeInfo ChangeInfo)
{
	bool bIsFChange = (
		ChangeInfo.Type == EDynamicMeshChangeType::MeshChange
		|| ChangeInfo.Type == EDynamicMeshChangeType::MeshVertexChange
		|| ChangeInfo.Type == EDynamicMeshChangeType::MeshReplacementChange);

	if (bIsFChange)
	{
		if (bInvalidateProxyOnChange)
		{
			NotifyMeshUpdated();
		}

		OnMeshChanged.Broadcast();

		if (ChangeInfo.Type == EDynamicMeshChangeType::MeshVertexChange)
		{
			OnMeshVerticesChanged.Broadcast(this, ChangeInfo.VertexChange, ChangeInfo.bIsRevertChange);
		}
	}
	else
	{
		if (ChangeInfo.Type == EDynamicMeshChangeType::DeformationEdit)
		{
			// if ChangeType is a vertex deformation, we can do a fast-update of the vertex buffers
			// without fully rebuilding the SceneProxy
			EMeshRenderAttributeFlags UpdateFlags = UELocal::ConvertChangeFlagsToUpdateFlags(ChangeInfo.Flags);
			FastNotifyVertexAttributesUpdated(UpdateFlags);
		}
		else
		{
			NotifyMeshUpdated();
		}
		OnMeshChanged.Broadcast();
	}

}

void USimpleDynamicMeshComponent::SetSceneProxyVerifyUsedMaterials(bool bState)
{
	bProxyVerifyUsedMaterials = bState;
	if (FSimpleDynamicMeshSceneProxy* Proxy = GetCurrentSceneProxy())
	{
		Proxy->SetVerifyUsedMaterials(bState);
	}
}

FColor USimpleDynamicMeshComponent::GetGroupColor(const FDynamicMesh3* Meshin, int TriangleID) const
{
	int32 GroupID = Meshin->HasTriangleGroups() ? Meshin->GetTriangleGroup(TriangleID) : 0;
	return LinearColors::SelectFColor(GroupID);
}

bool USimpleDynamicMeshComponent::HasVertexColorRemappingFunction()
{
	return !!VertexColorMappingFunc;
}

void USimpleDynamicMeshComponent::SetVertexColorRemappingFunction(
	TUniqueFunction<void(FVector4f&)> ColorMapFuncIn,
	EDynamicMeshComponentRenderUpdateMode UpdateMode)
{
	VertexColorMappingFunc = MoveTemp(ColorMapFuncIn);

	if (UpdateMode == EDynamicMeshComponentRenderUpdateMode::FastUpdate)
	{
		FastNotifyColorsUpdated();
	}
	else if (UpdateMode == EDynamicMeshComponentRenderUpdateMode::FullUpdate)
	{
		NotifyMeshUpdated();
	}
}

void USimpleDynamicMeshComponent::ClearVertexColorRemappingFunction(EDynamicMeshComponentRenderUpdateMode UpdateMode)
{
	if (VertexColorMappingFunc)
	{
		VertexColorMappingFunc = nullptr;

		if (UpdateMode == EDynamicMeshComponentRenderUpdateMode::FastUpdate)
		{
			FastNotifyColorsUpdated();
		}
		else if (UpdateMode == EDynamicMeshComponentRenderUpdateMode::FullUpdate)
		{
			NotifyMeshUpdated();
		}
	}
}

void USimpleDynamicMeshComponent::RemapVertexColor(FVector4f& VertexColorInOut)
{
	if (VertexColorMappingFunc)
	{
		VertexColorMappingFunc(VertexColorInOut);
	}
}

FSimpleDynamicMeshSceneProxy* USimpleDynamicMeshComponent::GetCurrentSceneProxy()
{
	if (bProxyValid)
	{
		return (FSimpleDynamicMeshSceneProxy*)SceneProxy;
	}
	return nullptr;
}

void USimpleDynamicMeshComponent::NotifyMeshVertexAttributesModified(bool bPositions, bool bNormals, bool bUVs, bool bColors)
{
	EMeshRenderAttributeFlags Flags = EMeshRenderAttributeFlags::None;
	if (bPositions)
	{
		Flags |= EMeshRenderAttributeFlags::Positions;
	}
	if (bNormals)
	{
		Flags |= EMeshRenderAttributeFlags::VertexNormals;
	}
	if (bUVs)
	{
		Flags |= EMeshRenderAttributeFlags::VertexUVs;
	}
	if (bColors)
	{
		Flags |= EMeshRenderAttributeFlags::VertexColors;
	}

	if (Flags == EMeshRenderAttributeFlags::None)
	{
		return;
	}
	FastNotifyVertexAttributesUpdated(Flags);
}

void USimpleDynamicMeshComponent::SetRenderMeshPostProcessor(TUniquePtr<IRenderMeshPostProcessor> Processor)
{
	RenderMeshPostProcessor = MoveTemp(Processor);
	if (RenderMeshPostProcessor)
	{
		if (!RenderMesh)
		{
			RenderMesh = MakeUnique<FDynamicMesh3>(*GetMesh());
		}
	}
	else
	{
		// No post processor, no render mesh
		RenderMesh = nullptr;
	}
}

FDynamicMesh3* USimpleDynamicMeshComponent::GetRenderMesh()
{
	if (RenderMeshPostProcessor && RenderMesh)
	{
		return RenderMesh.Get();
	}
	else
	{
		return GetMesh();
	}
}

const FDynamicMesh3* USimpleDynamicMeshComponent::GetRenderMesh() const
{
	if (RenderMeshPostProcessor && RenderMesh)
	{
		return RenderMesh.Get();
	}
	else
	{
		return GetMesh();
	}
}

void USimpleDynamicMeshComponent::ConfigureMaterialSet(const TArray<UMaterialInterface*>& NewMaterialSet)
{
	for (int k = 0; k < NewMaterialSet.Num(); ++k)
	{
		SetMaterial(k, NewMaterialSet[k]);
	}
}

void USimpleDynamicMeshComponent::SetSimpleCollisionShapes(const struct FKAggregateGeom& AggGeomIn, bool bUpdateCollision)
{
	AggGeom = AggGeomIn;
	if (bUpdateCollision)
	{
		UpdateCollision(false);
	}
}

void USimpleDynamicMeshComponent::UpdateCollision(bool bOnlyIfPending)
{
	if (bOnlyIfPending == false || bCollisionUpdatePending)
	{
		RebuildPhysicsData();
	}
}

UBodySetup* USimpleDynamicMeshComponent::GetBodySetup()
{
	if (MeshBodySetup == nullptr)
	{
		UBodySetup* NewBodySetup = CreateBodySetupHelper();

		SetBodySetup(NewBodySetup);
	}

	return MeshBodySetup;
}

void USimpleDynamicMeshComponent::SetBodySetup(UBodySetup* NewSetup)
{
	if (ensure(NewSetup))
	{
		MeshBodySetup = NewSetup;
	}
}

void USimpleDynamicMeshComponent::RebuildPhysicsData()
{
	UWorld* World = GetWorld();
	const bool bUseAsyncCook = World && World->IsGameWorld() && bUseAsyncCooking;

	UBodySetup* BodySetup = nullptr;
	if (bUseAsyncCook)
	{
		// Abort all previous ones still standing
		for (UBodySetup* OldBody : AsyncBodySetupQueue)
		{
			OldBody->AbortPhysicsMeshAsyncCreation();
		}

		BodySetup = CreateBodySetupHelper();
		if (BodySetup)
		{
			AsyncBodySetupQueue.Add(BodySetup);
		}
	}
	else
	{
		AsyncBodySetupQueue.Empty();	// If for some reason we modified the async at runtime, just clear any pending async body setups
		BodySetup = GetBodySetup();
	}

	if (!BodySetup)
	{
		return;
	}

	BodySetup->CollisionTraceFlag = this->CollisionType;
	// Note: Directly assigning AggGeom wouldn't do some important-looking cleanup (clearing pointers on convex elements)
	//  so we RemoveSimpleCollision then AddCollisionFrom instead
	BodySetup->RemoveSimpleCollision();
	BodySetup->AddCollisionFrom(this->AggGeom);

	if (bUseAsyncCook)
	{
		BodySetup->CreatePhysicsMeshesAsync(FOnAsyncPhysicsCookFinished::CreateUObject(this, &USimpleDynamicMeshComponent::FinishPhysicsAsyncCook, BodySetup));
	}
	else
	{
		// New GUID as collision has changed
		BodySetup->BodySetupGuid = FGuid::NewGuid();
		// Also we want cooked data for this
		BodySetup->bHasCookedCollisionData = true;
		BodySetup->InvalidatePhysicsData();
		BodySetup->CreatePhysicsMeshes();
		RecreatePhysicsState();

		bCollisionUpdatePending = false;
	}
}

void USimpleDynamicMeshComponent::InvalidatePhysicsData()
{
	if (GetBodySetup())
	{
		GetBodySetup()->InvalidatePhysicsData();
		bCollisionUpdatePending = true;
	}
}

UBodySetup* USimpleDynamicMeshComponent::CreateBodySetupHelper()
{
	UBodySetup* NewBodySetup = nullptr;
	{
		FGCScopeGuard Scope;

		// Below flags are copied from UProceduralMeshComponent::CreateBodySetupHelper(). Without these flags, DynamicMeshComponents inside
		// a DynamicMeshActor BP will result on a GLEO error after loading and modifying a saved Level (but *not* on the initial save)
		// The UBodySetup in a template needs to be public since the property is Instanced and thus is the archetype of the instance meaning there is a direct reference
		NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public | RF_ArchetypeObject : RF_NoFlags));
	}
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	NewBodySetup->bGenerateMirroredCollision = false;
	NewBodySetup->CollisionTraceFlag = this->CollisionType;

	NewBodySetup->DefaultInstance.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	NewBodySetup->bSupportUVsAndFaceRemap = false; /* bSupportPhysicalMaterialMasks; */

	return NewBodySetup;
}

void USimpleDynamicMeshComponent::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
	TArray<UBodySetup*> NewQueue;
	NewQueue.Reserve(AsyncBodySetupQueue.Num());

	int32 FoundIdx;
	if (AsyncBodySetupQueue.Find(FinishedBodySetup, FoundIdx))
	{
		// Note: currently no-cook-needed is reported identically to cook failed.
		// Checking AggGeom.GetElemCount() here is a hack to distinguish the no-cook-needed case
		// TODO: remove this hack to distinguish the no-cook-needed case when/if that is no longer identical to the cook failed case
		if (bSuccess || FinishedBodySetup->AggGeom.GetElementCount() > 0)
		{
			// The new body was found in the array meaning it's newer, so use it
			MeshBodySetup = FinishedBodySetup;
			RecreatePhysicsState();

			// remove any async body setups that were requested before this one
			for (int32 AsyncIdx = FoundIdx + 1; AsyncIdx < AsyncBodySetupQueue.Num(); ++AsyncIdx)
			{
				NewQueue.Add(AsyncBodySetupQueue[AsyncIdx]);
			}

			AsyncBodySetupQueue = NewQueue;
		}
		else
		{
			AsyncBodySetupQueue.RemoveAt(FoundIdx);
		}
	}
}

void USimpleDynamicMeshComponent::EnableComplexAsSimpleCollision()
{
	SetComplexAsSimpleCollisionEnabled(true, true);
}

void USimpleDynamicMeshComponent::SetComplexAsSimpleCollisionEnabled(bool bEnabled, bool bImmediateUpdate)
{
	bool bModified = false;
	if (bEnabled)
	{
		if (bEnableComplexCollision == false)
		{
			bEnableComplexCollision = true;
			bModified = true;
		}
		if (CollisionType != ECollisionTraceFlag::CTF_UseComplexAsSimple)
		{
			CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
			bModified = true;
		}
	}
	else
	{
		if (bEnableComplexCollision == true)
		{
			bEnableComplexCollision = false;
			bModified = true;
		}
		if (CollisionType == ECollisionTraceFlag::CTF_UseComplexAsSimple)
		{
			CollisionType = ECollisionTraceFlag::CTF_UseDefault;
			bModified = true;
		}
	}
	if (bModified)
	{
		InvalidatePhysicsData();
	}
	if (bImmediateUpdate)
	{
		UpdateCollision(true);
	}
}