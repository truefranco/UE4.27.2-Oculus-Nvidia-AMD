#pragma once
#include "CoreMinimal.h"
#include "MTSReader.generated.h"


USTRUCT()
struct FLodList {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString lod;
	UPROPERTY()
		int8 variation;
	UPROPERTY()
		FString path;
	UPROPERTY()
		FString name;
	UPROPERTY()
		FString nameOverride;
	UPROPERTY()
		FString lodObjectName;
	UPROPERTY()
		FString format;
	UPROPERTY()
		FString type;
};

USTRUCT()
struct FChannelsData {
	GENERATED_BODY()
public:
	UPROPERTY()
		TArray<FString> Alpha;
	UPROPERTY()
		TArray<FString> Red;
	UPROPERTY()
		TArray<FString> Blue;
	UPROPERTY()
		TArray<FString> Green;
	UPROPERTY()
		TArray<FString> Grayscale;
};


USTRUCT()
struct FMeshList {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString format;
	UPROPERTY()
		FString type;
	UPROPERTY()
		int16 resolution;
	UPROPERTY()
		FString name;
	UPROPERTY()
		FString nameOverride;
	UPROPERTY()
		FString path;

};


USTRUCT()
struct FComponents {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString format;
	UPROPERTY()
		FString type;
	UPROPERTY()
		int16 resolution;
	UPROPERTY()
		FString name;
	UPROPERTY()
		FString nameOverride;
	UPROPERTY()
		FString path;

};


USTRUCT()
struct FMeta {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString key;
	UPROPERTY()
		FString name;
	UPROPERTY()
		bool value;

};

USTRUCT()
struct FMapNameOverrides {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString albedo;
	UPROPERTY()
		FString ao;
	UPROPERTY()
		FString bump;
	UPROPERTY()
		FString cavity;
	UPROPERTY()
		FString curvature;
	UPROPERTY()
		FString diffuse;
	UPROPERTY()
		FString displacement;
	UPROPERTY()
		FString fuzz;
	UPROPERTY()
		FString gloss;
	UPROPERTY()
		FString mask;
	UPROPERTY()
		FString metalness;
	UPROPERTY()
		FString normal;
	UPROPERTY()
		FString normalObject;
	UPROPERTY()
		FString roughness;
	UPROPERTY()
		FString specular;
	UPROPERTY()
		FString thickness;
	UPROPERTY()
		FString transmission;

};

USTRUCT()
struct FNamingConvention {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString texture;
	UPROPERTY()
		FString model;
	UPROPERTY()
		FString folder;

};


USTRUCT()
struct FMaterials {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString opacityType;
	UPROPERTY()
		FString materialName;
	UPROPERTY()
		FString materialId;
	UPROPERTY()
		TArray<FString> textureSets;

};

USTRUCT()
struct FTextureSets {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString textureSetName;
	UPROPERTY()
		bool isUdim;
	UPROPERTY()
		FString udimTile;

};


USTRUCT()
struct FPackedTex {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString namingConvention;
	UPROPERTY()
		FString format;
	UPROPERTY()
		FString type;
	UPROPERTY()
		FString resolution;
	UPROPERTY()
		FString name;
	UPROPERTY()
		FString nameOverride;
	UPROPERTY()
		FString lod;
	UPROPERTY()
		FString path;
	UPROPERTY()
		FChannelsData channelsData;
	UPROPERTY()
		TArray<FString> textureSets;
};


USTRUCT()
struct FMTSJson {
	GENERATED_BODY()
public:
	UPROPERTY()
		FString minLOD;
	UPROPERTY()
		FString scriptFilePath;
	UPROPERTY()
		FString message;
	UPROPERTY()
		int8 version;
	UPROPERTY()
		FString resolution;
	UPROPERTY()
		int16 resolutionValue;
	UPROPERTY()
		FString category;
	UPROPERTY()
		FString type;
	UPROPERTY()
		FString id;
	UPROPERTY()
		FString name;
	UPROPERTY()
		FString path;
	UPROPERTY()
		FString exportAs;
	UPROPERTY()
		FString textureFormat;
	UPROPERTY()
		FString meshFormat;
	UPROPERTY()
		FString previewImage;
	UPROPERTY()
		FString averageColor;
	UPROPERTY()
		TArray<FString> tags;
	UPROPERTY()
		FString activeLOD;
	UPROPERTY()
		TArray<FString> categories;
	UPROPERTY()
		bool isExternal;
	UPROPERTY()
		FString exportPath;
	UPROPERTY()
		FNamingConvention namingConvention;
	UPROPERTY()
		FString folderNamingConvention;
	UPROPERTY()
		FMapNameOverrides mapNameOverride;
	UPROPERTY()
		TArray<FMeta> meta; 
	UPROPERTY()
		TArray<FMaterials> materials;
	UPROPERTY()
		TArray<FTextureSets> textureSets;
	UPROPERTY()
		FString workflow;
	UPROPERTY()
		TArray<FComponents> components;
	UPROPERTY()
		TArray<FMeshList> meshList;
	UPROPERTY()
		TArray<FPackedTex> packedTextures;
	UPROPERTY()
		TArray<FLodList> lodList;
	UPROPERTY()
		int8 meshVersion;
	UPROPERTY()
		TArray<FString> components_billboard;
	UPROPERTY()
		bool isCustom;
	UPROPERTY()
		bool isModularAsset;
};


class FMTSHandler {
private:
	FMTSHandler() = default;
	static TSharedPtr<FMTSHandler> MTSInst;
public:
	static TSharedPtr<FMTSHandler> Get();

	FMTSJson MTSJson;
	void GetMTSData(FString& ReceivedJson);
};
