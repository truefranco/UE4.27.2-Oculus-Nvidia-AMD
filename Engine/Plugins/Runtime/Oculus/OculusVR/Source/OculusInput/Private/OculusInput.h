// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "IOculusInputModule.h"

#if OCULUS_INPUT_SUPPORTED_PLATFORMS
#include "OculusHMDModule.h"
#include "GenericPlatform/IInputInterface.h"
#include "XRMotionControllerBase.h"
#include "IHapticDevice.h"
#include "OculusInputState.h"

#if PLATFORM_SUPPORTS_PRAGMA_PACK
	#pragma pack (push,8)
#endif

#include "OculusPluginWrapper.h"

#if PLATFORM_SUPPORTS_PRAGMA_PACK
	#pragma pack (pop)
#endif

DEFINE_LOG_CATEGORY_STATIC(LogOcInput, Log, All);


namespace OculusInput
{

//-------------------------------------------------------------------------------------------------
// FOculusInput
//-------------------------------------------------------------------------------------------------

class FOculusInput : public IInputDevice, public FXRMotionControllerBase, public IHapticDevice
{
	friend class FOculusHandTracking;

public:

	/** Constructor that takes an initial message handler that will receive motion controller events */
	FOculusInput( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	/** Clean everything up */
	virtual ~FOculusInput();

	static void PreInit();

	/** Loads any settings from the config folder that we need */
	static void LoadConfig();

	// IInputDevice overrides
	virtual void Tick( float DeltaTime ) override;
	virtual void SendControllerEvents() override;
	virtual void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler ) override;
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override;
	virtual void SetChannelValue( int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value ) override;
	virtual void SetChannelValues( int32 ControllerId, const FForceFeedbackValues& Values ) override;
	virtual bool SupportsForceFeedback(int32 ControllerId) override;

	// IMotionController overrides
	virtual FName GetMotionControllerDeviceTypeName() const override;
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const override;
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const override;
	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const override;

	// IHapticDevice overrides
	IHapticDevice* GetHapticDevice() override { return (IHapticDevice*)this; }
	virtual void SetHapticFeedbackValues(int32 ControllerId, int32 Hand, const FHapticFeedbackValues& Values) override;

	void PlayHapticEffect(
		class UHapticFeedbackEffect_Base* HapticEffect,
		EControllerHand Hand,
		EOculusHandHapticsLocation Location = EOculusHandHapticsLocation::Hand,
		bool bAppend = false,
		float Scale = 1.f,
		bool bLoop = false);
	int PlayHapticEffect(EControllerHand Hand, int SamplesCount, void* Samples, int SampleRate = -1, bool bPCM = false, bool bAppend = false);
	void SetHapticsByValue(float Frequency, float Amplitude, EControllerHand Hand, EOculusHandHapticsLocation Location = EOculusHandHapticsLocation::Hand);

	virtual void GetHapticFrequencyRange(float& MinFrequency, float& MaxFrequency) const override;
	virtual float GetHapticAmplitudeScale() const override;

	uint32 GetNumberOfTouchControllers() const;
	uint32 GetNumberOfHandControllers() const;

	float GetControllerSampleRateHz(EControllerHand Hand);
	int GetMaxHapticDuration(EControllerHand Hand);
private:

	/** Applies force feedback settings to the controller */
	void UpdateForceFeedback( const FOculusControllerPair& ControllerPair, const EControllerHand Hand );

	bool OnControllerButtonPressed( const FOculusButtonState& ButtonState, int32 ControllerId, bool IsRepeat );
	bool OnControllerButtonReleased( const FOculusButtonState& ButtonState, int32 ControllerId, bool IsRepeat );

	void SetHapticFeedbackValues(int32 ControllerId, int32 Hand, const FHapticFeedbackValues& Values, TSharedPtr<struct FOculusHapticsDesc> HapticsDesc);
	ovrpHapticsLocation GetOVRPHapticsLocation(EOculusHandHapticsLocation InLocation);

	void ProcessHaptics(const float DeltaTime);
	bool GetOvrpHapticsDesc(int Hand);
private:

	void* OVRPluginHandle;

	/** The recipient of motion controller input events */
	TSharedPtr< FGenericApplicationMessageHandler > MessageHandler;

	/** List of the connected pairs of controllers, with state for each controller device */
	TArray< FOculusControllerPair > ControllerPairs;

	FOculusRemoteControllerState Remote;

	ovrpHapticsDesc OvrpHapticsDesc;

	int LocalTrackingSpaceRecenterCount;

	TSharedPtr<struct FActiveHapticFeedbackEffect> ActiveHapticEffect_Left;
	TSharedPtr<struct FActiveHapticFeedbackEffect> ActiveHapticEffect_Right;
	TSharedPtr<struct FOculusHapticsDesc> HapticsDesc_Left;
	TSharedPtr<struct FOculusHapticsDesc> HapticsDesc_Right;
	double StartTime = 0.0;

	/** Threshold for treating trigger pulls as button presses, from 0.0 to 1.0 */
	static float TriggerThreshold;

	/** Are Remote keys mapped to gamepad or not. */
	static bool bRemoteKeysMappedToGamepad;

	/** Repeat key delays, loaded from config */
	static float InitialButtonRepeatDelay;
	static float ButtonRepeatDelay;

	static bool bPulledHapticsDesc;
};


} // namespace OculusInput

#endif //OCULUS_INPUT_SUPPORTED_PLATFORMS
