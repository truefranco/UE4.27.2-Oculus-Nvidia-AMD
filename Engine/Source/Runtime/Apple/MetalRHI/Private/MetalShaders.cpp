// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalShaders.cpp: Metal shader RHI implementation.
=============================================================================*/

#include "MetalRHIPrivate.h"

#include "Shaders/Debugging/MetalShaderDebugCache.h"
#include "Shaders/MetalCompiledShaderKey.h"
#include "Shaders/MetalCompiledShaderCache.h"
#include "Shaders/MetalShaderLibrary.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "MetalShaderResources.h"
#include "MetalResources.h"
#include "MetalProfiler.h"
#include "MetalCommandBuffer.h"
#include "Serialization/MemoryReader.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeRWLock.h"
#include "Misc/Compression.h"
#include "Misc/MessageDialog.h"

#define SHADERCOMPILERCOMMON_API
#	include "Developer/ShaderCompilerCommon/Public/ShaderCompilerCommon.h"
#undef SHADERCOMPILERCOMMON_API


NSString* DecodeMetalSourceCode(uint32 CodeSize, TArray<uint8> const& CompressedSource)
{
	NSString* GlslCodeNSString = nil;
	if (CodeSize && CompressedSource.Num())
	{
		TArray<ANSICHAR> UncompressedCode;
		UncompressedCode.AddZeroed(CodeSize+1);
		bool bSucceed = FCompression::UncompressMemory(NAME_Zlib, UncompressedCode.GetData(), CodeSize, CompressedSource.GetData(), CompressedSource.Num());
		if (bSucceed)
		{
			GlslCodeNSString = [[NSString stringWithUTF8String:UncompressedCode.GetData()] retain];
		}
	}
	return GlslCodeNSString;
}

mtlpp::LanguageVersion ValidateVersion(uint8 Version)
{
	static uint32 MetalMacOSVersions[][3] = {
		{10,00,6},
		{11,00,6},
		{12,00,6},
		{13,00,0},
		{14,00,0},
	};
	static uint32 MetaliOSVersions[][3] = {
		{13,0,0},
		{14,0,0},
		{15,0,0},
		{16,0,0},
		{17,0,0},
	};
	static TCHAR const* StandardNames[] =
	{
		TEXT("Metal 2.1"),
		TEXT("Metal 2.2"),
		TEXT("Metal 2.3"),
		TEXT("Metal 2.4"),
		TEXT("Metal 3.0"),
	};
	
	mtlpp::LanguageVersion Result = mtlpp::LanguageVersion::Version2_4;
	switch(Version)
	{
		case 4:
			Result = mtlpp::LanguageVersion::Version3_0;
			break;
		case 3:
			Result = mtlpp::LanguageVersion::Version2_4;
			break;
		case 2:
			Result = mtlpp::LanguageVersion::Version2_3;
			break;
		case 1:
			Result = mtlpp::LanguageVersion::Version2_2;
			break;
		case 0:
		default:
#if PLATFORM_MAC
			Result = mtlpp::LanguageVersion::Version2_4;
#else
			Result = mtlpp::LanguageVersion::Version2_4;
#endif
			break;
	}
	
	if (!FApplePlatformMisc::IsOSAtLeastVersion(MetalMacOSVersions[Version], MetaliOSVersions[Version], MetaliOSVersions[Version]))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ShaderVersion"), FText::FromString(FString(StandardNames[Version])));
#if PLATFORM_MAC
		Args.Add(TEXT("RequiredOS"), FText::FromString(FString::Printf(TEXT("macOS %d.%d.%d"), MetalMacOSVersions[Version][0], MetalMacOSVersions[Version][1], MetalMacOSVersions[Version][2])));
#else
		Args.Add(TEXT("RequiredOS"), FText::FromString(FString::Printf(TEXT("%d.%d.%d"), MetaliOSVersions[Version][0], MetaliOSVersions[Version][1], MetaliOSVersions[Version][2])));
#endif
		FText LocalizedMsg = FText::Format(NSLOCTEXT("MetalRHI", "ShaderVersionUnsupported", "The current OS version does not support {ShaderVersion} required by the project. You must upgrade to {RequiredOS} to run this project."),Args);
		
		FText Title = NSLOCTEXT("MetalRHI", "ShaderVersionUnsupportedTitle", "Shader Version Unsupported");
		FMessageDialog::Open(EAppMsgType::Ok, LocalizedMsg, &Title);
		
		FPlatformMisc::RequestExit(true);
	}
	
	return Result;
}
