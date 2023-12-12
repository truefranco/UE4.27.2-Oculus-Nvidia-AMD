// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"


class UFbxImportUI;
class UStaticMesh;
class UAbcImportSettings;


class FMeshOps
{
private:
	FMeshOps() = default;
	static TSharedPtr<FMeshOps> MeshOpsInst;
	UFbxImportUI* GetFbxOptions();
	//UFbxImportUI* ImportOptions;


	template<class T> FString ImportFile(T* ImportOptions, const FString& Destination, const FString& AssetName, const FString& Source);
	UAbcImportSettings* GetAbcSettings();

public:
	static TSharedPtr<FMeshOps> Get();
	FString ImportMesh(const FString& MeshPath, const FString& Destination, const FString& AssetName = TEXT(""));
	void ApplyLods(const TArray<FString>& LodList, UStaticMesh* SourceMesh);
	TArray<FString> ImportLodsAsStaticMesh(const TArray<FString>& LodList, const FString& AssetDestination);
	void ApplyAbcLods(UStaticMesh* SourceMesh, const TArray<FString>& LodPathList, const FString& AssetDestination);
	void CreateFoliageAsset(const FString& FoliagePath, UStaticMesh* SourceAsset, const FString& FoliageAssetName, bool bSavePackage = false);
	void RemoveExtraMaterialSlot(UStaticMesh* SourceMesh);
	//void SetFbxOptions(bool bCombineMeshes = false, bool bGenerateLightmapUVs = false, bool bAutoGenerateCollision = false, bool bImportMesh = true, bool bImportAnimations = false, bool bImportMaterials = false, bool bImportAsSkeletal = false);
	void LodDistanceTest(UStaticMesh* SourceMesh);
	bool bCombineMeshes = false;
};