// Copyright Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneAudioTemplate.h"


#include "Engine/EngineTypes.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "GameFramework/Actor.h"
#include "AudioThread.h"
#include "AudioDevice.h"
#include "ActiveSound.h"
#include "Sections/MovieSceneAudioSection.h"
#include "Tracks/MovieSceneAudioTrack.h"
#include "Sound/SoundWave.h"
#include "MovieScene.h"
#include "Evaluation/MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"
#include "GameFramework/WorldSettings.h"


DECLARE_CYCLE_STAT(TEXT("Audio Track Evaluate"), MovieSceneEval_AudioTrack_Evaluate, STATGROUP_MovieSceneEval);
DECLARE_CYCLE_STAT(TEXT("Audio Track Tear Down"), MovieSceneEval_AudioTrack_TearDown, STATGROUP_MovieSceneEval);
DECLARE_CYCLE_STAT(TEXT("Audio Track Token Execute"), MovieSceneEval_AudioTrack_TokenExecute, STATGROUP_MovieSceneEval);

static float MaxSequenceAudioDesyncToleranceCVar = 0.5f;
FAutoConsoleVariableRef CVarMaxSequenceAudioDesyncTolerance(
	TEXT("Sequencer.Audio.MaxDesyncTolerance"),
	MaxSequenceAudioDesyncToleranceCVar,
	TEXT("Controls how many seconds an audio track can be out of sync in a Sequence before we attempt a time correction.\n"),
	ECVF_Default);

static bool bIgnoreAudioSyncDuringWorldTimeDilationCVar = true;
FAutoConsoleVariableRef CVarIgnoreAudioSyncDuringWorldTimeDilation(
	TEXT("Sequencer.Audio.IgnoreAudioSyncDuringWorldTimeDilation"),
	bIgnoreAudioSyncDuringWorldTimeDilationCVar,
	TEXT("Ignore correcting audio if there is world time dilation.\n"),
	ECVF_Default);

static int32 UseAudioClockForSequencerDesyncCVar = 0;
FAutoConsoleVariableRef CVaUseAudioClockForSequencerDesync(
	TEXT("Sequencer.Audio.UseAudioClockForAudioDesync"),
	UseAudioClockForSequencerDesyncCVar,
	TEXT("When set to 1, we will use the audio render thread directly to query whether audio has went out of sync with the sequence.\n"),
	ECVF_Default);

static int32 UseTimeDilationToFixPlayDuration = 1;
FAutoConsoleVariableRef CVaUseTimeDilationToFixPlayDuration(
	TEXT("Sequencer.Audio.UseTimeDilationToFixPlayDuration"),
	UseTimeDilationToFixPlayDuration,
	TEXT("When set to 1, we will use the effective time dilation to scale the current time of audio. Fixing the audio looping when it shouldn't be.\n"),
	ECVF_Default);

/** Stop audio from playing */
struct FStopAudioPreAnimatedToken : IMovieScenePreAnimatedToken
{
	static FMovieSceneAnimTypeID GetAnimTypeID()
	{
		return TMovieSceneAnimTypeID<FStopAudioPreAnimatedToken>();
	}

	virtual void RestoreState(UObject& InObject, const UE::MovieScene::FRestoreStateParams& Params) override
	{
		UAudioComponent* AudioComponent = CastChecked<UAudioComponent>(&InObject);
		if (AudioComponent)
		{
			AudioComponent->Stop();
			AudioComponent->DestroyComponent();
		}
	}

	struct FProducer : IMovieScenePreAnimatedTokenProducer
	{
		virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const override
		{
			return FStopAudioPreAnimatedToken();
		}
	};
};

/** Destroy a transient audio component */
struct FDestroyAudioPreAnimatedToken : IMovieScenePreAnimatedToken
{
	static FMovieSceneAnimTypeID GetAnimTypeID()
	{
		return TMovieSceneAnimTypeID<FDestroyAudioPreAnimatedToken>();
	}

	virtual void RestoreState(UObject& InObject, const UE::MovieScene::FRestoreStateParams& Params) override
	{
		UAudioComponent* AudioComponent = CastChecked<UAudioComponent>(&InObject);
		if (AudioComponent)
		{
			AudioComponent->DestroyComponent();
		}
	}

	struct FProducer : IMovieScenePreAnimatedTokenProducer
	{
		virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const override
		{
			return FDestroyAudioPreAnimatedToken();
		}
	};
};

struct FCachedAudioTrackData : IPersistentEvaluationData
{
	TMap<FObjectKey, TMap<FObjectKey, TWeakObjectPtr<UAudioComponent>>> AudioComponentsByActorKey;
	
	FCachedAudioTrackData()
	{
		// Create the container for master tracks, which do not have an actor to attach to
		AudioComponentsByActorKey.Add(FObjectKey(), TMap<FObjectKey, TWeakObjectPtr<UAudioComponent>>());
	}

	/** Set whenever we ask the Audio component to start playing a sound. Used to detect desyncs caused when Sequencer evaluates at more-than-real-time. */
	TMap<TWeakObjectPtr<UAudioComponent>, float> SoundLastPlayedAtTime;
	TMap<TWeakObjectPtr<UAudioComponent>, float> LastAudioTime;
	TMap<TWeakObjectPtr<UAudioComponent>, float> LastContextTime;

	UAudioComponent* GetAudioComponent(FObjectKey ActorKey, FObjectKey SectionKey)
	{
		if (TMap<FObjectKey, TWeakObjectPtr<UAudioComponent>>* Map = AudioComponentsByActorKey.Find(ActorKey))
		{
			// First, check for an exact match for this section
			TWeakObjectPtr<UAudioComponent> ExistingComponentPtr = Map->FindRef(SectionKey);
			if (ExistingComponentPtr.IsValid())
			{
				return ExistingComponentPtr.Get();
			}

			// If no exact match, check for any AudioComponent that isn't busy
			for (TPair<FObjectKey, TWeakObjectPtr<UAudioComponent >> Pair : *Map)
			{
				UAudioComponent* ExistingComponent = Map->FindRef(Pair.Key).Get();
				if (ExistingComponent && !ExistingComponent->IsPlaying())
				{
					// Replace this entry with the new SectionKey to claim it
					Map->Remove(Pair.Key);
					Map->Add(SectionKey, ExistingComponent);
					return ExistingComponent;
				}
			}
		}

		return nullptr;
	}

	/** Only to be called on the game thread */
	UAudioComponent* AddAudioComponentForRow(int32 RowIndex, FObjectKey SectionKey, UObject& PrincipalObject, IMovieScenePlayer& Player)
	{
		FObjectKey ObjectKey(&PrincipalObject);
		
		if (!AudioComponentsByActorKey.Contains(ObjectKey))
		{
			AudioComponentsByActorKey.Add(ObjectKey, TMap<FObjectKey, TWeakObjectPtr<UAudioComponent>>());
		}

		UAudioComponent* ExistingComponent = GetAudioComponent(ObjectKey, SectionKey);
		if (!ExistingComponent)
		{
			USoundCue* TempPlaybackAudioCue = NewObject<USoundCue>();

			AActor* Actor = nullptr;
			USceneComponent* SceneComponent = nullptr;
			FString ObjectName;

			if (PrincipalObject.IsA<AActor>())
			{
				Actor = Cast<AActor>(&PrincipalObject);
				SceneComponent = Actor->GetRootComponent();
				ObjectName =
#if WITH_EDITOR
					Actor->GetActorLabel();
#else
					Actor->GetName();
#endif
			}
			else if (PrincipalObject.IsA<UActorComponent>())
			{
				UActorComponent* ActorComponent = Cast<UActorComponent>(&PrincipalObject);
				Actor = ActorComponent->GetOwner();
				SceneComponent = Cast<USceneComponent>(ActorComponent);
				ObjectName = ActorComponent->GetName();
			}

			if (!Actor || !SceneComponent)
			{
				UE_LOG(LogMovieScene, Warning, TEXT("Failed to find scene component for spatialized audio track (row %d)."), RowIndex);
				return nullptr;
			}

			FAudioDevice::FCreateComponentParams Params(Actor->GetWorld(), Actor);
			ExistingComponent = FAudioDevice::CreateComponent(TempPlaybackAudioCue, Params);

			if (!ExistingComponent)
			{
				UE_LOG(LogMovieScene, Warning, TEXT("Failed to create audio component for spatialized audio track (row %d on %s)."), RowIndex, *ObjectName);
				return nullptr;
			}

			Player.SavePreAnimatedState(*ExistingComponent, FMovieSceneAnimTypeID::Unique(), FDestroyAudioPreAnimatedToken::FProducer());

			AudioComponentsByActorKey[ObjectKey].Add(SectionKey, ExistingComponent);

			ExistingComponent->SetFlags(RF_Transient);
			ExistingComponent->AttachToComponent(SceneComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}

		return ExistingComponent;
	}

	/** Only to be called on the game thread */
	UAudioComponent* AddMasterAudioComponentForRow(int32 RowIndex, FObjectKey SectionKey, UWorld* World, IMovieScenePlayer& Player)
	{
		UAudioComponent* ExistingComponent = GetAudioComponent(FObjectKey(), SectionKey);
		if (!ExistingComponent)
		{
			USoundCue* TempPlaybackAudioCue = NewObject<USoundCue>();

			FAudioDevice::FCreateComponentParams Params(World);

			ExistingComponent = FAudioDevice::CreateComponent(TempPlaybackAudioCue, Params);

			if (!ExistingComponent)
			{
				UE_LOG(LogMovieScene, Warning, TEXT("Failed to create audio component for master audio track (row %d)."), RowIndex);
				return nullptr;
			}

			Player.SavePreAnimatedState(*ExistingComponent, FMovieSceneAnimTypeID::Unique(), FDestroyAudioPreAnimatedToken::FProducer());
			
			ExistingComponent->SetFlags(RF_Transient);

			AudioComponentsByActorKey[FObjectKey()].Add(SectionKey, ExistingComponent);
		}

		return ExistingComponent;
	}

	void StopAllSounds()
	{
		for (TPair<FObjectKey, TMap<FObjectKey, TWeakObjectPtr<UAudioComponent>>>& Map : AudioComponentsByActorKey)
		{
			for (TPair<FObjectKey, TWeakObjectPtr<UAudioComponent>>& Pair : Map.Value)
			{
				if (UAudioComponent* AudioComponent = Pair.Value.Get())
				{
					AudioComponent->Stop();
				}
			}
		}
	}

	void StopSoundsOnSection(FObjectKey ObjectKey)
	{
		for (TPair<FObjectKey, TMap<FObjectKey, TWeakObjectPtr<UAudioComponent>>>& Pair : AudioComponentsByActorKey)
		{
			if (UAudioComponent* AudioComponent = Pair.Value.FindRef(ObjectKey).Get())
			{
				AudioComponent->Stop();
			}
		}
	}
};


struct FAudioSectionExecutionToken : IMovieSceneExecutionToken
{
	FAudioSectionExecutionToken(const UMovieSceneAudioSection* InAudioSection)
		: AudioSection(InAudioSection), SectionKey(InAudioSection)
	{}


	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
	{
		FCachedAudioTrackData& TrackData = PersistentData.GetOrAddTrackData<FCachedAudioTrackData>();

		if ((Context.GetStatus() != EMovieScenePlayerStatus::Playing && Context.GetStatus() != EMovieScenePlayerStatus::Scrubbing && Context.GetStatus() != EMovieScenePlayerStatus::Stepping) || Context.HasJumped() || Context.GetDirection() == EPlayDirection::Backwards)
		{
			// stopped, recording, etc
			TrackData.StopAllSounds();
		}

		// Master audio track
		else if (!Operand.ObjectBindingID.IsValid())
		{
			UObject* PlaybackContext = Player.GetPlaybackContext();

			const FMovieSceneActorReferenceData& AttachActorData = AudioSection->GetAttachActorData();

			USceneComponent* AttachComponent = nullptr;
			FMovieSceneActorReferenceKey AttachKey;
			AttachActorData.Evaluate(Context.GetTime(), AttachKey);
			FMovieSceneObjectBindingID AttachBindingID = AttachKey.Object;
			if (AttachBindingID.IsValid())
			{
				// If the transform is set, otherwise use the bound actor's transform
				for (TWeakObjectPtr<> WeakObject : AttachBindingID.ResolveBoundObjects(Operand.SequenceID, Player))
				{
					AActor* AttachActor = Cast<AActor>(WeakObject.Get());
					if (AttachActor)
					{
						AttachComponent = AudioSection->GetAttachComponent(AttachActor, AttachKey);
					}
					if (AttachComponent)
					{
						break;
					}
				}
			}

			UAudioComponent* AudioComponent = TrackData.GetAudioComponent(FObjectKey(), SectionKey);
			if (!AudioComponent)
			{
				// Initialize the sound
				AudioComponent = TrackData.AddMasterAudioComponentForRow(AudioSection->GetRowIndex(), SectionKey, PlaybackContext ? PlaybackContext->GetWorld() : nullptr, Player);

				if (AudioComponent)
				{
					if (AudioSection->GetOnQueueSubtitles().IsBound())
					{
						AudioComponent->OnQueueSubtitles = AudioSection->GetOnQueueSubtitles();
					}
					if (AudioSection->GetOnAudioFinished().IsBound())
					{
						AudioComponent->OnAudioFinished = AudioSection->GetOnAudioFinished();
					}
					if (AudioSection->GetOnAudioPlaybackPercent().IsBound())
					{
						AudioComponent->OnAudioPlaybackPercent = AudioSection->GetOnAudioPlaybackPercent();
					}
				}
			}

			if (AudioComponent)
			{
				if (AttachComponent && (AudioComponent->GetAttachParent() != AttachComponent || AudioComponent->GetAttachSocketName() != AttachKey.SocketName))
				{
					AudioComponent->AttachToComponent(AttachComponent, FAttachmentTransformRules::KeepRelativeTransform, AttachKey.SocketName);
				}

				EnsureAudioIsPlaying(*AudioComponent, PersistentData, Context, AudioComponent->GetAttachParent() != nullptr, Player);
			}
		}

		// Object binding audio track
		else
		{
			for (TWeakObjectPtr<> Object : Player.FindBoundObjects(Operand))
			{
				UAudioComponent* AudioComponent = TrackData.GetAudioComponent(Object.Get(), SectionKey);
				if (!AudioComponent)
				{
					// Initialize the sound
					AudioComponent = TrackData.AddAudioComponentForRow(AudioSection->GetRowIndex(), SectionKey, *Object.Get(), Player);

					if (AudioComponent)
					{
						if (AudioSection->GetOnQueueSubtitles().IsBound())
						{
							AudioComponent->OnQueueSubtitles = AudioSection->GetOnQueueSubtitles();
						}
						if (AudioSection->GetOnAudioFinished().IsBound())
						{
							AudioComponent->OnAudioFinished = AudioSection->GetOnAudioFinished();
						}
						if (AudioSection->GetOnAudioPlaybackPercent().IsBound())
						{
							AudioComponent->OnAudioPlaybackPercent = AudioSection->GetOnAudioPlaybackPercent();
						}
					}
				}

				if (AudioComponent)
				{
					EnsureAudioIsPlaying(*AudioComponent, PersistentData, Context, true, Player);
				}
			}
		}
	}

	void EnsureAudioIsPlaying(UAudioComponent& AudioComponent, FPersistentEvaluationData& PersistentData, const FMovieSceneContext& Context, bool bAllowSpatialization, IMovieScenePlayer& Player) const
	{
		Player.SavePreAnimatedState(AudioComponent, FStopAudioPreAnimatedToken::GetAnimTypeID(), FStopAudioPreAnimatedToken::FProducer());

		float AudioVolume = 1.f;
		AudioSection->GetSoundVolumeChannel().Evaluate(Context.GetTime(), AudioVolume);
		AudioVolume = AudioVolume * AudioSection->EvaluateEasing(Context.GetTime());
		if (AudioComponent.VolumeMultiplier != AudioVolume)
		{
			AudioComponent.SetVolumeMultiplier(AudioVolume);
		}

		float PitchMultiplier = 1.f;
		AudioSection->GetPitchMultiplierChannel().Evaluate(Context.GetTime(), PitchMultiplier);
		if (AudioComponent.PitchMultiplier != PitchMultiplier)
		{
			AudioComponent.SetPitchMultiplier(PitchMultiplier);
		}

		float SectionStartTimeSeconds = (AudioSection->HasStartFrame() ? AudioSection->GetInclusiveStartFrame() : 0) / AudioSection->GetTypedOuter<UMovieScene>()->GetTickResolution();
		// Sounds looping when they shouldn't be with TimeDilation Active
		float InverseTimeDilation = 1.0f; 
		if (UseTimeDilationToFixPlayDuration)
		{
			InverseTimeDilation = (1.0f / Player.GetPlaybackContext()->GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation());
			SectionStartTimeSeconds *= InverseTimeDilation;
		}
		// Sounds looping when they shouldn't be with TimeDilation Active
		FCachedAudioTrackData& TrackData = PersistentData.GetOrAddTrackData<FCachedAudioTrackData>();
		const FFrameNumber AudioStartOffset = AudioSection->GetStartOffset();
		USoundBase* Sound = AudioSection->GetSound();

		// Multiply parts of this by InverseTimeDilation where appropriate.
		float AudioTime = ((Context.GetTime() / Context.GetFrameRate())* (InverseTimeDilation) - SectionStartTimeSeconds + (float)Context.GetFrameRate().AsSeconds(AudioStartOffset));
		if (AudioTime >= 0.f && Sound)
		{
			// Keep track of when we asked this audio clip to play (in game time) so that we can figure out if there's a significant desync in the future.
			if (UseTimeDilationToFixPlayDuration && Player.GetPlaybackContext()->GetWorld())
			{
				// Keep track of initial Audio and Context times when sound started playing.
				// We'll hit this first time audiotime is > 0 (which means its time to start)
				if (!TrackData.LastAudioTime.Contains(&AudioComponent))
				{
					// Add the audio components to trackers
					TrackData.LastAudioTime.Add(&AudioComponent);
					TrackData.LastContextTime.Add(&AudioComponent);
					// Store current audio time and current context time
					TrackData.LastAudioTime[&AudioComponent] = AudioTime;
					TrackData.LastContextTime[&AudioComponent] = Context.GetTime() / Context.GetFrameRate();
				}
				else
				{
					// Get previous AudioTime
					AudioTime = TrackData.LastAudioTime[&AudioComponent] < AudioTime ? TrackData.LastAudioTime[&AudioComponent] : AudioTime;
					float CurrContextTime = (Context.GetTime() / Context.GetFrameRate());
					float PrevContextTime = TrackData.LastContextTime[&AudioComponent];
					PrevContextTime = PrevContextTime < CurrContextTime ? PrevContextTime : CurrContextTime;
					
					// Get Time Delta between previous time in sequencer context and current time, not taking into account dilation. (multiplied by 1/dilation)
					// Add to previous frame's audio time
					AudioTime += (CurrContextTime - PrevContextTime) * (InverseTimeDilation);
					TrackData.LastAudioTime[&AudioComponent] = AudioTime;
					TrackData.LastContextTime[&AudioComponent] = CurrContextTime;
				}
			}

			const float Duration = MovieSceneHelpers::GetSoundDuration(Sound);

			if (!AudioSection->GetLooping() && AudioTime > Duration && Duration != 0.f)
			{
				AudioComponent.Stop();
				TrackData.LastAudioTime.Remove(&AudioComponent);
				TrackData.LastContextTime.Remove(&AudioComponent);
				return;
			}

			// The fmod logic below kind of breaks looping as a sound will not start from 0 anymore
			// and overtime the start just gets further and further.
			// This is especially noticeable when time dilation is turned on
			if (AudioTime > Duration && Duration != 0.0f)
			{
				AudioTime = 0.0f;
			}
			else 
			{
				AudioTime = Duration > 0.f ? FMath::Fmod(AudioTime, Duration) : AudioTime;
			}
		}

		// If the audio component is not playing we (may) need a state change. If the audio component is playing
		// the wrong sound then we need a state change. If the audio playback time is significantly out of sync 
		// with the desired time then we need a state change.
		bool bSoundNeedsStateChange = !AudioComponent.IsPlaying() || AudioComponent.Sound != Sound;
		bool bSoundNeedsTimeSync = false;

		// Sync only if there is no time dilation because otherwise the system will constantly resync because audio 
		// playback is not dilated and will never match the expected playback time.
		const bool bDoTimeSync = 
			Player.GetPlaybackContext()->GetWorld() && 
			(FMath::IsNearlyEqual(Player.GetPlaybackContext()->GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation(), 1.f) ||
			 !bIgnoreAudioSyncDuringWorldTimeDilationCVar);

		if (bDoTimeSync)
		{
			float CurrentGameTime = 0.0f;

			FAudioDevice* AudioDevice = Player.GetPlaybackContext()->GetWorld()->GetAudioDeviceRaw();
			if (UseAudioClockForSequencerDesyncCVar && AudioDevice)
			{
				CurrentGameTime = AudioDevice->GetAudioClock();
			}
			else
			{
				CurrentGameTime = Player.GetPlaybackContext()->GetWorld()->GetAudioTimeSeconds();
			}

			// This tells us how much time has passed in the game world (and thus, reasonably, the audio playback)
			// so if we calculate that we should be playing say, 15s into the section during evaluation, but
			// we're only 5s since the last Play call, then we know we're out of sync. 
			if (TrackData.SoundLastPlayedAtTime.Contains(&AudioComponent))
			{
				float SoundLastPlayedAtTime = TrackData.SoundLastPlayedAtTime[&AudioComponent];

				float GameTimeDelta = CurrentGameTime - SoundLastPlayedAtTime;
				if (!FMath::IsNearlyZero(MaxSequenceAudioDesyncToleranceCVar) && FMath::Abs(GameTimeDelta - AudioTime) > MaxSequenceAudioDesyncToleranceCVar)
				{
					UE_LOG(LogMovieScene, Verbose, TEXT("Audio Component detected a significant mismatch in (assumed) playback time versus the desired time. Time since last play call: %6.2f(s) Desired Time: %6.2f(s). Component: %s sound: %s"), GameTimeDelta, AudioTime, *AudioComponent.GetName(), *GetNameSafe(AudioComponent.Sound));
					bSoundNeedsTimeSync = true;
				}
			}
		}

		if (bSoundNeedsStateChange || bSoundNeedsTimeSync)
		{
			AudioComponent.bAllowSpatialization = bAllowSpatialization;

			if (AudioSection->GetOverrideAttenuation())
			{
				AudioComponent.AttenuationSettings = AudioSection->GetAttenuationSettings();
			}

			// Only call stop on the sound if it is actually playing. This prevents spamming
			// stop calls when a sound cue with a duration of zero is played.
			if (AudioComponent.IsPlaying() || bSoundNeedsTimeSync)
			{
				UE_LOG(LogMovieScene, Verbose, TEXT("Audio Component stopped due to needing a state change bIsPlaying: %d bNeedsTimeSync: %d. Component: %s sound: %s"), AudioComponent.IsPlaying(), bSoundNeedsTimeSync, *AudioComponent.GetName(), *AudioComponent.Sound->GetName());
				AudioComponent.Stop();
			}

			// Only change the sound clip if it has actually changed. This calls Stop internally if needed.
			if (AudioComponent.Sound != Sound)
			{
				UE_LOG(LogMovieScene, Verbose, TEXT("Audio Component calling SetSound due to new sound. Component: %s OldSound: %s NewSound: %s"), *AudioComponent.GetName(), *GetNameSafe(AudioComponent.Sound), *AudioComponent.Sound->GetName());
				AudioComponent.SetSound(Sound);
			}
#if WITH_EDITOR
			UObject* PlaybackContext = Player.GetPlaybackContext();
			UWorld* World = PlaybackContext ? PlaybackContext->GetWorld() : nullptr;
			if (GIsEditor && World != nullptr && !World->IsPlayInEditor())
			{
				AudioComponent.bIsUISound = true;
				AudioComponent.bIsPreviewSound = true;
			}
			else
#endif // WITH_EDITOR
			{
				AudioComponent.bIsUISound = false;
			}

			if (AudioTime >= 0.f)
			{
				UE_LOG(LogMovieScene, Verbose, TEXT("Audio Component Play at Local Time: %6.2f CurrentTime: %6.2f(s) SectionStart: %6.2f(s), SoundDur: %6.2f OffsetIntoClip: %6.2f sound: %s"), AudioTime, (Context.GetTime() / Context.GetFrameRate()), SectionStartTimeSeconds, AudioComponent.Sound->GetDuration(), (float)Context.GetFrameRate().AsSeconds(AudioStartOffset), *AudioComponent.Sound->GetName());
				AudioComponent.Play(AudioTime);

				// Keep track of when we asked this audio clip to play (in game time) so that we can figure out if there's a significant desync in the future.
				if (Player.GetPlaybackContext()->GetWorld())
				{
					if (!TrackData.SoundLastPlayedAtTime.Contains(&AudioComponent))
					{
						TrackData.SoundLastPlayedAtTime.Add(&AudioComponent);
					}

					FAudioDevice* AudioDevice = Player.GetPlaybackContext()->GetWorld()->GetAudioDeviceRaw();
					if (UseAudioClockForSequencerDesyncCVar && AudioDevice)
					{
						TrackData.SoundLastPlayedAtTime[&AudioComponent] = AudioDevice->GetAudioClock() - AudioTime;
					}
					else
					{
						TrackData.SoundLastPlayedAtTime[&AudioComponent] = Player.GetPlaybackContext()->GetWorld()->GetAudioTimeSeconds() - AudioTime;
					}
					
				}
			}

			if (Context.GetStatus() == EMovieScenePlayerStatus::Scrubbing || Context.GetStatus() == EMovieScenePlayerStatus::Stepping)
			{
				// While scrubbing, play the sound for a short time and then cut it.
				AudioComponent.StopDelayed(AudioTrackConstants::ScrubDuration);
			}
		}

		if (bAllowSpatialization)
		{
			if (FAudioDevice* AudioDevice = AudioComponent.GetAudioDevice())
			{
				DECLARE_CYCLE_STAT(TEXT("FAudioThreadTask.MovieSceneUpdateAudioTransform"), STAT_MovieSceneUpdateAudioTransform, STATGROUP_TaskGraphTasks);

				const FTransform ActorTransform = AudioComponent.GetComponentTransform();
				const uint64 ActorComponentID = AudioComponent.GetAudioComponentID();
				FAudioThread::RunCommandOnAudioThread([AudioDevice, ActorComponentID, ActorTransform]()
				{
					if (FActiveSound* ActiveSound = AudioDevice->FindActiveSound(ActorComponentID))
					{
						ActiveSound->bLocationDefined = true;
						ActiveSound->Transform = ActorTransform;
					}
				}, GET_STATID(STAT_MovieSceneUpdateAudioTransform));
			}
		}
	}

	const UMovieSceneAudioSection* AudioSection;
	FObjectKey SectionKey;
};

FMovieSceneAudioSectionTemplate::FMovieSceneAudioSectionTemplate()
	: AudioSection()
{
}

FMovieSceneAudioSectionTemplate::FMovieSceneAudioSectionTemplate(const UMovieSceneAudioSection& Section)
	: AudioSection(&Section)
{
}


void FMovieSceneAudioSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_AudioTrack_Evaluate)

	if (GEngine && GEngine->UseSound() && Context.GetStatus() != EMovieScenePlayerStatus::Jumping)
	{
		ExecutionTokens.Add(FAudioSectionExecutionToken(AudioSection));
	}
}

void FMovieSceneAudioSectionTemplate::TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_AudioTrack_TearDown)

	if (GEngine && GEngine->UseSound())
	{
		FCachedAudioTrackData& TrackData = PersistentData.GetOrAddTrackData<FCachedAudioTrackData>();

		TrackData.StopSoundsOnSection(AudioSection);
	}
}
