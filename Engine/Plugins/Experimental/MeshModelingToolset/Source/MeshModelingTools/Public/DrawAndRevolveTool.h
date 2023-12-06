// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "ModelingOperators.h" //IDynamicMeshOperatorFactory
#include "InteractiveTool.h" //UInteractiveToolPropertySet
#include "InteractiveToolBuilder.h" //UInteractiveToolBuilder
#include "InteractiveToolChange.h" //FToolCommandChange
#include "MeshOpPreviewHelpers.h" //FDynamicMeshOpResult
#include "Properties/MeshMaterialProperties.h"
#include "PropertySets/CreateMeshObjectTypeProperties.h"
#include "Properties/RevolveProperties.h"

#include "DrawAndRevolveTool.generated.h"

class UCollectSurfacePathMechanic;
class UConstructionPlaneMechanic;
class UCurveControlPointsMechanic;
class FCurveSweepOp;

UCLASS()
class MESHMODELINGTOOLS_API UDrawAndRevolveToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:

	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};


UCLASS()
class MESHMODELINGTOOLS_API URevolveToolProperties : public URevolveProperties
{
	GENERATED_BODY()

public:

	/** Determines how end caps are created. This is not relevant if the end caps are not visible or if the path is not closed. */
	UPROPERTY(EditAnywhere, Category = Revolve, AdvancedDisplay, meta = (DisplayAfter = "QuadSplitMode",
		EditCondition = "HeightOffsetPerDegree != 0 || RevolveDegrees != 360", ValidEnumValues = "None, CenterFan, Delaunay"))
	ERevolvePropertiesCapFillMode CapFillMode = ERevolvePropertiesCapFillMode::Delaunay;

	/** Connect the ends of an open path to the axis to add caps to the top and bottom of the revolved result.
	  * This is not relevant for paths that are already closed. */
	UPROPERTY(EditAnywhere, Category = Revolve, AdvancedDisplay)
	bool bClosePathToAxis = true;
	
	/** Determines whether plane control widget snaps to world grid (only relevant if world coordinate mode is active in viewport) .*/
	UPROPERTY(EditAnywhere, Category = DrawPlane)
	bool bSnapToWorldGrid = false;

	/** Sets the draw plane origin. The revolution axis is the X axis in the plane. */
	UPROPERTY(EditAnywhere, Category = DrawPlane, meta = (DisplayName = "Origin", EditCondition = "bAllowedToEditDrawPlane", HideEditConditionToggle,
		Delta = 5, LinearDeltaSensitivity = 1))
	FVector DrawPlaneOrigin = FVector(0, 0, 0);

	/** Sets the draw plane orientation. The revolution axis is the X axis in the plane. */
	UPROPERTY(EditAnywhere, Category = DrawPlane, meta = (
		EditCondition = "bAllowedToEditDrawPlane", HideEditConditionToggle, 
		UIMin = -180, UIMax = 180, ClampMin = -180000, ClampMax = 18000))
	FRotator DrawPlaneOrientation = FRotator(90, 0, 0);

	/** Enables/disables snapping while editing the profile curve. */
	UPROPERTY(EditAnywhere, Category = ProfileCurve)
	bool bEnableSnapping = true;

	// Not user visible- used to disallow draw plane modification.
	UPROPERTY(meta = (TransientToolProperty))
	bool bAllowedToEditDrawPlane = true;

protected:
	virtual ERevolvePropertiesCapFillMode GetCapFillMode() const override
	{
		return CapFillMode;
	}
};

UCLASS()
class MESHMODELINGTOOLS_API URevolveOperatorFactory : public UObject, public IDynamicMeshOperatorFactory
{
	GENERATED_BODY()

public:
	// IDynamicMeshOperatorFactory API
	virtual TUniquePtr<FDynamicMeshOperator> MakeNewOperator() override;

	UPROPERTY()
	UDrawAndRevolveTool* RevolveTool;
};


/** Draws a profile curve and revolves it around an axis. */
UCLASS()
class MESHMODELINGTOOLS_API UDrawAndRevolveTool : public UInteractiveTool
{
	GENERATED_BODY()

public:
	virtual void SetWorld(UWorld* World) { TargetWorld = World; }
	virtual void SetAssetAPI(IToolsContextAssetAPI* NewAssetApi) { AssetAPI = NewAssetApi; }

	virtual void RegisterActions(FInteractiveToolActionSet& ActionSet) override;
	void OnBackspacePress();

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

protected:

	UWorld* TargetWorld;
	IToolsContextAssetAPI* AssetAPI;

	FViewCameraState CameraState;

	// This information is replicated in the user-editable transform in the settings and in the PlaneMechanic
	// plane, but the tool turned out to be much easier to write and edit with this decoupling.
	FVector3d RevolutionAxisOrigin;
	FVector3d RevolutionAxisDirection;
	
	bool bProfileCurveComplete = false;

	void UpdateRevolutionAxis();

	UPROPERTY()
	UCurveControlPointsMechanic* ControlPointsMechanic = nullptr;

	UPROPERTY()
	UConstructionPlaneMechanic* PlaneMechanic = nullptr;

	/** Property set for type of output object (StaticMesh, Volume, etc) */
	UPROPERTY()
	UCreateMeshObjectTypeProperties* OutputTypeProperties;

	UPROPERTY()
	URevolveToolProperties* Settings = nullptr;

	UPROPERTY()
	UNewMeshMaterialProperties* MaterialProperties;

	UPROPERTY()
	UMeshOpPreviewWithBackgroundCompute* Preview = nullptr;

	void StartPreview();

	void GenerateAsset(const FDynamicMeshOpResult& Result);

	friend class URevolveOperatorFactory;

private:
	constexpr static double FarDrawPlaneThreshold = 100 * 100;
	bool bHasFarPlaneWarning = false;
};
