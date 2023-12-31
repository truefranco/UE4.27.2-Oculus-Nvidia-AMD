// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UObject/ObjectMacros.h"
#include "Engine/SceneCapture2D.h"
#include "AudioResampler.h"
#include "OculusPluginWrapper.h"

#include "OculusMR_CastingCameraActor.generated.h"

#if PLATFORM_ANDROID
#define MRC_SWAPCHAIN_LENGTH 3
#endif

class UOculusMR_PlaneMeshComponent;
class UMaterial;
class AOculusMR_BoundaryActor;
class UTextureRenderTarget2D;
class UOculusMR_Settings;
class UOculusMR_State;

/**
* The camera actor in the level that tracks the binded physical camera in game
*/
UCLASS(ClassGroup = OculusMR, NotPlaceable, NotBlueprintable)
class AOculusMR_CastingCameraActor : public ASceneCapture2D
{
	GENERATED_BODY()

public:
	AOculusMR_CastingCameraActor(const FObjectInitializer& ObjectInitializer);

	/** Initialize the MRC settings and states */
	void InitializeStates(UOculusMR_Settings* MRSettingsIn, UOculusMR_State* MRStateIn);

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaTime) override;

	virtual void BeginDestroy() override;

	UPROPERTY()
	class UVRNotificationsComponent* VRNotificationComponent;

	UPROPERTY()
	UOculusMR_PlaneMeshComponent* PlaneMeshComponent;

	UPROPERTY()
	UMaterial* OpaqueColoredMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* BackdropMaterialInstance;

	UPROPERTY()
	class UTexture2D* DefaultTexture_White;

	bool TrackedCameraCalibrationRequired;
	bool HasTrackedCameraCalibrationCalibrated;
	FQuat InitialCameraAbsoluteOrientation;
	FVector InitialCameraAbsolutePosition;
	FQuat InitialCameraRelativeOrientation;
	FVector InitialCameraRelativePosition;

	int32 RefreshBoundaryMeshCounter;

private:

	/** Move the casting camera to follow the tracking reference (i.e. player) */
	void RequestTrackedCameraCalibration();

	bool RefreshExternalCamera();

	void CalibrateTrackedCameraPose();
	void SetTrackedCameraUserPoseWithCameraTransform();
	void SetTrackedCameraInitialPoseWithPlayerTransform();
	void UpdateTrackedCameraPosition();

	/** Initialize the tracked physical camera */
	void SetupTrackedCamera();

	/** Close the tracked physical camera */
	void CloseTrackedCamera();

	void OnHMDRecentered();

	const FColor& GetForegroundLayerBackgroundColor() const { return ForegroundLayerBackgroundColor; }

	void SetBackdropMaterialColor();
	void SetupBackdropMaterialInstance();
	void RepositionPlaneMesh();
	void RefreshBoundaryMesh();
	void UpdateRenderTargetSize();
	void SetupMRCScreen();
	void CloseMRCScreen();

	void Execute_BindToTrackedCameraIndexIfAvailable();

	FColor ForegroundLayerBackgroundColor;
	float ForegroundMaxDistance;

	UPROPERTY()
	TArray<UTextureRenderTarget2D*> BackgroundRenderTargets;

	UPROPERTY()
	ASceneCapture2D* ForegroundCaptureActor;

	UPROPERTY()
	TArray<UTextureRenderTarget2D*> ForegroundRenderTargets;

	UPROPERTY()
	TArray<double> PoseTimes;

	UPROPERTY()
	UOculusMR_Settings* MRSettings;

	UPROPERTY()
	UOculusMR_State* MRState;

#if PLATFORM_ANDROID
	TArray<Audio::AlignedFloatBuffer> AudioBuffers;
	TArray<double> AudioTimes;

	int SyncId;

	const unsigned int NumRTs = MRC_SWAPCHAIN_LENGTH;
	unsigned int RenderedRTs;
	unsigned int CaptureIndex;
#endif
};
