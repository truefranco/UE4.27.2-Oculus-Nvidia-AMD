// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_CustomPresent.h"
#include "OculusHMDPrivateRHI.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
#include "OculusHMD.h"

#include "VulkanRHIPrivate.h"
#include "VulkanPendingState.h"
#include "VulkanContext.h"

#if PLATFORM_WINDOWS
#ifndef WINDOWS_PLATFORM_TYPES_GUARD
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#endif

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FCustomPresentVulkan
//-------------------------------------------------------------------------------------------------

class FVulkanCustomPresent : public FCustomPresent
{
public:
	FVulkanCustomPresent(FOculusHMD* InOculusHMD);

	// Implementation of FCustomPresent, called by Plugin itself
	virtual bool IsUsingCorrectDisplayAdapter() const override;
	virtual void* GetOvrpInstance() const override;
	virtual void* GetOvrpPhysicalDevice() const override;
	virtual void* GetOvrpDevice() const override;
	virtual void* GetOvrpCommandQueue() const override;
	virtual FTextureRHIRef CreateTexture_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, FClearValueBinding InBinding, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, ERHIResourceType InResourceType, ovrpTextureHandle InTexture, ETextureCreateFlags InTexCreateFlags) override;
	virtual void UpdateFoveationOffsets_RHIThread(bool bUseOffsets, FIntPoint Offsets[2]) override;
};


FVulkanCustomPresent::FVulkanCustomPresent(FOculusHMD* InOculusHMD) :
	FCustomPresent(InOculusHMD, ovrpRenderAPI_Vulkan, PF_R8G8B8A8, true)
{
#if PLATFORM_ANDROID
	if (GRHISupportsRHIThread && GIsThreadedRendering && GUseRHIThread_InternalUseOnly)
	{
		SetRHIThreadEnabled(false, false);
	}
#endif

#if PLATFORM_WINDOWS
/*
	switch (GPixelFormats[PF_DepthStencil].PlatformFormat)
	{
	case VK_FORMAT_D24_UNORM_S8_UINT:
		DefaultDepthOvrpTextureFormat = ovrpTextureFormat_D24_S8;
		break;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		DefaultDepthOvrpTextureFormat = ovrpTextureFormat_D32_S824_FP;
		break;
	default:
		UE_LOG(LogHMD, Error, TEXT("Unrecognized depth buffer format"));
		break;
	}
*/
#endif
	FVulkanDynamicRHI* const DynamicRHI = static_cast<FVulkanDynamicRHI*>(GDynamicRHI);

	bSupportsSubsampled = DynamicRHI->GetDevice()->GetOptionalExtensions().HasEXTFragmentDensityMap2;
}


bool FVulkanCustomPresent::IsUsingCorrectDisplayAdapter() const
{
#if PLATFORM_WINDOWS
	const void* luid;

	if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetDisplayAdapterId2(&luid)) &&
		luid &&
		GVulkanRHI->GetOptionalExtensions().HasKHRGetPhysicalDeviceProperties2)
	{
		const VkPhysicalDeviceIDPropertiesKHR& vkPhysicalDeviceIDProperties = GVulkanRHI->GetDevice()->GetDeviceIdProperties();
		if (vkPhysicalDeviceIDProperties.deviceLUIDValid)
		{
			return !FMemory::Memcmp(luid, &vkPhysicalDeviceIDProperties.deviceLUID, sizeof(LUID));
		}
	}
#endif

	// Not enough information.  Assume that we are using the correct adapter.
	return true;
}


void* FVulkanCustomPresent::GetOvrpInstance() const
{
	return GVulkanRHI->GetInstance();
}


void* FVulkanCustomPresent::GetOvrpPhysicalDevice() const
{
	return GVulkanRHI->GetDevice()->GetPhysicalHandle();
}


void* FVulkanCustomPresent::GetOvrpDevice() const
{
	return GVulkanRHI->GetDevice()->GetInstanceHandle();
}


void* FVulkanCustomPresent::GetOvrpCommandQueue() const
{
	return GVulkanRHI->GetDevice()->GetGraphicsQueue()->GetHandle();
}


FTextureRHIRef FVulkanCustomPresent::CreateTexture_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, FClearValueBinding InBinding, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, ERHIResourceType InResourceType, ovrpTextureHandle InTexture, ETextureCreateFlags InTexCreateFlags)
{
	CheckInRenderThread();

	VkImageSubresourceRange SubresourceRangeAll = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
	FVulkanCommandListContext& ImmediateContext = GVulkanRHI->GetDevice()->GetImmediateContext();
	FVulkanCmdBuffer* CmdBuffer = ImmediateContext.GetCommandBufferManager()->GetActiveCmdBuffer();

	if (InTexCreateFlags & TexCreate_RenderTargetable)
	{
		GVulkanRHI->VulkanSetImageLayout(CmdBuffer->GetHandle(), (VkImage)InTexture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, SubresourceRangeAll);
	}
	else if (InTexCreateFlags & TexCreate_Foveation)
	{
		GVulkanRHI->VulkanSetImageLayout(CmdBuffer->GetHandle(), (VkImage)InTexture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT, SubresourceRangeAll);
	}

	switch (InResourceType)
	{
	case RRT_Texture2D:
		return GVulkanRHI->RHICreateTexture2DFromResource(InFormat, InSizeX, InSizeY, InNumMips, InNumSamples, (VkImage) InTexture, InTexCreateFlags).GetReference();

	case RRT_Texture2DArray:
		return GVulkanRHI->RHICreateTexture2DArrayFromResource(InFormat, InSizeX, InSizeY, 2, InNumMips, InNumSamples, (VkImage) InTexture, InTexCreateFlags).GetReference();

	case RRT_TextureCube:
		return GVulkanRHI->RHICreateTextureCubeFromResource(InFormat, InSizeX, false, 1, InNumMips, (VkImage) InTexture, InTexCreateFlags).GetReference();

	default:
		return nullptr;
	}
}

void FVulkanCustomPresent::UpdateFoveationOffsets_RHIThread(bool bUseOffsets, FIntPoint Offsets[2])
{
	CheckInRHIThread();

	SCOPED_NAMED_EVENT(UpdateFoveationOffsets_RHIThread, FColor::Red);

	GVulkanRHI->VulkanSetQcomFragmentDensityMapOffsets(bUseOffsets, Offsets);
}

//-------------------------------------------------------------------------------------------------
// APIs
//-------------------------------------------------------------------------------------------------

FCustomPresent* CreateCustomPresent_Vulkan(FOculusHMD* InOculusHMD)
{
	return new FVulkanCustomPresent(InOculusHMD);
}


} // namespace OculusHMD

#if PLATFORM_WINDOWS
#undef WINDOWS_PLATFORM_TYPES_GUARD
#endif

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
