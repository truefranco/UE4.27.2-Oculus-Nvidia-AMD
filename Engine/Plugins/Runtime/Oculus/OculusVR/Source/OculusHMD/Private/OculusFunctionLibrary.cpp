// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusFunctionLibrary.h"
#include "OculusHMDPrivate.h"
#include "OculusHMD.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "OculusFunctionLibrary"

//-------------------------------------------------------------------------------------------------
// UOculusFunctionLibrary
//-------------------------------------------------------------------------------------------------

UOculusFunctionLibrary::UOculusFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

OculusHMD::FOculusHMD* UOculusFunctionLibrary::GetOculusHMD()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	if (GEngine && GEngine->XRSystem.IsValid())
	{
		if (GEngine->XRSystem->GetSystemName() == OculusHMD::FOculusHMD::OculusSystemName)
		{
			return static_cast<OculusHMD::FOculusHMD*>(GEngine->XRSystem.Get());
		}
	}
#endif
	return nullptr;
}

void UOculusFunctionLibrary::GetPose(FRotator& DeviceRotation, FVector& DevicePosition, FVector& NeckPosition, bool bUseOrienationForPlayerCamera, bool bUsePositionForPlayerCamera, const FVector PositionScale)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD && OculusHMD->IsHeadTrackingAllowed())
	{
		FQuat HeadOrientation = FQuat::Identity;
		FVector HeadPosition = FVector::ZeroVector;

		OculusHMD->GetCurrentPose(OculusHMD->HMDDeviceId, HeadOrientation, HeadPosition);

		DeviceRotation = HeadOrientation.Rotator();
		DevicePosition = HeadPosition;
		NeckPosition = OculusHMD->GetNeckPosition(HeadOrientation, HeadPosition);
	}
	else
#endif // #if OCULUS_HMD_SUPPORTED_PLATFORMS
	{
		DeviceRotation = FRotator::ZeroRotator;
		DevicePosition = FVector::ZeroVector;
		NeckPosition = FVector::ZeroVector;
	}
}

void UOculusFunctionLibrary::SetBaseRotationAndBaseOffsetInMeters(FRotator Rotation, FVector BaseOffsetInMeters, EOrientPositionSelector::Type Options)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		if ((Options == EOrientPositionSelector::Orientation) || (Options == EOrientPositionSelector::OrientationAndPosition))
		{
			OculusHMD->SetBaseRotation(Rotation);
		}
		if ((Options == EOrientPositionSelector::Position) || (Options == EOrientPositionSelector::OrientationAndPosition))
		{
			OculusHMD->SetBaseOffsetInMeters(BaseOffsetInMeters);
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

void UOculusFunctionLibrary::GetBaseRotationAndBaseOffsetInMeters(FRotator& OutRotation, FVector& OutBaseOffsetInMeters)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		OutRotation = OculusHMD->GetBaseRotation();
		OutBaseOffsetInMeters = OculusHMD->GetBaseOffsetInMeters();
	}
	else
	{
		OutRotation = FRotator::ZeroRotator;
		OutBaseOffsetInMeters = FVector::ZeroVector;
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

void UOculusFunctionLibrary::GetRawSensorData(FVector& AngularAcceleration, FVector& LinearAcceleration, FVector& AngularVelocity, FVector& LinearVelocity, float& TimeInSeconds, ETrackedDeviceType DeviceType)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr && OculusHMD->IsHMDActive())
	{
		ovrpPoseStatef state;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetNodePoseState3(ovrpStep_Render, OVRP_CURRENT_FRAMEINDEX, OculusHMD::ToOvrpNode(DeviceType), &state)))
		{
			AngularAcceleration = OculusHMD::ToFVector(state.AngularAcceleration);
			LinearAcceleration = OculusHMD::ToFVector(state.Acceleration);
			AngularVelocity = OculusHMD::ToFVector(state.AngularVelocity);
			LinearVelocity = OculusHMD::ToFVector(state.Velocity);
			TimeInSeconds = state.Time;
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

bool UOculusFunctionLibrary::IsDeviceTracked(ETrackedDeviceType DeviceType)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr && OculusHMD->IsHMDActive())
	{
		ovrpBool Present;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetNodePresent2(OculusHMD::ToOvrpNode(DeviceType), &Present)))
		{
			return Present != ovrpBool_False;
		}
		else
		{
			return false;
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	return false;
}

void UOculusFunctionLibrary::GetSuggestedCpuAndGpuPerformanceLevels(EProcessorPerformanceLevel& CpuPerfLevel, EProcessorPerformanceLevel& GpuPerfLevel)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr && OculusHMD->IsHMDActive())
	{
		OculusHMD->GetSuggestedCpuAndGpuPerformanceLevels(CpuPerfLevel, GpuPerfLevel);
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

void UOculusFunctionLibrary::SetSuggestedCpuAndGpuPerformanceLevels(EProcessorPerformanceLevel CpuPerfLevel, EProcessorPerformanceLevel GpuPerfLevel)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr && OculusHMD->IsHMDActive())
	{
		OculusHMD->SetSuggestedCpuAndGpuPerformanceLevels(CpuPerfLevel, GpuPerfLevel);
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

void UOculusFunctionLibrary::SetCPUAndGPULevels(int CPULevel, int GPULevel)
{
	// Deprecated. Please use Get/SetSuggestedCpuAndGpuPerformanceLevels instead.
}

bool UOculusFunctionLibrary::GetUserProfile(FHmdUserProfile& Profile)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		OculusHMD::FOculusHMD::UserProfile Data;
		if (OculusHMD->GetUserProfile(Data))
		{
			Profile.Name = "";
			Profile.Gender = "Unknown";
			Profile.PlayerHeight = 0.0f;
			Profile.EyeHeight = Data.EyeHeight;
			Profile.IPD = Data.IPD;
			Profile.NeckToEyeDistance = FVector2D(Data.EyeDepth, 0.0f);
			return true;
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	return false;
}

void UOculusFunctionLibrary::SetBaseRotationAndPositionOffset(FRotator BaseRot, FVector PosOffset, EOrientPositionSelector::Type Options)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		if (Options == EOrientPositionSelector::Orientation || Options == EOrientPositionSelector::OrientationAndPosition)
		{
			OculusHMD->SetBaseRotation(BaseRot);
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

void UOculusFunctionLibrary::GetBaseRotationAndPositionOffset(FRotator& OutRot, FVector& OutPosOffset)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		OutRot = OculusHMD->GetBaseRotation();
		OutPosOffset = FVector::ZeroVector;
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

void UOculusFunctionLibrary::AddLoadingSplashScreen(class UTexture2D* Texture, FVector TranslationInMeters, FRotator Rotation, FVector2D SizeInMeters, FRotator DeltaRotation, bool bClearBeforeAdd)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		OculusHMD::FSplash* Splash = OculusHMD->GetSplash();
		if (Splash)
		{
			if (bClearBeforeAdd)
			{
				Splash->ClearSplashes();
			}

			FOculusSplashDesc Desc;
			Desc.LoadingTexture = Texture;
			Desc.QuadSizeInMeters = SizeInMeters;
			Desc.TransformInMeters = FTransform(Rotation, TranslationInMeters);
			Desc.DeltaRotation = FQuat(DeltaRotation);
			Splash->AddSplash(Desc);
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

void UOculusFunctionLibrary::ClearLoadingSplashScreens()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		OculusHMD::FSplash* Splash = OculusHMD->GetSplash();
		if (Splash)
		{
			Splash->ClearSplashes();
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

bool UOculusFunctionLibrary::HasInputFocus()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	const OculusHMD::FOculusHMD* const OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr && OculusHMD->IsHMDActive())
	{
		ovrpBool HasFocus;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetAppHasInputFocus(&HasFocus)))
		{
			return HasFocus != ovrpBool_False;
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	return false;
}

bool UOculusFunctionLibrary::HasSystemOverlayPresent()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	const OculusHMD::FOculusHMD* const OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr && OculusHMD->IsHMDActive())
	{
		ovrpBool HasFocus;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetAppHasInputFocus(&HasFocus)))
		{
			return HasFocus == ovrpBool_False;
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	return false;
}

void UOculusFunctionLibrary::GetGPUUtilization(bool& IsGPUAvailable, float& GPUUtilization)
{
	IsGPUAvailable = false;
	GPUUtilization = 0.0f;

#if OCULUS_HMD_SUPPORTED_PLATFORMS
	const OculusHMD::FOculusHMD* const OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpBool GPUAvailable;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetGPUUtilSupported(&GPUAvailable)))
		{
			IsGPUAvailable = (GPUAvailable != ovrpBool_False);
			FOculusHMDModule::GetPluginWrapper().GetGPUUtilLevel(&GPUUtilization);
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

float UOculusFunctionLibrary::GetGPUFrameTime()
{
	float frameTime = 0;
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	const OculusHMD::FOculusHMD* const OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetGPUFrameTime(&frameTime)))
		{
			return frameTime;
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	return 0.0f;
}

EFoveatedRenderingMethod UOculusFunctionLibrary::GetFoveatedRenderingMethod()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpBool enabled;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetFoveationEyeTracked(&enabled)))
		{
			return enabled == ovrpBool_True ? EFoveatedRenderingMethod::EyeTrackedFoveatedRendering : EFoveatedRenderingMethod::FixedFoveatedRendering;
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	return EFoveatedRenderingMethod::FixedFoveatedRendering;
}

void UOculusFunctionLibrary::SetFoveatedRenderingMethod(EFoveatedRenderingMethod Method)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		OculusHMD->SetFoveatedRenderingMethod(Method);
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

void UOculusFunctionLibrary::SetFoveatedRenderingLevel(EFoveatedRenderingLevel level, bool isDynamic)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		OculusHMD->SetFoveatedRenderingLevel(level, isDynamic);
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
}

EFoveatedRenderingLevel UOculusFunctionLibrary::GetFoveatedRenderingLevel()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpTiledMultiResLevel Lvl;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetTiledMultiResLevel(&Lvl)))
		{
			return (EFoveatedRenderingLevel)Lvl;
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	return EFoveatedRenderingLevel::Off;
}

bool UOculusFunctionLibrary::GetEyeTrackedFoveatedRenderingSupported()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpBool Supported;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetFoveationEyeTrackedSupported(&Supported)))
		{
			return Supported == ovrpBool_True;
		}
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	return false;
}

FString UOculusFunctionLibrary::GetDeviceName()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		const char* NameString;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetSystemProductName2(&NameString)) && NameString)
		{
			return FString(NameString);
		}
	}
#endif
	return FString();
}

EOculusDeviceType UOculusFunctionLibrary::GetDeviceType()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		if (OculusHMD->GetSettings())
		{
			switch (OculusHMD->GetSettings()->SystemHeadset) {
			case ovrpSystemHeadset_Oculus_Quest:
				return EOculusDeviceType::OculusQuest_Deprecated;
			case ovrpSystemHeadset_Oculus_Quest_2:
				return EOculusDeviceType::OculusQuest2;
			case ovrpSystemHeadset_Meta_Quest_Pro:
				return EOculusDeviceType::MetaQuestPro;
			case ovrpSystemHeadset_Meta_Quest_3:
				return EOculusDeviceType::MetaQuest3;
			case ovrpSystemHeadset_Rift_CV1:
				return EOculusDeviceType::Rift;
			case ovrpSystemHeadset_Rift_S:
				return EOculusDeviceType::Rift_S;
			case ovrpSystemHeadset_Oculus_Link_Quest:
				return EOculusDeviceType::Quest_Link_Deprecated;
			case ovrpSystemHeadset_Oculus_Link_Quest_2:
				return EOculusDeviceType::Quest2_Link;
			case ovrpSystemHeadset_Meta_Link_Quest_Pro:
				return EOculusDeviceType::MetaQuestProLink;
			case ovrpSystemHeadset_Meta_Link_Quest_3:
				return EOculusDeviceType::MetaQuest3Link;
			default:
				break;
			}
		}
	}
#endif
	return EOculusDeviceType::OculusUnknown;
}

EOculusControllerType UOculusFunctionLibrary::GetControllerType(EControllerHand deviceHand)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	auto getOVRPHand = [](EControllerHand hand) {
		switch (hand)
		{
		case EControllerHand::Left:
			return ovrpHand::ovrpHand_Left;
		case EControllerHand::Right:
			return ovrpHand::ovrpHand_Right;
		default:
			return ovrpHand::ovrpHand_None;
		}
		return ovrpHand::ovrpHand_None;
	};

	auto getEControllerType = [](ovrpInteractionProfile profile) {
		switch (profile)
		{
		case ovrpInteractionProfile::ovrpInteractionProfile_Touch:
			return EOculusControllerType::MetaQuestTouch;
		case ovrpInteractionProfile::ovrpInteractionProfile_TouchPro:
			return EOculusControllerType::MetaQuestTouchPro;
		case ovrpInteractionProfile::ovrpInteractionProfile_TouchPlus:
			return EOculusControllerType::MetaQuestTouchPlus;
		default:
			return EOculusControllerType::None;
		}
		return EOculusControllerType::None;
	};

	ovrpInteractionProfile interactionProfile = ovrpInteractionProfile::ovrpInteractionProfile_None;
	ovrpHand hand = getOVRPHand(deviceHand);
	if(hand == ovrpHand::ovrpHand_None)
		return EOculusControllerType::Unknown;
	if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetCurrentInteractionProfile(hand, &interactionProfile)))
	{
		return getEControllerType(interactionProfile);
	}
	return EOculusControllerType::Unknown;
#endif
	return EOculusControllerType::Unknown;
}

TArray<float> UOculusFunctionLibrary::GetAvailableDisplayFrequencies()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		int NumberOfFrequencies;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetSystemDisplayAvailableFrequencies(NULL, &NumberOfFrequencies)))
		{
			TArray<float> freqArray;
			freqArray.SetNum(NumberOfFrequencies);
			FOculusHMDModule::GetPluginWrapper().GetSystemDisplayAvailableFrequencies(freqArray.GetData(), &NumberOfFrequencies);
			return freqArray;
		}
	}
#endif
	return TArray<float>();
}

float UOculusFunctionLibrary::GetCurrentDisplayFrequency()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		float Frequency;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetSystemDisplayFrequency2(&Frequency)))
		{
			return Frequency;
		}
	}
#endif
	return 0.0f;
}

void UOculusFunctionLibrary::SetDisplayFrequency(float RequestedFrequency)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		FOculusHMDModule::GetPluginWrapper().SetSystemDisplayFrequency(RequestedFrequency);
	}
#endif
}

void UOculusFunctionLibrary::EnablePositionTracking(bool bPositionTracking)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		FOculusHMDModule::GetPluginWrapper().SetTrackingPositionEnabled2(bPositionTracking);
	}
#endif
}


void UOculusFunctionLibrary::EnableOrientationTracking(bool bOrientationTracking)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		FOculusHMDModule::GetPluginWrapper().SetTrackingOrientationEnabled2(bOrientationTracking);
	}
#endif
}

void UOculusFunctionLibrary::SetColorScaleAndOffset(FLinearColor ColorScale, FLinearColor ColorOffset, bool bApplyToAllLayers)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		OculusHMD->SetColorScaleAndOffset(ColorScale, ColorOffset, bApplyToAllLayers);
	}
#endif
}

class IStereoLayers* UOculusFunctionLibrary::GetStereoLayers()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		return OculusHMD;
	}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	return nullptr;
}

/** Helper that converts EBoundaryType to ovrpBoundaryType */
#if OCULUS_HMD_SUPPORTED_PLATFORMS
static ovrpBoundaryType ToOvrpBoundaryType(EBoundaryType Source)
{
	switch (Source)
	{
	case EBoundaryType::Boundary_PlayArea:
		return ovrpBoundary_PlayArea;

	case EBoundaryType::Boundary_Outer:
	default:
		return ovrpBoundary_Outer;
	}
}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS

bool UOculusFunctionLibrary::IsGuardianConfigured()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpBool boundaryConfigured;
		return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetBoundaryConfigured2(&boundaryConfigured)) && boundaryConfigured;
	}
#endif
	return false;
}

bool UOculusFunctionLibrary::IsGuardianDisplayed()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpBool boundaryVisible;
		return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetBoundaryVisible2(&boundaryVisible)) && boundaryVisible;
	}
#endif
	return false;
}

TArray<FVector> UOculusFunctionLibrary::GetGuardianPoints(EBoundaryType BoundaryType, bool UsePawnSpace /* = false */)
{
	TArray<FVector> BoundaryPointList;
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpBoundaryType obt = ToOvrpBoundaryType(BoundaryType);
		int NumPoints = 0;

		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetBoundaryGeometry3(obt, NULL, &NumPoints)))
		{
			//allocate points
			const int BufferSize = NumPoints;
			ovrpVector3f* BoundaryPoints = new ovrpVector3f[BufferSize];

			if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetBoundaryGeometry3(obt, BoundaryPoints, &NumPoints)))
			{
				NumPoints = FMath::Min(BufferSize, NumPoints);
				check(NumPoints <= BufferSize); // For static analyzer
				BoundaryPointList.Reserve(NumPoints);

				for (int i = 0; i < NumPoints; i++)
				{
					FVector point;
					if (UsePawnSpace)
					{
						point = OculusHMD->ConvertVector_M2U(BoundaryPoints[i]);
					}
					else
					{
						point = OculusHMD->ScaleAndMovePointWithPlayer(BoundaryPoints[i]);
					}
					BoundaryPointList.Add(point);
				}
			}

			delete[] BoundaryPoints;
		}
	}
#endif
	return BoundaryPointList;
}

FVector UOculusFunctionLibrary::GetGuardianDimensions(EBoundaryType BoundaryType)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpBoundaryType obt = ToOvrpBoundaryType(BoundaryType);
		ovrpVector3f Dimensions;

		if (OVRP_FAILURE(FOculusHMDModule::GetPluginWrapper().GetBoundaryDimensions2(obt, &Dimensions)))
			return FVector::ZeroVector;

		Dimensions.z *= -1.0;
		return OculusHMD->ConvertVector_M2U(Dimensions);
	}
#endif
	return FVector::ZeroVector;
}

FTransform UOculusFunctionLibrary::GetPlayAreaTransform()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		int NumPoints = 4;
		ovrpVector3f BoundaryPoints[4];

		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetBoundaryGeometry3(ovrpBoundary_PlayArea, BoundaryPoints, &NumPoints)))
		{	
			FVector ConvertedPoints[4];

			for (int i = 0; i < NumPoints; i++)
			{
				ConvertedPoints[i] = OculusHMD->ScaleAndMovePointWithPlayer(BoundaryPoints[i]);
			}

			float metersScale = OculusHMD->GetWorldToMetersScale();

			FVector Edge = ConvertedPoints[1] - ConvertedPoints[0];
			float Angle = FMath::Acos((Edge).GetSafeNormal() | FVector::RightVector);
			FQuat Rotation(FVector::UpVector, Edge.X < 0 ? Angle : -Angle);
			
			FVector Position = (ConvertedPoints[0] + ConvertedPoints[1] + ConvertedPoints[2] + ConvertedPoints[3]) / 4;
			FVector Scale(FVector::Distance(ConvertedPoints[3], ConvertedPoints[0]) / metersScale, FVector::Distance(ConvertedPoints[1], ConvertedPoints[0]) / metersScale, 1.0);

			return FTransform(Rotation, Position, Scale);
		}
	}
#endif
	return FTransform();
}

FGuardianTestResult UOculusFunctionLibrary::GetPointGuardianIntersection(const FVector Point, EBoundaryType BoundaryType)
{
	FGuardianTestResult InteractionInfo;
	memset(&InteractionInfo, 0, sizeof(FGuardianTestResult));

#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpVector3f OvrpPoint = OculusHMD->WorldLocationToOculusPoint(Point);
		ovrpBoundaryType OvrpBoundaryType = ToOvrpBoundaryType(BoundaryType);
		ovrpBoundaryTestResult InteractionResult;

		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().TestBoundaryPoint2(OvrpPoint, OvrpBoundaryType, &InteractionResult)))
		{
			InteractionInfo.IsTriggering = (InteractionResult.IsTriggering != 0);
			InteractionInfo.ClosestDistance = OculusHMD->ConvertFloat_M2U(InteractionResult.ClosestDistance);
			InteractionInfo.ClosestPoint = OculusHMD->ScaleAndMovePointWithPlayer(InteractionResult.ClosestPoint);
			InteractionInfo.ClosestPointNormal = OculusHMD->ConvertVector_M2U(InteractionResult.ClosestPointNormal);
			InteractionInfo.DeviceType = ETrackedDeviceType::None;
		}
	}
#endif

	return InteractionInfo;
}

FGuardianTestResult UOculusFunctionLibrary::GetNodeGuardianIntersection(ETrackedDeviceType DeviceType, EBoundaryType BoundaryType)
{
	FGuardianTestResult InteractionInfo;
	memset(&InteractionInfo, 0, sizeof(FGuardianTestResult));

#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpNode OvrpNode = OculusHMD::ToOvrpNode(DeviceType);
		ovrpBoundaryType OvrpBoundaryType = ToOvrpBoundaryType(BoundaryType);
		ovrpBoundaryTestResult TestResult;

		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().TestBoundaryNode2(OvrpNode, ovrpBoundary_PlayArea, &TestResult)) && TestResult.IsTriggering)
		{
			InteractionInfo.IsTriggering = true;
			InteractionInfo.DeviceType = OculusHMD::ToETrackedDeviceType(OvrpNode);
			InteractionInfo.ClosestDistance = OculusHMD->ConvertFloat_M2U(TestResult.ClosestDistance);
			InteractionInfo.ClosestPoint = OculusHMD->ScaleAndMovePointWithPlayer(TestResult.ClosestPoint);
			InteractionInfo.ClosestPointNormal = OculusHMD->ConvertVector_M2U(TestResult.ClosestPointNormal);
		}
	}
#endif

	return InteractionInfo;
}

void UOculusFunctionLibrary::SetGuardianVisibility(bool GuardianVisible)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		FOculusHMDModule::GetPluginWrapper().SetBoundaryVisible2(GuardianVisible);
	}
#endif
}

bool UOculusFunctionLibrary::GetSystemHmd3DofModeEnabled()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpBool enabled;
		return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetSystemHmd3DofModeEnabled(&enabled)) && enabled;
	}
#endif
	return false;
}

EColorSpace UOculusFunctionLibrary::GetHmdColorDesc()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpColorSpace HmdColorSpace;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetHmdColorDesc(&HmdColorSpace)))
		{
			return (EColorSpace)HmdColorSpace;
		}
	}
#endif
	return EColorSpace::Unknown;
}

void UOculusFunctionLibrary::SetClientColorDesc(EColorSpace ColorSpace)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpColorSpace ClientColorSpace = (ovrpColorSpace)ColorSpace;
#if PLATFORM_ANDROID
		if (ClientColorSpace == ovrpColorSpace_Unknown)
		{
			ClientColorSpace = ovrpColorSpace_Quest;
		}
#endif
		FOculusHMDModule::GetPluginWrapper().SetClientColorDesc(ClientColorSpace);
	}
#endif
}


bool UOculusFunctionLibrary::IsPassthroughSupported() {
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpInsightPassthroughCapabilityFlags capabilities;

		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetPassthroughCapabilityFlags(&capabilities))) {
			return (capabilities & ovrpInsightPassthroughCapabilityFlags::ovrpInsightPassthroughCapabilityFlags_Passthrough)
				== ovrpInsightPassthroughCapabilityFlags::ovrpInsightPassthroughCapabilityFlags_Passthrough;
		}

		return false;
	}
#endif
	return false;
}

bool UOculusFunctionLibrary::IsColorPassthroughSupported() {
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpInsightPassthroughCapabilityFlags capabilities;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetPassthroughCapabilityFlags(&capabilities))) {
			return (capabilities & ovrpInsightPassthroughCapabilityFlags::ovrpInsightPassthroughCapabilityFlags_Color)
				== ovrpInsightPassthroughCapabilityFlags::ovrpInsightPassthroughCapabilityFlags_Color;
		}

		return false;
	}
#endif
	return false;
}

void UOculusFunctionLibrary::SetLocalDimmingOn(bool LocalDimmingOn)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("SetLocalDimmingOn %d"), LocalDimmingOn);
		FOculusHMDModule::GetPluginWrapper().SetLocalDimming(LocalDimmingOn);
	}
#endif
}


bool UOculusFunctionLibrary::IsResultSuccess(EOculusResult::Type result)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	return OVRP_SUCCESS(result);
#endif
	return false;
}

void UOculusFunctionLibrary::SetEyeBufferSharpenType(EOculusEyeBufferSharpenType EyeBufferSharpenType)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		switch (EyeBufferSharpenType)
		{
		case EOculusEyeBufferSharpenType::SLST_Normal:
			FOculusHMDModule::GetPluginWrapper().SetEyeBufferSharpenType(ovrpLayerSubmitFlag_EfficientSharpen);
			break;
		case EOculusEyeBufferSharpenType::SLST_Quality:
			FOculusHMDModule::GetPluginWrapper().SetEyeBufferSharpenType(ovrpLayerSubmitFlag_QualitySharpen);
			break;
		default:
			FOculusHMDModule::GetPluginWrapper().SetEyeBufferSharpenType(ovrpLayerSubmitFlags(0));
			break;
		}
	}
#endif
}

bool UOculusFunctionLibrary::IsPassthroughRecommended()
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	const OculusHMD::FOculusHMD* OculusHMD = GetOculusHMD();
	if (OculusHMD != nullptr)
	{
		ovrpPassthroughPreferences Preferences;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetPassthroughPreferences(&Preferences)))
		{
			return (Preferences.Flags & ovrpPassthroughPreferenceFlags::ovrpPassthroughPreferenceFlags_DefaultToActive)
				== ovrpPassthroughPreferenceFlags::ovrpPassthroughPreferenceFlags_DefaultToActive;
		};
	}
#endif
	return false;
}

#undef LOCTEXT_NAMESPACE
