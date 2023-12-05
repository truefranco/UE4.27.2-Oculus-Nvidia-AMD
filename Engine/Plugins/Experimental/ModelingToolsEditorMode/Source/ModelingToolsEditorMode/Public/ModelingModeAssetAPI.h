// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tools/EditorToolAssetAPI.h"

/**
 * Implementation of ToolsContext Asset management API that is suitable for use
 * inside Modeling Tools Mode. 
 */
class MODELINGTOOLSEDITORMODE_API FModelingModeAssetAPI : public FEditorToolAssetAPI
{
public:
	/**
	 * Generate static mesh actor
	 */
	virtual AActor* GenerateStaticMeshActor(
		UWorld* TargetWorld,
		FTransform Transform,
		FString ObjectBaseName,
		FGeneratedStaticMeshAssetConfig&& AssetConfig) override;

	/**
	 * Save generated UTexture2D that is assumed to currently be in the Transient package
	 */
	virtual bool SaveGeneratedTexture2D(
		UTexture2D* GeneratedTexture,
		FString ObjectBaseName,
		const UObject* RelativeToAsset) override;

	/**
	 * Determines path and name for a new Actor/Asset based on current mode settings, etc
	 */
	virtual bool GetNewActorPackagePath(
		UWorld* TargetWorld,
		FString ObjectBaseName,
		const FGeneratedStaticMeshAssetConfig& AssetConfig,
		FString& PackageFolderPathOut,
		FString& ObjectBaseNameOut
		);
};

namespace UE
{
	namespace Modeling
	{
		/**
		 * Determines path and name for a new Asset based on current mode settings, etc
		 * @param BaseName desired initial string for the name
		 * @param TargetWorld World the new Asset will be used in, which can determine its generated path based on various settings
		 * @param SuggestedFolder a suggested path for the asset, ignored if empty
		 * @return Path+Name for the new Asset (package)
		 */
		MODELINGTOOLSEDITORMODE_API FString GetNewAssetPathName(const FString& BaseName, const UWorld* TargetWorld, FString SuggestedFolder);


		/*
		*Determines the global asset root path.
		* @return Global asset root path.
		*/
		MODELINGTOOLSEDITORMODE_API FString GetGlobalAssetRootPath();

		/**
 * Determines the world relative asset root path.
 * @param World World the new Asset will be used in; if this is a nullptr then it will be set using the current world context.
 * @return Asset root path relative to the world.
 */
		MODELINGTOOLSEDITORMODE_API FString GetWorldRelativeAssetRootPath(const UWorld* World);

		/**
 * Utility function that may auto-save a newly created asset depending on current mode settings,
 * and posts notifications for the Editor
 * @param Asset the new asset object (eg UStaticMesh, UTexture2D, etc)
 */
		MODELINGTOOLSEDITORMODE_API void OnNewAssetCreated(UObject* Asset);

	}
}
