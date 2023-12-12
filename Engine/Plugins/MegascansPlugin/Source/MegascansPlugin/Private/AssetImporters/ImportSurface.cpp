// Copyright Epic Games, Inc. All Rights Reserved.
#include "ImportSurface.h"

#include "Utilities/AssetsDatabase.h"
#include "Utilities/MiscUtils.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetImportTask.h"
#include "Editor/UnrealEd/Classes/Factories/MaterialInstanceConstantFactoryNew.h"
#include "Runtime/Engine/Classes/Materials/MaterialInstanceConstant.h"
#include "Runtime/Engine/Classes/Engine/Texture.h"
#include "UI/MSSettings.h"
#include "MaterialEditingLibrary.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "EditorAssetLibrary.h"

#include "Runtime/Engine/Classes/Engine/Selection.h"
#include "Editor.h"
#include "PackageTools.h"
#include "Utilities/MTSReader.h"
TSharedPtr<FImportSurface> FImportSurface::ImportSurfaceInst;

void FImportSurface::ImportAsset(TSharedPtr<FAssetTypeData> AssetImportData)
{
	TSharedPtr<FAssetImportParams> AssetSetupParameters = FAssetImportParams::Get();	
	TSharedPtr<ImportParamsSurfaceAsset> SurfaceImportParams = (AssetImportData->AssetMetaInfo->Type == TEXT("surface")) ? AssetSetupParameters->GetSurfaceParams(AssetImportData) : AssetSetupParameters->GetAtlasBrushParams(AssetImportData);
	UMaterialInstanceConstant* MaterialInstance = ImportSurface(AssetImportData, SurfaceImportParams->ParamsAssetType);
	AssetUtils::FocusOnSelected(SurfaceImportParams->ParamsAssetType->MaterialInstanceDestination);
	if (AssetImportData->AssetMetaInfo->Type == TEXT("surface") || AssetImportData->AssetMetaInfo->Type == TEXT("atlas") || AssetImportData->AssetMetaInfo->Type == TEXT("brush"))
	{
		AssetRecord MSRecord;
		MSRecord.ID = AssetImportData->AssetMetaInfo->Id;
		MSRecord.Name = SurfaceImportParams->BaseParams->AssetName;
		MSRecord.Path = SurfaceImportParams->BaseParams->AssetDestination;
		MSRecord.Type = AssetImportData->AssetMetaInfo->Type;
		FAssetsDatabase::Get()->AddRecord(MSRecord);
	}

	if (MaterialInstance == nullptr) {
		return;
	}

	const UMegascansSettings* MegascansSettings = GetDefault<UMegascansSettings>();
	if (MegascansSettings->bApplyToSelection)
	{
		ApplyMaterialToSelection(MaterialInstance);
	}

	FImportSurface::Get()->AllTextureMaps.Empty();

}

UMaterialInstanceConstant* FImportSurface::ImportSurface(TSharedPtr<FAssetTypeData> AssetImportData, TSharedPtr<SurfaceParams> SurfaceImportParams, bool is3DPlant)
{

	// Create Material Instance
	UMaterialInstanceConstant* MaterialInstance = CreateInstanceMaterial(AssetImportData,SurfaceImportParams);

	// Import all the textures
	if (AllTextureMaps.Num() == 0 || is3DPlant) {
		// Import channel packed maps  - TArray<TMap<FString, TSharedPtr<FAssetPackedTextures>>>
		PackedImportData = ImportPackedMaps(AssetImportData, SurfaceImportParams->TexturesDestination);

		// Get list of maps to be filetered on import. All maps that are in channel packed maps will be filetered upon import.
		const TArray<FString> FilteredTextureTypes = GetFilteredMaps(PackedImportData, MaterialInstance);

		AllTextureMaps = ImportTextureMaps(AssetImportData, SurfaceImportParams, FilteredTextureTypes);
		NormalizeTextureNamesInJson(AssetImportData);
	}
	// Exit if material instance creation failed.
	if (MaterialInstance == nullptr)
	{
		return nullptr;
	}


	// Plug the imported textures into the Material Instance.
	MInstanceApplyTextures(AllTextureMaps, MaterialInstance, SurfaceImportParams, AssetImportData);
	if (SurfaceImportParams->bContainsPackedMaps) {
		MInstanceApplyPackedMaps(PackedImportData, MaterialInstance,SurfaceImportParams, AssetImportData);
	}

	//Force Save Material
	//if (AssetImportData->AssetMetaInfo->bSavePackages)
	//{
		AssetUtils::SavePackage(MaterialInstance);
	//}



	return MaterialInstance;

}

TSharedPtr<FImportSurface> FImportSurface::Get()
{
	if (!ImportSurfaceInst.IsValid())
	{
		ImportSurfaceInst = MakeShareable(new FImportSurface);
	}
	return ImportSurfaceInst;
}


TArray<TMap<FString, TextureData>> FImportSurface::ImportTextureMaps(TSharedPtr<FAssetTypeData> AssetImportData, TSharedPtr<SurfaceParams> SurfaceImportParams , const TArray<FString>& FilteredTextureTypes)
{	
	TArray<TMap<FString, TextureData>> TextureMapsList;
	TMap<FString, TextureData> TextureMaps;
	for (TSharedPtr<FAssetTextureData> TextureMetaData : AssetImportData->TextureComponents)
	{
		
		FString TextureType = TextureMetaData->Type;

		if (FilteredTextureTypes.Contains(TextureType)) continue;

		UAssetImportTask* TextureImportTask = CreateImportTask(TextureMetaData, SurfaceImportParams->TexturesDestination);
		TextureData TextureImportData = ImportTexture(TextureImportTask);

		if (TextureType == TEXT("normal"))
		{
			
			TextureImportData.TextureAsset->bFlipGreenChannel = 1;
		}
		

		if (TextureType == TEXT("opacity"))
			TextureImportData.TextureAsset->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

		if (AssetImportData->AssetMetaInfo->bIsUdim) {
			TextureImportData.TextureAsset->VirtualTextureStreaming = 1;
		}
		else {
			TextureImportData.TextureAsset->VirtualTextureStreaming = 0;
		}


		if (NonLinearMaps.Contains(TextureType))
		{
			TextureImportData.TextureAsset->SRGB = 1;
		}
		else
		{
			TextureImportData.TextureAsset->SRGB = 0;
		}

		if (MapCompressionType.Contains(TextureType))
		{
			
			TextureImportData.TextureAsset->CompressionSettings = MapCompressionType[TextureType];
		}
		TextureImportData.TextureAsset->SetFlags(RF_Standalone);
		TextureImportData.TextureAsset->MarkPackageDirty();
		TextureImportData.TextureAsset->PostEditChange();
		if (AssetImportData->AssetMetaInfo->bSavePackages)
		{
			AssetUtils::SavePackage(TextureImportData.TextureAsset);
		}
		TextureMaps.Add(TextureType, TextureImportData);
		TextureMapsList.Add(TextureMaps);

	}
	return TextureMapsList;
}




UAssetImportTask* FImportSurface::CreateImportTask(TSharedPtr<FAssetTextureData> TextureMetaData, const FString& TexturesDestination)
{
	FString Filename;
	Filename = FPaths::GetBaseFilename(TextureMetaData->NameOverride);
	UAssetImportTask* TextureImportTask = NewObject<UAssetImportTask>();
	TextureImportTask->bAutomated = true;
	TextureImportTask->bSave = false;
	TextureImportTask->Filename = TextureMetaData->Path;
	TextureImportTask->DestinationName = RemoveReservedKeywords(NormalizeString(Filename));
	TextureImportTask->DestinationPath = TexturesDestination;
	TextureImportTask->bReplaceExisting = true;
	return TextureImportTask;
}

void FImportSurface::ApplyMaterialToSelection(UMaterialInstanceConstant* MaterialInstance)
{
	USelection* SelectedActors = GEditor->GetSelectedActors();
	TArray<AActor*> Actors;
	TArray<ULevel*> UniqueLevels;
	//EAppReturnType::Type applyMat = EAppReturnType::NoAll;
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		//if(applyMat == EAppReturnType::NoAll)
		//	applyMat = FMessageDialog::Open(EAppMsgType::OkCancel, FText(FText::FromString("Do you want to apply imported material to selected assets ?")));
		//if (applyMat == EAppReturnType::Ok) {

			AActor* Actor = Cast<AActor>(*Iter);
			TArray<UStaticMeshComponent*> Components;
			Actor->GetComponents<UStaticMeshComponent>(Components);
			for (int32 i = 0; i < Components.Num(); i++)
			{
				UStaticMeshComponent* MeshComponent = Components[i];
				int32 mCnt = MeshComponent->GetNumMaterials();
				for (int j = 0; j < mCnt; j++)
					MeshComponent->SetMaterial(j, MaterialInstance);

			}
		//}
	}
}

TextureData FImportSurface::ImportTexture(UAssetImportTask* TextureImportTask)
{
	TextureData TextureImportData;
	TArray<UAssetImportTask*> ImportTasks;
	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	ImportTasks.Add(TextureImportTask);

	AssetTools.ImportAssetTasks(ImportTasks);
	for (UAssetImportTask* ImpTask : ImportTasks)
	{
		if (ImpTask->ImportedObjectPaths.Num() > 0)
		{
			TextureImportData.TextureAsset = CastChecked<UTexture>(UEditorAssetLibrary::LoadAsset(ImpTask->ImportedObjectPaths[0]));
			TextureImportData.Path = ImpTask->ImportedObjectPaths[0];
		}
	}
	return TextureImportData;
}

//UMaterialInstanceConstant* FImportSurface::CreateInstanceMaterial(const FString & MasterMaterialPath, const FString& InstanceDestination, const FString& MInstanceName)
//{
//	if (!UEditorAssetLibrary::DoesAssetExist(MasterMaterialPath)) return nullptr;
//
//	UMaterialInterface* MasterMaterial = CastChecked<UMaterialInterface>(LoadAsset(MasterMaterialPath));
//	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
//	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
//
//	UObject* MaterialInstAsset = AssetTools.CreateAsset(MInstanceName, InstanceDestination, UMaterialInstanceConstant::StaticClass(), Factory);
//	UMaterialInstanceConstant *MaterialInstance = CastChecked<UMaterialInstanceConstant>(MaterialInstAsset);
//	
//	UMaterialEditingLibrary::SetMaterialInstanceParent(MaterialInstance, MasterMaterial);
//	if (MaterialInstance)
//	{
//		MaterialInstance->SetFlags(RF_Standalone);
//		MaterialInstance->MarkPackageDirty();
//		MaterialInstance->PostEditChange();
//
//		
//	}
//	
//	return MaterialInstance;
//}

UMaterialInstanceConstant* FImportSurface::CreateInstanceMaterial(TSharedPtr<FAssetTypeData> AssetImportData, TSharedPtr<SurfaceParams> SurfaceImportParams)
{
	if (!UEditorAssetLibrary::DoesAssetExist(SurfaceImportParams->MasterMaterialPath)) return nullptr;
	UMaterialInterface* MasterMaterial = CastChecked<UMaterialInterface>(LoadAsset(SurfaceImportParams->MasterMaterialPath));
	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	UObject* MaterialInstAsset = AssetTools.CreateAsset(SanitizeName(SurfaceImportParams->MaterialInstanceName), SurfaceImportParams->MaterialInstanceDestination, UMaterialInstanceConstant::StaticClass(), Factory);
	if (MaterialInstAsset == nullptr) return NULL; 
	UMaterialInstanceConstant* MaterialInstance = CastChecked<UMaterialInstanceConstant>(MaterialInstAsset);

	UMaterialEditingLibrary::SetMaterialInstanceParent(MaterialInstance, MasterMaterial);
	if (MaterialInstance)
	{
		if (AssetImportData->AssetMetaInfo->bIsMetal) {
			UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(MaterialInstance, "Metallic Controls", FLinearColor(1, 0, 1, 1));
			UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, "Metallic Map Intensity", 1);
		}

		if (AssetImportData->AssetMetaInfo->bIsMixerAsset) {
			UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, "Cutoff Threshold", SurfaceImportParams->cutoutValue);
			MaterialInstance->BasePropertyOverrides.bOverride_TwoSided = SurfaceImportParams->TwoSided;
			MaterialInstance->BasePropertyOverrides.TwoSided = SurfaceImportParams->TwoSided;
		}

		MaterialInstance->SetFlags(RF_Standalone);
		MaterialInstance->MarkPackageDirty();
		MaterialInstance->PostEditChange();
	}
	return MaterialInstance;
}

void FImportSurface::MInstanceApplyTextures(TArray<TMap<FString, TextureData>> TextureMapsList, UMaterialInstanceConstant* MaterialInstance, TSharedPtr<SurfaceParams> SurfaceImportParams, TSharedPtr<FAssetTypeData> AssetImportData)
{	
	FString Path, Name;
	for (TMap<FString, TextureData> TextureMaps : TextureMapsList) {
		for (const TPair<FString, TextureData>& TextureData : TextureMaps)
		{
			if (UMaterialEditingLibrary::GetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*TextureData.Key)))
			{


				//UTexture* TextureAsset = Cast<UTexture>(LoadAsset(TextureData.Value.Path));

				if (AssetImportData->AssetMetaInfo->bIsMTS && !AssetImportData->AssetMetaInfo->bIsUdim) {
					TextureData.Value.Path.Split(TEXT("."), &Path, &Name);
					for (int8 Counter = 0; Counter < AssetImportData->TextureComponents.Num(); Counter++) {
						if (AssetImportData->TextureComponents[Counter]->NameOverride.StartsWith(Name)) {

							for (TSharedPtr<FAssetMaterialData> MaterialInstanceValue : AssetImportData->MaterialList) {
								if (MaterialInstanceValue->MaterialName + "_inst" == SurfaceImportParams->MaterialInstanceName) {



									for (FString MatTextureSetValue : MaterialInstanceValue->TextureSets)
									{
										for (FString TextureSetValue : AssetImportData->TextureComponents[Counter]->TextureSets)
										{
											if (MatTextureSetValue == TextureSetValue) {

												UTexture* TextureAsset = TextureData.Value.TextureAsset;
												if (UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*TextureData.Key), TextureAsset))
												{

													MaterialInstance->SetFlags(RF_Standalone);
													MaterialInstance->MarkPackageDirty();
													MaterialInstance->PostEditChange();
												}
											}
										}
									}
								}
							}
						}
					}
				}
				else {
					if (AssetImportData->AssetMetaInfo->bIsUdim) {
						TextureData.Value.Path.Split(TEXT("."), &Path, &Name);
						for (int8 Counter = 0; Counter < AssetImportData->TextureComponents.Num(); Counter++) {
							//UE_LOG(LogTemp, Warning, TEXT(" Name  %s"), *Name);
							//UE_LOG(LogTemp, Warning, TEXT(" Res  %s"), *AssetImportData->TextureComponents[Counter]->Resolution);
							//UE_LOG(LogTemp, Warning, TEXT(" Tex name  %s"), *AssetImportData->TextureComponents[Counter]->Name);
							//if (AssetImportData->TextureComponents[Counter]->NameOverride.StartsWith(Name)) {
								if (Name.Contains(AssetImportData->TextureComponents[Counter]->Resolution)) {

								for (TSharedPtr<FAssetMaterialData> MaterialInstanceValue : AssetImportData->MaterialList) {
									if (MaterialInstanceValue->MaterialName + "_inst" == SurfaceImportParams->MaterialInstanceName) {




										for (FString MatTextureSetValue : MaterialInstanceValue->TextureSets)
										{
											for (FString TextureSetValue : AssetImportData->TextureComponents[Counter]->TextureSets)
											{
												if (MatTextureSetValue == TextureSetValue) {

														UTexture* TextureAsset = TextureData.Value.TextureAsset;
														if (UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*TextureData.Key), TextureAsset))
														{

															MaterialInstance->SetFlags(RF_Standalone);
															MaterialInstance->MarkPackageDirty();
															MaterialInstance->PostEditChange();
														}
													}
												}
											}
										}
									}
								
							}
						}



					}
					else {
							UTexture* TextureAsset = TextureData.Value.TextureAsset;
							if (UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*TextureData.Key), TextureAsset))
							{
								MaterialInstance->SetFlags(RF_Standalone);
								MaterialInstance->MarkPackageDirty();
								MaterialInstance->PostEditChange();
							}
					}
				}
			}

		}
	}
}

TArray<FString> FImportSurface::GetPackedMapsList(TSharedPtr<FAssetTypeData> AssetImportData)
{
	TArray<FString> PackedMaps;
	return PackedMaps;
}


//TMap<FString, TSharedPtr<FAssetPackedTextures>> FImportSurface::ImportPackedMaps(TSharedPtr<FAssetTypeData> AssetImportData, const FString& TexturesDestination)
//{
//	TMap<FString, TSharedPtr<FAssetPackedTextures>> PackedImportData;
//	for (TSharedPtr<FAssetPackedTextures> PackedData : AssetImportData->PackedTextures)
//	{
//		UAssetImportTask* TextureImportTask = CreateImportTask(PackedData->PackedTextureData, TexturesDestination);
//		TextureData TextureImportData = ImportTexture(TextureImportTask);
//
//		//UTexture* TextureAsset = Cast<UTexture>(LoadAsset(TextureImportData.Path));
//		TextureImportData.TextureAsset->SRGB = 0;
//
//		FString ImportedMap = TextureImportData.Path;
//		if (PackedImportData.Contains(ImportedMap))
//		{
//			PackedImportData.Remove(ImportedMap);
//		}
//		PackedImportData.Add(ImportedMap, PackedData);
//		if (AssetImportData->AssetMetaInfo->bSavePackages)
//		{
//			AssetUtils::SavePackage(TextureImportData.TextureAsset);
//		}
//	}
//	return PackedImportData;
//}

TMap<FString, TSharedPtr<FAssetPackedTextures>> FImportSurface::ImportPackedMaps(TSharedPtr<FAssetTypeData> AssetImportData, const FString& TexturesDestination)
{
	//TArray<TMap<FString, TSharedPtr<FAssetPackedTextures>>> PackedImportDataArray;
	TMap<FString, TSharedPtr<FAssetPackedTextures>> PackedImportedData;
	for (TSharedPtr<FAssetPackedTextures> PackedData : AssetImportData->PackedTextures)
	{
		UAssetImportTask* TextureImportTask = CreateImportTask(PackedData->PackedTextureData, TexturesDestination);
		TextureData TextureImportData = ImportTexture(TextureImportTask);

		if (AssetImportData->AssetMetaInfo->bIsUdim) {
			TextureImportData.TextureAsset->VirtualTextureStreaming = 1;
		}
		else {
			TextureImportData.TextureAsset->VirtualTextureStreaming = 0;
		}

		//UTexture* TextureAsset = Cast<UTexture>(LoadAsset(TextureImportData.Path));
		TextureImportData.TextureAsset->SRGB = 0;
		TextureImportData.TextureAsset->CompressionSettings = TextureCompressionSettings::TC_Masks;

		FString ImportedMap = TextureImportData.Path;
		if (PackedImportedData.Contains(ImportedMap))
		{
			PackedImportedData.Remove(ImportedMap);
		}
		PackedImportedData.Add(ImportedMap, PackedData);

		if (AssetImportData->AssetMetaInfo->bSavePackages)
		{
			AssetUtils::SavePackage(TextureImportData.TextureAsset);
		}
		
		TextureImportData.TextureAsset->UpdateResource();
		//PackedImportDataArray.Add(PackedImportedData);
	}


	return PackedImportedData;
}

TArray<FString> FImportSurface::GetPackedTypes(ChannelPackedData PackedDataMap)
{
	TArray<FString> PackedTypes;
	//for (auto& PackedDataMap : PackedImportDataArray) {
		for (auto& PackedData : PackedDataMap)
		{
			for (auto& ChData : PackedData.Value->ChannelData)
			{
				if (ChData.Value[0] == TEXT("gray") || ChData.Value[0] == TEXT("empty") || ChData.Value[0] == TEXT("value")) continue;
				PackedTypes.Add(ChData.Value[0].ToLower());

			}

		}
	//}
	return PackedTypes;
}

void FImportSurface::MInstanceApplyPackedMaps(TMap<FString, TSharedPtr<FAssetPackedTextures>> PackedMapData, UMaterialInstanceConstant* MaterialInstance, TSharedPtr<SurfaceParams> SurfaceImportParams, TSharedPtr<FAssetTypeData> AssetImportData)
{
	int8 MapCount = 0;
	FString Extension, Path;
	//for (auto& PackedMapData : PackedImportDataArray) {
		for (auto& PackedData : PackedMapData)
		{
			FString ChannelPackedType = TEXT("");
			//FString PackedMap = PackedData.Key;
			UTexture* PackedAsset = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(PackedData.Key));
			if (PackedAsset == nullptr) continue;
			for (auto& ChData : PackedData.Value->ChannelData)
			{
				if (ChData.Value[0] == TEXT("gray") || ChData.Value[0] == TEXT("empty") || ChData.Value[0] == TEXT("value")) continue;
				ChannelPackedType = ChannelPackedType + ChData.Value[0].Left(1);
			}

			// 
			if (AssetImportData->AssetMetaInfo->bIsMTS && !AssetImportData->AssetMetaInfo->bIsUdim && AssetImportData->TextureSets.Num() > 1) {
				//Compare Material TextureSet vs Texture TextureSet
				PackedData.Value->PackedTextureData->Path.Split(TEXT("."), &Path, &Extension);
				for (int8 Counter = 0; Counter < AssetImportData->PackedTextures.Num(); Counter++) {
					if (Path.EndsWith(AssetImportData->PackedTextures[Counter]->PackedTextureData->Name)) {
						for (TSharedPtr<FAssetMaterialData> MaterialInstanceValue : AssetImportData->MaterialList) {

							if (MaterialInstanceValue->MaterialName + "_inst" == SurfaceImportParams->MaterialInstanceName) {
								for (FString MatTextureSetValue : MaterialInstanceValue->TextureSets) {
									for (FString TextureSetValue : AssetImportData->PackedTextures[Counter]->TextureSets) {
											if (MatTextureSetValue == TextureSetValue) {
												if (UMaterialEditingLibrary::GetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*ChannelPackedType)))
												{
													UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*ChannelPackedType), PackedAsset);
													MaterialInstance->SetFlags(RF_Standalone);
													MaterialInstance->MarkPackageDirty();
													MaterialInstance->PostEditChange();
													return;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			
			else {
				//Compare Material TextureSet vs Texture TextureSet
				PackedData.Value->PackedTextureData->Path.Split(TEXT("."), &Path, &Extension);
				for (int8 Counter = 0; Counter < AssetImportData->PackedTextures.Num(); Counter++) {
					if (Extension.Contains(AssetImportData->TextureComponents[Counter]->Resolution)) {
					//if (Path.EndsWith(AssetImportData->PackedTextures[Counter]->PackedTextureData->Name)) {
						for (TSharedPtr<FAssetMaterialData> MaterialInstanceValue : AssetImportData->MaterialList) {

							if (MaterialInstanceValue->MaterialName + "_inst" == SurfaceImportParams->MaterialInstanceName) {
								for (FString MatTextureSetValue : MaterialInstanceValue->TextureSets) {
									for (FString TextureSetValue : AssetImportData->PackedTextures[Counter]->TextureSets) {
										if (MatTextureSetValue == TextureSetValue) {
											if (ChannelPackedType == "R") ChannelPackedType = "RM";
											if (UMaterialEditingLibrary::GetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*ChannelPackedType)))
											{
												UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*ChannelPackedType), PackedAsset);
												MaterialInstance->SetFlags(RF_Standalone);
												MaterialInstance->MarkPackageDirty();
												MaterialInstance->PostEditChange();
												return;
											}
										}
									}
								}
							}
						}
					}
				}
				if (ChannelPackedType == "R") ChannelPackedType = "RM";
				if (UMaterialEditingLibrary::GetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*ChannelPackedType)))
				{
					UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*ChannelPackedType), PackedAsset);
					MaterialInstance->SetFlags(RF_Standalone);
					MaterialInstance->MarkPackageDirty();
					MaterialInstance->PostEditChange();

				}
			}
			
		}
		MapCount++;

	//}

}



FString FImportSurface::GetMaterialOverride(TSharedPtr<FAssetTypeData> AssetImportData)
{
	TArray<FString> SupportedMatOverrides = { "3d", "3dplant", "surface" };
	FString AssetType = AssetImportData->AssetMetaInfo->Type;
	const UMaterialPresetsSettings* MatOverrideSettings = GetDefault< UMaterialPresetsSettings>();
	if (SupportedMatOverrides.Contains(AssetType))
	{
		if (AssetType == "surface" && MatOverrideSettings->MasterMaterialSurface != nullptr) return MatOverrideSettings->MasterMaterialSurface->GetPathName();
		if (AssetType == "3d" && MatOverrideSettings->MasterMaterial3d != nullptr) return MatOverrideSettings->MasterMaterial3d->GetPathName();
		if (AssetType == "3dplant" && MatOverrideSettings->MasterMaterialPlant != nullptr) return MatOverrideSettings->MasterMaterialPlant->GetPathName();

	}
	return TEXT("");

}


FString FImportSurface::GetSurfaceType(TSharedPtr<FAssetTypeData> AssetImportData)
{
	FString SurfaceType = TEXT("");
	FString AssetType = AssetImportData->AssetMetaInfo->Type;
	auto& TagsList = AssetImportData->AssetMetaInfo->Tags;
	auto& CategoriesList = AssetImportData->AssetMetaInfo->Categories;

	if(TagsList.Contains(TEXT("metal"))) SurfaceType = TEXT("metal");
	else if(TagsList.Contains(TEXT("surface imperfection"))) SurfaceType = TEXT("imperfection");
	else if (TagsList.Contains(TEXT("tileable displacement"))) SurfaceType = TEXT("displacement");
	
	else if (CategoriesList[0] == TEXT("brush")) SurfaceType = TEXT("brush");
	else if (CategoriesList[0] == TEXT("atlas") && TagsList.Contains(TEXT("decal"))) SurfaceType = TEXT("decal");
	else if (CategoriesList[0] == TEXT("atlas") && !TagsList.Contains(TEXT("decal"))) SurfaceType = TEXT("atlas");

	return SurfaceType;
}



TArray<FString> FImportSurface::GetFilteredMaps(ChannelPackedData PackedImportedData, UMaterialInstanceConstant* MaterialInstance)
{
	TArray<FString> FilteredMaps = GetPackedTypes(PackedImportedData);
	const UMegascansSettings* MegascansSettings = GetDefault<UMegascansSettings>();
	bool bFalsifyThis = false;	
	if (MaterialInstance !=nullptr && MegascansSettings->bFilterMasterMaterialMaps)
	{		
		for (FString MapType : AllMapTypes)
		{
			if (UMaterialEditingLibrary::GetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*MapType)) == nullptr)
			{
				FilteredMaps.Add(MapType);
			}
		}

	}

	return FilteredMaps;


}

void FImportSurface::NormalizeTextureNamesInJson(TSharedPtr<FAssetTypeData> AssetImportData) {
	for (int8 TexCount = 0; TexCount < AssetImportData->TextureComponents.Num(); TexCount++) {
		AssetImportData->TextureComponents[TexCount]->NameOverride = RemoveReservedKeywords(NormalizeString(AssetImportData->TextureComponents[TexCount]->NameOverride));
	}
}


