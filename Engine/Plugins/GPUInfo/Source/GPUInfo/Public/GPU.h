// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "GPU.generated.h"

UCLASS()
class UGPUInfoLib : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get GPU Name"), Category = "Hardware")
	static FString GetGPUBrandName();

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get CPU Name"), Category = "Hardware")
	static FString GetCPUBrandName();
};