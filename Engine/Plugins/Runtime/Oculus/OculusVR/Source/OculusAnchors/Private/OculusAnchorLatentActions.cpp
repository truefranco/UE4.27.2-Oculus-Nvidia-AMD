/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusAnchorLatentActions.h"
#include "OculusAnchorsPrivate.h"
#include "OculusHMD.h"

//
// Create Spatial Anchor
//
void UOculusAsyncAction_CreateSpatialAnchor::Activate()
{
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid Target Actor passed to CreateSpatialAnchor latent action."));

		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	EOculusResult::Type Result;
	bool bStartedAsync = OculusAnchors::FOculusAnchors::CreateSpatialAnchor(
		AnchorTransform,
		TargetActor,
		FOculusSpatialAnchorCreateDelegate::CreateUObject(this, &UOculusAsyncAction_CreateSpatialAnchor::HandleCreateComplete),
		&Result
	);

	if (!bStartedAsync)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async OVR Plugin call for CreateSpatialAnchor latent action."));
		Failure.Broadcast(Result);
	}
}

UOculusAsyncAction_CreateSpatialAnchor* UOculusAsyncAction_CreateSpatialAnchor::OculusAsyncCreateSpatialAnchor(AActor* TargetActor, const FTransform& AnchorTransform)
{
	UOculusAsyncAction_CreateSpatialAnchor* Action = NewObject<UOculusAsyncAction_CreateSpatialAnchor>();
	Action->TargetActor = TargetActor;
	Action->AnchorTransform = AnchorTransform;

	if (IsValid(TargetActor))
	{
		Action->RegisterWithGameInstance(TargetActor->GetWorld());
	}
	else
	{
		Action->RegisterWithGameInstance(GWorld);
	}
		
	return Action;
}

void UOculusAsyncAction_CreateSpatialAnchor::HandleCreateComplete(EOculusResult::Type CreateResult, UOculusAnchorComponent* Anchor)
{
	if (UOculusFunctionLibrary::IsResultSuccess(CreateResult))
	{
		Success.Broadcast(Anchor, CreateResult);
	}
	else
	{
		Failure.Broadcast(CreateResult);
	}

	SetReadyToDestroy();
}


//
// Erase Space
//
void UOculusAsyncAction_EraseAnchor::Activate()
{
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid Target Actor passed to EraseSpace latent action."));

		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	UOculusAnchorComponent* AnchorComponent = TargetActor->FindComponentByClass<UOculusAnchorComponent>();
	if (AnchorComponent == nullptr)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("No anchor on actor in EraseSpace latent action."));

		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	EOculusResult::Type Result;
	bool bStartedAsync = OculusAnchors::FOculusAnchors::EraseAnchor(
		AnchorComponent,
		FOculusAnchorEraseDelegate::CreateUObject(this, &UOculusAsyncAction_EraseAnchor::HandleEraseAnchorComplete),
		&Result
	);

	if (!bStartedAsync)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async OVR Plugin call for EraseSpace latent action."));

		Failure.Broadcast(Result);
	}
}

UOculusAsyncAction_EraseAnchor* UOculusAsyncAction_EraseAnchor::OculusAsyncEraseAnchor(AActor* TargetActor)
{
	UOculusAsyncAction_EraseAnchor* Action = NewObject<UOculusAsyncAction_EraseAnchor>();
	Action->TargetActor = TargetActor;

	if (IsValid(TargetActor))
	{
		Action->RegisterWithGameInstance(TargetActor->GetWorld());
	}
	else
	{
		Action->RegisterWithGameInstance(GWorld);
	}

	return Action;
}

void UOculusAsyncAction_EraseAnchor::HandleEraseAnchorComplete(EOculusResult::Type EraseResult, FUUID UUID)
{
	if (UOculusFunctionLibrary::IsResultSuccess(EraseResult))
	{
		Success.Broadcast(TargetActor, UUID, EraseResult);
	}
	else
	{
		Failure.Broadcast(EraseResult);
	}

	SetReadyToDestroy();
}


//
// Save Space
//
void UOculusAsyncAction_SaveAnchor::Activate()
{
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid Target Actor passed to SaveSpace latent action."));

		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	UOculusAnchorComponent* AnchorComponent = TargetActor->FindComponentByClass<UOculusAnchorComponent>();
	if (AnchorComponent == nullptr)
	{
		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	UE_LOG(LogOculusAnchors, Log, TEXT("Attempting to save anchor: %s to location %s"), IsValid(AnchorComponent) ? *AnchorComponent->GetName() : TEXT("INVALID ANCHOR"), *UEnum::GetValueAsString(StorageLocation));

	EOculusResult::Type Result;
	bool bStartedAsync = OculusAnchors::FOculusAnchors::SaveAnchor(
		AnchorComponent,
		StorageLocation,
		FOculusAnchorSaveDelegate::CreateUObject(this, &UOculusAsyncAction_SaveAnchor::HandleSaveAnchorComplete),
		&Result
	);

	if (!bStartedAsync)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async OVR Plugin call for SaveSpace latent action."));
		Failure.Broadcast(Result);
	}
}

UOculusAsyncAction_SaveAnchor* UOculusAsyncAction_SaveAnchor::OculusAsyncSaveAnchor(AActor* TargetActor, EOculusSpaceStorageLocation StorageLocation)
{
	UOculusAsyncAction_SaveAnchor* Action = NewObject<UOculusAsyncAction_SaveAnchor>();
	Action->TargetActor = TargetActor;
	Action->StorageLocation = StorageLocation;
	
	if (IsValid(TargetActor))
	{
		Action->RegisterWithGameInstance(TargetActor->GetWorld());
	}
	else
	{
		Action->RegisterWithGameInstance(GWorld);
	}

	return Action;
}

void UOculusAsyncAction_SaveAnchor::HandleSaveAnchorComplete(EOculusResult::Type SaveResult, UOculusAnchorComponent* Anchor)
{
	if (UOculusFunctionLibrary::IsResultSuccess(SaveResult))
	{
		Success.Broadcast(Anchor, SaveResult);
	}
	else
	{
		Failure.Broadcast(SaveResult);
	}

	SetReadyToDestroy();
}



//
// Save Spaces
//
void UOculusAsyncAction_SaveAnchors::Activate()
{
	if (TargetAnchors.Num() == 0)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Empty Target Actor array passed to SaveSpaces latent action."));

		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	EOculusResult::Type Result;
	bool bStartedAsync = OculusAnchors::FOculusAnchors::SaveAnchorList(
		TargetAnchors,
		StorageLocation,
		FOculusAnchorSaveListDelegate::CreateUObject(this, &UOculusAsyncAction_SaveAnchors::HandleSaveAnchorsComplete),
		&Result
	);

	if (!bStartedAsync)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async OVR Plugin call for SaveSpaceList latent action."));

		Failure.Broadcast(Result);
	}
}

UOculusAsyncAction_SaveAnchors* UOculusAsyncAction_SaveAnchors::OculusAsyncSaveAnchors(const TArray<AActor*>& TargetActors, EOculusSpaceStorageLocation StorageLocation)
{
	UOculusAsyncAction_SaveAnchors* Action = NewObject<UOculusAsyncAction_SaveAnchors>();

	auto ValidActorPtr = TargetActors.FindByPredicate([](AActor* Actor) { return IsValid(Actor); });

	for (auto& it : TargetActors)
	{
		if (!IsValid(it))
		{
			continue;
		}

		UOculusAnchorComponent* AnchorComponent = it->FindComponentByClass<UOculusAnchorComponent>();
		Action->TargetAnchors.Add(AnchorComponent);
	}

	Action->StorageLocation = StorageLocation;

	if (ValidActorPtr != nullptr)
	{
		Action->RegisterWithGameInstance(*ValidActorPtr);
	}
	else
	{
		Action->RegisterWithGameInstance(GWorld);
	}

	return Action;
}

void UOculusAsyncAction_SaveAnchors::HandleSaveAnchorsComplete(EOculusResult::Type SaveResult, const TArray<UOculusAnchorComponent*>& SavedSpaces)
{
	if (UOculusFunctionLibrary::IsResultSuccess(SaveResult))
	{
		Success.Broadcast(SavedSpaces, SaveResult);
	}
	else
	{
		Failure.Broadcast(SaveResult);
	}

	SetReadyToDestroy();
}


//
// Query Spaces
//
void UOculusAsyncAction_QueryAnchors::Activate()
{
	EOculusResult::Type Result;
	bool bStartedAsync = OculusAnchors::FOculusAnchors::QueryAnchorsAdvanced(
		QueryInfo,
		FOculusAnchorQueryDelegate::CreateUObject(this, &UOculusAsyncAction_QueryAnchors::HandleQueryAnchorsResults),
		&Result
	);

	if (!bStartedAsync)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async OVR Plugin call for QuerySpaces latent action."));

		Failure.Broadcast(Result);
	}
}

UOculusAsyncAction_QueryAnchors* UOculusAsyncAction_QueryAnchors::OculusAsyncQueryAnchors(EOculusSpaceStorageLocation Location, const TArray<FUUID>& UUIDs, int32 MaxAnchors)
{
	FOculusSpaceQueryInfo QueryInfo;
	QueryInfo.FilterType = EOculusSpaceQueryFilterType::FilterByIds;
	QueryInfo.IDFilter = UUIDs;
	QueryInfo.Location = Location;
	QueryInfo.MaxQuerySpaces = MaxAnchors;

	UOculusAsyncAction_QueryAnchors* Action = NewObject<UOculusAsyncAction_QueryAnchors>();
	Action->QueryInfo = QueryInfo;

	return Action;
}

UOculusAsyncAction_QueryAnchors* UOculusAsyncAction_QueryAnchors::OculusAsyncQueryAnchorsAdvanced(const FOculusSpaceQueryInfo& QueryInfo)
{
	UOculusAsyncAction_QueryAnchors* Action = NewObject<UOculusAsyncAction_QueryAnchors>();
	Action->QueryInfo = QueryInfo;

	return Action;
}

void UOculusAsyncAction_QueryAnchors::HandleQueryAnchorsResults(EOculusResult::Type QueryResult, const TArray<FOculusSpaceQueryResult>& Results)
{
	QueryResults = Results;

	if (UOculusFunctionLibrary::IsResultSuccess(QueryResult))
	{
		Success.Broadcast(QueryResults, QueryResult);
	}
	else
	{
		Failure.Broadcast(QueryResult);
	}

	SetReadyToDestroy();
}


//
// Set Component Status
//
void UOculusAsyncAction_SetAnchorComponentStatus::Activate()
{
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid Target Actor passed to SetComponentStatus latent action."));

		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	UOculusAnchorComponent* AnchorComponent = TargetActor->FindComponentByClass<UOculusAnchorComponent>();
	if (AnchorComponent == nullptr)
	{
		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	EOculusResult::Type Result;
	bool bStartedAsync = OculusAnchors::FOculusAnchors::SetAnchorComponentStatus(
		AnchorComponent,
		ComponentType,
		bEnabled,
		0,
		FOculusAnchorSetComponentStatusDelegate::CreateUObject(this, &UOculusAsyncAction_SetAnchorComponentStatus::HandleSetComponentStatusComplete),
		&Result
	);

	if (!bStartedAsync)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async OVR Plugin call for SetComponentStatus latent action."));

		Failure.Broadcast(Result);
	}
}

UOculusAsyncAction_SetAnchorComponentStatus* UOculusAsyncAction_SetAnchorComponentStatus::OculusAsyncSetAnchorComponentStatus(AActor* TargetActor, EOculusSpaceComponentType ComponentType, bool bEnabled)
{
	UOculusAsyncAction_SetAnchorComponentStatus* Action = NewObject<UOculusAsyncAction_SetAnchorComponentStatus>();
	Action->TargetActor = TargetActor;
	Action->ComponentType = ComponentType;
	Action->bEnabled = bEnabled;
	
	if (IsValid(TargetActor))
	{
		Action->RegisterWithGameInstance(TargetActor->GetWorld());
	}
	else
	{
		Action->RegisterWithGameInstance(GWorld);
	}

	return Action;
}

void UOculusAsyncAction_SetAnchorComponentStatus::HandleSetComponentStatusComplete(EOculusResult::Type SetStatusResult, UOculusAnchorComponent* Anchor, EOculusSpaceComponentType SpaceComponentType, bool bResultEnabled)
{
	if (UOculusFunctionLibrary::IsResultSuccess(SetStatusResult))
	{
		Success.Broadcast(Anchor, SpaceComponentType, bResultEnabled, SetStatusResult);
	}
	else
	{
		Failure.Broadcast(SetStatusResult);
	}

	SetReadyToDestroy();
}


//
// Share Spaces
//
void UOculusAsyncAction_ShareAnchors::Activate()
{
	if (TargetAnchors.Num() == 0)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Empty Target Actors array passed to ShareSpaces latent action."));

		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	if (ToShareWithIds.Num() == 0)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Empty Target Player IDs array passed to ShareSpaces latent action."));

		Failure.Broadcast(EOculusResult::Failure);
		return;
	}

	EOculusResult::Type Result;
	bool bStartedAsync = OculusAnchors::FOculusAnchors::ShareAnchors(
		TargetAnchors,
		ToShareWithIds,
		FOculusAnchorShareDelegate::CreateUObject(this, &UOculusAsyncAction_ShareAnchors::HandleShareAnchorsComplete),
		&Result
	);

	if (!bStartedAsync)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async OVR Plugin call for ShareSpaces latent action."));

		Failure.Broadcast(Result);
	}
}

UOculusAsyncAction_ShareAnchors* UOculusAsyncAction_ShareAnchors::OculusAsyncShareAnchors(const TArray<AActor*>& TargetActors, const TArray<FString>& ToShareWithIds)
{
	auto ValidActorPtr = TargetActors.FindByPredicate([](AActor* Actor) { return IsValid(Actor); });

	UOculusAsyncAction_ShareAnchors* Action = NewObject<UOculusAsyncAction_ShareAnchors>();
	Action->ToShareWithIds = ToShareWithIds;

	for (auto& it : TargetActors)
	{
		if (!IsValid(it))
		{
			continue;
		}

		UOculusAnchorComponent* AnchorComponent = it->FindComponentByClass<UOculusAnchorComponent>();
		Action->TargetAnchors.Add(AnchorComponent);
	}

	if (ValidActorPtr != nullptr)
	{
		Action->RegisterWithGameInstance(*ValidActorPtr);
	}
	else
	{
		Action->RegisterWithGameInstance(GWorld);
	}

	return Action;
}

void UOculusAsyncAction_ShareAnchors::HandleShareAnchorsComplete(EOculusResult::Type ShareResult, const TArray<UOculusAnchorComponent*>& SharedAnchors, const TArray<FString>& OculusUserIDs)
{
	if (UOculusFunctionLibrary::IsResultSuccess(ShareResult))
	{
		Success.Broadcast(SharedAnchors, OculusUserIDs, ShareResult);
	}
	else
	{
		Failure.Broadcast(ShareResult);
	}

	// Unbind and mark for destruction
	SetReadyToDestroy();
}