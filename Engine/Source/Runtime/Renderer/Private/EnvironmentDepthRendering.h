// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
EnvironmentDepthRendering.h: environment depth rendering declarations.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "ShaderParameters.h"
#include "UniformBuffer.h"
#include "Matrix3x4.h"
#include "SystemTextures.h"

class FShaderParameterMap;
class FSceneView;
struct FMobileBasePassTextures;
struct FSceneTextures;

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FEnvironmentDepthUniformParameters, )
	SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray, EnvironmentDepthTexture)
	SHADER_PARAMETER_SAMPLER(SamplerState, EnvironmentDepthSampler)
	SHADER_PARAMETER(FVector2D, DepthFactors)
	SHADER_PARAMETER_ARRAY(FMatrix, ScreenToDepthMatrices, [2])
	SHADER_PARAMETER_ARRAY(FMatrix, DepthViewProjMatrices, [2])
END_GLOBAL_SHADER_PARAMETER_STRUCT()

extern void SetupEnvironmentDepthUniformParameters(const FViewInfo& View, FEnvironmentDepthUniformParameters& OutParameters);