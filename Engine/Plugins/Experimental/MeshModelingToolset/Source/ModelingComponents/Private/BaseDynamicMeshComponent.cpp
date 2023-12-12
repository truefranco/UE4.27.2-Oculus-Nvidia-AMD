// Copyright Epic Games, Inc. All Rights Reserved. 

#include "BaseDynamicMeshComponent.h"
#include "BaseDynamicMeshSceneProxy.h"
//#include "ConversionUtils/VolumeToDynamicMesh.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "TargetInterfaces/MeshDescriptionCommitter.h"
#include "TargetInterfaces/MeshDescriptionProvider.h"
#include "DynamicMeshToMeshDescription.h"
#include "MeshConversionOptions.h" //FConversionToMeshDescriptionOptions
#include "MeshDescriptionToDynamicMesh.h"

#include "CompGeom/PolygonTriangulation.h"
#include "ConstrainedDelaunay2.h"
#include "Polygon2.h"
#include "DynamicMesh3.h"
#include "MeshNormals.h"
#include "MeshTransforms.h"
#include "GameFramework/Volume.h"
#include "MeshBoundaryLoops.h"
#include "MeshQueries.h"
#include "MeshRegionBoundaryLoops.h"
#include "Model.h"
#include "Operations/MergeCoincidentMeshEdges.h"
#include "Operations/MinimalHoleFiller.h"
#include "Operations/PlanarFlipsOptimization.h"
#include "Components/BrushComponent.h"

#if WITH_EDITOR
#include "Engine/Polys.h"
#endif

UBaseDynamicMeshComponent::UBaseDynamicMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}



void UBaseDynamicMeshComponent::SetOverrideRenderMaterial(UMaterialInterface* Material)
{
	if (OverrideRenderMaterial != Material)
	{
		OverrideRenderMaterial = Material;
		NotifyMaterialSetUpdated();
	}
}

void UBaseDynamicMeshComponent::ClearOverrideRenderMaterial()
{
	if (OverrideRenderMaterial != nullptr)
	{
		OverrideRenderMaterial = nullptr;
		NotifyMaterialSetUpdated();
	}
}





void UBaseDynamicMeshComponent::SetSecondaryRenderMaterial(UMaterialInterface* Material)
{
	if (SecondaryRenderMaterial != Material)
	{
		SecondaryRenderMaterial = Material;
		NotifyMaterialSetUpdated();
	}
}

void UBaseDynamicMeshComponent::ClearSecondaryRenderMaterial()
{
	if (SecondaryRenderMaterial != nullptr)
	{
		SecondaryRenderMaterial = nullptr;
		NotifyMaterialSetUpdated();
	}
}



void UBaseDynamicMeshComponent::SetSecondaryBuffersVisibility(bool bSecondaryVisibility)
{
	bDrawSecondaryBuffers = bSecondaryVisibility;
}

bool UBaseDynamicMeshComponent::GetSecondaryBuffersVisibility() const
{
	return bDrawSecondaryBuffers;
}

void UBaseDynamicMeshComponent::SetShadowsEnabled(bool bEnabled)
{
	FlushRenderingCommands();
	SetCastShadow(bEnabled);
	OnRenderingStateChanged(true);
}

void UBaseDynamicMeshComponent::SetViewModeOverridesEnabled(bool bEnabled)
{
	if (bEnableViewModeOverrides != bEnabled)
	{
		bEnableViewModeOverrides = bEnabled;
		OnRenderingStateChanged(false);
	}
}

void UBaseDynamicMeshComponent::SetEnableRaytracing(bool bSetEnabled)
{
	if (bEnableRaytracing != bSetEnabled)
	{
		bEnableRaytracing = bSetEnabled;
		OnRenderingStateChanged(true);
	}
}

bool UBaseDynamicMeshComponent::GetEnableRaytracing() const
{
	return bEnableRaytracing;
}


int32 UBaseDynamicMeshComponent::GetNumMaterials() const
{
	return BaseMaterials.Num();
}

void UBaseDynamicMeshComponent::OnRenderingStateChanged(bool bForceImmedateRebuild)
{
	if (bForceImmedateRebuild)
	{
		// finish any drawing so that we can be certain our SceneProxy is no longer in use before we rebuild it below
		FlushRenderingCommands(false);

		// force immediate rebuild of the SceneProxy
		if (IsRegistered())
		{
			ReregisterComponent();
		}
	}
	else
	{
		MarkRenderStateDirty();
	}
}

UMaterialInterface* UBaseDynamicMeshComponent::GetMaterial(int32 ElementIndex) const 
{
	return (ElementIndex >= 0 && ElementIndex < BaseMaterials.Num()) ? BaseMaterials[ElementIndex] : nullptr;
}

void UBaseDynamicMeshComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* Material)
{
	check(ElementIndex >= 0);
	if (ElementIndex >= BaseMaterials.Num())
	{
		BaseMaterials.SetNum(ElementIndex + 1, false);
	}
	BaseMaterials[ElementIndex] = Material;
}


void UBaseDynamicMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	UMeshComponent::GetUsedMaterials(OutMaterials, bGetDebugMaterials);
	if (OverrideRenderMaterial != nullptr)
	{
		OutMaterials.Add(OverrideRenderMaterial);
	}
	if (SecondaryRenderMaterial != nullptr)
	{
		OutMaterials.Add(SecondaryRenderMaterial);
	}
}

void UBaseDynamicMeshComponent::SetColorOverrideMode(EDynamicMeshComponentColorOverrideMode NewMode)
{
	if (ColorMode != NewMode)
	{
		ColorMode = NewMode;
		OnRenderingStateChanged(false);
	}
}

void UBaseDynamicMeshComponent::SetConstantOverrideColor(FColor NewColor)
{
	if (ConstantColor != NewColor)
	{
		ConstantColor = NewColor;
		OnRenderingStateChanged(false);
	}

}

void UBaseDynamicMeshComponent::SetVertexColorSpaceTransformMode(EDynamicMeshVertexColorTransformMode NewMode)
{
	if (ColorSpaceMode != NewMode)
	{
		ColorSpaceMode = NewMode;
		OnRenderingStateChanged(false);
	}
}

FDynamicMesh3 UE::DyVIA::GetDynamicMeshViaMeshDescription(
	IMeshDescriptionProvider& MeshDescriptionProvider)
{
	FDynamicMesh3 DynamicMesh;
	FMeshDescriptionToDynamicMesh Converter;
	Converter.bVIDsFromNonManifoldMeshDescriptionAttr = true;
	Converter.Convert(MeshDescriptionProvider.GetMeshDescription(), DynamicMesh);
	return DynamicMesh;
}

void UE::DyVIA::CommitDynamicMeshViaMeshDescription(
	FMeshDescription&& CurrentMeshDescription,
	IMeshDescriptionCommitter& MeshDescriptionCommitter,
	const FDynamicMesh3& Mesh, const IDynamicMeshCommitter::FDynamicMeshCommitInfo& CommitInfo)
{
	FConversionToMeshDescriptionOptions ConversionOptions;
	ConversionOptions.bSetPolyGroups = CommitInfo.bPolygroupsChanged;
	ConversionOptions.bUpdatePositions = CommitInfo.bPositionsChanged;
	ConversionOptions.bUpdateNormals = CommitInfo.bNormalsChanged;
	ConversionOptions.bUpdateTangents = CommitInfo.bTangentsChanged;
	ConversionOptions.bUpdateUVs = CommitInfo.bUVsChanged;
	ConversionOptions.bUpdateVtxColors = CommitInfo.bVertexColorsChanged;
	ConversionOptions.bTransformVtxColorsSRGBToLinear = CommitInfo.bTransformVertexColorsSRGBToLinear;

	FDynamicMeshToMeshDescription Converter(ConversionOptions);
	if (!CommitInfo.bTopologyChanged)
	{
		Converter.UpdateUsingConversionOptions(&Mesh, CurrentMeshDescription);
	}
	else
	{
		// Do a full conversion.
		Converter.Convert(&Mesh, CurrentMeshDescription);
	}

	MeshDescriptionCommitter.CommitMeshDescription(MoveTemp(CurrentMeshDescription));
}
