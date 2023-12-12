// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "AssetImportData.h"

#include "ImportFactory.h"
#include "Utilities/AssetData.h"


class UMaterialInstanceConstant;
class UTexture;

struct TextureData
{
	FString Path;
	UTexture* TextureAsset;
};

struct SurfaceImportParams
{
	
	FString TexturesDestination;
	FString AssetName;
	//FString MasterMaterialPath;
	FString MInstanceDestination;
	FString AssetDestination;
	FString MeshDestination;
	FString FoliageDestination;
	FString MInstanceName;
	bool bExrEnabled;
};


class FImportSurface : public IImportAsset
{
	typedef TMap<FString, TSharedPtr<FAssetPackedTextures>> ChannelPackedData;


private:
	FImportSurface() = default;
	bool bEnableExrDisplacement;	
	static TSharedPtr<FImportSurface> ImportSurfaceInst;
	TextureData ImportTexture(UAssetImportTask * TextureImportTask);
	UMaterialInstanceConstant* CreateInstanceMaterial(const FString& MasterMaterialPath, const FString& InstanceDestination, const FString& AssetName);
	void MInstanceApplyTextures(TArray<TMap<FString, TextureData>> TextureMaps, UMaterialInstanceConstant* MaterialInstance, TSharedPtr<SurfaceParams> SurfaceImportParams, TSharedPtr<FAssetTypeData> AssetImportData);
	TArray<FString> GetPackedMapsList(TSharedPtr<FAssetTypeData> AssetImportData);
	TMap<FString, TSharedPtr<FAssetPackedTextures>> ImportPackedMaps(TSharedPtr<FAssetTypeData> AssetImportData, const FString& TexturesDestination);
	FString GetSurfaceType(TSharedPtr<FAssetTypeData> AssetImportData);		
	TArray<FString> GetFilteredMaps(ChannelPackedData PackedImportData, UMaterialInstanceConstant* MaterialInstance);
	const TArray<FString> NonLinearMaps = { "albedo", "diffuse", "translucency", "specular", "emission"};
	const TArray<FString> ColorPackedMaps = { "albedo", "diffuse", "translucency", "specular" , "normal", "emission" };
	void NormalizeTextureNamesInJson(TSharedPtr<FAssetTypeData> AssetImportData);
	const TArray<FString> AllMapTypes = {
		  "opacity",
		  "normal",
		  "bump",
		  "gloss" ,
		  "displacement",
		  "ao",
		  "thickness",
		  "cavity",
		  "specular",
		  "normalbump",
		  "translucency",
		  "albedo",
		  "roughness",
		  "diffuse",
		  "fuzz",
		  "metalness",
		  "emission"
	};

	TMap< FString, TextureCompressionSettings> MapCompressionType = {
		{TEXT("albedo"),  TextureCompressionSettings::TC_Default},
		{TEXT("emission"),  TextureCompressionSettings::TC_Default},
		{TEXT("ao"),  TextureCompressionSettings::TC_Alpha},
		{TEXT("roughness"),  TextureCompressionSettings::TC_Alpha},
		{TEXT("metalness"),  TextureCompressionSettings::TC_Alpha},
		{TEXT("displacement"),  TextureCompressionSettings::TC_Alpha},
		{TEXT("translucency"),  TextureCompressionSettings::TC_Default},
		{TEXT("opacity"),  TextureCompressionSettings::TC_Alpha},
		{TEXT("normal"),  TextureCompressionSettings::TC_Normalmap}

	};

	TArray<FString> GetPackedTypes(ChannelPackedData PackedImportData);
	void MInstanceApplyPackedMaps(TMap<FString, TSharedPtr<FAssetPackedTextures>> PackedImportData, UMaterialInstanceConstant* MaterialInstance, TSharedPtr<SurfaceParams> SurfaceImportParams, TSharedPtr<FAssetTypeData> AssetImportData);
	TMap<FString, TSharedPtr<FAssetPackedTextures>> PackedImportData;

	FString  GetMaterialOverride(TSharedPtr<FAssetTypeData> AssetImportData);
	// New implementation
	//TMap<FString, TSharedPtr<FAssetPackedTextures>> ImportPackedMaps(TArray<TSharedPtr<FAssetPackedTextures>> PackedTextures, const FString& TexturesDestination);
	UMaterialInstanceConstant* CreateInstanceMaterial(TSharedPtr<FAssetTypeData> AssetImportData, TSharedPtr<SurfaceParams> SurfaceImportParams);
	TArray<TMap<FString, TextureData>> ImportTextureMaps(TSharedPtr<FAssetTypeData> AssetImportData, TSharedPtr<SurfaceParams> SurfaceImportParams, const TArray<FString>& FilteredTextureTypes);
	UAssetImportTask* CreateImportTask(TSharedPtr<FAssetTextureData> TextureMetaData, const FString& TexturesDestination);
	void ApplyMaterialToSelection(UMaterialInstanceConstant* MaterialInstance);

public:
	virtual void ImportAsset(TSharedPtr<FAssetTypeData> AssetImportData) override;
	static TSharedPtr<FImportSurface> Get();	
	
	TArray<TMap<FString, TextureData>> AllTextureMaps;

	UMaterialInstanceConstant* ImportSurface( TSharedPtr<FAssetTypeData> AssetImportData, TSharedPtr<SurfaceParams> SurfaceImportParams, bool is3DPlant=false);
};