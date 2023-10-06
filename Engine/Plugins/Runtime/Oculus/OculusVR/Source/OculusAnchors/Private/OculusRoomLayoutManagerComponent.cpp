/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#include "OculusRoomLayoutManagerComponent.h"
#include "OculusHMD.h"
#include "OculusRoomLayoutManager.h"
#include "OculusAnchorDelegates.h"

UOculusRoomLayoutManagerComponent::UOculusRoomLayoutManagerComponent(const FObjectInitializer& ObjectInitializer)
{
	bWantsInitializeComponent = true; // so that InitializeComponent() gets called
}

void UOculusRoomLayoutManagerComponent::OnRegister()
{
	Super::OnRegister();

	FOculusAnchorEventDelegates::OculusSceneCaptureComplete.AddUObject(this, &UOculusRoomLayoutManagerComponent::OculusRoomLayoutSceneCaptureComplete_Handler);
}

void UOculusRoomLayoutManagerComponent::OnUnregister()
{
	Super::OnUnregister();

	FOculusAnchorEventDelegates::OculusSceneCaptureComplete.RemoveAll(this);
}

void UOculusRoomLayoutManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UOculusRoomLayoutManagerComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

bool UOculusRoomLayoutManagerComponent::LaunchCaptureFlow()
{
	uint64 OutRequest = 0;
	const bool bSuccess = OculusAnchors::FOculusRoomLayoutManager::RequestSceneCapture(OutRequest);
	if (bSuccess)
	{
		EntityRequestList.Add(OutRequest);
	}
	
	return bSuccess;
}

bool UOculusRoomLayoutManagerComponent::GetRoomLayout(FUInt64 Space, FRoomLayout& RoomLayoutOut, int32 MaxWallsCapacity)
{
	if (MaxWallsCapacity <= 0)
	{
		return false;
	}

	FUUID OutCeilingUuid;
	FUUID OutFloorUuid;
	TArray<FUUID> OutWallsUuid;

	const bool bSuccess = OculusAnchors::FOculusRoomLayoutManager::GetSpaceRoomLayout(Space.Value, static_cast<uint32>(MaxWallsCapacity), OutCeilingUuid, OutFloorUuid, OutWallsUuid);

	if (bSuccess)
	{
		RoomLayoutOut.CeilingUuid = OutCeilingUuid;
		
		RoomLayoutOut.FloorUuid = OutFloorUuid;

		RoomLayoutOut.WallsUuid.InsertZeroed(0, OutWallsUuid.Num());
		for (int32 i = 0; i < OutWallsUuid.Num(); ++i)
		{
			RoomLayoutOut.WallsUuid[i]= OutWallsUuid[i];
		}
	}

	return bSuccess;
}