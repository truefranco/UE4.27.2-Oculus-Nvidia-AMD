// Copyright Epic Games, Inc. All Rights Reserved.

#include "ToolContextInterfaces.h"
#include "MeshDescription.h"
#include "InteractiveToolManager.h"
#include "InteractiveGizmoManager.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"



	static bool IsHiddenActor(const AActor* Actor)
	{
#if WITH_EDITOR
		return (Actor->IsHiddenEd());
#else
		return (Actor->IsHidden());
#endif
	}

	static bool IsHiddenComponent(const UPrimitiveComponent* Component)
	{
#if WITH_EDITOR
		return (Component->IsVisibleInEditor() == false);
#else
		return (Component->IsVisible() == false);
#endif
	}


static bool IsVisibleObject(const UPrimitiveComponent* Component)
{
	if (Component == nullptr)
	{
		return false;
	}
	AActor* Actor = Component->GetOwner();
	if (Actor == nullptr)
	{
		return false;
	}

	if (IsHiddenActor(Actor))
	{
		return false;
	}

	if (IsHiddenComponent(Component))
	{
		return false;
	}
	return true;
	}
	





bool FSceneQueryVisibilityFilter::IsVisible(const UPrimitiveComponent* Component) const
{
	if (Component == nullptr)
	{
		return false;
	}

	bool bIsVisible = IsVisibleObject(Component);
	if (bIsVisible || (InvisibleComponentsToInclude != nullptr && InvisibleComponentsToInclude->Contains(Component)))
	{
		return (ComponentsToIgnore == nullptr) || (ComponentsToIgnore->Contains(Component) == false);
	}
	return false;
}






void FSceneHitQueryResult::InitializeHitResult(const FSceneHitQueryRequest& FromRequest)
{
	HitResult = FHitResult(TargetActor, TargetComponent, (FVector)Position, (FVector)Normal);

	HitResult.Distance = static_cast<float>(FromRequest.WorldRay.GetParameter(Position));
	HitResult.FaceIndex = HitTriIndex;
	// initialize .Time? Need start/end for that...
}

UObject* UContextObjectStore::FindContextByClass(UClass* InClass) const
{
	for (UObject* ContextObject : ContextObjects)
	{
		if (ContextObject && ContextObject->IsA(InClass))
		{
			return ContextObject;
		}
	}

	if (UContextObjectStore* ParentStore = Cast<UContextObjectStore>(GetOuter()))
	{
		return ParentStore->FindContextByClass(InClass);
	}

	return nullptr;
}

bool UContextObjectStore::AddContextObject(UObject* InContextObject)
{
	if (InContextObject)
	{
		return ContextObjects.AddUnique(InContextObject) != INDEX_NONE;
	}

	return false;
}

bool UContextObjectStore::RemoveContextObject(UObject* InContextObject)
{
	return ContextObjects.RemoveSingle(InContextObject) != 0;
}

bool UContextObjectStore::RemoveContextObjectsOfType(const UClass* InClass)
{
	return (ContextObjects.RemoveAll([InClass](UObject* InObject)
		{
			return InObject->IsA(InClass);
		}) > 0);
}

void UContextObjectStore::Shutdown()
{
	ContextObjects.Empty();
}

USceneSnappingManager* USceneSnappingManager::Find(UInteractiveToolManager* ToolManager)
{
	if (ensure(ToolManager))
	{
		USceneSnappingManager* Found = ToolManager->GetContextObjectStore()->FindContext<USceneSnappingManager>();
		if (Found != nullptr)
		{
			return Found;
		}
	}
	return nullptr;
}


USceneSnappingManager* USceneSnappingManager::Find(UInteractiveGizmoManager* GizmoManager)
{
	if (ensure(GizmoManager))
	{
		USceneSnappingManager* Found = GizmoManager->GetContextObjectStore()->FindContext<USceneSnappingManager>();
		if (Found != nullptr)
		{
			return Found;
		}
	}
	return nullptr;
}

FGeneratedStaticMeshAssetConfig::~FGeneratedStaticMeshAssetConfig()
{
}