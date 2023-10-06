/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusAnchorBPFunctionLibrary.h"
#include "OculusHMD.h"
#include "OculusSpatialAnchorComponent.h"
#include "OculusAnchorsPrivate.h"

AActor* UOculusAnchorBPFunctionLibrary::SpawnActorWithAnchorHandle(UObject* WorldContextObject, FUInt64 Handle, FUUID UUID, EOculusSpaceStorageLocation Location, UClass* ActorClass,
	AActor* Owner, APawn* Instigator, ESpawnActorCollisionHandlingMethod CollisionHandlingMethod)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = Owner;
	SpawnInfo.Instigator = Instigator;
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.SpawnCollisionHandlingOverride = CollisionHandlingMethod;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid WorldContext Object for SpawnActorWithAnchorHandle."));
		return nullptr;
	}

	AActor* NewSpatialAnchorActor = World->SpawnActor(ActorClass, nullptr, nullptr, SpawnInfo);
	if (NewSpatialAnchorActor == nullptr)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to spawn Actor in SpawnActorWithAnchorHandle"));
		return nullptr;
	}

	UOculusSpatialAnchorComponent* SpatialAnchorComponent = NewSpatialAnchorActor->FindComponentByClass<UOculusSpatialAnchorComponent>();
	if (SpatialAnchorComponent == nullptr)
	{
		SpatialAnchorComponent = Cast<UOculusSpatialAnchorComponent>(NewSpatialAnchorActor->AddComponentByClass(UOculusSpatialAnchorComponent::StaticClass(), false, FTransform::Identity, false));
	}

	if (!IsValid(SpatialAnchorComponent))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to find or spawn Spatial Anchor component in SpawnActorWithAnchorHandle"));
		return nullptr;
	}

	SpatialAnchorComponent->SetHandle(Handle);
	SpatialAnchorComponent->SetUUID(UUID);
	SpatialAnchorComponent->SetStoredLocation(Location, true);
	return NewSpatialAnchorActor;
}

AActor* UOculusAnchorBPFunctionLibrary::SpawnActorWithAnchorQueryResults(UObject* WorldContextObject, const FOculusSpaceQueryResult& QueryResult, UClass* ActorClass, AActor* Owner, APawn* Instigator, ESpawnActorCollisionHandlingMethod CollisionHandlingMethod)
{
	return SpawnActorWithAnchorHandle(WorldContextObject, QueryResult.Space, QueryResult.UUID, QueryResult.Location, ActorClass, Owner, Instigator, CollisionHandlingMethod);
}

bool UOculusAnchorBPFunctionLibrary::GetAnchorComponentStatus(AActor* TargetActor, EOculusSpaceComponentType ComponentType, bool& bIsEnabled)
{
	UOculusAnchorComponent* AnchorComponent = Cast<UOculusAnchorComponent>(TargetActor->GetComponentByClass(UOculusAnchorComponent::StaticClass()));

	if (!IsValid(AnchorComponent))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid Anchor Component provided to GetAnchorComponentStatus"));
		bIsEnabled = false;
		return false;
	}

	bool bOutIsEnabled = false;
	bool bIsChangePending = false;

	bool bDidCallStart = OculusAnchors::FOculusAnchors::GetAnchorComponentStatus(AnchorComponent, ComponentType, bOutIsEnabled, bIsChangePending);
	if (!bDidCallStart)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start call to internal GetAnchorComponentStatus"));
		bIsEnabled = false;
		return false;
	}

	bIsEnabled = bOutIsEnabled;
	return bIsEnabled;
}

bool UOculusAnchorBPFunctionLibrary::GetAnchorTransformByHandle(const FUInt64& Handle, FTransform& OutTransform)
{
	OculusHMD::FOculusHMD* OutHMD = GEngine->XRSystem.IsValid() ? (OculusHMD::FOculusHMD*)(GEngine->XRSystem->GetHMDDevice()) : nullptr;
	if (!OutHMD)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Unable to retrieve OculusHMD, cannot calculate anchor transform."));
		return false;
	}

	ovrpTrackingOrigin ovrpOrigin = ovrpTrackingOrigin_EyeLevel;
	const bool bTrackingOriginSuccess = FOculusHMDModule::GetPluginWrapper().GetInitialized() && OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetTrackingOriginType2(&ovrpOrigin));
	if (!bTrackingOriginSuccess)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Unable to get tracking origin, cannot calculate anchor transform."));
		return false;
	}

	const ovrpUInt64 ovrpSpace = Handle.GetValue();
	ovrpPosef ovrpPose;

	const bool bSuccess = FOculusHMDModule::GetPluginWrapper().GetInitialized() && OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().LocateSpace(&ovrpPose, &ovrpSpace, ovrpOrigin));
	if (bSuccess)
	{
		OculusHMD::FPose Pose;
		OutHMD->ConvertPose(ovrpPose, Pose);
		const FTransform trackingToWorld = OutHMD->GetLastTrackingToWorld();

		OutTransform.SetLocation(trackingToWorld.TransformPosition(Pose.Position));
		OutTransform.SetRotation(FRotator(trackingToWorld.TransformRotation(FQuat(Pose.Orientation))).Quaternion());
	}

	return bSuccess;
}

FString UOculusAnchorBPFunctionLibrary::AnchorHandleToString(const FUInt64 Value)
{
	return FString::Printf(TEXT("%llu"), Value.Value);
}

FString UOculusAnchorBPFunctionLibrary::AnchorUUIDToString(const FUUID& Value)
{
	return Value.ToString();
}

FUUID UOculusAnchorBPFunctionLibrary::StringToAnchorUUID(const FString& Value)
{ 
	// Static size for the max length of the string, two chars per hex digit, 16 digits.
	checkf(Value.Len() == 32, TEXT("'%s' is not a valid UUID"), *Value);

	ovrpUuid newID;
	HexToBytes(Value, newID.data);

	return FUUID(newID.data);
}