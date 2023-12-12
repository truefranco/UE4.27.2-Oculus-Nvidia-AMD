// Copyright Epic Games, Inc. All Rights Reserved.
#include "Import3D.h"
#include "Utilities/AssetsDatabase.h"
#include "AssetImporters/ImportSurface.h"


#include "Runtime/Engine/Classes/Engine/StaticMesh.h"
#include "Utilities/MeshOp.h"
#include "Utilities/MiscUtils.h"
#include "Utilities/MaterialUtils.h"

#include "EditorAssetLibrary.h"
#include "EditorStaticMeshLibrary.h"
#include "UI/MSSettings.h"
#include "Runtime/Core/Public/Misc/MessageDialog.h"
#include "Runtime/Core/Public/Internationalization/Text.h"
#include "Engine/RendererSettings.h"



TSharedPtr<FImport3d> FImport3d::Import3dInst;

TSharedPtr<FImport3d> FImport3d::Get()
{
	if (!Import3dInst.IsValid())
	{
		Import3dInst = MakeShareable(new FImport3d);
	}
	return Import3dInst;
}


void FImport3d::ImportAsset(TSharedPtr<FAssetTypeData> AssetImportData)
{
	const UMegascansSettings* MegascansSettings = GetDefault<UMegascansSettings>();
	TSharedPtr<FAssetImportParams> AssetSetupParameters = FAssetImportParams::Get();
	TSharedPtr<ImportParams3DAsset> Asset3DParameters = AssetSetupParameters->Get3DAssetsParams(AssetImportData);

	if (AssetImportData->MeshList.Num() == 0) return;

	if (AssetImportData->AssetMetaInfo->ActiveLOD == TEXT("high"))
	{
		EAppReturnType::Type ContinueImport = FMessageDialog::Open(EAppMsgType::OkCancel, FText(FText::FromString("You are about to import a high poly mesh. This may cause Unreal to stop responding. Press Ok to continue.")));
		if (ContinueImport == EAppReturnType::Cancel) return;
	}

	TArray<UMaterialInstanceConstant*> MaterialInstance;
	for (int8 Count = 0; Count < Asset3DParameters->ParamsAssetType->MaterialParams.Num(); Count++) {
		MaterialInstance.Add(FImportSurface::Get()->ImportSurface(AssetImportData, Asset3DParameters->ParamsAssetType->MaterialParams[Count]));
	}

	if (Asset3DParameters->ParamsAssetType->SubType == EAsset3DSubtype::MULTI_MESH || AssetImportData->AssetMetaInfo->bIsModularWindow) {
		if (AssetImportData->AssetMetaInfo->bIsMTS) {
			ImportMTS(AssetImportData, MaterialInstance, Asset3DParameters);
			//Turn on Virtual Rendering
			//if (AssetImportData->TextureSets[0]->bIsUdim) {
			//	URendererSettings* ProjectSettings = GetMutableDefault<URendererSettings>();
			//	ProjectSettings->bVirtualTextures = true;
			//}
		}
		else{ 
			ImportScatter(AssetImportData, MaterialInstance, Asset3DParameters);
		}
	}
	else {
		ImportNormal(AssetImportData, MaterialInstance, Asset3DParameters);
	}

	AssetUtils::FocusOnSelected(Asset3DParameters->BaseParams->AssetDestination);
	AssetRecord MSRecord;
	MSRecord.ID = AssetImportData->AssetMetaInfo->Id;
	MSRecord.Name = Asset3DParameters->BaseParams->AssetName;
	MSRecord.Path = Asset3DParameters->BaseParams->AssetDestination;
	MSRecord.Type = AssetImportData->AssetMetaInfo->Type;
	FAssetsDatabase::Get()->AddRecord(MSRecord);
	
	FImportSurface::Get()->AllTextureMaps.Empty();

}


void FImport3d::ImportMTS(TSharedPtr<FAssetTypeData> AssetImportData, TArray<UMaterialInstanceConstant*> MaterialInstance, TSharedPtr<ImportParams3DAsset> Asset3DParameters)
{
	TSharedPtr<FMeshOps> MeshUtils = FMeshOps::Get();

	const UMegascansSettings* MegascansSettings = GetDefault<UMegascansSettings>();
	TArray<FString> ExistingAssets = GetAssetsList(Asset3DParameters->BaseParams->AssetDestination);
	MeshUtils->ImportMesh(AssetImportData->MeshList[0]->Path, Asset3DParameters->ParamsAssetType->MeshDestination, "");
	TArray<FString> ImportedAssets;
	TArray<FString> AllAssets = GetAssetsList(Asset3DParameters->ParamsAssetType->MeshDestination);

	TMap<FString, TArray<UStaticMesh*>> ScatterLods;

	for (FString Asset : AllAssets)
	{
		if (!ExistingAssets.Contains(Asset))
		{
			ImportedAssets.Add(Asset);
			UStaticMesh* ImportedMesh = CastChecked<UStaticMesh>(UEditorAssetLibrary::LoadAsset(Asset));

			//if (AssetImportData->AssetMetaInfo->bIsUdim) {
				//ImportedMesh->SetMaterial(0, CastChecked<UMaterialInterface>(MaterialInstance[0]));
			//}
			//else {
				for (int8 MatCount = 0; MatCount < MaterialInstance.Num(); MatCount++) {
					int8 MaterialSlotCount = ImportedMesh->GetMaterialIndex(*MaterialInstance[MatCount]->GetName().Left(MaterialInstance[MatCount]->GetName().Len() - 5));
					if (MaterialSlotCount == -1)
					{
						continue;
					}
					else {
						if (MaterialInstance[MatCount] != nullptr)
							ImportedMesh->SetMaterial(MaterialSlotCount, CastChecked<UMaterialInterface>(MaterialInstance[MatCount]));
					}
					ImportedMesh->PostEditChange();
				}
			//}
		}
	}

	if (MegascansSettings->bEnableLods && AssetImportData->LodList.Num() > 0) {
		TArray<FString> LodPathList = ParseLodList(AssetImportData);
		FString LodDestination = FPaths::Combine(Asset3DParameters->ParamsAssetType->MeshDestination, TEXT("Lods"));
		int32 LodCounter = 1;
		for (FString LodPath : LodPathList)
		{
			if (LodCounter > 7) continue;

			MeshUtils->ImportMesh(LodPath, LodDestination);
			TArray<FString> ImportedLods = UEditorAssetLibrary::ListAssets(LodDestination);

			for (FString MeshLod : ImportedLods)
			{
				FString MeshLodName = FPaths::GetBaseFilename(MeshLod);

				TArray<FString> NameTags;
				FString LodNameRefined = TEXT("");

				MeshLodName.ParseIntoArray(NameTags, TEXT("_"));
				for (FString Tag : NameTags)
				{
					if (!Tag.Contains(TEXT("LOD")))
					{
						LodNameRefined.Append(Tag);
					}
				}
				for (int8 MatCount = 0; MatCount < MaterialInstance.Num(); MatCount++) {
					int8 MaterialSlotCount = CastChecked<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshLod))->GetMaterialIndex(*MaterialInstance[MatCount]->GetName().Left(MaterialInstance[MatCount]->GetName().Len() - 5));
					if (MaterialSlotCount == -1)
					{
						continue;
					}
					else {
						if (MaterialInstance[MatCount] != nullptr)
							CastChecked<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshLod))->SetMaterial(MaterialSlotCount, CastChecked<UMaterialInterface>(MaterialInstance[MatCount]));
					}
				}
			}
		}
	}
	MeshUtils.Reset();

}

void FImport3d::ImportScatter(TSharedPtr<FAssetTypeData> AssetImportData, TArray<UMaterialInstanceConstant*> MaterialInstance, TSharedPtr<ImportParams3DAsset> Asset3DParameters)
{
	TSharedPtr<FMeshOps> MeshUtils = FMeshOps::Get();

	const UMegascansSettings* MegascansSettings = GetDefault<UMegascansSettings>();
	TArray<FString> ExistingAssets = GetAssetsList(Asset3DParameters->BaseParams->AssetDestination);
	MeshUtils->ImportMesh(AssetImportData->MeshList[0]->Path, Asset3DParameters->ParamsAssetType->MeshDestination, "");
	TArray<FString> ImportedAssets;
	TArray<FString> AllAssets = GetAssetsList(Asset3DParameters->ParamsAssetType->MeshDestination);

	TMap<FString, TArray<UStaticMesh*>> ScatterLods;



	for (FString Asset : AllAssets)
	{
		if (!ExistingAssets.Contains(Asset))
		{
			ImportedAssets.Add(Asset);
			UStaticMesh* ImportedMesh = CastChecked<UStaticMesh>(UEditorAssetLibrary::LoadAsset(Asset));
			if (MaterialInstance[0] != nullptr)
				ImportedMesh->SetMaterial(0, CastChecked<UMaterialInterface>(MaterialInstance[0]));

			ImportedMesh->PostEditChange();
		}
	}

	if (MegascansSettings->bEnableLods && AssetImportData->LodList.Num() > 0) {

		TArray<FString> LodPathList = ParseLodList(AssetImportData);
		FString LodDestination = FPaths::Combine(Asset3DParameters->ParamsAssetType->MeshDestination, TEXT("Lods"));
		int32 LodCounter = 1;
		for (FString LodPath : LodPathList)
		{
			if (LodCounter > 7) continue;

			MeshUtils->ImportMesh(LodPath, LodDestination);
			TArray<FString> ImportedLods = UEditorAssetLibrary::ListAssets(LodDestination);

			for (FString MeshLod : ImportedLods)
			{
				FString MeshLodName = FPaths::GetBaseFilename(MeshLod);

				TArray<FString> NameTags;
				FString LodNameRefined = TEXT("");

				MeshLodName.ParseIntoArray(NameTags, TEXT("_"));
				for (FString Tag : NameTags)
				{
					if (!Tag.Contains(TEXT("LOD")))
					{
						LodNameRefined.Append(Tag);
					}
				}



				for (FString SourceMeshPath : ImportedAssets)
				{
					FString SourceMeshName = FPaths::GetBaseFilename(SourceMeshPath);
					TArray<FString> MeshNameTags;
					FString MeshNameRefined = TEXT("");
					UStaticMesh* ImportedScatter = CastChecked<UStaticMesh>(UEditorAssetLibrary::LoadAsset(SourceMeshPath));

					SourceMeshName.ParseIntoArray(MeshNameTags, TEXT("_"));

					for (FString Tag : MeshNameTags)
					{
						if (!Tag.Contains(TEXT("LOD")))
						{
							MeshNameRefined.Append(Tag);
						}
					}

					if (MeshNameRefined == LodNameRefined)
					{

						UEditorStaticMeshLibrary::SetLodFromStaticMesh(ImportedScatter, LodCounter, CastChecked<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshLod)), 0, true);

					}
					ImportedScatter->PostEditChange();
					ImportedScatter->MarkPackageDirty();

				}


			}
			LodCounter++;
			UEditorAssetLibrary::DeleteDirectory(LodDestination);
		}
	}

	int32 VarCounter = 1;
	FString OverridenName;
	FString OverridenExtension;
	AssetImportData->MeshList[0]->NameOverride.Split(TEXT("."), &OverridenName, &OverridenExtension);

	OverridenName = SanitizeName(OverridenName);
	for (FString Asset : ImportedAssets)
	{
		FString BasePath = FPaths::GetPath(Asset);
		FString RenamedPath = FPaths::Combine(BasePath, (OverridenName + TEXT("_") + FString::FromInt(VarCounter)));

		if (MaterialInstance[0] != nullptr)
			CastChecked<UStaticMesh>(UEditorAssetLibrary::LoadAsset(Asset))->SetMaterial(1, CastChecked<UMaterialInterface>(MaterialInstance[0]));

		UEditorAssetLibrary::RenameAsset(Asset, RenamedPath);


		UStaticMesh* ImportedScatter = CastChecked<UStaticMesh>(UEditorAssetLibrary::LoadAsset(RenamedPath));
		MeshUtils->RemoveExtraMaterialSlot(ImportedScatter);

		if (MegascansSettings->bCreateFoliage)
		{

			MeshUtils->CreateFoliageAsset(Asset3DParameters->ParamsAssetType->MeshDestination, ImportedScatter, (OverridenName + TEXT("_") + FString::FromInt(VarCounter)));
		}
		ImportedScatter->MarkPackageDirty();
		if (AssetImportData->AssetMetaInfo->bSavePackages)
		{
			AssetUtils::SavePackage(ImportedScatter);
		}
		//AssetUtils::SavePackage(ImportedScatter);
		VarCounter++;
	}

	MeshUtils.Reset();

}

void FImport3d::ImportNormal(TSharedPtr<FAssetTypeData> AssetImportData, TArray<UMaterialInstanceConstant*> MaterialInstance, TSharedPtr<ImportParams3DAsset> Asset3DParameters)
{
	const UMegascansSettings* MegascansSettings = GetDefault<UMegascansSettings>();
	FString FileExtension;
	FString FilePath;
	AssetImportData->MeshList[0]->Path.Split(TEXT("."), &FilePath, &FileExtension);
	FileExtension = FPaths::GetExtension(AssetImportData->MeshList[0]->Path);

	FString OverridenName;
	FString OverridenExtension;
	AssetImportData->MeshList[0]->NameOverride.Split(TEXT("."), &OverridenName, &OverridenExtension);

	TSharedPtr<FMeshOps> MeshUtils = FMeshOps::Get();
	FString AssetPath = MeshUtils->ImportMesh(AssetImportData->MeshList[0]->Path, Asset3DParameters->ParamsAssetType->MeshDestination, SanitizeName(OverridenName));

	if (!UEditorAssetLibrary::DoesAssetExist(AssetPath)) return;
	UStaticMesh* ImportedAsset = CastChecked<UStaticMesh>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (ImportedAsset == nullptr) return;

	for (int8 Count = 0; Count < MaterialInstance.Num(); Count++) {
		if (MaterialInstance[Count] != nullptr)
			ImportedAsset->SetMaterial(Count, CastChecked<UMaterialInterface>(MaterialInstance[Count]));
	}
	if (MegascansSettings->bEnableLods && AssetImportData->LodList.Num() > 0)
	{
		TArray<FString> LodPathList = ParseLodList(AssetImportData);
		if (FileExtension == TEXT("abc"))
		{

			MeshUtils->ApplyAbcLods(ImportedAsset, LodPathList, Asset3DParameters->ParamsAssetType->MeshDestination);
		}
		else {

			MeshUtils->ApplyLods(LodPathList, ImportedAsset);
		}
		//if (!AssetImportData->AssetMetaInfo->bIsModularWindow)
		//{
			MeshUtils->RemoveExtraMaterialSlot(ImportedAsset);
		//}

		//else {
		//	FString GlassMaterialPath = GetMaterial(TEXT("GlassMasterMaterial"));
		//	UMaterialInstanceConstant* GlassInstance = FMaterialUtils::CreateInstanceMaterial(GlassMaterialPath, Asset3DParameters->BaseParams->AssetDestination, TEXT("GlassMaterial_inst"));
		//	for (const TPair<FString, TArray<int8>> MaterialInfo : AssetImportData->AssetMetaInfo->MaterialTypes)
		//	{
		//		if (MaterialInfo.Key == TEXT("glass"))
		//		{
		//			for (int8 MatId : MaterialInfo.Value)
		//			{
		//				ImportedAsset->SetMaterial(MatId - 1, CastChecked<UMaterialInterface>(GlassInstance));
		//			}
		//		}
		//		else
		//		{
		//			for (int8 MatId : MaterialInfo.Value)
		//			{
		//				ImportedAsset->SetMaterial(MatId - 1, CastChecked<UMaterialInterface>(MaterialInstance[0]));
		//			}

		//		}
		//	}

		//}
	}
	ImportedAsset->MarkPackageDirty();
	ImportedAsset->PostEditChange();
	if (AssetImportData->AssetMetaInfo->bSavePackages)
	{
		AssetUtils::SavePackage(ImportedAsset);
	}
	//AssetUtils::SavePackage(ImportedAsset);	
	MeshUtils.Reset();
}
