// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "MaterialEditorSettings.generated.h"

/**
 * Enumerates offline shader compiler for the material editor
 */
UENUM()
enum class EOfflineShaderCompiler : uint8
{
	Mali UMETA(DisplayName = "Mali Offline Compiler"),
	Adreno UMETA(DisplayName = "Adreno Offline Compiler")
};

UCLASS(config = EditorPerProjectUserSettings)
class MATERIALEDITOR_API UMaterialEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	/**
	Offline shader compiler to use.
	Mali Offline Compiler: Official website address: https://developer.arm.com/products/software-development-tools/graphics-development-tools/mali-offline-compiler/downloads
	Adreno Offline Compiler: Official website address: TODO: add a website here.
	*/
	UPROPERTY(config, EditAnywhere, Category = "Offline Shader Compilers", meta = (DisplayName = "Offline Shader Compiler"))
	EOfflineShaderCompiler OfflineCompiler;

	/**
	Path to user installed shader compiler that can be used by the material editor to compile and extract shader informations for Android platforms.
	*/
	UPROPERTY(config, EditAnywhere, Category = "Offline Shader Compilers", meta = (DisplayName = "Offline Compiler Path"))
	FFilePath OfflineCompilerPath;

	/**
	The GPU target if the offline shader compiler needs one.
	*/
	UPROPERTY(config, EditAnywhere, Category = "Offline Shader Compilers", meta = (DisplayName = "GPU Target"))
	FString GPUTarget;

	/**
	Whether to save offline compiler stats files to Engine\Shaders
	*/
	UPROPERTY(config, EditAnywhere, Category = "Offline Shader Compilers", meta = (DisplayName = "Save Compiler Stats Files"))
	bool bSaveCompilerStatsFiles = false;

protected:
	// The width (in pixels) of the preview viewport when a material editor is first opened
	UPROPERTY(config, EditAnywhere, meta=(ClampMin=1, ClampMax=4096), Category="User Interface Domain")
	int32 DefaultPreviewWidth = 250;

	// The height (in pixels) of the preview viewport when a material editor is first opened
	UPROPERTY(config, EditAnywhere, meta=(ClampMin=1, ClampMax=4096), Category="User Interface Domain")
	int32 DefaultPreviewHeight = 250;

public:
	FIntPoint GetPreviewViewportStartingSize() const
	{
		return FIntPoint(DefaultPreviewWidth, DefaultPreviewHeight);
	}
};