// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
 EnvironmentDepthRendering.cpp
=============================================================================*/

#include "EnvironmentDepthRendering.h"
#include "MobileBasePassRendering.h"

void SetupEnvironmentDepthUniformParameters(const FViewInfo& View, FEnvironmentDepthUniformParameters& OutParameters)
{
	if (View.EnvironmentDepthTexture != nullptr)
	{
		OutParameters.EnvironmentDepthTexture = View.EnvironmentDepthTexture;
	}
	else
	{
		OutParameters.EnvironmentDepthTexture = GBlackAlpha1VolumeTexture->GetPassthroughRDG();
	}
	OutParameters.EnvironmentDepthSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp, 0, 0, 0, SCF_Less>::GetRHI();
	OutParameters.DepthFactors = View.DepthFactors;
	for (int i = 0; i < 2; ++i)
	{
		OutParameters.ScreenToDepthMatrices[i] = View.ScreenToDepthMatrices[i];
		OutParameters.DepthViewProjMatrices[i] = View.DepthViewProjMatrices[i];
	}
}

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FEnvironmentDepthUniformParameters, "EnvironmentDepthStruct");
