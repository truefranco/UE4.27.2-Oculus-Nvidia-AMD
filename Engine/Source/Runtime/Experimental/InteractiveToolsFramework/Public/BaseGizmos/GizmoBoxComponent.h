// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "BaseGizmos/TransformGizmo.h"
#include "GizmoBoxComponent.generated.h"

/**
 * Simple Component intended to be used as part of 3D Gizmos. 
 * Draws outline of 3D Box based on parameters.
 */
UCLASS(ClassGroup = Utility, HideCategories = (Physics, Collision, Lighting, Rendering, Mobile))
class INTERACTIVETOOLSFRAMEWORK_API UGizmoBoxComponent : public UGizmoBaseComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Options)
	FVector Origin = FVector(0, 0, 0);

	UPROPERTY(EditAnywhere, Category = Options)
	FQuat Rotation = FQuat::Identity;

	UPROPERTY(EditAnywhere, Category = Options)
	FVector Dimensions = FVector(20.0f, 20.0f, 20.0f);

	UPROPERTY(EditAnywhere, Category = Options)
	float LineThickness = 2.0f;

	UPROPERTY(EditAnywhere, Category = Options)
	bool bRemoveHiddenLines = true;

	UPROPERTY(EditAnywhere, Category = Options)
	bool bEnableAxisFlip = true;

	void SetGizmoViewContext(UGizmoViewContext* GizmoViewContextIn)
	{
		GizmoViewContext = GizmoViewContextIn;
		bIsViewDependent = (GizmoViewContext != nullptr);
	}

private:
	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool LineTraceComponent(FHitResult& OutHit, const FVector Start, const FVector End, const FCollisionQueryParams& Params) override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.

	// if true, we drew along -Direction instead of Direction, and so should hit-test accordingly
	bool bFlippedX;
	bool bFlippedY;
	bool bFlippedZ;

	// gizmo visibility
	bool bRenderVisibility = true;

protected:

	// hover state
	bool bHovering = false;

	// world/local coordinates state
	bool bWorld = false;

	UPROPERTY()
	UGizmoViewContext* GizmoViewContext = nullptr;

	// True when GizmoViewContext is not null. Here as a boolean so it
	// can be pointed to by the proxy to see what it should do.
	bool bIsViewDependent = false;

};