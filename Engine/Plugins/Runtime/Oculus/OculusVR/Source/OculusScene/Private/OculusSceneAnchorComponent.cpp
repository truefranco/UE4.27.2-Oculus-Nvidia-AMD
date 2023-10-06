/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusSceneAnchorComponent.h"

#include "Engine/StaticMeshActor.h"


UOculusSceneAnchorComponent::UOculusSceneAnchorComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bUpdateHeadSpaceTransform = false;
}

void UOculusSceneAnchorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Parent = GetOwner();

	if (Parent == nullptr)
	{
		return;
	}

	if (GetHandle().Value == 0)
	{
		return;
	}

	if (ForceParallelToFloor)
	{
		FRotator rotation = Parent->GetActorRotation();
		FVector angles = rotation.Euler();
		bool isNeg = angles.Y < 0.f;
		angles.Y = 90 * int32((abs(angles.Y) / 90.f) + 0.5f);

		if (isNeg)
		{
			angles.Y *= -1.f;
		}

		rotation = rotation.MakeFromEuler(angles);
		Parent->SetActorRotation(rotation, ETeleportType::ResetPhysics);
	}

	if(!AddOffset.Equals(FVector::ZeroVector))
	{
		const auto ParentActor = Cast<AStaticMeshActor>(Parent);
		if(ParentActor != nullptr)
		{
			const auto SMComponent = ParentActor->GetStaticMeshComponent();
			if (SMComponent != nullptr)
			{
				SMComponent->AddLocalOffset(AddOffset, false, nullptr, ETeleportType::ResetPhysics);
			}
		}	
	}
}