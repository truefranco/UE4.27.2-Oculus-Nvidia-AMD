// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InteractiveTool.h"
#include "ToolTargets/ToolTarget.h"
#include "ComponentSourceInterfaces.h"
#include "SingleSelectionTool.generated.h"

UCLASS(Transient)
class INTERACTIVETOOLSFRAMEWORK_API USingleSelectionTool : public UInteractiveTool
{
GENERATED_BODY()
public:
	void SetSelection(TUniquePtr<FPrimitiveComponentTarget> ComponentTargetIn)
    {
		ComponentTarget = MoveTemp(ComponentTargetIn);
	}
	void SetTarget(UToolTarget* TargetsIn)
	{
		Target = TargetsIn;
	}
	/**
	 * @return the ToolTarget for the Tool
	 */
	virtual UToolTarget* GetTarget()
	{
		return Target;
	}
	/**
	 * @return true if all ComponentTargets of this tool are still valid
	 */
	virtual bool AreAllTargetsValid() const
	{
		if (ComponentTarget != NULL)
		{
			return ComponentTarget ? ComponentTarget->IsValid() : false;
		}
		if (Target != NULL)
		{
			return Target ? Target->IsValid() : false;
		}
		return true;
	}


public:
	virtual bool CanAccept() const override
	{
		return AreAllTargetsValid();
	}

protected:
	TUniquePtr<FPrimitiveComponentTarget> ComponentTarget{};
	UToolTarget* Target{};
};
