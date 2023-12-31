// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicMeshAttributeSet.h"
#include "IndexTypes.h"
#include "Async/ParallelFor.h"




void FDynamicMeshAttributeSet::Copy(const FDynamicMeshAttributeSet& Copy)
{
	SetNumUVLayers(Copy.NumUVLayers());
	for (int UVIdx = 0; UVIdx < NumUVLayers(); UVIdx++)
	{
		UVLayers[UVIdx].Copy(Copy.UVLayers[UVIdx]);
	}
	SetNumNormalLayers(Copy.NumNormalLayers());
	for (int NormalLayerIndex = 0; NormalLayerIndex < NumNormalLayers(); NormalLayerIndex++)
	{
		NormalLayers[NormalLayerIndex].Copy(Copy.NormalLayers[NormalLayerIndex]);
	}
	if (Copy.ColorLayer)
	{
		EnablePrimaryColors();
		ColorLayer->Copy(*(Copy.ColorLayer));
	}
	else
	{
		DisablePrimaryColors();
	}
	if (Copy.MaterialIDAttrib)
	{
		EnableMaterialID();
		MaterialIDAttrib->Copy(*(Copy.MaterialIDAttrib));
	}
	else
	{
		DisableMaterialID();
	}

	SetNumPolygroupLayers(Copy.NumPolygroupLayers());
	for (int GroupIdx = 0; GroupIdx < NumPolygroupLayers(); ++GroupIdx)
	{
		PolygroupLayers[GroupIdx].Copy(Copy.PolygroupLayers[GroupIdx]);
	}

	GenericAttributes.Reset();
	ResetRegisteredAttributes();
	for (const TPair<FName, TUniquePtr<FDynamicMeshAttributeBase>>& AttribPair : Copy.GenericAttributes)
	{
		AttachAttribute(AttribPair.Key, AttribPair.Value->MakeCopy(ParentMesh));
	}

	// parent mesh is *not* copied!
}


bool FDynamicMeshAttributeSet::IsCompact()
{
	for (int UVIdx = 0; UVIdx < NumUVLayers(); UVIdx++)
	{
		if (!UVLayers[UVIdx].IsCompact())
		{
			return false;
		}
	}
	for (int NormalLayerIndex = 0; NormalLayerIndex < NumNormalLayers(); NormalLayerIndex++)
	{
		if (!NormalLayers[NormalLayerIndex].IsCompact())
		{
			return false;
		}
	}
	if (HasPrimaryColors())
	{
		if (!ColorLayer->IsCompact())
		{
			return false;
		}
	}
	
	// material ID and generic per-element attributes currently cannot be non-compact
	return true;
}



void FDynamicMeshAttributeSet::CompactCopy(const FCompactMaps& CompactMaps, const FDynamicMeshAttributeSet& Copy)
{
	SetNumUVLayers(Copy.NumUVLayers());
	for (int UVIdx = 0; UVIdx < NumUVLayers(); UVIdx++)
	{
		UVLayers[UVIdx].CompactCopy(CompactMaps, Copy.UVLayers[UVIdx]);
	}
	SetNumNormalLayers(Copy.NumNormalLayers());
	for (int NormalLayerIndex = 0; NormalLayerIndex < NumNormalLayers(); NormalLayerIndex++)
	{
		NormalLayers[NormalLayerIndex].CompactCopy(CompactMaps, Copy.NormalLayers[NormalLayerIndex]);
	}
	if (Copy.ColorLayer)
	{
		EnablePrimaryColors();
		ColorLayer->CompactCopy(CompactMaps, *(Copy.ColorLayer));
	}
	else
	{
		DisablePrimaryColors();
	}
	if (Copy.MaterialIDAttrib)
	{
		EnableMaterialID();
		MaterialIDAttrib->CompactCopy(CompactMaps, *(Copy.MaterialIDAttrib));
	}
	else
	{
		DisableMaterialID();
	}

	SetNumPolygroupLayers(Copy.NumPolygroupLayers());
	for (int GroupIdx = 0; GroupIdx < NumPolygroupLayers(); ++GroupIdx)
	{
		PolygroupLayers[GroupIdx].CompactCopy(CompactMaps, Copy.PolygroupLayers[GroupIdx]);
	}

	GenericAttributes.Reset();
	ResetRegisteredAttributes();
	for (const TPair<FName, TUniquePtr<FDynamicMeshAttributeBase>>& AttribPair : Copy.GenericAttributes)
	{
		AttachAttribute(AttribPair.Key, AttribPair.Value->MakeCompactCopy(CompactMaps, ParentMesh));
	}

	// parent mesh is *not* copied!
}




void FDynamicMeshAttributeSet::CompactInPlace(const FCompactMaps& CompactMaps)
{
	for (int UVIdx = 0; UVIdx < NumUVLayers(); UVIdx++)
	{
		UVLayers[UVIdx].CompactInPlace(CompactMaps);
	}
	for (int NormalLayerIndex = 0; NormalLayerIndex < NumNormalLayers(); NormalLayerIndex++)
	{
		NormalLayers[NormalLayerIndex].CompactInPlace(CompactMaps);
	}
	if (ColorLayer.IsValid())
	{
		ColorLayer->CompactInPlace(CompactMaps);
	}
	if (MaterialIDAttrib.IsValid())
	{
		MaterialIDAttrib->CompactInPlace(CompactMaps);
	}

	for (int GroupIdx = 0; GroupIdx < NumPolygroupLayers(); ++GroupIdx)
	{
		PolygroupLayers[GroupIdx].CompactInPlace(CompactMaps);
	}

	for (const TPair<FName, TUniquePtr<FDynamicMeshAttributeBase>>& AttribPair : GenericAttributes)
	{
		AttribPair.Value->CompactInPlace(CompactMaps);
	}
}



void FDynamicMeshAttributeSet::SplitAllBowties(bool bParallel)
{
	int32 UVLayerCount = NumUVLayers();
	int32 NormalLayerCount = NumNormalLayers();

	ParallelFor(UVLayerCount + NormalLayerCount, [&](int32 idx)
	{
		if (idx < NormalLayerCount)
		{
			GetNormalLayer(idx)->SplitBowties();
		}
		else
		{
			GetUVLayer(idx - NormalLayerCount)->SplitBowties();
		}
	}, (bParallel) ? EParallelForFlags::Unbalanced : EParallelForFlags::ForceSingleThread );
}



void FDynamicMeshAttributeSet::EnableMatchingAttributes(const FDynamicMeshAttributeSet& ToMatch, bool bClearExisting, bool bDiscardExtraAttributes)
{
	int32 ExistingUVLayers = NumUVLayers();
	int32 RequiredUVLayers = (bClearExisting || bDiscardExtraAttributes) ? ToMatch.NumUVLayers() : FMath::Max(ExistingUVLayers, ToMatch.NumUVLayers());
	SetNumUVLayers(RequiredUVLayers);
	for (int32 k = bClearExisting ? 0 : ExistingUVLayers; k < NumUVLayers(); k++)
	{
		UVLayers[k].ClearElements();
	}

	int32 ExistingNormalLayers = NumNormalLayers();
	int32 RequiredNormalLayers = (bClearExisting || bDiscardExtraAttributes) ? ToMatch.NumNormalLayers() : FMath::Max(ExistingNormalLayers, ToMatch.NumNormalLayers());
	SetNumNormalLayers(RequiredNormalLayers);
	for (int32 k = bClearExisting ? 0 : ExistingNormalLayers; k < NumNormalLayers(); k++)
	{
		NormalLayers[k].ClearElements();
	}

	bool bWantColorLayer = (bClearExisting || bDiscardExtraAttributes) ? ToMatch.HasPrimaryColors() : ( ToMatch.HasPrimaryColors() || this->HasPrimaryColors() );
	if (bClearExisting || bWantColorLayer == false)
	{
		DisablePrimaryColors();
	}
	if (bWantColorLayer)
	{
		EnablePrimaryColors();
	}

	bool bWantMaterialID = (bClearExisting || bDiscardExtraAttributes) ? ToMatch.HasMaterialID() : ( ToMatch.HasMaterialID() || this->HasMaterialID() );
	if (bClearExisting || bWantMaterialID == false)
	{
		DisableMaterialID();
	}
	if (bWantMaterialID)
	{
		EnableMaterialID();
	}

	// polygroup layers are handled by count, not by name...maybe wrong
	int32 ExistingPolygroupLayers = NumPolygroupLayers();
	int32 RequiredPolygroupLayers = (bClearExisting || bDiscardExtraAttributes) ? ToMatch.NumPolygroupLayers() : FMath::Max(ExistingPolygroupLayers, ToMatch.NumPolygroupLayers());
	SetNumPolygroupLayers(RequiredPolygroupLayers);
	for (int32 k = bClearExisting ? 0 : ExistingPolygroupLayers; k < NumPolygroupLayers(); k++)
	{
		PolygroupLayers[k].Initialize((int32)0);
		if (PolygroupLayers[k].GetName() == NAME_None && k < ToMatch.NumPolygroupLayers())
		{
			PolygroupLayers[k].SetName( ToMatch.GetPolygroupLayer(k)->GetName() );
		}
	}
	if (bClearExisting)
	{
		GenericAttributes.Reset();
		ResetRegisteredAttributes();

		for (const TPair<FName, TUniquePtr<FDynamicMeshAttributeBase>>& AttribPair : ToMatch.GenericAttributes)
		{
			AttachAttribute(AttribPair.Key, AttribPair.Value->MakeNew(ParentMesh));
		}
	}
	else
	{
		// get rid of any attributes in current SkinWeights and Generic sets that are not in ToMatch
		if (bDiscardExtraAttributes)
		{
			GenericAttributesMap ExistingGenericAttributes = MoveTemp(GenericAttributes);
			GenericAttributes.Reset();
			ResetRegisteredAttributes();
			for (TPair<FName, TUniquePtr<FDynamicMeshAttributeBase>>& AttribPair : ExistingGenericAttributes)
			{
				if (ToMatch.GenericAttributes.Contains(AttribPair.Key))
				{
					AttachAttribute(AttribPair.Key, AttribPair.Value.Release());
				}
			}
		}
		for (const TPair<FName, TUniquePtr<FDynamicMeshAttributeBase>>& AttribPair : ToMatch.GenericAttributes)
		{
			TUniquePtr<FDynamicMeshAttributeBase>* FoundMatch = GenericAttributes.Find(AttribPair.Key);
			if (FoundMatch == nullptr)
			{
				AttachAttribute(AttribPair.Key, AttribPair.Value->MakeNew(ParentMesh));
			}
		}

	}
	
	
}



void FDynamicMeshAttributeSet::Reparent(FDynamicMesh3* NewParent)
{
	ParentMesh = NewParent;

	for (int UVIdx = 0; UVIdx < NumUVLayers(); UVIdx++)
	{
		UVLayers[UVIdx].Reparent(NewParent);
	}
	for (int NormalLayerIndex = 0; NormalLayerIndex < NumNormalLayers(); NormalLayerIndex++)
	{
		NormalLayers[NormalLayerIndex].Reparent(NewParent);
	}
	if (ColorLayer)
	{
		ColorLayer->Reparent(NewParent);
	}
	if (MaterialIDAttrib)
	{
		MaterialIDAttrib->Reparent(NewParent);
	}

	for (int GroupIdx = 0; GroupIdx < NumPolygroupLayers(); ++GroupIdx)
	{
		PolygroupLayers[GroupIdx].Reparent(NewParent);
	}

	for (const TPair<FName, TUniquePtr<FDynamicMeshAttributeBase>>& AttribPair : GenericAttributes)
	{
		AttribPair.Value->Reparent(NewParent);
	}
}



void FDynamicMeshAttributeSet::SetNumUVLayers(int Num)
{
	if (UVLayers.Num() == Num)
	{
		return;
	}
	if (Num >= UVLayers.Num())
	{
		for (int i = (int)UVLayers.Num(); i < Num; ++i)
		{
			FDynamicMeshUVOverlay* NewUVLayer = new FDynamicMeshUVOverlay(ParentMesh);
			NewUVLayer->InitializeTriangles(ParentMesh->MaxTriangleID());
			UVLayers.Add(NewUVLayer);
		}
	}
	else
	{
		UVLayers.RemoveAt(Num, UVLayers.Num() - Num);
	}
	check(UVLayers.Num() == Num);
}



void FDynamicMeshAttributeSet::EnableTangents()
{
	SetNumNormalLayers(3);
}

void FDynamicMeshAttributeSet::DisableTangents()
{
	SetNumNormalLayers(1);
}


void FDynamicMeshAttributeSet::SetNumNormalLayers(int Num)
{
	if (NormalLayers.Num() == Num)
	{
		return;
	}
	if (Num >= NormalLayers.Num())
	{
		for (int32 i = NormalLayers.Num(); i < Num; ++i)
		{
			FDynamicMeshNormalOverlay* NewNormalLayer = new FDynamicMeshNormalOverlay(ParentMesh);
			NewNormalLayer->InitializeTriangles(ParentMesh->MaxTriangleID());
			NormalLayers.Add(NewNormalLayer);
		}
	}
	else
	{
		NormalLayers.RemoveAt(Num, UVLayers.Num() - Num);
	}
	check(NormalLayers.Num() == Num);
}

void FDynamicMeshAttributeSet::EnablePrimaryColors()
{
	if (HasPrimaryColors() == false)
	{
		ColorLayer = MakeUnique<FDynamicMeshColorOverlay>(ParentMesh);
		ColorLayer->InitializeTriangles(ParentMesh->MaxTriangleID());
	}
}

void FDynamicMeshAttributeSet::DisablePrimaryColors()
{
	ColorLayer.Reset();
}

int32 FDynamicMeshAttributeSet::NumPolygroupLayers() const
{
	return PolygroupLayers.Num();
}


void FDynamicMeshAttributeSet::SetNumPolygroupLayers(int32 Num)
{
	if (PolygroupLayers.Num() == Num)
	{
		return;
	}
	if (Num >= PolygroupLayers.Num())
	{
		for (int i = (int)PolygroupLayers.Num(); i < Num; ++i)
		{
			PolygroupLayers.Add(new FDynamicMeshPolygroupAttribute(ParentMesh));
		}
	}
	else
	{
		PolygroupLayers.RemoveAt(Num, PolygroupLayers.Num() - Num);
	}
	check(PolygroupLayers.Num() == Num);
}

FDynamicMeshPolygroupAttribute* FDynamicMeshAttributeSet::GetPolygroupLayer(int Index)
{
	return &PolygroupLayers[Index];
}

const FDynamicMeshPolygroupAttribute* FDynamicMeshAttributeSet::GetPolygroupLayer(int Index) const
{
	return &PolygroupLayers[Index];
}



void FDynamicMeshAttributeSet::EnableMaterialID()
{
	if (HasMaterialID() == false)
	{
		MaterialIDAttrib = MakeUnique<FDynamicMeshMaterialAttribute>(ParentMesh);
		MaterialIDAttrib->Initialize((int32)0);
	}
}

void FDynamicMeshAttributeSet::DisableMaterialID()
{
	MaterialIDAttrib.Reset();
}



bool FDynamicMeshAttributeSet::IsSeamEdge(int eid) const
{
	for (const FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		if (UVLayer.IsSeamEdge(eid))
		{
			return true;
		}
	}

	for (const FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		if (NormalLayer.IsSeamEdge(eid))
		{
			return true;
		}
	}
	if (ColorLayer && ColorLayer->IsSeamEdge(eid))
	{
		return true;
	}
	return false;
}

bool FDynamicMeshAttributeSet::IsSeamEndEdge(int eid) const
{
	for (const FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		if (UVLayer.IsSeamEndEdge(eid))
		{
			return true;
		}
	}

	for (const FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		if (NormalLayer.IsSeamEndEdge(eid))
		{
			return true;
		}
	}
	if (ColorLayer && ColorLayer->IsSeamEndEdge(eid))
	{
		return true;
	}
	return false;
}

bool FDynamicMeshAttributeSet::IsSeamEdge(int EdgeID, bool& bIsUVSeamOut, bool& bIsNormalSeamOut) const
{
	bIsUVSeamOut = false;
	for (const FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		if (UVLayer.IsSeamEdge(EdgeID))
		{
			bIsUVSeamOut = true;
		}
	}

	bIsNormalSeamOut = false;
	for (const FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		if (NormalLayer.IsSeamEdge(EdgeID))
		{
			bIsNormalSeamOut = true;
		}
	}
	return (bIsUVSeamOut || bIsNormalSeamOut);
}


bool FDynamicMeshAttributeSet::IsSeamVertex(int VID, bool bBoundaryIsSeam) const
{
	for (const FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		if (UVLayer.IsSeamVertex(VID, bBoundaryIsSeam))
		{
			return true;
		}
	}
	for (const FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		if (NormalLayer.IsSeamVertex(VID, bBoundaryIsSeam))
		{
			return true;
		}
	}
	return false;
}

bool FDynamicMeshAttributeSet::IsMaterialBoundaryEdge(int EdgeID) const
{
	if ( MaterialIDAttrib == nullptr )
	{
		return false;
	}
	check(ParentMesh->IsEdge(EdgeID));
	const FDynamicMesh3::FEdge Edge = ParentMesh->GetEdge(EdgeID);
	const int Tri0 = Edge.Tri[0];
	const int Tri1 = Edge.Tri[1];
	if (( Tri0 == IndexConstants::InvalidID ) || (Tri1 == IndexConstants::InvalidID))
	{
		return false;
	}
	const int MatID0 = MaterialIDAttrib->GetValue(Tri0);
	const int MatID1 = MaterialIDAttrib->GetValue(Tri1);
	return MatID0 != MatID1;
}

void FDynamicMeshAttributeSet::OnNewVertex(int VertexID, bool bInserted)
{
	FDynamicMeshAttributeSetBase::OnNewVertex(VertexID, bInserted);
}


void FDynamicMeshAttributeSet::OnRemoveVertex(int VertexID)
{
	FDynamicMeshAttributeSetBase::OnRemoveVertex(VertexID);
}


void FDynamicMeshAttributeSet::OnNewTriangle(int TriangleID, bool bInserted)
{
	FDynamicMeshAttributeSetBase::OnNewTriangle(TriangleID, bInserted);

	for (FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		UVLayer.InitializeNewTriangle(TriangleID);
	}
	for (FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		NormalLayer.InitializeNewTriangle(TriangleID);
	}
	if (ColorLayer)
	{
		ColorLayer->InitializeNewTriangle(TriangleID);
	}
	if (MaterialIDAttrib)
	{
		int NewValue = 0;
		MaterialIDAttrib->SetNewValue(TriangleID, &NewValue);
	}

	for (FDynamicMeshPolygroupAttribute& PolygroupLayer : PolygroupLayers)
	{
		int32 NewGroup = 0;
		PolygroupLayer.SetNewValue(TriangleID, &NewGroup);
	}
}


void FDynamicMeshAttributeSet::OnRemoveTriangle(int TriangleID)
{
	FDynamicMeshAttributeSetBase::OnRemoveTriangle(TriangleID);

	for (FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		UVLayer.OnRemoveTriangle(TriangleID);
	}
	for (FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		NormalLayer.OnRemoveTriangle(TriangleID);
	}
	if (ColorLayer)
	{
		ColorLayer->OnRemoveTriangle(TriangleID);
	}
	// has no effect on MaterialIDAttrib
}

void FDynamicMeshAttributeSet::OnReverseTriOrientation(int TriangleID)
{
	FDynamicMeshAttributeSetBase::OnReverseTriOrientation(TriangleID);

	for (FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		UVLayer.OnReverseTriOrientation(TriangleID);
	}
	for (FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		NormalLayer.OnReverseTriOrientation(TriangleID);
	}
	if (ColorLayer)
	{
		ColorLayer->OnReverseTriOrientation(TriangleID);
	}
	// has no effect on MaterialIDAttrib
}

void FDynamicMeshAttributeSet::OnSplitEdge(const FDynamicMesh3::FEdgeSplitInfo& SplitInfo)
{
	FDynamicMeshAttributeSetBase::OnSplitEdge(SplitInfo);

	for (FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		UVLayer.OnSplitEdge(SplitInfo);
	}
	for (FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		NormalLayer.OnSplitEdge(SplitInfo);
	}
	if (ColorLayer)
	{
		ColorLayer->OnSplitEdge(SplitInfo);
	}
	if (MaterialIDAttrib)
	{
		MaterialIDAttrib->OnSplitEdge(SplitInfo);
	}

	for (FDynamicMeshPolygroupAttribute& PolygroupLayer : PolygroupLayers)
	{
		PolygroupLayer.OnSplitEdge(SplitInfo);
	}
}

void FDynamicMeshAttributeSet::OnFlipEdge(const FDynamicMesh3::FEdgeFlipInfo & flipInfo)
{
	FDynamicMeshAttributeSetBase::OnFlipEdge(flipInfo);

	for (FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		UVLayer.OnFlipEdge(flipInfo);
	}
	for (FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		NormalLayer.OnFlipEdge(flipInfo);
	}
	if (ColorLayer)
	{
		ColorLayer->OnFlipEdge(flipInfo);
	}
	if (MaterialIDAttrib)
	{
		MaterialIDAttrib->OnFlipEdge(flipInfo);
	}

	for (FDynamicMeshPolygroupAttribute& PolygroupLayer : PolygroupLayers)
	{
		PolygroupLayer.OnFlipEdge(flipInfo);
	}
}


void FDynamicMeshAttributeSet::OnCollapseEdge(const FDynamicMesh3::FEdgeCollapseInfo & collapseInfo)
{
	FDynamicMeshAttributeSetBase::OnCollapseEdge(collapseInfo);

	for (FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		UVLayer.OnCollapseEdge(collapseInfo);
	}
	for (FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		NormalLayer.OnCollapseEdge(collapseInfo);
	}
	if (ColorLayer)
	{
		ColorLayer->OnCollapseEdge(collapseInfo);
	}
	if (MaterialIDAttrib)
	{
		MaterialIDAttrib->OnCollapseEdge(collapseInfo);
	}

	for (FDynamicMeshPolygroupAttribute& PolygroupLayer : PolygroupLayers)
	{
		PolygroupLayer.OnCollapseEdge(collapseInfo);
	}
}

void FDynamicMeshAttributeSet::OnPokeTriangle(const FDynamicMesh3::FPokeTriangleInfo & pokeInfo)
{
	FDynamicMeshAttributeSetBase::OnPokeTriangle(pokeInfo);

	for (FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		UVLayer.OnPokeTriangle(pokeInfo);
	}
	for (FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		NormalLayer.OnPokeTriangle(pokeInfo);
	}
	if (ColorLayer)
	{
		ColorLayer->OnPokeTriangle(pokeInfo);
	}
	if (MaterialIDAttrib)
	{
		MaterialIDAttrib->OnPokeTriangle(pokeInfo);
	}

	for (FDynamicMeshPolygroupAttribute& PolygroupLayer : PolygroupLayers)
	{
		PolygroupLayer.OnPokeTriangle(pokeInfo);
	}
}

void FDynamicMeshAttributeSet::OnMergeEdges(const FDynamicMesh3::FMergeEdgesInfo & mergeInfo)
{
	FDynamicMeshAttributeSetBase::OnMergeEdges(mergeInfo);

	for (FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		UVLayer.OnMergeEdges(mergeInfo);
	}
	for (FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		NormalLayer.OnMergeEdges(mergeInfo);
	}
	if (ColorLayer)
	{
		ColorLayer->OnMergeEdges(mergeInfo);
	}
	if (MaterialIDAttrib)
	{
		MaterialIDAttrib->OnMergeEdges(mergeInfo);
	}

	for (FDynamicMeshPolygroupAttribute& PolygroupLayer : PolygroupLayers)
	{
		PolygroupLayer.OnMergeEdges(mergeInfo);
	}
}

void FDynamicMeshAttributeSet::OnSplitVertex(const DynamicMeshInfo::FVertexSplitInfo& SplitInfo, const TArrayView<const int>& TrianglesToUpdate)
{
	FDynamicMeshAttributeSetBase::OnSplitVertex(SplitInfo, TrianglesToUpdate);

	for (FDynamicMeshUVOverlay& UVLayer : UVLayers)
	{
		UVLayer.OnSplitVertex(SplitInfo, TrianglesToUpdate);
	}
	for (FDynamicMeshNormalOverlay& NormalLayer : NormalLayers)
	{
		NormalLayer.OnSplitVertex(SplitInfo, TrianglesToUpdate);
	}
	if (ColorLayer)
	{
		ColorLayer->OnSplitVertex(SplitInfo, TrianglesToUpdate);
	}
	if (MaterialIDAttrib)
	{
		MaterialIDAttrib->OnSplitVertex(SplitInfo, TrianglesToUpdate);
	}

	for (FDynamicMeshPolygroupAttribute& PolygroupLayer : PolygroupLayers)
	{
		PolygroupLayer.OnSplitVertex(SplitInfo, TrianglesToUpdate);
	}
}

