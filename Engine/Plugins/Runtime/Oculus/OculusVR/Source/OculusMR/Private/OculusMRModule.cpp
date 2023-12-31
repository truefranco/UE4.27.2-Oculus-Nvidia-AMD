// Copyright Epic Games, Inc. All Rights Reserved.
#include "OculusMRModule.h"

#include "Engine/Engine.h"
#include "ISpectatorScreenController.h"
#include "IXRTrackingSystem.h"
#include "StereoRendering.h"
#include "StereoRenderTargetManager.h"
#include "SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "PostProcess/SceneRenderTargets.h"
#include "Kismet/GameplayStatics.h"
#include "OculusHMDModule.h"
#include "OculusHMD.h"
#include "OculusMRFunctionLibrary.h"
#include "OculusMRPrivate.h"
#include "OculusMR_Settings.h"
#include "OculusMR_State.h"
#include "OculusMR_CastingCameraActor.h"
#include "AudioDevice.h"
#if PLATFORM_ANDROID
#include "VulkanRHIBridge.h"
#include "VulkanRHIPrivate.h"
#endif

#if WITH_EDITOR
#include "Editor.h" // for FEditorDelegates::PostPIEStarted
#endif

#define LOCTEXT_NAMESPACE "OculusMR"

FOculusMRModule::FOculusMRModule()
	: bInitialized(false)
	, MRSettings(nullptr)
	, MRState(nullptr)
	, MRActor(nullptr)
	, CurrentWorld(nullptr)
	, WorldAddedEventBinding()
	, WorldDestroyedEventBinding()
	, WorldLoadEventBinding()
#if PLATFORM_ANDROID
	, bActivated(false)
	, InitialWorldAddedEventBinding()
	, InitialWorldLoadEventBinding()
	, PreWorldTickEventBinding()
#endif
#if WITH_EDITOR
	, PieBeginEventBinding()
	, PieStartedEventBinding()
	, PieEndedEventBinding()
#endif
{}

FOculusMRModule::~FOculusMRModule()
{}

void FOculusMRModule::StartupModule()
{
#if OCULUS_MR_SUPPORTED_PLATFORMS
#if PLATFORM_WINDOWS
	const TCHAR* CmdLine = FCommandLine::Get();
	const bool bAutoOpenFromParams = FParse::Param(CmdLine, TEXT("mixedreality"));

	if (bAutoOpenFromParams && FOculusHMDModule::Get().PreInit())
	{
		if (FOculusHMDModule::GetPluginWrapper().GetInitialized())
		{
			if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().InitializeMixedReality()))
			{
				InitMixedRealityCapture();
			}
			else
			{
				UE_LOG(LogMR, Error, TEXT("ovrp_InitializeMixedReality() failed"));
			}
		}
		else
		{
			UE_LOG(LogMR, Error, TEXT("OVRPlugin has not been initialized"));
		}
	}
#elif PLATFORM_ANDROID
	// On Android, FOculusHMDModule::GetPluginWrapper().Media_Initialize() needs OVRPlugin to be initialized first, so we should handle that when the world is created
	if (GEngine)
	{
		InitialWorldAddedEventBinding = GEngine->OnWorldAdded().AddRaw(this, &FOculusMRModule::OnInitialWorldCreated);
	}
	InitialWorldLoadEventBinding = FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FOculusMRModule::OnInitialWorldCreated);
#endif // PLATFORM_WINDOWS || PLATFORM_ANDROID
#endif // OCULUS_MR_SUPPORTED_PLATFORMS
}

void FOculusMRModule::ShutdownModule()
{
#if OCULUS_MR_SUPPORTED_PLATFORMS
	if (bInitialized)
	{
		if (GEngine)
		{
			GEngine->OnWorldAdded().Remove(WorldAddedEventBinding);
			GEngine->OnWorldDestroyed().Remove(WorldDestroyedEventBinding);
			FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(WorldLoadEventBinding);
#if WITH_EDITOR
			FEditorDelegates::PostPIEStarted.Remove(PieStartedEventBinding);
			FEditorDelegates::PrePIEEnded.Remove(PieEndedEventBinding);
#else
			// Stop casting and close camera with module if it's the game
			MRSettings->SetIsCasting(false);
#endif
		}
#if PLATFORM_ANDROID
		ovrpBool mediaInit = false;
		if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().Media_GetInitialized(&mediaInit)) && mediaInit == ovrpBool_True)
		{
			FOculusHMDModule::GetPluginWrapper().Media_Shutdown();
		}
#endif 
		FOculusHMDModule::GetPluginWrapper().ShutdownMixedReality();

		if (MRSettings->IsRooted())
		{
			MRSettings->RemoveFromRoot();
		}
		if (MRState->IsRooted())
		{
			MRState->RemoveFromRoot();
		}
	}
#if PLATFORM_ANDROID
	if (InitialWorldAddedEventBinding.IsValid() && GEngine)
	{
		GEngine->OnWorldAdded().Remove(InitialWorldAddedEventBinding);
		InitialWorldAddedEventBinding.Reset();
	}
	if (InitialWorldLoadEventBinding.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(InitialWorldLoadEventBinding);
		InitialWorldLoadEventBinding.Reset();
	}
#endif
#endif // OCULUS_MR_SUPPORTED_PLATFORMS
}

bool FOculusMRModule::IsActive()
{
	bool bReturn = bInitialized && MRSettings && MRSettings->GetIsCasting();
#if PLATFORM_ANDROID
	bReturn = bReturn && bActivated;
#endif
	return bReturn;
}

UOculusMR_Settings* FOculusMRModule::GetMRSettings()
{
	return MRSettings;
}

UOculusMR_State* FOculusMRModule::GetMRState()
{
	return MRState;
}

void FOculusMRModule::OnWorldCreated(UWorld* NewWorld)
{
#if PLATFORM_WINDOWS
#if WITH_EDITORONLY_DATA
	const bool bIsGameInst = !IsRunningCommandlet() && NewWorld->IsGameWorld();
	if (bIsGameInst)
#endif 
	{
		CurrentWorld = NewWorld;
		SetupInGameCapture();
	}
#endif
#if PLATFORM_ANDROID
	CurrentWorld = NewWorld;
	// Check MRC activation state initially when loading world
	ChangeCaptureState();
	// Poll MRC activation state for future changes
	PreWorldTickEventBinding = FWorldDelegates::OnWorldPreActorTick.AddRaw(this, &FOculusMRModule::OnWorldTick);
#endif
}

void FOculusMRModule::OnWorldDestroyed(UWorld* NewWorld)
{
	CurrentWorld = nullptr;
#if PLATFORM_ANDROID
	if (PreWorldTickEventBinding.IsValid())
	{
		FWorldDelegates::OnWorldPreActorTick.Remove(PreWorldTickEventBinding);
		PreWorldTickEventBinding.Reset();
	}
#endif // PLATFORM_ANDROID
}

void FOculusMRModule::InitMixedRealityCapture()
{
	bInitialized = true;

	MRSettings = NewObject<UOculusMR_Settings>((UObject*)GetTransientPackage(), FName("OculusMR_Settings"), RF_MarkAsRootSet);
	MRState = NewObject<UOculusMR_State>((UObject*)GetTransientPackage(), FName("OculusMR_State"), RF_MarkAsRootSet);

	// Always bind the event handlers in case devs call them without MRC on
	MRSettings->TrackedCameraIndexChangeDelegate.BindRaw(this, &FOculusMRModule::OnTrackedCameraIndexChanged);
	MRSettings->CompositionMethodChangeDelegate.BindRaw(this, &FOculusMRModule::OnCompositionMethodChanged);
	MRSettings->IsCastingChangeDelegate.BindRaw(this, &FOculusMRModule::OnIsCastingChanged);

	ResetSettingsAndState();

	WorldAddedEventBinding = GEngine->OnWorldAdded().AddRaw(this, &FOculusMRModule::OnWorldCreated);
	WorldDestroyedEventBinding = GEngine->OnWorldDestroyed().AddRaw(this, &FOculusMRModule::OnWorldDestroyed);
	WorldLoadEventBinding = FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FOculusMRModule::OnWorldCreated);

#if WITH_EDITOR
	// Bind events on PIE start/end to open/close camera
	PieBeginEventBinding = FEditorDelegates::BeginPIE.AddRaw(this, &FOculusMRModule::OnPieBegin);
	PieStartedEventBinding = FEditorDelegates::PostPIEStarted.AddRaw(this, &FOculusMRModule::OnPieStarted);
	PieEndedEventBinding = FEditorDelegates::PrePIEEnded.AddRaw(this, &FOculusMRModule::OnPieEnded);
#else // WITH_EDITOR
	// Start casting and open camera with the module if it's the game
	MRSettings->SetIsCasting(true);
#endif // WITH_EDITOR
}

void FOculusMRModule::SetupExternalCamera()
{
	using namespace OculusHMD;

	if (!MRSettings->GetIsCasting())
	{
		return;
	}

	// Always request the MRC actor to handle a camera state change on its end
	MRState->ChangeCameraStateRequested = true;
}

void FOculusMRModule::SetupInGameCapture()
{
	// Don't do anything if we don't have a UWorld or if we are not casting
	if (CurrentWorld == nullptr || !MRSettings->GetIsCasting())
	{
		return;
	}

	// Set the bind camera request to true
	MRState->BindToTrackedCameraIndexRequested = true;

	// Don't add another actor if there's already a MRC camera actor
	for (TActorIterator<AOculusMR_CastingCameraActor> ActorIt(CurrentWorld); ActorIt; ++ActorIt)
	{
		if (!ActorIt->IsPendingKillOrUnreachable() && ActorIt->IsValidLowLevel())
		{
			MRActor = *ActorIt;
			return;
		}
	}

	// Spawn an MRC camera actor if one wasn't already there
	MRActor = CurrentWorld->SpawnActorDeferred<AOculusMR_CastingCameraActor>(AOculusMR_CastingCameraActor::StaticClass(), FTransform::Identity);
	MRActor->InitializeStates(MRSettings, MRState);
	UGameplayStatics::FinishSpawningActor(MRActor, FTransform::Identity);
}

void FOculusMRModule::CloseInGameCapture()
{
	// Destory actor and close the camera when we turn MRC off
	if (MRActor != nullptr && MRActor->GetWorld() != nullptr)
	{
		MRActor->Destroy();
		MRActor = nullptr;
	}
}

void FOculusMRModule::ResetSettingsAndState()
{
	// Reset MR State
	MRState->TrackedCamera = FTrackedCamera();
	MRState->TrackingReferenceComponent = nullptr;
	MRState->ChangeCameraStateRequested = false;
	MRState->BindToTrackedCameraIndexRequested = false;

	// Reset MR Settings
	const bool bAutoOpenInExternalComposition = FParse::Param(FCommandLine::Get(), TEXT("externalcomposition"));
	MRSettings->BindToTrackedCameraIndexIfAvailable(0);
	MRSettings->LoadFromIni();

	// Save right after load to write defaults to the config if they weren't already there
	MRSettings->SaveToIni();

	if (bAutoOpenInExternalComposition)
	{
		MRSettings->CompositionMethod = EOculusMR_CompositionMethod::ExternalComposition;
	}
}

#if PLATFORM_ANDROID
void FOculusMRModule::ChangeCaptureState()
{
	ovrpBool activated;
	// Set up or close in-game capture when activation state changes
	if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().Media_Update()) && OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().Media_IsMrcActivated(&activated)) && activated == ovrpBool_True)
	{
		if (!bActivated)
		{
			UE_LOG(LogMR, Log, TEXT("Activating MR Capture"))
			bActivated = true;

			// UE resizes the main scene color and depth targets to the maximum dimensions of all rendertargets,
			// which causes rendering issues if it doesn't match the compositor-allocated eye textures. This is
			// a hacky fix by making sure that the scene capture rendertarget is no larger than the eye.
			int frameWidth;
			int frameHeight;
			FOculusHMDModule::GetPluginWrapper().Media_GetMrcFrameSize(&frameWidth, &frameHeight);
			uint32 maxWidth = frameWidth / 2;
			uint32 maxHeight = frameHeight;
			IStereoRenderTargetManager* const StereoRenderTargetManager = GEngine->StereoRenderingDevice->GetRenderTargetManager();
			if (StereoRenderTargetManager)
			{
				StereoRenderTargetManager->CalculateRenderTargetSize(*(FViewport*)GEngine->GameViewport->GetGameViewport(), maxWidth, maxHeight);
			}
			maxWidth *= 2;
			frameWidth = frameWidth > maxWidth ? maxWidth : frameWidth;
			frameHeight = frameHeight > maxHeight ? maxHeight : frameHeight;
			FOculusHMDModule::GetPluginWrapper().Media_SetMrcFrameSize(frameWidth, frameHeight);
			UE_LOG(LogMR, Log, TEXT("MRC Frame width: %d height %d"), frameWidth, frameHeight);

			SetupInGameCapture();
		}
	}
	else
	{
		if (bActivated)
		{
			UE_LOG(LogMR, Log, TEXT("Deactivating MR Capture"))
			bActivated = false;
			CloseInGameCapture();
		}
	}
}

void FOculusMRModule::OnWorldTick(UWorld* World, ELevelTick Tick, float Delta)
{
	// Poll MRC activation state
	if (CurrentWorld && World == CurrentWorld)
	{
		ChangeCaptureState();
	}
}

void FOculusMRModule::OnInitialWorldCreated(UWorld* NewWorld)
{
	// Remove the initial world load handlers 
	if (InitialWorldAddedEventBinding.IsValid())
	{
		GEngine->OnWorldAdded().Remove(InitialWorldAddedEventBinding);
		InitialWorldAddedEventBinding.Reset();
	}
	if (InitialWorldLoadEventBinding.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(InitialWorldLoadEventBinding);
		InitialWorldLoadEventBinding.Reset();
	}

	// Initialize and check if MRC is enabled
	if (FOculusHMDModule::Get().PreInit())
	{
		if (FOculusHMDModule::GetPluginWrapper().GetInitialized())
		{
			if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().InitializeMixedReality()))
			{
				ovrpBool mrcEnabled;

				if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().Media_Initialize()))
				{
					UE_LOG(LogMR, Log, TEXT("MRC Initialized"));

					if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().Media_IsMrcEnabled(&mrcEnabled)) && mrcEnabled == ovrpBool_True)
					{
						UE_LOG(LogMR, Log, TEXT("MRC Enabled"));

						// Find a free queue index for vulkan
						if (IsVulkanPlatform(GMaxRHIShaderPlatform))
						{
							unsigned int queueIndex = 0;
							ExecuteOnRenderThread([&queueIndex]()
							{
								ExecuteOnRHIThread([&queueIndex]()
								{
									FVulkanDevice* VulkanDevice = VulkanRHIBridge::GetDevice(GVulkanRHI);
									int gfxIndex = VulkanDevice->GetGraphicsQueue() ? VulkanDevice->GetGraphicsQueue()->GetQueueIndex() : -1;
									if (gfxIndex == queueIndex) {
										++queueIndex;
									}
								});
							});
							FOculusHMDModule::GetPluginWrapper().Media_SetAvailableQueueIndexVulkan(queueIndex);
						}

						FOculusHMDModule::GetPluginWrapper().Media_SetMrcInputVideoBufferType(ovrpMediaInputVideoBufferType_TextureHandle);

						FAudioDeviceHandle AudioDevice = FAudioDevice::GetMainAudioDevice();
						if (AudioDevice.GetAudioDevice())
						{
							float SampleRate = AudioDevice->GetSampleRate();
							FOculusHMDModule::GetPluginWrapper().Media_SetMrcAudioSampleRate((int)SampleRate);
						}

						InitMixedRealityCapture();
						OnWorldCreated(NewWorld);
					}
					else
					{
						// Shut down if MRC not enabled or the media couldn't be enabled
						FOculusHMDModule::GetPluginWrapper().Media_Shutdown();
						FOculusHMDModule::GetPluginWrapper().ShutdownMixedReality();
					}
				}
				else
				{
					// Shut down if MRC not enabled or the media couldn't be enabled
					FOculusHMDModule::GetPluginWrapper().ShutdownMixedReality();
				}
			}
			else
			{
				UE_LOG(LogMR, Error, TEXT("ovrp_InitializeMixedReality() failed"));
			}
		}
		else
		{
			UE_LOG(LogMR, Error, TEXT("OVRPlugin has not been initialized"));
		}
	}
}
#endif

void FOculusMRModule::OnTrackedCameraIndexChanged(int OldVal, int NewVal)
{
	if (OldVal == NewVal)
	{
		return;
	}
	MRState->BindToTrackedCameraIndexRequested = true;
}

void FOculusMRModule::OnCompositionMethodChanged(EOculusMR_CompositionMethod OldVal, EOculusMR_CompositionMethod NewVal)
{
	if (OldVal == NewVal)
	{
		return;
	}
	SetupExternalCamera();
}

void FOculusMRModule::OnIsCastingChanged(bool OldVal, bool NewVal)
{
	if (OldVal == NewVal)
	{
		return;
	}
	if (NewVal == true)
	{
#if PLATFORM_ANDROID
		FOculusHMDModule::GetPluginWrapper().Media_SetMrcActivationMode(ovrpMediaMrcActivationMode_Automatic);
#endif
		// Initialize everything again if we turn MRC on
		SetupExternalCamera();
		SetupInGameCapture();
	}
	else
	{
#if PLATFORM_ANDROID
		FOculusHMDModule::GetPluginWrapper().Media_SetMrcActivationMode(ovrpMediaMrcActivationMode_Disabled);
#endif
		CloseInGameCapture();
	}
}

void FOculusMRModule::OnUseDynamicLightingChanged(bool OldVal, bool NewVal)
{
	if (OldVal == NewVal)
	{
		return;
	}
	SetupExternalCamera();
}

void FOculusMRModule::OnDepthQualityChanged(EOculusMR_DepthQuality OldVal, EOculusMR_DepthQuality NewVal)
{
	if (OldVal == NewVal)
	{
		return;
	}
	SetupExternalCamera();
}

#if WITH_EDITOR
void FOculusMRModule::OnPieBegin(bool bIsSimulating)
{
	// Reset all the parameters and start casting when PIE starts but before the game is initialized
	if (!bIsSimulating)
	{

		ResetSettingsAndState();

		// Always start casting with PIE (since this can only be reached if the command line param is on)
		MRSettings->SetIsCasting(true);
	}
}

void FOculusMRModule::OnPieStarted(bool bIsSimulating)
{
	// Handle the PIE world as a normal game world
	UWorld* PieWorld = GEditor->GetPIEWorldContext()->World();
	if (!bIsSimulating && PieWorld)
	{
		OnWorldCreated(PieWorld);
	}
}

void FOculusMRModule::OnPieEnded(bool bIsSimulating)
{
	UWorld* PieWorld = GEditor->GetPIEWorldContext()->World();
	if (!bIsSimulating && PieWorld)
	{
		// Stop casting when PIE ends
		MRSettings->SetIsCasting(false);
		OnWorldDestroyed(PieWorld);
	}
}
#endif // WITH_EDITOR

IMPLEMENT_MODULE( FOculusMRModule, OculusMR )

#undef LOCTEXT_NAMESPACE
