// Copyright Epic Games, Inc. All Rights Reserved.
#include "AssetData.h"
#include "Utilities/MiscUtils.h"
#include "UI/MSSettings.h"
#include "EditorAssetLibrary.h"
#include "Utilities/MTSReader.h"





TSharedPtr<FAssetImportParams> FAssetImportParams::ImportParamsInst;
TSharedPtr<FAssetImportParams> FAssetImportParams::Get()
{
	if (!ImportParamsInst.IsValid())
	{
		ImportParamsInst = MakeShareable(new FAssetImportParams);
	}
	return ImportParamsInst;
}


TSharedPtr<ImportParams3DAsset> FAssetImportParams::Get3DAssetsParams(TSharedPtr<FAssetTypeData> AssetImportData)
{
	TSharedPtr<ImportParams3DAsset> AssetParams = MakeShareable(new ImportParams3DAsset);
	TSharedPtr<Asset3DParams>  Asset3DParameters = MakeShareable(new Asset3DParams);
	int8 Count = 0;
	do { 
		const UMaterialAssetSettings* MatAssetPathsSettings = GetDefault<UMaterialAssetSettings>();
		TSharedPtr<SurfaceParams> MaterialParams = MakeShareable(new SurfaceParams);
		AssetParams->BaseParams = GetAssetImportParams(AssetImportData);

		Asset3DParameters->SubType = Get3DSubtype(AssetImportData);
		AssetImportData->AssetMetaInfo->bIsMetal = CheckMetalness(AssetImportData);
		Asset3DParameters->MeshDestination = AssetParams->BaseParams->AssetDestination;
		Asset3DParameters->Asset3DMaterialType = GetS3DMasterMaterialType(AssetImportData, Count);

		//TODO : create mapping of material with texture sets
		if (Asset3DParameters->Asset3DMaterialType != ES3DMasterMaterialType::CUSTOM) {
			MaterialParams->MasterMaterialName = MasterMaterialType[Asset3DParameters->Asset3DMaterialType];
			MaterialParams->MasterMaterialPath = GetMasterMaterial(MaterialParams->MasterMaterialName);
		}
		else
		{
			MaterialParams->MasterMaterialName = TEXT("Custom");
			MaterialParams->MasterMaterialPath = MatAssetPathsSettings->MasterMaterial3d;
		}

		MaterialParams->MaterialInstanceDestination = AssetParams->BaseParams->AssetDestination;
		if (AssetImportData->AssetMetaInfo->bIsMTS) {
			MaterialParams->MaterialInstanceName = AssetImportData->MaterialList[Count]->MaterialName + TEXT("_inst");
		}
		else {
			MaterialParams->MaterialInstanceName = AssetParams->BaseParams->AssetName + TEXT("_inst");
		}
		MaterialParams->TexturesDestination = AssetParams->BaseParams->AssetDestination;
		MaterialParams->bContainsPackedMaps = (AssetImportData->PackedTextures.Num() > 0) ? true : false;
		if (AssetImportData->AssetMetaInfo->bIsMixerAsset) {
			MaterialParams->TwoSided = AssetImportData->MaterialList[Count]->TwoSided;
			MaterialParams->cutoutValue = AssetImportData->MaterialList[Count]->cutoutValue;
		}
		Asset3DParameters->MaterialParams.Add(MaterialParams);
		Count++;
	} while (Count < AssetImportData->MaterialList.Num());//&& !AssetImportData->AssetMetaInfo->bIsUdim);//FMTSHandler::Get()->MTSJson.materials.Num());
	AssetParams->ParamsAssetType = Asset3DParameters;
	
	return AssetParams;
}

ES3DMasterMaterialType FAssetImportParams::GetS3DMasterMaterialType(TSharedPtr<FAssetTypeData> AssetImportData, int8 Count)
{
	const UMegascansSettings* MegascansSettings = GetDefault<UMegascansSettings>();	
	const UMaterialAssetSettings* MatAssetPathsSettings = GetDefault<UMaterialAssetSettings>();
	
	if (AssetImportData->AssetMetaInfo->Type == TEXT("3d") && MatAssetPathsSettings->MasterMaterial3d != "None" && MatAssetPathsSettings->MasterMaterial3d != "")
	{
		if (UEditorAssetLibrary::DoesAssetExist(MatAssetPathsSettings->MasterMaterial3d))
		{
			return ES3DMasterMaterialType::CUSTOM;
		}
	}
	if (AssetImportData->AssetMetaInfo->Type == TEXT("surface") && MatAssetPathsSettings->MasterMaterialSurface != "None" && MatAssetPathsSettings->MasterMaterialSurface != "")
	{
		if (UEditorAssetLibrary::DoesAssetExist(MatAssetPathsSettings->MasterMaterialSurface))
		{
			return ES3DMasterMaterialType::CUSTOM;
		}

	}

	if (AssetImportData->AssetMetaInfo->bIsModularWindow) {
		if (AssetImportData->MaterialList[Count]->OpacityType == "Transparent") {
			return ES3DMasterMaterialType::MODULAR_WINDOWS;
		}
	}

	// FAssetTextureData type in auto
	for (auto& TextureComponent : AssetImportData->TextureComponents)
	{
		if (TextureComponent->Type == TEXT("fuzz")) {
			return ES3DMasterMaterialType::NORMAL_FUZZ;
		}
	}
	if (AssetImportData->AssetMetaInfo->bIsUdim) {
		if (AssetImportData->PackedTextures.Num() > 0) {
			if (AssetImportData->MaterialList[Count]->OpacityType == "transparent") {
				return ES3DMasterMaterialType::OPACITY_TRANSPARENT_CP_UDIM;
			}
			if (AssetImportData->MaterialList[Count]->OpacityType == "fade") {
				return ES3DMasterMaterialType::OPACITY_FADE_CP_UDIM;
			}
			if (AssetImportData->MaterialList[Count]->OpacityType == "cutout") {
				return ES3DMasterMaterialType::OPACITY_FADE_CP_UDIM;
			}
			if (AssetImportData->MaterialList[Count]->OpacityType == "opaque") {
				return ES3DMasterMaterialType::CHANNEL_PACK_UDIM;
			}
		}

		if (AssetImportData->MaterialList[Count]->OpacityType == "transparent") {
			return ES3DMasterMaterialType::OPACITY_TRANSPARENT_UDIM;
		}
		if (AssetImportData->MaterialList[Count]->OpacityType == "cutout") {
			return ES3DMasterMaterialType::OPACITY_FADE_UDIM;
		}
		if (AssetImportData->MaterialList[Count]->OpacityType == "fade") {
			return ES3DMasterMaterialType::OPACITY_FADE_UDIM;
		}
		return ES3DMasterMaterialType::UDIM;
	}

	if(AssetImportData->MaterialList.Num() > 0){
		if (AssetImportData->PackedTextures.Num() > 0) {
			if (AssetImportData->MaterialList[Count]->OpacityType == "transparent") {
				return ES3DMasterMaterialType::OPACITY_TRANSPARENT_CP;
			}
			if (AssetImportData->MaterialList[Count]->OpacityType == "cutout") {
				return ES3DMasterMaterialType::OPACITY_CUTOUT_CP;
			}
			if (AssetImportData->MaterialList[Count]->OpacityType == "fade") {
				return ES3DMasterMaterialType::OPACITY_FADE_CP;
			}
		}

		if (AssetImportData->MaterialList[Count]->OpacityType == "transparent") {
			return ES3DMasterMaterialType::OPACITY_TRANSPARENT;
		}
		if (AssetImportData->MaterialList[Count]->OpacityType == "cutout") {
			return ES3DMasterMaterialType::OPACITY_CUTOUT;
		}
		if (AssetImportData->MaterialList[Count]->OpacityType == "fade") {
			return ES3DMasterMaterialType::OPACITY_FADE;
		}
	}

	if (MegascansSettings->bEnableDisplacement) {
		return ES3DMasterMaterialType::NORMAL_DISPLACEMENT;
	}

	if (AssetImportData->PackedTextures.Num() > 0) {
		return ES3DMasterMaterialType::CHANNEL_PACK;
	}

	return ES3DMasterMaterialType::NORMAL;
}

E3DPlantBillboardMaterial FAssetImportParams::GetBillboardMaterialType(TSharedPtr<FAssetTypeData> AssetImportData)
{
	// Type of master material for billboards
	
	return (AssetImportData->AssetMetaInfo->bUseBillboardMaterial) ? E3DPlantBillboardMaterial::CAMERA_FACING : E3DPlantBillboardMaterial::NORMAL_BILLBOARD;
}


TSharedPtr<ImportParamsSurfaceAsset> FAssetImportParams::GetSurfaceParams(TSharedPtr<FAssetTypeData> AssetImportData)
{	
	int8 Count = 0;
	TSharedPtr< ImportParamsSurfaceAsset> AssetParams = MakeShareable(new ImportParamsSurfaceAsset);
	AssetParams->BaseParams = GetAssetImportParams(AssetImportData);
	const UMaterialAssetSettings* MatAssetPathsSettings = GetDefault<UMaterialAssetSettings>();
	TSharedPtr<SurfaceParams> SurfaceParameters = MakeShareable( new SurfaceParams);
	SurfaceParameters->SSubType = GetSurfaceSubtype(AssetImportData);
	AssetImportData->AssetMetaInfo->bIsMetal = CheckMetalness(AssetImportData);
	SurfaceParameters->MasterMaterialPath = TEXT("");
	SurfaceParameters->MasterMaterialName = (SurfaceParameters->SSubType != ESurfaceSubType::SURFACE && SurfaceParameters->SSubType != ESurfaceSubType::METAL) ? SurfaceSubTypeMaterials[SurfaceParameters->SSubType] :MasterMaterialType[GetS3DMasterMaterialType(AssetImportData, Count)];
	if (SurfaceParameters->MasterMaterialName == TEXT("Custom"))
	{		
		SurfaceParameters->MasterMaterialPath = MatAssetPathsSettings->MasterMaterialSurface;		
	}
	else {
		SurfaceParameters->MasterMaterialPath = GetMasterMaterial(SurfaceParameters->MasterMaterialName);
	}		
	SurfaceParameters->MaterialInstanceDestination = AssetParams->BaseParams->AssetDestination;	
	SurfaceParameters->MaterialInstanceName = AssetParams->BaseParams->AssetName + TEXT("_inst");	
	SurfaceParameters->TexturesDestination = AssetParams->BaseParams->AssetDestination;
	SurfaceParameters->bContainsPackedMaps = (AssetImportData->PackedTextures.Num() > 0) ? true : false;	
	AssetParams->ParamsAssetType = SurfaceParameters;
	return AssetParams;
}

TSharedPtr<ImportParams3DPlantAsset> FAssetImportParams::Get3DPlantParams(TSharedPtr<FAssetTypeData> AssetImportData)
{	
	TSharedPtr<ImportParams3DPlantAsset>  AssetParams = MakeShareable(new ImportParams3DPlantAsset);
	AssetParams->BaseParams = GetAssetImportParams(AssetImportData);
	AssetParams->ParamsAssetType = MakeShareable(new Asset3DPlantParams);

	const UMaterialAssetSettings* MatAssetPathsSettings = GetDefault<UMaterialAssetSettings>();		
	AssetParams->ParamsAssetType->FoliageDestination = FPaths::Combine(AssetParams->BaseParams->AssetDestination, TEXT("Foliage"));
	AssetParams->ParamsAssetType->MeshDestination = AssetParams->BaseParams->AssetDestination;
	
	//Plants main master material parameters
	TSharedPtr < SurfaceParams> MasterMaterialParams = MakeShareable( new SurfaceParams);
	MasterMaterialParams->MaterialInstanceDestination = AssetParams->BaseParams->AssetDestination;
	MasterMaterialParams->MaterialInstanceName = AssetParams->BaseParams->AssetName + TEXT("_inst");
	MasterMaterialParams->TexturesDestination = AssetParams->BaseParams->AssetDestination;
	MasterMaterialParams->MasterMaterialName = PlantsMasterMaterial;
	MasterMaterialParams->MasterMaterialPath = GetMasterMaterial(MasterMaterialParams->MasterMaterialName);
	
	if (MatAssetPathsSettings->MasterMaterialPlant != "None" && MatAssetPathsSettings->MasterMaterialPlant != "")
	{
		if (UEditorAssetLibrary::DoesAssetExist(MatAssetPathsSettings->MasterMaterialPlant))
		{
			
			MasterMaterialParams->MasterMaterialName = TEXT("Custom");
			MasterMaterialParams->MasterMaterialPath = MatAssetPathsSettings->MasterMaterialPlant;
		}
	}
	//MasterMaterialParams->bContainsPackedMaps = (AssetImportData->PackedTextures.Num() > 0) ? true : false;
	
	//MasterMaterialParams->SSubType = ESurfaceSubType::SURFACE;
	AssetParams->ParamsAssetType->PlantsMasterMaterial = MasterMaterialParams;	
	
	//Plants billboard master material parameters
	TSharedPtr < SurfaceParams> BillboardMasterParams = MakeShareable( new SurfaceParams) ;
	BillboardMasterParams->MasterMaterialName = BillboardMaterials[GetBillboardMaterialType(AssetImportData)];
	BillboardMasterParams->MaterialInstanceDestination = FPaths::Combine(AssetParams->BaseParams->AssetDestination, TEXT("Billboard"));	
	BillboardMasterParams->MaterialInstanceName = AssetParams->BaseParams->AssetName + TEXT("_Billboard_inst");
	BillboardMasterParams->TexturesDestination = BillboardMasterParams->MaterialInstanceDestination;	
	BillboardMasterParams->MasterMaterialPath = GetMasterMaterial(BillboardMasterParams->MasterMaterialName);	
	AssetParams->ParamsAssetType->BillboardMasterMaterial = BillboardMasterParams;	
	
	return AssetParams;

}

TSharedPtr<AssetImportParams> FAssetImportParams::GetAssetImportParams(TSharedPtr<FAssetTypeData> AssetImportData)
{
	TSharedPtr<AssetImportParams>  AssetImportparameters = MakeShareable(new AssetImportParams);	
	FString AssetType = AssetImportData->AssetMetaInfo->Type;
	FString RootDestination = GetRootDestination(AssetImportData->AssetMetaInfo->ExportPath);
	FString AssetTypePath = GetAssetTypePath(RootDestination, AssetImportData->AssetMetaInfo->Type);
	FString AssetNameOverride = AssetImportData->AssetMetaInfo->FolderNamingConvention;
	if (AssetNameOverride == TEXT(""))
	{
		AssetNameOverride = AssetImportData->AssetMetaInfo->Name;
	}

	AssetImportparameters->AssetName = GetUniqueAssetName(AssetTypePath, RemoveReservedKeywords(NormalizeString(AssetNameOverride)));
	AssetImportparameters->AssetDestination = FPaths::Combine(AssetTypePath, AssetImportparameters->AssetName);

	return AssetImportparameters;
}

TSharedPtr<ImportParamsSurfaceAsset> FAssetImportParams::GetAtlasBrushParams(TSharedPtr<FAssetTypeData> AssetImportData)
{
	TSharedPtr< ImportParamsSurfaceAsset> AssetParams = MakeShareable(new ImportParamsSurfaceAsset);
	AssetParams->BaseParams = GetAssetImportParams(AssetImportData);	
	TSharedPtr<SurfaceParams> SurfaceParameters = MakeShareable(new SurfaceParams);
	SurfaceParameters->MasterMaterialName = (AssetImportData->AssetMetaInfo->Type == TEXT("atlas")) ? AtlasSubTypeMaterials[GetAtlasSubtype(AssetImportData)] : BrushMasterMaterial;
	SurfaceParameters->MasterMaterialPath = GetMasterMaterial(SurfaceParameters->MasterMaterialName);	
	SurfaceParameters->MaterialInstanceDestination = AssetParams->BaseParams->AssetDestination;
	SurfaceParameters->MaterialInstanceName = AssetParams->BaseParams->AssetName + TEXT("_inst");
	SurfaceParameters->TexturesDestination = AssetParams->BaseParams->AssetDestination;
	SurfaceParameters->bContainsPackedMaps = false;

	AssetParams->ParamsAssetType = SurfaceParameters;
	return AssetParams;
}

FString FAssetImportParams::GetAssetTypePath(const FString& RootDestination, const FString& AssetType)
{
	TMap<FString, FString> TypeNames = {
		{TEXT("3d"),TEXT("3D_Assets")},
		{TEXT("3dplant"),TEXT("3D_Plants")},
		{TEXT("surface"),TEXT("Surfaces")},
		{TEXT("atlas"),TEXT("Atlases")},
		{TEXT("brush"),TEXT("Brushes")},
		{TEXT("3datlas"),TEXT("3D_Atlases")}
	};
	return FPaths::Combine(RootDestination, TypeNames[AssetType]);
}

bool FAssetImportParams::CheckMetalness(TSharedPtr<FAssetTypeData> AssetImportData) {
	auto& TagsList = AssetImportData->AssetMetaInfo->Tags;
	if ((TagsList.Contains(TEXT("metal"))) || (AssetImportData->AssetMetaInfo->Categories.Contains(TEXT("metal")))) {
		return true;
	}
	return false;
}


EAtlasSubType FAssetImportParams::GetAtlasSubtype(TSharedPtr<FAssetTypeData> AssetImportData)
{
	auto& TagsList = AssetImportData->AssetMetaInfo->Tags;
	return (!TagsList.Contains(TEXT("decal"))) ? EAtlasSubType::ATLAS: EAtlasSubType::DECAL;
}
ESurfaceSubType FAssetImportParams::GetSurfaceSubtype(TSharedPtr<FAssetTypeData> AssetImportData)
{
	auto& TagsList = AssetImportData->AssetMetaInfo->Tags;
	return (TagsList.Contains(TEXT("metal"))) ? ESurfaceSubType::METAL : (TagsList.Contains(TEXT("surface imperfection"))) ? ESurfaceSubType::IMPERFECTION : (TagsList.Contains(TEXT("tileable displacement"))) ? ESurfaceSubType::DISPLACEMENT : ESurfaceSubType::SURFACE;
}

EAsset3DSubtype FAssetImportParams::Get3DSubtype(TSharedPtr<FAssetTypeData> AssetImportData)
{
	return (AssetImportData->AssetMetaInfo->Categories.Contains(TEXT("scatter")) || AssetImportData->AssetMetaInfo->Tags.Contains(TEXT("scatter")) || AssetImportData->AssetMetaInfo->Categories.Contains(TEXT("cmb_asset")) || AssetImportData->AssetMetaInfo->Tags.Contains(TEXT("cmb_asset")) || AssetImportData->AssetMetaInfo->bIsMTS) ? EAsset3DSubtype::MULTI_MESH : EAsset3DSubtype::NORMAL_3D;
	
}

FString FAssetImportParams::GetMasterMaterial(const FString& SelectedMaterial)
{
	
	//// Needs Fix
	//ManageMaterialVersion(SelectedMaterial);
	CopyPresetTextures();
	// Updated MSTextures but have to handle previous textures list
	CopyUpdatedPresetTextures();
	FString MaterialPath = GetMaterial(SelectedMaterial);
	return (MaterialPath == TEXT("")) ? TEXT("") : MaterialPath;
}





