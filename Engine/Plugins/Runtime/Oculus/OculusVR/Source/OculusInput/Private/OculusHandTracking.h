// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OculusHMDModule.h"
#include "OculusInput.h"
#include "Engine/SkeletalMesh.h"
#include "Components/CapsuleComponent.h"

#include "OculusInputFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "OculusHandTracking"

DEFINE_LOG_CATEGORY_STATIC(LogOcHandTracking, Log, All);

//-------------------------------------------------------------------------------------------------
// FOculusHandTracking
//-------------------------------------------------------------------------------------------------
namespace OculusInput
{
class FOculusHandTracking
{
public:
	// Oculus Hand Tracking
	static FQuat GetBoneRotation(const int32 ControllerIndex, const EOculusHandType DeviceHand, const EBone BoneId);
	static float GetHandScale(const int32 ControllerIndex, const EOculusHandType DeviceHand);
	static ETrackingConfidence GetTrackingConfidence(const int32 ControllerIndex, const EOculusHandType DeviceHand);
	static ETrackingConfidence GetFingerTrackingConfidence(const int32 ControllerIndex, const EOculusHandType DeviceHand, const EOculusHandAxes Finger); // OCULUS STRIKE
	static FTransform GetPointerPose(const int32 ControllerIndex, const EOculusHandType DeviceHand, const float WorldToMeters = 100.f);
	static bool IsPointerPoseValid(const int32 ControllerIndex, const EOculusHandType DeviceHand);
	static bool GetHandSkeletalMesh(USkeletalMesh* HandSkeletalMesh, const EOculusHandType SkeletonType, const EOculusHandType MeshType, const float WorldToMeters = 100.f);
	static TArray<FOculusCapsuleCollider> InitializeHandPhysics(const EOculusHandType SkeletonType, USkinnedMeshComponent* HandComponent, const float WorldToMeters = 100.f);
	static ETrackingConfidence ToETrackingConfidence(ovrpTrackingConfidence Confidence);
	static bool IsHandTrackingEnabled();
	static bool IsHandDominant(const int32 ControllerIndex, const EOculusHandType DeviceHand);
	static bool IsHandPositionValid(int32 ControllerIndex, EOculusHandType DeviceHand);
	static void SetMultimodalHandsControllersSupported(bool Supported);
	static void SetSimultaneousHandsAndControllersEnabled(bool Enabled);
	static bool GetControllerIsInHand(EOculusHandType DeviceHand);
	static void SetControllerDrivenHandPoses(bool ControllerDrivenHandPose);
	static void SetControllerDrivenHandPosesAreNatural(bool ControllerDrivenHandPosesAreNatural);

	// Helper functions
	static ovrpBoneId ToOvrBone(EBone Bone);
	static FString GetBoneName(uint8 Bone);

	// Converters for converting from ovr bone space (should match up with ovr avatar)
	static FVector OvrBoneVectorToFVector(ovrpVector3f ovrpVector, float WorldToMeters);
	static FQuat OvrBoneQuatToFQuat(ovrpQuatf ovrpQuat);

private:
	// Initializers for runtime hand assets
	static void InitializeHandMesh(USkeletalMesh* SkeletalMesh, const ovrpMesh* OvrMesh, const float WorldToMeters);
	static void InitializeHandSkeleton(USkeletalMesh* SkeletalMesh, const ovrpSkeleton2* OvrSkeleton, const float WorldToMeters);
};


} // namespace OculusInput

#undef LOCTEXT_NAMESPACE
