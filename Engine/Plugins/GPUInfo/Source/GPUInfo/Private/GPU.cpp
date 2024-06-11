// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.


#include "GPU.h"

UGPUInfoLib::UGPUInfoLib(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

FString UGPUInfoLib::GetGPUBrandName()
{
	return FWindowsPlatformMisc::GetPrimaryGPUBrand();
}

FString UGPUInfoLib::GetCPUBrandName()
{
	return FWindowsPlatformMisc::GetCPUBrand();
}