// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SceneViewExtension.h"

class FMobileFSRViewExtension final : public FSceneViewExtensionBase
{
public:
	FMobileFSRViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {}

	void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override {}
	void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}

	MOBILEFSR_API void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
};