// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusSceneActor.h"
#include "OculusSceneModule.h"
#include "OculusHMDModule.h"
#include "OculusAnchorManager.h"
#include "OculusDelegates.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMeshActor.h"

#define LOCTEXT_NAMESPACE "OculusSceneActor"

//////////////////////////////////////////////////////////////////////////
// ASceneActor

AOculusSceneActor::AOculusSceneActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ResetStates();

	// Create required components
	RoomLayoutManagerComponent = CreateDefaultSubobject<UOculusRoomLayoutManagerComponent>(TEXT("RoomLayoutManagerComponent"));

	// Following are the semantic labels we want to support default properties for.  User can always add new ones through the properties panel if needed.
	const FString defaultSemanticClassification[] = {
		TEXT("WALL_FACE"),
		TEXT("CEILING"),
		TEXT("FLOOR"),
		TEXT("COUCH"),
		TEXT("DESK"),
		TEXT("DOOR_FRAME"),
		TEXT("WINDOW_FRAME"),
		TEXT("OTHER")
	};

	FSpawnedSceneAnchorProperties spawnedAnchorProps;

	// Populate default UPROPERTY for the "ScenePlaneSpawnedSceneAnchorProperties" member
	spawnedAnchorProps.StaticMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath("/OculusVR/Meshes/ScenePlane.ScenePlane"));
	for (int32 i = 0; i < sizeof(defaultSemanticClassification) / sizeof(defaultSemanticClassification[0]); ++i)
	{
		FSpawnedSceneAnchorProperties& props = ScenePlaneSpawnedSceneAnchorProperties.Add(defaultSemanticClassification[i], spawnedAnchorProps);
		
		// Orientation constraints
		if (defaultSemanticClassification[i] == "CEILING" ||
			defaultSemanticClassification[i] == "FLOOR" ||
			defaultSemanticClassification[i] == "COUCH" ||
			defaultSemanticClassification[i] == "DESK" ||
			defaultSemanticClassification[i] == "DOOR_FRAME" ||
			defaultSemanticClassification[i] == "WINDOW_FRAME" ||
			defaultSemanticClassification[i] == "OTHER")
		{
			props.ForceParallelToFloor = true;
		}
	}

	// Populate default UPROPERTY for the "SceneVolumeSpawnedSceneAnchorProperties" member
	// For the time being, only "OTHER" semantic label is used for volume scene anchors.
	spawnedAnchorProps.StaticMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath("/OculusVR/Meshes/SceneVolume.SceneVolume"));
	FSpawnedSceneAnchorProperties& props = SceneVolumeSpawnedSceneAnchorProperties.Add(TEXT("OTHER"), spawnedAnchorProps);
	props.ForceParallelToFloor = true;
}

void AOculusSceneActor::ResetStates()
{		
	bCaptureFlowWasLaunched = false;
	ClearScene();
}

void AOculusSceneActor::BeginPlay()
{
	Super::BeginPlay();

	// Create a scene component as root so we can attach spawned actors to it
	USceneComponent* rootSceneComponent = NewObject<USceneComponent>(this, USceneComponent::StaticClass());
	rootSceneComponent->SetMobility(EComponentMobility::Static);
	rootSceneComponent->RegisterComponent();
	SetRootComponent(rootSceneComponent);

	// Register delegates
	RoomLayoutManagerComponent->OculusRoomLayoutSceneCaptureCompleteNative.AddUObject(this, &AOculusSceneActor::SceneCaptureComplete_Handler);

	// Make an initial request to query for the room layout if bPopulateSceneOnBeginPlay was set to true
	if (bPopulateSceneOnBeginPlay)
	{
		PopulateScene();
	}
}

void AOculusSceneActor::EndPlay(EEndPlayReason::Type Reason)
{
	// Unregister delegates
	RoomLayoutManagerComponent->OculusRoomLayoutSceneCaptureCompleteNative.RemoveAll(this);

	// Calling ResetStates will reset member variables to their default values (including the request IDs).
	ResetStates();

	Super::EndPlay(Reason);
}

void AOculusSceneActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool AOculusSceneActor::QuerySpatialAnchors(const bool bRoomLayoutOnly)
{
	bool bResult = false;

	FOculusSpaceQueryInfo queryInfo;
	queryInfo.MaxQuerySpaces = MaxQueries;
	queryInfo.FilterType = EOculusSpaceQueryFilterType::FilterByComponentType;

	if (bRoomLayoutOnly)
	{		
		queryInfo.ComponentFilter.Add(EOculusSpaceComponentType::RoomLayout);
		bResult = OculusAnchors::FOculusAnchors::QueryAnchorsAdvanced(queryInfo, FOculusAnchorQueryDelegate::CreateUObject(this, &AOculusSceneActor::AnchorQueryComplete_Handler));
	}	
	else
	{
		queryInfo.ComponentFilter.Add(EOculusSpaceComponentType::ScenePlane);
		bResult = OculusAnchors::FOculusAnchors::QueryAnchorsAdvanced(queryInfo, FOculusAnchorQueryDelegate::CreateUObject(this, &AOculusSceneActor::AnchorQueryComplete_Handler));

		queryInfo.ComponentFilter.Empty();
		queryInfo.ComponentFilter.Add(EOculusSpaceComponentType::SceneVolume);
		bResult &= OculusAnchors::FOculusAnchors::QueryAnchorsAdvanced(queryInfo, FOculusAnchorQueryDelegate::CreateUObject(this, &AOculusSceneActor::AnchorQueryComplete_Handler));
	}

	return bResult;
}

bool AOculusSceneActor::IsValidUuid(const FUUID& Uuid)
{
	return Uuid.UUIDBytes != nullptr;
}

void AOculusSceneActor::LaunchCaptureFlow()
{
	if (RoomLayoutManagerComponent)
	{
		const bool bResult = RoomLayoutManagerComponent->LaunchCaptureFlow();
		if (!bResult)
		{
			UE_LOG(LogOculusScene, Error, TEXT("LaunchCaptureFlow() failed!"));
		}
	}
}

void AOculusSceneActor::LaunchCaptureFlowIfNeeded()
{
	// Depending on LauchCaptureFlowWhenMissingScene, we might not want to launch Capture Flow
	if (LauchCaptureFlowWhenMissingScene != ELaunchCaptureFlowWhenMissingScene::NEVER)
	{
		if (LauchCaptureFlowWhenMissingScene == ELaunchCaptureFlowWhenMissingScene::ALWAYS ||
			(!bCaptureFlowWasLaunched && LauchCaptureFlowWhenMissingScene == ELaunchCaptureFlowWhenMissingScene::ONCE))
		{
			if (bVerboseLog)
			{
				UE_LOG(LogOculusScene, Display, TEXT("Requesting to launch Capture Flow."));
			}
			
			LaunchCaptureFlow();
		}
	}
}

bool AOculusSceneActor::SpawnSceneAnchor(const FUInt64& Space, const FVector& BoundedSize, const TArray<FString>& SemanticClassifications, const EOculusSpaceComponentType AnchorComponentType)
{
	if (Space.Value == 0)
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusSceneActor::SpawnSceneAnchor Invalid Space handle."));
		return false;
	}

	if (!(AnchorComponentType == EOculusSpaceComponentType::ScenePlane || AnchorComponentType == EOculusSpaceComponentType::SceneVolume))
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusSceneActor::SpawnSceneAnchor Anchor doesn't have ScenePlane or SceneVolume component active."));
		return false;
	}

	if (0 == SemanticClassifications.Num())
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusSceneActor::SpawnSceneAnchor No semantic classification found."));
		return false;
	}

	TSoftClassPtr<UOculusSceneAnchorComponent>* sceneAnchorComponentClassPtrRef = nullptr;
	TSoftObjectPtr<UStaticMesh>* staticMeshObjPtrRef = nullptr;

	FSpawnedSceneAnchorProperties* foundProperties = (AnchorComponentType == EOculusSpaceComponentType::ScenePlane) ? ScenePlaneSpawnedSceneAnchorProperties.Find(SemanticClassifications[0]) : SceneVolumeSpawnedSceneAnchorProperties.Find(SemanticClassifications[0]);

	if (!foundProperties)
	{
		UE_LOG(LogOculusScene, Warning, TEXT("AOculusSceneActor::SpawnSceneAnchor Scene object has an unknown semantic label.  Will not be spawned."));
		return false;
	}

	sceneAnchorComponentClassPtrRef = &foundProperties->ActorComponent;
	staticMeshObjPtrRef = &foundProperties->StaticMesh;

	UClass* sceneAnchorComponentInstanceClass = sceneAnchorComponentClassPtrRef->LoadSynchronous();
	if (!sceneAnchorComponentInstanceClass)
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusSceneActor::SpawnSceneAnchor Scene anchor component class is invalid!  Cannot spawn actor to populate the scene."));
		return false;
	}
	
	FActorSpawnParameters actorSpawnParams;
	actorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* newActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(), FRotator(), actorSpawnParams);
	newActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	newActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	
	if (staticMeshObjPtrRef->IsPending())
	{
		staticMeshObjPtrRef->LoadSynchronous();
	}
	newActor->GetStaticMeshComponent()->SetStaticMesh(staticMeshObjPtrRef->Get());
	
	UOculusSceneAnchorComponent* sceneAnchorComponent = NewObject<UOculusSceneAnchorComponent>(newActor, sceneAnchorComponentInstanceClass);
	sceneAnchorComponent->CreationMethod = EComponentCreationMethod::Instance;
	newActor->AddOwnedComponent(sceneAnchorComponent);

	OculusAnchors::FOculusAnchors::SetAnchorComponentStatus(sceneAnchorComponent, EOculusSpaceComponentType::Locatable, true, 0.0f, FOculusAnchorSetComponentStatusDelegate());

	sceneAnchorComponent->RegisterComponent();
	sceneAnchorComponent->InitializeComponent();
	sceneAnchorComponent->Activate();
	sceneAnchorComponent->SetHandle(Space);
	sceneAnchorComponent->SemanticClassifications = SemanticClassifications;
	sceneAnchorComponent->ForceParallelToFloor = foundProperties->ForceParallelToFloor;
	sceneAnchorComponent->AddOffset = foundProperties->AddOffset;

	// Setup scale based on bounded size and the actual size of the mesh
	UStaticMesh* staticMesh = newActor->GetStaticMeshComponent()->GetStaticMesh();
	FBoxSphereBounds staticMeshBounds;
	staticMeshBounds.BoxExtent = FVector{1.f, 1.f, 1.f};
	if (staticMesh)
	{
		staticMeshBounds = staticMesh->GetBounds();
	}
	const float worldToMeters = GetWorld()->GetWorldSettings()->WorldToMeters;

	newActor->SetActorScale3D(FVector(
		(BoundedSize.X / (staticMeshBounds.BoxExtent.X * 2.f)) * worldToMeters,
		(BoundedSize.Y / (staticMeshBounds.BoxExtent.Y * 2.f)) * worldToMeters,
		(BoundedSize.Z / (staticMeshBounds.BoxExtent.Z * 2.f)) * worldToMeters)
	);

	return true;
}

bool AOculusSceneActor::IsScenePopulated()
{
	if (!RootComponent)
		return false;
	return RootComponent->GetNumChildrenComponents() > 0;
}

bool AOculusSceneActor::IsRoomLayoutValid()
{
	return bRoomLayoutIsValid;
}

void AOculusSceneActor::PopulateScene()
{
	if (!RootComponent)
		return;

	if (IsScenePopulated())
	{
		UE_LOG(LogOculusScene, Display, TEXT("PopulateScene Scene is already populated.  Clear it first."));
		return;
	}

	const bool bResult = QuerySpatialAnchors(true);
	if (bResult)
	{
		if (bVerboseLog)
		{
			UE_LOG(LogOculusScene, Display, TEXT("PopulateScene Made a request to query spatial anchors"));
		}
	}
	else
	{
		UE_LOG(LogOculusScene, Error, TEXT("PopulateScene Failed to query spatial anchors!"));
	}
}

void AOculusSceneActor::ClearScene()
{
	if (!RootComponent)
		return;

	TArray<USceneComponent*> childrenComponents = RootComponent->GetAttachChildren();
	for (USceneComponent* SceneComponent : childrenComponents)
	{
		Cast<AActor>(SceneComponent->GetOuter())->Destroy();
	}

	bRoomLayoutIsValid = false;
	bFoundCapturedScene = false;
}

void AOculusSceneActor::SetVisibilityToAllSceneAnchors(const bool bIsVisible)
{
	if (!RootComponent)
		return;

	TArray<USceneComponent*> childrenComponents = RootComponent->GetAttachChildren();
	for (USceneComponent* sceneComponent : childrenComponents)
	{
		sceneComponent->SetVisibility(bIsVisible);
	}
}

void AOculusSceneActor::SetVisibilityToSceneAnchorsBySemanticLabel(const FString SemanticLabel, const bool bIsVisible)
{
	if (!RootComponent)
		return;

	TArray<USceneComponent*> childrenComponents = RootComponent->GetAttachChildren();
	for (USceneComponent* sceneComponent : childrenComponents)
	{
		UObject* outerObject = sceneComponent->GetOuter();
		if (!outerObject)
		{
			continue;
		}

		AActor* outerActor = Cast<AActor>(outerObject);
		if (!outerActor)
		{
			continue;
		}

		UActorComponent* sceneAnchorComponent = outerActor->GetComponentByClass(UOculusSceneAnchorComponent::StaticClass());
		if (!sceneAnchorComponent)
		{
			continue;
		}
		
		if (Cast<UOculusSceneAnchorComponent>(sceneAnchorComponent)->SemanticClassifications.Contains(SemanticLabel))
		{
			sceneComponent->SetVisibility(bIsVisible);
		}
	}
}

TArray<AActor*> AOculusSceneActor::GetActorsBySemanticLabel(const FString SemanticLabel)
{
	TArray<AActor*> actors;

	if (!RootComponent)
		return actors;

	TArray<USceneComponent*> childrenComponents = RootComponent->GetAttachChildren();
	for (USceneComponent* sceneComponent : childrenComponents)
	{
		UObject* outerObject = sceneComponent->GetOuter();
		if (!outerObject)
		{
			continue;
		}

		AActor* outerActor = Cast<AActor>(outerObject);
		if (!outerActor)
		{
			continue;
		}

		UActorComponent* sceneAnchorComponent = outerActor->GetComponentByClass(UOculusSceneAnchorComponent::StaticClass());
		if (!sceneAnchorComponent)
		{
			continue;
		}

		if (Cast<UOculusSceneAnchorComponent>(sceneAnchorComponent)->SemanticClassifications.Contains(SemanticLabel))
		{
			actors.Add(outerActor);
		}
	}

	return actors;
}


// DELEGATE HANDLERS
void AOculusSceneActor::AnchorQueryComplete_Handler(EOculusResult::Type Result, const TArray<FOculusSpaceQueryResult>& Results)
{
	for (auto& QueryResult : Results)
	{
		// Call the existing logic for each result
		SpatialAnchorQueryResult_Handler(0, QueryResult.Space, QueryResult.UUID);
	}

	// Call the complete handler at the end to check if we need to do room layout stuff
	SpatialAnchorQueryComplete_Handler(0, UOculusFunctionLibrary::IsResultSuccess(Result));
}

void AOculusSceneActor::SpatialAnchorQueryResult_Handler(FUInt64 RequestId, FUInt64 Space, FUUID Uuid)
{
	if (bVerboseLog)
	{
		UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler (requestId = %llu, space = %llu, uuid = %s"), RequestId.Value, Space.Value, *BytesToHex(Uuid.UUIDBytes, OCULUS_UUID_SIZE));
	}

	bool bOutPending = false;
	bool bIsRoomLayout = false;

	EOculusResult::Type Result = OculusAnchors::FOculusAnchorManager::GetSpaceComponentStatus(Space.Value, EOculusSpaceComponentType::RoomLayout, bIsRoomLayout, bOutPending);
	bool ResultSuccess = UOculusFunctionLibrary::IsResultSuccess(Result);

	if (ResultSuccess && bIsRoomLayout)
	{
		if (bVerboseLog)
		{
			UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler Found a room layout."));
		}

		// This is a room layout.  We can now populate it with the room layout manager component
		FRoomLayout roomLayout;
		const bool bGetRoomLayoutResult = RoomLayoutManagerComponent->GetRoomLayout(Space, roomLayout, MaxQueries);
		if (bGetRoomLayoutResult)
		{
			// If we get here, then we know that captured scene was already created by the end-user in Capture Flow
			bFoundCapturedScene = true;

			// We can now validate the room
			bRoomLayoutIsValid = true;

			bRoomLayoutIsValid &= IsValidUuid(roomLayout.CeilingUuid);
			bRoomLayoutIsValid &= IsValidUuid(roomLayout.FloorUuid);
			bRoomLayoutIsValid &= roomLayout.WallsUuid.Num() > 3;

			for (int32 i = 0; i < roomLayout.WallsUuid.Num(); ++i)
			{
				bRoomLayoutIsValid &= IsValidUuid(roomLayout.WallsUuid[i]);
			}

			if (bRoomLayoutIsValid || !bEnsureRoomIsValid)
			{
				if (bVerboseLog)
				{
					UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler Room is valid = %d (# walls = %d)."), bRoomLayoutIsValid, roomLayout.WallsUuid.Num());
				}

				// We found a valid room, we can now query all ScenePlane/SceneVolume anchors
				ResultSuccess = QuerySpatialAnchors(false);
				if (ResultSuccess)
				{
					if (bVerboseLog)
					{
						UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler Made a request to query spatial anchors"));
					}
				}
				else
				{
					UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to query spatial anchors!"));
				}
			}
			else
			{
				UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler Room is invalid."));
				ClearScene();
				LaunchCaptureFlowIfNeeded();
			}
		}
	}
	else
	{
		// Is it a ScenePlane anchor?
		bool bIsScenePlane = false;
		Result = OculusAnchors::FOculusAnchorManager::GetSpaceComponentStatus(Space.Value, EOculusSpaceComponentType::ScenePlane, bIsScenePlane, bOutPending);
		ResultSuccess = UOculusFunctionLibrary::IsResultSuccess(Result);

		if (ResultSuccess && bIsScenePlane)
		{
			if (bVerboseLog)
			{
				UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler Found a ScenePlane anchor."));
			}

			FVector scenePlanePos;
			FVector scenePlaneSize;
			ResultSuccess = OculusAnchors::FOculusAnchors::GetSpaceScenePlane(Space.Value, scenePlanePos, scenePlaneSize);
			if (ResultSuccess)
			{
				if (bVerboseLog)
				{
					UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler ScenePlane pos = [%.2f, %.2f, %.2f], size = [%.2f, %.2f, %.2f]."),
						scenePlanePos.X, scenePlanePos.Y, scenePlanePos.Z,
						scenePlaneSize.X, scenePlaneSize.Y, scenePlaneSize.Z);
				}
			}
			else
			{
				UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to get bounds for ScenePlane space."));
			}

			TArray<FString> semanticClassifications;
			ResultSuccess = OculusAnchors::FOculusAnchors::GetSpaceSemanticClassification(Space.Value, semanticClassifications);
			if (ResultSuccess)
			{
				if (bVerboseLog)
				{
					UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler Semantic Classifications:"));
					for (FString& label : semanticClassifications)
					{
						UE_LOG(LogOculusScene, Display, TEXT("%s"), *label);
					}
				}
			}
			else
			{
				UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to get semantic classification space."));
			}

			if (!SpawnSceneAnchor(Space, scenePlaneSize, semanticClassifications, EOculusSpaceComponentType::ScenePlane))
			{
				UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to spawn scene anchor."));
			}
		}
		else
		{
			// Is it a scenevolume anchor?
			bool bIsSceneVolume = false;
			Result = OculusAnchors::FOculusAnchorManager::GetSpaceComponentStatus(Space.Value, EOculusSpaceComponentType::SceneVolume, bIsSceneVolume, bOutPending);
			ResultSuccess = UOculusFunctionLibrary::IsResultSuccess(Result);

			if (ResultSuccess && bIsSceneVolume)
			{
				if (bVerboseLog)
				{
					UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler Found a SceneVolume anchor."));
				}

				FVector sceneVolumePos;
				FVector sceneVolumeSize;
				ResultSuccess = OculusAnchors::FOculusAnchors::GetSpaceSceneVolume(Space.Value, sceneVolumePos, sceneVolumeSize);
				if (ResultSuccess)
				{
					if (bVerboseLog)
					{
						UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler SceneVolume pos = [%.2f, %.2f, %.2f], size = [%.2f, %.2f, %.2f]."),
							sceneVolumePos.X, sceneVolumePos.Y, sceneVolumePos.Z,
							sceneVolumeSize.X, sceneVolumeSize.Y, sceneVolumeSize.Z);
					}
				}
				else
				{
					UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to get bounds for SceneVolume space."));
				}

				TArray<FString> semanticClassifications;
				ResultSuccess = OculusAnchors::FOculusAnchors::GetSpaceSemanticClassification(Space.Value, semanticClassifications);
				if (ResultSuccess)
				{
					if (bVerboseLog)
					{
						UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryResult_Handler Semantic Classifications:"));
						for (FString& label : semanticClassifications)
						{
							UE_LOG(LogOculusScene, Display, TEXT("%s"), *label);
						}
					}
				}
				else
				{
					UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to get semantic classifications space."));
				}

				if (!SpawnSceneAnchor(Space, sceneVolumeSize, semanticClassifications, EOculusSpaceComponentType::SceneVolume))
				{
					UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to spawn scene anchor."));
				}
			}
		}
	}
}

void AOculusSceneActor::SpatialAnchorQueryComplete_Handler(FUInt64 RequestId, bool bResult)
{
	if (bVerboseLog)
	{
		UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryComplete_Handler (requestId = %llu)"), RequestId.Value);
	}

	if (!bFoundCapturedScene) // only try to launch capture flow if a captured scene was not found
	{
		if (!bResult)
		{
			if (bVerboseLog)
			{
				UE_LOG(LogOculusScene, Display, TEXT("SpatialAnchorQueryComplete_Handler No scene found."));
			}
			LaunchCaptureFlowIfNeeded();
		}
	}
}

void AOculusSceneActor::SceneCaptureComplete_Handler(FUInt64 RequestId, bool bResult)
{	
	if (bVerboseLog)
	{
		UE_LOG(LogOculusScene, Display, TEXT("SceneCaptureComplete_Handler (requestId = %llu)"), RequestId);
	}

	if (!bResult)
	{
		UE_LOG(LogOculusScene, Error, TEXT("Scene Capture Complete failed!"));
		return;
	}
	
	// Mark that we already launched Capture Flow and try to query spatial anchors again
	bCaptureFlowWasLaunched = true;

	ClearScene();
	
	const bool bQueryResult = QuerySpatialAnchors(true);
	if (bResult)
	{
		if (bVerboseLog)
		{
			UE_LOG(LogOculusScene, Display, TEXT("Made a request to query spatial anchors"));
		}
	}
	else
	{
		UE_LOG(LogOculusScene, Error, TEXT("Failed to query spatial anchors!"));
	}
}

#undef LOCTEXT_NAMESPACE
