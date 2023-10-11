// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/UnrealType.h"
#include "UObject/ScriptMacros.h"
#include "BlueprintFunctionLibrary.generated.h"


UENUM(BlueprintType)
enum class EOculusEyeBufferSharpenType : uint8
{
	/** No Sharpening */
	SLST_None UMETA(DisplayName = "No Sharpening"),

	/** Normal Sharpening */
	SLST_Normal UMETA(DisplayName = "Normal Sharpening"),

	/** Quality Sharpening */
	SLST_Quality UMETA(DisplayName = "Quality Sharpening"),

	SLST_MAX,
};

// This class is a base class for any function libraries exposed to blueprints.
// Methods in subclasses are expected to be static, and no methods should be added to this base class.
UCLASS(Abstract, MinimalAPI)
class UBlueprintFunctionLibrary : public UObject
{
	GENERATED_UCLASS_BODY()

	// UObject interface
	ENGINE_API virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	// End of UObject interface
};
