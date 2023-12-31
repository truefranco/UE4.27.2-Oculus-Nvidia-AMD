// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalDynamicRHI_Shaders.cpp: Metal Dynamic RHI Class Shader Methods.
=============================================================================*/


#include "MetalRHIPrivate.h"
#include "MetalShaderTypes.h"
#include "Shaders/MetalShaderLibrary.h"


//------------------------------------------------------------------------------

#pragma mark - Metal Dynamic RHI Shader Methods


FVertexShaderRHIRef FMetalDynamicRHI::RHICreateVertexShader(TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	@autoreleasepool {
		FMetalVertexShader* Shader = new FMetalVertexShader(Code);
		return Shader;
	}
}

FPixelShaderRHIRef FMetalDynamicRHI::RHICreatePixelShader(TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	@autoreleasepool {
		FMetalPixelShader* Shader = new FMetalPixelShader(Code);
		return Shader;
	}
}

FHullShaderRHIRef FMetalDynamicRHI::RHICreateHullShader(TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	@autoreleasepool {
#if PLATFORM_SUPPORTS_TESSELLATION_SHADERS
		FMetalHullShader* Shader = new FMetalHullShader(Code);
#else
		FMetalHullShader* Shader = new FMetalHullShader;
		FMetalCodeHeader Header;
		Shader->Init(Code, Header);
#endif // PLATFORM_SUPPORTS_TESSELLATION_SHADERS
		return Shader;
	}
}

FDomainShaderRHIRef FMetalDynamicRHI::RHICreateDomainShader(TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	@autoreleasepool {
#if PLATFORM_SUPPORTS_TESSELLATION_SHADERS
		FMetalDomainShader* Shader = new FMetalDomainShader(Code);
#else
		FMetalDomainShader* Shader = new FMetalDomainShader;
		FMetalCodeHeader Header;
		Shader->Init(Code, Header);
#endif // PLATFORM_SUPPORTS_TESSELLATION_SHADERS
		return Shader;
	}
}

FGeometryShaderRHIRef FMetalDynamicRHI::RHICreateGeometryShader(TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	@autoreleasepool {
		FMetalGeometryShader* Shader = new FMetalGeometryShader;
		FMetalCodeHeader Header;
		Shader->Init(Code, Header);
		return Shader;
	}
}

FComputeShaderRHIRef FMetalDynamicRHI::RHICreateComputeShader(TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	@autoreleasepool {
		return new FMetalComputeShader(Code);
	}
}

FRHIShaderLibraryRef FMetalDynamicRHI::RHICreateShaderLibrary(EShaderPlatform Platform, FString const& FilePath, FString const& Name)
{
	FString METAL_MAP_EXTENSION(TEXT(".metalmap"));

	@autoreleasepool {
		FRHIShaderLibraryRef Result = nullptr;

		FName PlatformName = LegacyShaderPlatformToShaderFormat(Platform);
		FString LibName = FString::Printf(TEXT("%s_%s"), *Name, *PlatformName.GetPlainNameString());
		LibName.ToLowerInline();

		FString BinaryShaderFile = FilePath / LibName + METAL_MAP_EXTENSION;

		if (IFileManager::Get().FileExists(*BinaryShaderFile) == false)
		{
			// the metal map files are stored in UFS file system
			// for pak files this means they might be stored in a different location as the pak files will mount them to the project content directory
			// the metal libraries are stores non UFS and could be anywhere on the file system.
			// if we don't find the metalmap file straight away try the pak file path
			BinaryShaderFile = FPaths::ProjectContentDir() / LibName + METAL_MAP_EXTENSION;
		}

		FScopeLock Lock(&FMetalShaderLibrary::LoadedShaderLibraryMutex);

		FRHIShaderLibrary** FoundShaderLibrary = FMetalShaderLibrary::LoadedShaderLibraryMap.Find(BinaryShaderFile);
		if (FoundShaderLibrary)
		{
			return *FoundShaderLibrary;
		}

		FArchive* BinaryShaderAr = IFileManager::Get().CreateFileReader(*BinaryShaderFile);

		if( BinaryShaderAr != NULL )
		{
			FMetalShaderLibraryHeader Header;
			FSerializedShaderArchive SerializedShaders;
			TArray<uint8> ShaderCode;

			*BinaryShaderAr << Header;
			*BinaryShaderAr << SerializedShaders;
			*BinaryShaderAr << ShaderCode;
			BinaryShaderAr->Flush();
			delete BinaryShaderAr;

			// Would be good to check the language version of the library with the archive format here.
			if (Header.Format == PlatformName.GetPlainNameString())
			{
				check(((SerializedShaders.GetNumShaders() + Header.NumShadersPerLibrary - 1) / Header.NumShadersPerLibrary) == Header.NumLibraries);

				TArray<mtlpp::Library> Libraries;
				Libraries.Empty(Header.NumLibraries);

				for (uint32 i = 0; i < Header.NumLibraries; i++)
				{
					FString MetalLibraryFilePath = (FilePath / LibName) + FString::Printf(TEXT(".%d.metallib"), i);
					MetalLibraryFilePath = FPaths::ConvertRelativePathToFull(MetalLibraryFilePath);
#if !PLATFORM_MAC
					MetalLibraryFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*MetalLibraryFilePath);
#endif // !PLATFORM_MAC

					METAL_GPUPROFILE(FScopedMetalCPUStats CPUStat(FString::Printf(TEXT("NewLibraryFile: %s"), *MetalLibraryFilePath)));
					NSError* Error;
PRAGMA_DISABLE_DEPRECATION_WARNINGS
					mtlpp::Library Library = [GetMetalDeviceContext().GetDevice() newLibraryWithFile:MetalLibraryFilePath.GetNSString() error:&Error];
PRAGMA_ENABLE_DEPRECATION_WARNINGS
					if (Library != nil)
					{
						Libraries.Add(Library);
					}
					else
					{
						UE_LOG(LogMetal, Display, TEXT("Failed to create library: %s"), *FString(Error.description));
						return nullptr;
					}
				}

				Result = new FMetalShaderLibrary(Platform, Name, BinaryShaderFile, Header, SerializedShaders, ShaderCode, Libraries);
				FMetalShaderLibrary::LoadedShaderLibraryMap.Add(BinaryShaderFile, Result.GetReference());
			}
			//else
			//{
			//	UE_LOG(LogMetal, Display, TEXT("Wrong shader platform wanted: %s, got: %s"), *LibName, *Map.Format);
			//}
		}
		else
		{
			UE_LOG(LogMetal, Display, TEXT("No .metalmap file found for %s!"), *LibName);
		}

		return Result;
	}
}

FBoundShaderStateRHIRef FMetalDynamicRHI::RHICreateBoundShaderState(
	FRHIVertexDeclaration* VertexDeclarationRHI,
	FRHIVertexShader* VertexShaderRHI,
	FRHIHullShader* HullShaderRHI,
	FRHIDomainShader* DomainShaderRHI,
	FRHIPixelShader* PixelShaderRHI,
	FRHIGeometryShader* GeometryShaderRHI)
{
	NOT_SUPPORTED("RHICreateBoundShaderState");
	return nullptr;
}

FVertexShaderRHIRef FMetalDynamicRHI::CreateVertexShader_RenderThread(class FRHICommandListImmediate& RHICmdList, TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	return RHICreateVertexShader(Code, Hash);
}

FHullShaderRHIRef FMetalDynamicRHI::CreateHullShader_RenderThread(class FRHICommandListImmediate& RHICmdList, TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	return RHICreateHullShader(Code, Hash);
}

FDomainShaderRHIRef FMetalDynamicRHI::CreateDomainShader_RenderThread(class FRHICommandListImmediate& RHICmdList, TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	return RHICreateDomainShader(Code, Hash);
}

FGeometryShaderRHIRef FMetalDynamicRHI::CreateGeometryShader_RenderThread(class FRHICommandListImmediate& RHICmdList, TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	return RHICreateGeometryShader(Code, Hash);
}

FPixelShaderRHIRef FMetalDynamicRHI::CreatePixelShader_RenderThread(class FRHICommandListImmediate& RHICmdList, TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	return RHICreatePixelShader(Code, Hash);
}

FComputeShaderRHIRef FMetalDynamicRHI::CreateComputeShader_RenderThread(class FRHICommandListImmediate& RHICmdList, TArrayView<const uint8> Code, const FSHAHash& Hash)
{
	return RHICreateComputeShader(Code, Hash);
}

FRHIShaderLibraryRef FMetalDynamicRHI::RHICreateShaderLibrary_RenderThread(class FRHICommandListImmediate& RHICmdList, EShaderPlatform Platform, FString FilePath, FString Name)
{
	return RHICreateShaderLibrary(Platform, FilePath, Name);
}
