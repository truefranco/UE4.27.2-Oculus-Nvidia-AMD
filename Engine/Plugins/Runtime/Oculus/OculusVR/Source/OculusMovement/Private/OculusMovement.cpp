/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusMovement.h"
#include "OculusMovementModule.h"
#include "OculusHMDPrivate.h"
#include "OculusHMD.h"
#include "OculusPluginWrapper.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "OculusMovement"

namespace XRSpaceFlags
{
	const uint64 XR_SPACE_LOCATION_ORIENTATION_VALID_BIT = 0x00000001;
	const uint64 XR_SPACE_LOCATION_POSITION_VALID_BIT = 0x00000002;
} // namespace XRSpaceFlags

bool OculusMovement::GetBodyState(FOculusBodyState& outOculusBodyState, float WorldToMeters)
{
	static_assert(ovrpBoneId_Body_End == (int)EOculusBoneID::COUNT, "The size of the OVRPlugin Bone ID enum should be the same as the EOculusBoneID enum.");

	checkf(outOculusBodyState.Joints.Num() >= ovrpBoneId_Body_End, TEXT("Not enough joints in FOculusBodyState::Joints array. You must have at least %d joints"), ovrpBoneId_Body_End);

	ovrpBodyState OVRBodyState;
	ovrpResult OVRBodyStateResult = FOculusHMDModule::GetPluginWrapper().GetBodyState(ovrpStep_Render, OVRP_CURRENT_FRAMEINDEX, &OVRBodyState);
	ensureMsgf(OVRBodyStateResult != ovrpFailure_NotYetImplemented, TEXT("Body tracking is not implemented on this platform."));

	if (OVRP_SUCCESS(OVRBodyStateResult))
	{
		outOculusBodyState.IsActive = (OVRBodyState.IsActive == ovrpBool_True);
		outOculusBodyState.Confidence = OVRBodyState.Confidence;
		outOculusBodyState.SkeletonChangedCount = OVRBodyState.SkeletonChangedCount;
		outOculusBodyState.Time = static_cast<float>(OVRBodyState.Time);

		for (int i = 0; i < ovrpBoneId_Body_End; ++i)
		{
			ovrpBodyJointLocation OVRJointLocation = OVRBodyState.JointLocations[i];
			ovrpPosef OVRJointPose = OVRJointLocation.Pose;

			FOculusBodyJoint& OculusBodyJoint = outOculusBodyState.Joints[i];
			OculusBodyJoint.LocationFlags = OVRJointLocation.LocationFlags;
			OculusBodyJoint.bIsValid = OVRJointLocation.LocationFlags & (XRSpaceFlags::XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XRSpaceFlags::XR_SPACE_LOCATION_POSITION_VALID_BIT);
			OculusBodyJoint.Orientation = FRotator(OculusHMD::ToFQuat(OVRJointPose.Orientation));
			OculusBodyJoint.Position = OculusHMD::ToFVector(OVRJointPose.Position) * WorldToMeters;
		}

		return true;
	}

	return false;
}

bool OculusMovement::IsBodyTrackingEnabled()
{
	bool bResult = false;

	ovrpBool IsEnabled = ovrpBool_False;
	ovrpResult TrackingEnabledResult = FOculusHMDModule::GetPluginWrapper().GetBodyTrackingEnabled(&IsEnabled);

	if (OVRP_SUCCESS(TrackingEnabledResult))
	{
		bResult = (IsEnabled == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::IsBodyTrackingSupported()
{
	bool bResult = false;

	ovrpBool IsSupported = ovrpBool_False;
	ovrpResult TrackingSupportedResult = FOculusHMDModule::GetPluginWrapper().GetBodyTrackingSupported(&IsSupported);

	if (OVRP_SUCCESS(TrackingSupportedResult))
	{
		bResult = (IsSupported == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::StartBodyTracking()
{
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StartBodyTracking());
}

bool OculusMovement::StopBodyTracking()
{
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StopBodyTracking());
}

bool OculusMovement::GetFaceState(FOculusFaceState& outOculusFaceState)
{
	static_assert(ovrpFaceExpression_Max == (int)EOculusFaceExpression::COUNT, "The size of the OVRPlugin Face Expression enum should be the same as the EOculusFaceExpression enum.");

	checkf(outOculusFaceState.ExpressionWeightConfidences.Num() >= ovrpFaceConfidence_Max, TEXT("Not enough expression weight confidences in FOculusFaceState::ExpressionWeightConfidences. Requires %d available elements in the array."), ovrpFaceConfidence_Max);
	checkf(outOculusFaceState.ExpressionWeights.Num() >= ovrpFaceExpression_Max, TEXT("Not enough expression weights in FOculusFaceState::ExpressionWeights. Requires %d available elements in the array."), ovrpFaceExpression_Max);

	ovrpFaceState OVRFaceState;
	ovrpResult OVRFaceStateResult = FOculusHMDModule::GetPluginWrapper().GetFaceState(ovrpStep_Render, OVRP_CURRENT_FRAMEINDEX, &OVRFaceState);
	ensureMsgf(OVRFaceStateResult != ovrpFailure_NotYetImplemented, TEXT("Face tracking is not implemented on this platform."));

	if (OVRP_SUCCESS(OVRFaceStateResult))
	{
		outOculusFaceState.bIsValid = (OVRFaceState.Status.IsValid == ovrpBool_True);
		outOculusFaceState.bIsEyeFollowingBlendshapesValid = (OVRFaceState.Status.IsEyeFollowingBlendshapesValid == ovrpBool_True);
		outOculusFaceState.Time = static_cast<float>(OVRFaceState.Time);

		for (int i = 0; i < ovrpFaceExpression_Max; ++i)
		{
			outOculusFaceState.ExpressionWeights[i] = OVRFaceState.ExpressionWeights[i];
		}

		for (int i = 0; i < ovrpFaceConfidence_Max; ++i)
		{
			outOculusFaceState.ExpressionWeightConfidences[i] = OVRFaceState.ExpressionWeightConfidences[i];
		}

		return true;
	}

	return false;
}

bool OculusMovement::IsFaceTrackingEnabled()
{
	bool bResult = false;

	ovrpBool IsEnabled = ovrpBool_False;
	ovrpResult TrackingEnabledResult = FOculusHMDModule::GetPluginWrapper().GetFaceTrackingEnabled(&IsEnabled);

	if (OVRP_SUCCESS(TrackingEnabledResult))
	{
		bResult = (IsEnabled == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::IsFaceTrackingSupported()
{
	bool bResult = false;

	ovrpBool IsSupported = ovrpBool_False;
	ovrpResult TrackingSupportedResult = FOculusHMDModule::GetPluginWrapper().GetFaceTrackingSupported(&IsSupported);

	if (OVRP_SUCCESS(TrackingSupportedResult))
	{
		bResult = (IsSupported == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::StartFaceTracking()
{
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StartFaceTracking());
}

bool OculusMovement::StopFaceTracking()
{

	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StopFaceTracking());
}

bool OculusMovement::GetEyeGazesState(FOculusEyeGazesState& outOculusEyeGazesState, float WorldToMeters)
{
	static_assert(ovrpEye_Count == (int)EOculusEye::COUNT, "The size of the OVRPlugin Eye enum should be the same as the EOculusEye enum.");

	checkf(outOculusEyeGazesState.EyeGazes.Num() >= ovrpEye_Count, TEXT("Not enough eye gaze states in FOculusEyeGazesState::EyeGazes. Requires %d available elements in the array."), ovrpEye_Count);

	ovrpEyeGazesState OVREyeGazesState;
	ovrpResult OVREyeGazesStateResult = FOculusHMDModule::GetPluginWrapper().GetEyeGazesState(ovrpStep_Render, OVRP_CURRENT_FRAMEINDEX, &OVREyeGazesState);
	ensureMsgf(OVREyeGazesStateResult != ovrpFailure_NotYetImplemented, TEXT("Eye tracking is not implemented on this platform."));

	if (OVRP_SUCCESS(OVREyeGazesStateResult))
	{
		outOculusEyeGazesState.Time = static_cast<float>(OVREyeGazesState.Time);
		for (int i = 0; i < ovrpEye_Count; ++i)
		{
			const auto& EyeGazePose = OVREyeGazesState.EyeGazes[i].Pose;
			outOculusEyeGazesState.EyeGazes[i].Orientation = FRotator(OculusHMD::ToFQuat(EyeGazePose.Orientation));
			outOculusEyeGazesState.EyeGazes[i].Position = OculusHMD::ToFVector(EyeGazePose.Position) * WorldToMeters;
			outOculusEyeGazesState.EyeGazes[i].bIsValid = (OVREyeGazesState.EyeGazes[i].IsValid == ovrpBool_True);
			outOculusEyeGazesState.EyeGazes[i].Confidence = OVREyeGazesState.EyeGazes[i].Confidence;
		}

		return true;
	}

	return false;
}

bool OculusMovement::IsEyeTrackingEnabled()
{
	bool bResult = false;

	ovrpBool IsEnabled = ovrpBool_False;
	ovrpResult TrackingEnabledResult = FOculusHMDModule::GetPluginWrapper().GetEyeTrackingEnabled(&IsEnabled);

	if (OVRP_SUCCESS(TrackingEnabledResult))
	{
		bResult = (IsEnabled == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::IsEyeTrackingSupported()
{
	bool bResult = false;

	ovrpBool IsSupported = ovrpBool_False;
	ovrpResult TrackingSupportedResult = FOculusHMDModule::GetPluginWrapper().GetEyeTrackingSupported(&IsSupported);

	if (OVRP_SUCCESS(TrackingSupportedResult))
	{
		bResult = (IsSupported == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::StartEyeTracking()
{
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StartEyeTracking());
}

bool OculusMovement::StopEyeTracking()
{
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StopEyeTracking());
}
