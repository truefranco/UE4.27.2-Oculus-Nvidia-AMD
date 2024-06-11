// Copyright Epic Games, Inc.All Rights Reserved.

/*=============================================================================
StereoRenderTargetManager.h: Abstract interface returned from IStereoRendering to support rendering into a texture
=============================================================================*/

#pragma once
#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"

/** 
 * The IStereoRenderTargetManager can be returned from IStereoRendering::GetRenderTargetManager() implementations.
 * Implement this interface if a stereo rendering device requires all output to be rendered into separate render targets and/or to customize how
 * separate render targets are allocated.
 */
class IStereoRenderTargetManager
{
public:
	/** 
	 * Whether a separate render target should be used or not.
	 * In case the stereo rendering implementation does not require special handling of separate render targets 
	 * at all, it can leave out implementing this interface completely and simply let the default implementation 
	 * of IStereoRendering::GetRenderTargetManager() return nullptr.
	 */
	virtual bool ShouldUseSeparateRenderTarget() const = 0;

	/**
	 * Updates viewport for direct rendering of distortion. Should be called on a game thread.
	 *
	 * @param bUseSeparateRenderTarget	Set to true if a separate render target will be used. Can potentiallt be true even if ShouldUseSeparateRenderTarget() returned false earlier.
	 * @param Viewport					The Viewport instance calling this method.
	 * @param ViewportWidget			(optional) The Viewport widget containing the view. Can be used to access SWindow object.
	 */
	virtual void UpdateViewport(bool bUseSeparateRenderTarget, const class FViewport& Viewport, class SViewport* ViewportWidget = nullptr) = 0;

	/**
	 * Calculates dimensions of the render target texture for direct rendering of distortion.
	 */
	virtual void CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY) = 0;

	/**
	 * Returns true, if render target texture must be re-calculated.
	 */
	virtual bool NeedReAllocateViewportRenderTarget(const class FViewport& Viewport) = 0;

	/**
	* Returns true, if render target texture must be re-calculated.
	*/
	virtual bool NeedReAllocateDepthTexture(const TRefCountPtr<struct IPooledRenderTarget>& DepthTarget) { return false; }

	/**
	* Returns true, if shading rate texture must be re-calculated.
	*/
	virtual bool NeedReAllocateShadingRateTexture(const TRefCountPtr<struct IPooledRenderTarget>& ShadingRateTarget) { return false; }

	// AppSpaceWarp
	/**
	* Returns true, if motion vector texture must be re-calculated.
	*/
	virtual bool NeedReAllocateMotionVectorTexture(const TRefCountPtr<struct IPooledRenderTarget>& MotionVectorTarget, const TRefCountPtr<struct IPooledRenderTarget>& MotionVectorDepthTarget) { return false; }

	/**
	 * Returns number of required buffered frames.
	 */
	virtual uint32 GetNumberOfBufferedFrames() const { return 1; }

	/**
	 * Allocates a render target texture.
	 * The default implementation always return false to indicate that the default texture allocation should be used instead.
	 *
	 * @param Index			(in) index of the buffer, changing from 0 to GetNumberOfBufferedFrames()
	 * @return				true, if texture was allocated; false, if the default texture allocation should be used.
	 */
	virtual bool AllocateRenderTargetTexture(uint32 Index, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, ETextureCreateFlags Flags, ETextureCreateFlags TargetableTextureFlags, FTexture2DRHIRef& OutTargetableTexture, FTexture2DRHIRef& OutShaderResourceTexture, uint32 NumSamples = 1) { return false; }

	/**
	 * Returns pixel format that the device created its swapchain with (which can be different than what was requested in AllocateRenderTargetTexture)
	 */
	virtual EPixelFormat GetActualColorSwapchainFormat() const { return PF_Unknown; }

	/**
	 * Acquires the next available color texture.
	 *
	 * @return				the index of the texture in the array returned by AllocateRenderTargetTexture.
	 */
	virtual int32 AcquireColorTexture() { return -1; }

	/**
	 * Allocates a depth texture.
	 *
	 * @param Index			(in) index of the buffer, changing from 0 to GetNumberOfBufferedFrames()
	 * @return				true, if texture was allocated; false, if the default texture allocation should be used.
	 */
	virtual bool AllocateDepthTexture(uint32 Index, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, ETextureCreateFlags Flags, ETextureCreateFlags TargetableTextureFlags, FTexture2DRHIRef& OutTargetableTexture, FTexture2DRHIRef& OutShaderResourceTexture, uint32 NumSamples = 1) { return false; }

	/**
	 * Allocates a shading rate texture.
	 * The default implementation always returns false to indicate that the default texture allocation should be used instead.
	 *
	 * @param Index			(in) index of the buffer, changing from 0 to GetNumberOfBufferedFrames()
	 * @return				true, if texture was allocated; false, if the default texture allocation should be used.
	 */
	virtual bool AllocateShadingRateTexture(uint32 Index, uint32 RenderSizeX, uint32 RenderSizeY, uint8 Format, uint32 NumMips, ETextureCreateFlags Flags, ETextureCreateFlags TargetableTextureFlags, FTexture2DRHIRef& OutTexture, FIntPoint& OutTextureSize) { return false; }

	// AppSpaceWarp
	/**
	 * Allocates a motion vector texture and motion vector depth texture.
	 * The default implementation always returns false to indicate that the default texture allocation should be used instead.
	 *
	 * @param Index			(in) index of the buffer, changing from 0 to GetNumberOfBufferedFrames()
	 * @return				true, if texture was allocated; false, if the default texture allocation should be used.
	 */
	virtual bool AllocateMotionVectorTexture(uint32 Index, uint8 Format, uint32 NumMips, uint32 InTexFlags, uint32 InTargetableTextureFlags, FTexture2DRHIRef& OutTexture, FIntPoint& OutTextureSize, FTexture2DRHIRef& OutDepthTexture, FIntPoint& OutDepthTextureSize) { return false; }

	// BEGIN META SECTION - XR Soft Occlusions
	/**
	 * Find the environment depth texture.
	 * The default implementation always returns false to indicate that the texture is not available.
	 *
	 * @param OutTexture				(out) The environment depth texture
	 * @param OutDepthFactors			(out) The depth factors used to transform Z coordinates
	 * @param OutScreenToDepthMatrices	(out) The screen to depth matrices used to transform UV coordinates
	 * @return							true, if texture was found; false, if the texture was not available.
	 */
	virtual bool FindEnvironmentDepthTexture_RenderThread(FTextureRHIRef& OutTexture, FVector2D& OutDepthFactors, FMatrix OutScreenToDepthMatrices[2], FMatrix OutDepthViewProjMatrices[2]) { return false; }
	// END META SECTION - XR Soft Occlusions

	static EPixelFormat GetStereoLayerPixelFormat() { return PF_B8G8R8A8; }

};