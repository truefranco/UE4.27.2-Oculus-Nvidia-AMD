// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "TargetInterfaces/PrimitiveComponentBackedTarget.h"
#include "ToolTargets/ToolTarget.h"

#include "PrimitiveComponentToolTarget.generated.h"

/** 
 * A tool target to share some reusable code for tool targets that are
 * backed by primitive components. 
 */
UCLASS(Transient)
class INTERACTIVETOOLSFRAMEWORK_API UPrimitiveComponentToolTarget : public UToolTarget, public IPrimitiveComponentBackedTarget
{
	GENERATED_BODY()
public:

	// UToolTarget
	virtual bool IsValid() const override;

	// IPrimitiveComponentBackedTarget implementation
	virtual UPrimitiveComponent* GetOwnerComponent() const override;
	virtual AActor* GetOwnerActor() const override;
	virtual void SetOwnerVisibility(bool bVisible) const override;
	virtual FTransform GetWorldTransform() const override;
	virtual bool HitTestComponent(const FRay& WorldRay, FHitResult& OutHit) const override;

	// UObject
	void BeginDestroy() override;

protected:
	friend class UPrimitiveComponentToolTargetFactory;
	
	void InitializeComponent(UPrimitiveComponent* ComponentIn);

	TWeakObjectPtr<UPrimitiveComponent> Component;

private:
#if WITH_EDITOR
	void OnObjectsReplaced(const FCoreUObjectDelegates::FReplacementObjectMap& Map);
#endif
};

/**
 * Factory for UPrimitiveComponentToolTarget to be used by the target manager.
 */
UCLASS(Transient)
class INTERACTIVETOOLSFRAMEWORK_API UPrimitiveComponentToolTargetFactory : public UToolTargetFactory
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) const override;
	virtual UToolTarget* BuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) override;
};

