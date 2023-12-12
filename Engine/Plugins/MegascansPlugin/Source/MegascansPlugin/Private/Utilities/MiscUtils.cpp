// Copyright Epic Games, Inc. All Rights Reserved.
#include "Utilities/MiscUtils.h"

#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/GenericPlatform/GenericPlatformFile.h"
#include "Runtime/Core/Public/HAL/FileManager.h"

#include "EditorAssetLibrary.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Runtime/Engine/Classes/Engine/Selection.h"

#include "Editor/UnrealEd/Classes/Factories/MaterialInstanceConstantFactoryNew.h"

#include "Runtime/Engine/Classes/Engine/StaticMesh.h"

#include "Runtime/Core/Public/Misc/MessageDialog.h"
#include "Runtime/Core/Public/Internationalization/Text.h"

#include "PackageTools.h"

#include <regex>
#include <ostream>
#include <sstream>

#include "Misc/ScopedSlowTask.h"
#include "Utilities/AssetsDatabase.h"
#include "EditorAssetLibrary.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/FileHelper.h"
#include "Kismet/GameplayStatics.h"

FString CommonPaths::CommonDestinationPath;
FString CommonPaths::ProjectCommonPath;
FString CommonPaths::SourceCommonPath;

TSharedPtr<FJsonObject> DeserializeJson(const FString& JsonStringData)
{
	TSharedPtr<FJsonObject> JsonDataObject;
	bool bIsDeseriliazeSuccessful = false;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonStringData);
	bIsDeseriliazeSuccessful = FJsonSerializer::Deserialize(JsonReader, JsonDataObject);
	return JsonDataObject;
}

void ShowErrorDialog(const FString& ErrorMessage)
{

}

FString GetPluginPath()
{
	FString PluginName = TEXT("MegascansPlugin");
	FString PluginsPath = FPaths::EnginePluginsDir();

	FString MSPluginPath = FPaths::Combine(PluginsPath, PluginName);
	return MSPluginPath;
}

FString GetSourceMSPresetsPath()
{
	FString MaterialPresetsPath = FPaths::Combine(GetPluginPath(), TEXT("Content"), GetMSPresetsName());
	return MaterialPresetsPath;
}

FString GetPreferencesPath()
{
	FString PrefFileName = TEXT("unreal_action_settings.json");
	return FPaths::Combine(GetPluginPath(), TEXT("Content"), TEXT("Python"), PrefFileName);
}

FString GetMSPresetsName()
{
	return TEXT("MSPresets");
}

FString GetMaterial(const FString& MaterialName)
{

	FString MaterialPath = FPaths::Combine(TEXT("/Game"), GetMSPresetsName(), MaterialName, MaterialName);

	if (!UEditorAssetLibrary::DoesAssetExist(MaterialPath))
	{
		//UE_LOG(LogTemp, Error, TEXT("Master Material doesnt exist."));
		if (CopyMaterialPreset(MaterialName))
		{
			if (UEditorAssetLibrary::DoesAssetExist(MaterialPath))
			{


				return MaterialPath;
			}

		}
	}
	else {
		return MaterialPath;
	}
	return TEXT("");
}

bool CopyMaterialPreset(const FString& MaterialName)
{
	FString MaterialSourceFolderPath = FPaths::Combine(GetSourceMSPresetsPath(), MaterialName);
	MaterialSourceFolderPath = FPaths::ConvertRelativePathToFull(MaterialSourceFolderPath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*MaterialSourceFolderPath))
	{
		return false;
	}

	FString MaterialDestinationPath = FPaths::ProjectContentDir();
	MaterialDestinationPath = FPaths::Combine(MaterialDestinationPath, GetMSPresetsName());
	MaterialDestinationPath = FPaths::Combine(MaterialDestinationPath, MaterialName);

	if (!PlatformFile.DirectoryExists(*MaterialDestinationPath))
	{

		if (!PlatformFile.CreateDirectoryTree(*MaterialDestinationPath))
		{

			return false;
		}

		if (!PlatformFile.CopyDirectoryTree(*MaterialDestinationPath, *MaterialSourceFolderPath, true))
		{

			return false;
		}

	}


	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FString> MaterialBasePath;
	FString BasePath = FPaths::Combine(FPaths::ProjectContentDir(), GetMSPresetsName());
	MaterialBasePath.Add("/Game/MSPresets");
	MaterialBasePath.Add(FPaths::Combine(TEXT("/Game/MSPresets"), MaterialName));
	if (PlatformFile.DirectoryExists(*FPaths::Combine(TEXT("/Game/MSPresets"), MaterialName, TEXT("Functions"))))
	{
		MaterialBasePath.Add(FPaths::Combine(TEXT("/Game/MSPresets"), MaterialName, TEXT("Functions")));
	}
	AssetRegistryModule.Get().ScanPathsSynchronous(MaterialBasePath, true);
	return true;

}


void ManageMaterialVersion(const FString& SelectedMaterial)
{
	float LatestMaterialVersion = GetLatestMaterialVersion(SelectedMaterial);
	float ExistingMaterialVersion = FAssetsDatabase::Get()->GetMaterialVersion(SelectedMaterial);
	FString VersionString = FString::SanitizeFloat(ExistingMaterialVersion).Replace(TEXT("."), TEXT(""));


	if (LatestMaterialVersion != ExistingMaterialVersion)
	{
		FString MaterialDestinationPath = FPaths::Combine(FPaths::ProjectContentDir(), GetMSPresetsName(), SelectedMaterial);
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (PlatformFile.DirectoryExists(*MaterialDestinationPath))
		{
			UE_LOG(LogTemp, Error, TEXT("Renaming folder."));
			FString MaterialPath = FPaths::Combine(TEXT("/Game"), GetMSPresetsName(), SelectedMaterial);
			UEditorAssetLibrary::RenameDirectory(MaterialPath, MaterialPath + FString::Printf(TEXT("_%s"), *VersionString));
		}
		FAssetsDatabase::Get()->AddMaterialRecord(SelectedMaterial, LatestMaterialVersion);
	}



}

float GetLatestMaterialVersion(const FString& MasterMaterialName)
{
	FString MaterialPresetsJson = FPaths::Combine(GetSourceMSPresetsPath(), TEXT("MSPresetsVerions.json"));
	FString MaterialsJsonString;
	FFileHelper::LoadFileToString(MaterialsJsonString, *MaterialPresetsJson);
	TSharedPtr<FJsonObject> MaterialsDataObject = DeserializeJson(MaterialsJsonString);
	TArray<TSharedPtr<FJsonValue> > MaterialsVersionArray = MaterialsDataObject->GetArrayField(TEXT("materials"));
	for (TSharedPtr<FJsonValue> MaterialVersionObject : MaterialsVersionArray)
	{
		if (MaterialVersionObject->AsObject()->GetStringField(TEXT("materialName")) == MasterMaterialName)
		{
			return MaterialVersionObject->AsObject()->GetNumberField("materialVersion");
		}
	}
	return 0.0f;

}


bool CopyPresetTextures()
{
	FString TexturesDestinationPath = FPaths::ProjectContentDir();

	TexturesDestinationPath = FPaths::Combine(TexturesDestinationPath, GetMSPresetsName(), TEXT("MSTextures"));

	FString TexturesSourceFolderPath = FPaths::Combine(GetSourceMSPresetsPath(), TEXT("MSTextures"));
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*TexturesDestinationPath))
	{
		if (!PlatformFile.CreateDirectoryTree(*TexturesDestinationPath))
		{

			return false;
		}

		if (!PlatformFile.CopyDirectoryTree(*TexturesDestinationPath, *TexturesSourceFolderPath, true))
		{

			return false;
		}
	}

	return true;
}

// Added with Opacity/Emission release after updating the list of textures in MSTextures
bool CopyUpdatedPresetTextures()
{
	FString TexturesDestinationPath = FPaths::ProjectContentDir();

	TexturesDestinationPath = FPaths::Combine(TexturesDestinationPath, GetMSPresetsName(), TEXT("MS_DefaultTextures"));

	FString TexturesSourceFolderPath = FPaths::Combine(GetSourceMSPresetsPath(), TEXT("MS_DefaultTextures"));
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*TexturesDestinationPath))
	{
		if (!PlatformFile.CreateDirectoryTree(*TexturesDestinationPath))
		{

			return false;
		}

		if (!PlatformFile.CopyDirectoryTree(*TexturesDestinationPath, *TexturesSourceFolderPath, true))
		{

			return false;
		}
	}

	return true;
}
UObject* LoadAsset(const FString& AssetPath)
{
	return UEditorAssetLibrary::LoadAsset(AssetPath);
}

TArray<FString> GetAssetsList(const FString& DirectoryPath)
{
	return UEditorAssetLibrary::ListAssets(DirectoryPath, false);

}
void SaveAsset(const FString& AssetPath)
{
	UEditorAssetLibrary::SaveAsset(AssetPath);
}


void DeleteExtraMesh(const FString& BasePath)
{
	TArray<FString> AssetBasePath;
	AssetBasePath.Add(BasePath);
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().ScanPathsSynchronous(AssetBasePath, true);
	FString ExtraMeshName = TEXT("StaticMesh_0");
	FString AssetPath = FPaths::Combine(BasePath, ExtraMeshName);

	AssetRegistryModule.Get().ScanPathsSynchronous(AssetBasePath, true);
}





//FString ResolvePath(const FString& PathConvention, TSharedPtr<FAssetTypeData> AssetsImportData, TSharedPtr<FAssetTextureData> TextureData)
//{
//	TMap<FString, FString> TypeNames = {
//		{TEXT("3d"),TEXT("3D_Assets")},
//		{TEXT("3dplant"),TEXT("3D_Plants")},
//		{TEXT("surface"),TEXT("Surfaces")},
//		{TEXT("atlas"),TEXT("Atlases")},
//		{TEXT("brush"),TEXT("Brushes")},
//		{TEXT("3datlas"),TEXT("3D_Atlases")}
//	};
//
//	
//	FString ResolvedPath = TEXT("");
//	FString ResolvedConvention = TEXT("");
//	
//	FString PlaceholderConventionTexture = "Megascans/{AssetType}/{AssetName}/";
//
//	TArray<FString> NamingConventions;
//	PathConvention.ParseIntoArray(NamingConventions, TEXT("/"));
//
//	for (FString NamingConvention : NamingConventions)
//	{
//		if (NamingConvention.Contains(TEXT("{")) || NamingConvention.Contains(TEXT("}")))
//		{
//			
//			ResolvedConvention = TEXT("");
//			NamingConvention = NamingConvention.Replace(TEXT("{"), TEXT(""));
//			NamingConvention = NamingConvention.Replace(TEXT("}"), TEXT(""));
//
//			if (NamingConvention == TEXT("AssetName"))
//			{
//				ResolvedConvention = AssetsImportData->AssetMetaInfo->Name;
//			}
//			else if (NamingConvention == TEXT("AssetType"))
//			{
//				if (TypeNames.Contains(AssetsImportData->AssetMetaInfo->Type))
//				{
//					ResolvedConvention = TypeNames[AssetsImportData->AssetMetaInfo->Type];
//				}
//				else {
//					ResolvedConvention = TEXT("Misc");
//				}
//			}
//			else if (NamingConvention == TEXT("TextureType"))
//			{
//				if (TextureData)
//				{
//					ResolvedConvention = TextureData->Type;
//				}
//				else {
//					ResolvedConvention = TEXT("TextureType");
//				}
//			}
//			else if (NamingConvention == TEXT("AssetId"))
//			{
//				ResolvedConvention = AssetsImportData->AssetMetaInfo->Id;
//			}
//			else if (NamingConvention == TEXT("TextureResolution"))
//			{
//				if (TextureData)
//				{
//					ResolvedConvention = TextureData->Resolution;
//				}
//				else {
//					ResolvedConvention = TEXT("Resolution");
//				}
//			}
//			else if (NamingConvention == TEXT("TextureName"))
//			{
//				if (TextureData)
//				{
//					ResolvedConvention = TextureData->Name;
//				}
//				else {
//					ResolvedConvention = TEXT("Name");
//				}
//			}
//
//			ResolvedConvention = NormalizeString(ResolvedConvention);
//			ResolvedPath = FPaths::Combine(ResolvedPath, ResolvedConvention);
//		}
//		else 
//		{
//			ResolvedPath = FPaths::Combine(ResolvedPath, NamingConvention);
//		}
//
//	}
//
//	return ResolvedPath;
//}

FString ResolveName(const FString& NamingConvention, TSharedPtr<FAssetTypeData> AssetsImportData, TSharedPtr<FAssetTextureData> TextureData)
{

	FString ResolvedName = TEXT("");
	FString DummyNameConvention = TEXT("prefix_{AssetName}{AssetType}_{TextureType}_{TextureName}_postfix");
	return ResolvedName;
}


FString NormalizeString(FString InputString)
{
	const std::string SInputString = std::string(TCHAR_TO_UTF8(*InputString));
	const std::regex SpecialCharacters("[^a-zA-Z0-9-]+");
	std::stringstream Result;
	std::regex_replace(std::ostream_iterator<char>(Result), SInputString.begin(), SInputString.end(), SpecialCharacters, "_");
	FString NormalizedString = FString(Result.str().c_str());

	return NormalizedString;
}

FString RemoveReservedKeywords(const FString& Name)
{
	FString ResolvedName = Name;
	TArray<FString> ReservedKeywrods = { "CON","PRN", "AUX", "CLOCK$", "NUL", "NONE", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
	TArray<FString> NameArray;
	Name.ParseIntoArray(NameArray, TEXT("_"));
	for (FString NameTag : NameArray)
	{
		if (ReservedKeywrods.Contains(NameTag))
		{
			ResolvedName = ResolvedName.Replace(*NameTag, TEXT(""));
		}
	}
	return ResolvedName;
}


TArray<FString> ParseLodList(TSharedPtr<FAssetTypeData> AssetImportData)
{

	FString FileExtension;
	FString FilePath;
	AssetImportData->MeshList[0]->Path.Split(TEXT("."), &FilePath, &FileExtension);
	FileExtension = FPaths::GetExtension(AssetImportData->MeshList[0]->Path);

	TArray<FString> LodPathList;
	int32 CurrentLod = FCString::Atoi(*AssetImportData->AssetMetaInfo->ActiveLOD.Replace(TEXT("lod"), TEXT("")));

	if (AssetImportData->AssetMetaInfo->ActiveLOD == TEXT("high")) CurrentLod = -1;
	for (TSharedPtr<FAssetLodData> LodData : AssetImportData->LodList)
	{
		if (LodData->Lod == TEXT("high")) continue;

		int32 LodNumber = FCString::Atoi(*LodData->Lod.Replace(TEXT("lod"), TEXT("")));
		if (LodNumber > CurrentLod)
		{
			FString LodExtension, LodName;
			LodData->Path.Split(TEXT("."), &LodName, &LodExtension);
			LodExtension = FPaths::GetExtension(LodData->Path);
			if (LodExtension == FileExtension)
			{

				LodPathList.Add(LodData->Path);
			}
		}
	}
	return LodPathList;
}


TArray<TSharedPtr<FAssetLodData>> ParsePlantsLodList(TSharedPtr<FAssetTypeData> AssetImportData)
{

	FString FileExtension;
	FileExtension = FPaths::GetExtension(AssetImportData->MeshList[0]->Path);
	TArray<TSharedPtr<FAssetLodData>> SelectedLods;


	int32 CurrentLod = FCString::Atoi(*AssetImportData->AssetMetaInfo->ActiveLOD.Replace(TEXT("lod"), TEXT("")));

	if (AssetImportData->AssetMetaInfo->ActiveLOD == TEXT("high")) CurrentLod = -1;
	for (TSharedPtr<FAssetLodData> LodData : AssetImportData->LodList)
	{
		if (LodData->Lod == TEXT("high")) continue;

		int32 LodNumber = FCString::Atoi(*LodData->Lod.Replace(TEXT("lod"), TEXT("")));
		if (LodNumber > CurrentLod)
		{
			FString LodExtension, LodName;
			LodData->Path.Split(TEXT("."), &LodName, &LodExtension);
			LodExtension = FPaths::GetExtension(LodData->Path);
			if (LodExtension == FileExtension)
			{
				SelectedLods.Add(LodData);
			}
		}
	}

	return SelectedLods;

}


FString GetRootDestination(const FString& ExportPath)
{
	TArray<FString> PathTokens;
	FString BasePath;
	FString DestinationPath;
	ExportPath.Split(TEXT("Content"), &BasePath, &DestinationPath);
	FString RootDestination = TEXT("/Game/");

#if PLATFORM_WINDOWS
	DestinationPath.ParseIntoArray(PathTokens, TEXT("\\"));
#elif PLATFORM_MAC
	DestinationPath.ParseIntoArray(PathTokens, TEXT("/"));
#elif PLATFORM_LINUX
	DestinationPath.ParseIntoArray(PathTokens, TEXT("/"));
#endif	

	for (FString Token : PathTokens)
	{
		FString NormalizedToken = RemoveReservedKeywords(NormalizeString(Token));
		RootDestination = FPaths::Combine(RootDestination, NormalizedToken);
	}
	if (RootDestination == TEXT("/Game/"))
	{
		RootDestination = FPaths::Combine(RootDestination, TEXT("Megascans"));
	}
	return RootDestination;
}




FString ResolveDestination(const FString& AssetDestination)
{
	return TEXT("Resolved destination path");
}

FString GetAssetName(TSharedPtr<FAssetTypeData> AssetImportData)
{

	return FString();
}

FString GetUniqueAssetName(const FString& AssetDestination, const FString AssetName, bool FileSearch)
{

	FString ResolvedAssetName = RemoveReservedKeywords(NormalizeString(AssetName));
	TArray<FString> AssetDirectories;
	auto& FileManager = IFileManager::Get();
	FileManager.FindFiles(AssetDirectories, *AssetDestination, FileSearch, !FileSearch);

	if (!UEditorAssetLibrary::DoesDirectoryExist(FPaths::Combine(AssetDestination, ResolvedAssetName)))
	{
		return ResolvedAssetName;
	}

	for (int i = 0; i < 200; i++)
	{
		FString ResolvedAssetDirectory = TEXT("");
		if (i < 10) {
			ResolvedAssetDirectory = ResolvedAssetName + TEXT("_0") + FString::FromInt(i);
		}
		else
		{
			ResolvedAssetDirectory = ResolvedAssetName + TEXT("_") + FString::FromInt(i);
		}

		if (!UEditorAssetLibrary::DoesDirectoryExist(FPaths::Combine(AssetDestination, ResolvedAssetDirectory)))
		{

			return ResolvedAssetDirectory;
		}

	}

	return TEXT("");
}

FString SanitizeName(const FString& InputName)
{
	return RemoveReservedKeywords(NormalizeString(InputName));
}




//template<typename T>
TArray<UMaterialInstanceConstant*> AssetUtils::GetSelectedAssets(const FString& AssetClass)
{
	TArray<UMaterialInstanceConstant*> ObjectArray;

	TArray<FAssetData> AssetDatas;
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
	ContentBrowserSingleton.GetSelectedAssets(AssetDatas);

	for (FAssetData SelectedAsset : AssetDatas)
	{

		if (SelectedAsset.AssetClass == FName(*AssetClass))
		{
			ObjectArray.Add(CastChecked<UMaterialInstanceConstant>(UEditorAssetLibrary::LoadAsset(SelectedAsset.ObjectPath.ToString())));
		}
	}
	return ObjectArray;
}

void AssetUtils::FocusOnSelected(const FString& Path)
{
	TArray<FString> Folders;
	Folders.Add(Path);
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
	ContentBrowserSingleton.SyncBrowserToFolders(Folders);
}


void AssetUtils::AddStaticMaterial(UStaticMesh* SourceMesh, UMaterialInstanceConstant* NewMaterial)
{
	if (NewMaterial == nullptr) return;

	UMaterialInterface* NewMatInterface = CastChecked<UMaterialInterface>(NewMaterial);
	FStaticMaterial MaterialSlot = FStaticMaterial(NewMatInterface);
	SourceMesh->GetStaticMaterials().Add(MaterialSlot);
}

void AssetUtils::SavePackage(UObject* SourceObject)
{
	TArray<UObject*> InputObjects;
	InputObjects.Add(SourceObject);
	UPackageTools::SavePackagesForObjects(InputObjects);

}

bool DHI::GetDHIJsonData(const FString& JsonStringData, TArray<FDHIData>& DHIAssetsData)
{
	FString StartString = TEXT("{\"DHIAssets\":");
	FString EndString = TEXT("}");
	FString FinalString = StartString + JsonStringData + EndString;
	TSharedPtr<FJsonObject> ImportDataObject = DeserializeJson(FinalString);

	TArray<TSharedPtr<FJsonValue> > AssetsImportDataArray = ImportDataObject->GetArrayField(TEXT("DHIAssets"));

	if (!AssetsImportDataArray[0]->AsObject()->HasField(TEXT("DHI"))) return false;

	for (TSharedPtr<FJsonValue> AssetDataObject : AssetsImportDataArray)
	{
		FDHIData CharacterData;
		TSharedPtr<FJsonObject> CharacterDataObject = AssetDataObject->AsObject()->GetObjectField(TEXT("DHI"));
		CharacterData.CharacterPath = CharacterDataObject->GetStringField(TEXT("characterPath"));
		CharacterData.CharacterName = CharacterDataObject->GetStringField(TEXT("name"));
		CharacterData.CommonPath = CharacterDataObject->GetStringField(TEXT("commonPath"));

		CharacterData.CharacterPath = FPaths::Combine(CharacterData.CharacterPath, CharacterData.CharacterName);
		DHIAssetsData.Add(CharacterData);

	}


	return true;

}

void DHI::CopyCharacter(const FDHIData& CharacterSourceData, bool OverWriteExisting)
{

	bool bOverwriteExisting = false;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FString> AssetsBasePath;
	AssetsBasePath.Add(TEXT("/Game/MetaHumans"));

	FString BPName = TEXT("BP_") + CharacterSourceData.CharacterName;
	BPName += TEXT(".") + BPName;
	FString BPPath = FPaths::Combine(TEXT("/Game/MetaHumans/"), CharacterSourceData.CharacterName, BPName);

	CommonPaths::CommonDestinationPath = FPaths::ProjectContentDir();
	FString MetaHumansRoot = FPaths::Combine(CommonPaths::CommonDestinationPath, TEXT("MetaHumans"));
	CommonPaths::CommonDestinationPath = FPaths::Combine(MetaHumansRoot, TEXT("Common"));
	FString RootFolder = TEXT("/Game/MetaHumans");
	FString CommonFolder = FPaths::Combine(RootFolder, TEXT("Common"));
	FString CharacterName = CharacterSourceData.CharacterName;
	FString CharacterDestination = FPaths::Combine(MetaHumansRoot, CharacterName);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	CommonPaths::ProjectCommonPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("MetaHumans"), TEXT("Common")));
	CommonPaths::SourceCommonPath = CharacterSourceData.CommonPath;
	FPaths::NormalizeDirectoryName(CommonPaths::SourceCommonPath);

	UClass* CharacterClass = nullptr;

	//// backward compatibility
	FString SourceVersionFilePathJSON = FPaths::Combine(CommonPaths::SourceCommonPath, TEXT("VersionInfo.json"));
	FString DestVersionFilePathJSON = FPaths::Combine(CommonPaths::ProjectCommonPath, TEXT("VersionInfo.json"));

	FString SourceVersionFilePathTXT = FPaths::Combine(CommonPaths::SourceCommonPath, TEXT("VersionInfo.txt"));
	FString DestVersionFilePathTXT = FPaths::Combine(CommonPaths::ProjectCommonPath, TEXT("VersionInfo.txt"));

	bool bOverwriteCommon = (PlatformFile.FileExists(*SourceVersionFilePathTXT) || PlatformFile.FileExists(*SourceVersionFilePathJSON)) &&
		!(PlatformFile.FileExists(*DestVersionFilePathTXT) || PlatformFile.FileExists(*DestVersionFilePathJSON));
	bool bDirectoryExists = UEditorAssetLibrary::DoesDirectoryExist(CommonFolder);

	if (bDirectoryExists && bOverwriteCommon)
	{
		const FString Message = FString::Printf(TEXT("Exporting this character will require you to update the common files. Please close UE and overwrite the common files manually \n FROM: \n '%s' \n TO: \n '%s'"), *CommonPaths::SourceCommonPath, *CommonPaths::ProjectCommonPath);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
		return;
	}

	if (PlatformFile.DirectoryExists(*CharacterDestination))
	{
		FString CharacterDirectory = FPaths::Combine(RootFolder, CharacterName);

		FString ProjectAbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("MetaHumans"), *CharacterName));
		FPaths::NormalizeDirectoryName(ProjectAbsolutePath);

		FString SourceAbsolutePath = CharacterSourceData.CharacterPath;
		FPaths::NormalizeDirectoryName(SourceAbsolutePath);

		CopyCommonFiles(CharacterSourceData);

		bool bTwoVersionFiles = PlatformFile.FileExists(*DestVersionFilePathTXT) && PlatformFile.FileExists(*DestVersionFilePathJSON);

		if (bTwoVersionFiles)
		{
			PlatformFile.DeleteFile(*DestVersionFilePathJSON);
		}

		const FString Message = FString::Printf(TEXT("The character you are trying to import already exists. Please close UE and overwrite the character manually \n FROM: \n '%s' \n TO: \n '%s'"), *SourceAbsolutePath, *ProjectAbsolutePath);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));

		return;

		// For Automated Overwrite Implementation
		//AssetUtils::DeleteDirectory(*CharacterDirectory);
		//bOverwriteExisting = true;
	}

	CopyCommonFiles(CharacterSourceData);

	FString CharacterCopyMsg = TEXT("Importing : ") + CharacterName;
	FText CharacterCopyMsgDialogMessage = FText::FromString(CharacterCopyMsg);
	FScopedSlowTask CharacterLoadprogress(1.0f, CharacterCopyMsgDialogMessage, true);
	CharacterLoadprogress.MakeDialog();
	CharacterLoadprogress.EnterProgressFrame(1.0f);

	PlatformFile.CopyDirectoryTree(*CharacterDestination, *CharacterSourceData.CharacterPath, true);

	bool bTwoVersionFiles = PlatformFile.FileExists(*DestVersionFilePathTXT) && PlatformFile.FileExists(*DestVersionFilePathJSON);

	if (bTwoVersionFiles)
	{
		PlatformFile.DeleteFile(*DestVersionFilePathJSON);
	}

	AssetRegistryModule.Get().ScanPathsSynchronous(AssetsBasePath, true);

	FAssetData CharacterAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*BPPath);
	UObject* CharacterObject = CharacterAssetData.GetAsset();
}



void AssetUtils::DeleteDirectory(FString TargetDirectory)
{

	TArray<FString> PreviewAssets = UEditorAssetLibrary::ListAssets(TargetDirectory);

	for (FString AssetPath : PreviewAssets)
	{
		DeleteAsset(AssetPath);
	}

	UEditorAssetLibrary::DeleteDirectory(TargetDirectory);


}

bool AssetUtils::DeleteAsset(const FString& AssetPath)
{
	IAssetRegistry& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FName(*AssetPath));

	if (AssetData.IsAssetLoaded())
	{
		if (UEditorAssetLibrary::DeleteLoadedAsset(AssetData.GetAsset()))
		{
			return true;
		}
	}
	else
	{
		if (UEditorAssetLibrary::DeleteAsset(AssetPath))
		{
			return true;
		}
	}

	return false;
}

void DHI::CopyCommonFiles(const FDHIData& CharacterSourceData)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> SourceCommonFiles;
	PlatformFile.FindFilesRecursively(SourceCommonFiles, *CharacterSourceData.CommonPath, NULL);

	TArray<FString> ProjectCommonFiles;
	PlatformFile.FindFilesRecursively(ProjectCommonFiles, *CommonPaths::ProjectCommonPath, NULL);

	TArray<FString> ExistingAssetStrippedPaths;

	for (FString ProjectCommonFile : ProjectCommonFiles)
	{
		ExistingAssetStrippedPaths.Add(ProjectCommonFile.Replace(*CommonPaths::ProjectCommonPath, TEXT("")));
	}
	{
		FString CommonCopyMsg = TEXT("Importing Common Assets.");
		FText CommonCopyMsgDialogMessage = FText::FromString(CommonCopyMsg);
		FScopedSlowTask AssetLoadprogress((float)SourceCommonFiles.Num(), CommonCopyMsgDialogMessage, true);
		AssetLoadprogress.MakeDialog();

		float Progress = 1.0f;

		for (FString FileToCopy : SourceCommonFiles)
		{
			FString NormalizedSourceFile = FileToCopy;
			FPaths::NormalizeFilename(NormalizedSourceFile);
			FString StrippedSourcePath = NormalizedSourceFile.Replace(*CommonPaths::SourceCommonPath, TEXT(""));

			if (!ExistingAssetStrippedPaths.Contains(StrippedSourcePath))
			{
				FString CommonFileDestination = FPaths::Combine(CommonPaths::CommonDestinationPath, FileToCopy.Replace(*CharacterSourceData.CommonPath, TEXT("")));

				FString FileDirectory = FPaths::GetPath(CommonFileDestination);

				PlatformFile.CreateDirectoryTree(*FileDirectory);

				AssetLoadprogress.EnterProgressFrame(Progress);

				Progress = 1.0f;

				PlatformFile.CopyFile(*CommonFileDestination, *FileToCopy);
			}
			else
			{
				Progress += 1.0f;
			}
		}
	}
}

bool DHIInLevel(const FString& CharacterBPPath)
{
	FString CharacterPathInLevel = CharacterBPPath + TEXT("_C");
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GEngine->GetWorldContexts()[0].World(), AActor::StaticClass(), FoundActors);

	for (AActor* FoundActor : FoundActors)
	{
		FString ActorPath = FoundActor->GetClass()->GetPathName();

		if (ActorPath.Equals(CharacterPathInLevel))
			return true;
	}

	return false;
}

TArray<FTransform> RemoveDHIActors(const FString& CharacterBPPath, UClass* CharacterClass)
{
	FString CharacterPathInLevel = CharacterBPPath + TEXT("_C");
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GEngine->GetWorldContexts()[0].World(), AActor::StaticClass(), FoundActors);

	UWorld* CurrentWorld = GEngine->GetWorldContexts()[0].World();
	TArray<FTransform> FoundActotsTransforms;
	for (AActor* FoundActor : FoundActors)
	{
		FString ActorPath = FoundActor->GetClass()->GetPathName();

		if (ActorPath.Equals(CharacterPathInLevel))
		{
			CharacterClass = FoundActor->GetClass();
			FTransform FoundActorTransform = FoundActor->GetTransform();
			FoundActotsTransforms.Add(FoundActorTransform);
			CurrentWorld->DestroyActor(FoundActor, true, true);

		}

	}
	return FoundActotsTransforms;

}

void SpawnDHICharacters(TArray<FTransform> SpawnLocations, FAssetData CharacterAssetData, FString CharacterName, UClass* ActorClass)
{
	UWorld* CurrentWorld = GEngine->GetWorldContexts()[0].World();
	UBlueprint* CharacterObject = Cast<UBlueprint>(CharacterAssetData.GetAsset());


	for (FTransform CurrentTransform : SpawnLocations)
	{
		AActor* NewActor = CurrentWorld->SpawnActor(CharacterObject->GeneratedClass, &CurrentTransform);
		NewActor->SetActorLabel(CharacterName);
	}

}