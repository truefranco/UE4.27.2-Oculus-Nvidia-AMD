// Copyright Epic Games, Inc. All Rights Reserved.

#include "BaseGizmos/TransformGizmo.h"
#include "InteractiveGizmoManager.h"
#include "BaseGizmos/AxisPositionGizmo.h"
#include "BaseGizmos/PlanePositionGizmo.h"
#include "BaseGizmos/AxisAngleGizmo.h"
#include "BaseGizmos/GizmoComponents.h"
#include "InteractiveToolsContext.h"
#include "InteractiveToolManager.h"

#include "BaseGizmos/GizmoArrowComponent.h"
#include "BaseGizmos/GizmoRectangleComponent.h"
#include "BaseGizmos/GizmoCircleComponent.h"
#include "BaseGizmos/GizmoBoxComponent.h"
#include "BaseGizmos/GizmoLineHandleComponent.h"
#include "BaseGizmos/RepositionableTransformGizmo.h"
// need this to implement hover
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"


#define LOCTEXT_NAMESPACE "UTransformGizmo"

namespace TransformGizmoLocals
{
	// Looks at a gizmo actor and figures out what sub element flags must have been active when creating it.
	ETransformGizmoSubElements GetSubElementFlagsFromActor(ATransformGizmoActor* GizmoActor)
	{
		ETransformGizmoSubElements Elements = ETransformGizmoSubElements::None;
		if (!GizmoActor)
		{
			return Elements;
		}

		if (GizmoActor->TranslateX) { Elements |= ETransformGizmoSubElements::TranslateAxisX; }
		if (GizmoActor->TranslateY) { Elements |= ETransformGizmoSubElements::TranslateAxisY; }
		if (GizmoActor->TranslateZ) { Elements |= ETransformGizmoSubElements::TranslateAxisZ; }
		if (GizmoActor->TranslateXY) { Elements |= ETransformGizmoSubElements::TranslatePlaneXY; }
		if (GizmoActor->TranslateYZ) { Elements |= ETransformGizmoSubElements::TranslatePlaneYZ; }
		if (GizmoActor->TranslateXZ) { Elements |= ETransformGizmoSubElements::TranslatePlaneXZ; }

		if (GizmoActor->RotateX) { Elements |= ETransformGizmoSubElements::RotateAxisX; }
		if (GizmoActor->RotateY) { Elements |= ETransformGizmoSubElements::RotateAxisY; }
		if (GizmoActor->RotateZ) { Elements |= ETransformGizmoSubElements::RotateAxisZ; }

		if (GizmoActor->AxisScaleX) { Elements |= ETransformGizmoSubElements::ScaleAxisX; }
		if (GizmoActor->AxisScaleY) { Elements |= ETransformGizmoSubElements::ScaleAxisY; }
		if (GizmoActor->AxisScaleZ) { Elements |= ETransformGizmoSubElements::ScaleAxisZ; }
		if (GizmoActor->PlaneScaleXY) { Elements |= ETransformGizmoSubElements::ScalePlaneXY; }
		if (GizmoActor->PlaneScaleYZ) { Elements |= ETransformGizmoSubElements::ScalePlaneYZ; }
		if (GizmoActor->PlaneScaleXZ) { Elements |= ETransformGizmoSubElements::ScalePlaneXZ; }

		if (GizmoActor->UniformScale) { Elements |= ETransformGizmoSubElements::ScaleUniform; }

		return Elements;
	}
}

ATransformGizmoActor::ATransformGizmoActor()
{
	// root component is a hidden sphere
	USphereComponent* SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("GizmoCenter"));
	RootComponent = SphereComponent;
	SphereComponent->InitSphereRadius(1.0f);
	SphereComponent->SetVisibility(false);
	SphereComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}



ATransformGizmoActor* ATransformGizmoActor::ConstructDefault3AxisGizmo(UWorld* World)
{
	return ConstructCustom3AxisGizmo(World,
		ETransformGizmoSubElements::TranslateAllAxes |
		ETransformGizmoSubElements::TranslateAllPlanes |
		ETransformGizmoSubElements::RotateAllAxes |
		ETransformGizmoSubElements::ScaleAllAxes |
		ETransformGizmoSubElements::ScaleAllPlanes |
		ETransformGizmoSubElements::ScaleUniform
	);
}


ATransformGizmoActor* ATransformGizmoActor::ConstructCustom3AxisGizmo(
	UWorld* World,
	ETransformGizmoSubElements Elements)
{
	FActorSpawnParameters SpawnInfo;
	ATransformGizmoActor* NewActor = World->SpawnActor<ATransformGizmoActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

	float GizmoLineThickness = 3.0f;


	auto MakeAxisArrowFunc = [&](const FLinearColor& Color, const FVector& Axis)
	{
		UGizmoArrowComponent* Component = AddDefaultArrowComponent(World, NewActor, Color, Axis, 60.0f);
		Component->Gap = 20.0f;
		Component->Thickness = GizmoLineThickness;
		Component->NotifyExternalPropertyUpdates();
		return Component;
	};
	if ((Elements & ETransformGizmoSubElements::TranslateAxisX) != ETransformGizmoSubElements::None)
	{
		NewActor->TranslateX = MakeAxisArrowFunc(FLinearColor::Red, FVector(1, 0, 0));
	}
	if ((Elements & ETransformGizmoSubElements::TranslateAxisY) != ETransformGizmoSubElements::None)
	{
		NewActor->TranslateY = MakeAxisArrowFunc(FLinearColor::Green, FVector(0, 1, 0));
	}
	if ((Elements & ETransformGizmoSubElements::TranslateAxisZ) != ETransformGizmoSubElements::None)
	{
		NewActor->TranslateZ = MakeAxisArrowFunc(FLinearColor::Blue, FVector(0, 0, 1));
	}


	auto MakePlaneRectFunc = [&](const FLinearColor& Color, const FVector& Axis0, const FVector& Axis1)
	{
		UGizmoRectangleComponent* Component = AddDefaultRectangleComponent(World, NewActor,  Color, Axis0, Axis1);
		Component->LengthX = Component->LengthY = 30.0f;
		Component->SegmentFlags = 0x2 | 0x4;
		Component->Thickness = GizmoLineThickness;
		Component->NotifyExternalPropertyUpdates();
		return Component;
	};
	if ((Elements & ETransformGizmoSubElements::TranslatePlaneYZ) != ETransformGizmoSubElements::None)
	{
		NewActor->TranslateYZ = MakePlaneRectFunc(FLinearColor::Red, FVector(0, 1, 0), FVector(0, 0, 1));
	}
	if ((Elements & ETransformGizmoSubElements::TranslatePlaneXZ) != ETransformGizmoSubElements::None)
	{
		NewActor->TranslateXZ = MakePlaneRectFunc(FLinearColor::Green, FVector(1, 0, 0), FVector(0, 0, 1));
	}
	if ((Elements & ETransformGizmoSubElements::TranslatePlaneXY) != ETransformGizmoSubElements::None)
	{
		NewActor->TranslateXY = MakePlaneRectFunc(FLinearColor::Blue, FVector(1, 0, 0), FVector(0, 1, 0));
	}

	auto MakeAxisRotateCircleFunc = [&](const FLinearColor& Color, const FVector& Axis)
	{
		UGizmoCircleComponent* Component = AddDefaultCircleComponent(World, NewActor,  Color, Axis, 120.0f);
		Component->Thickness = GizmoLineThickness;
		Component->NotifyExternalPropertyUpdates();
		return Component;
	};

	bool bAnyRotate = false;
	if ((Elements & ETransformGizmoSubElements::RotateAxisX) != ETransformGizmoSubElements::None)
	{
		NewActor->RotateX = MakeAxisRotateCircleFunc(FLinearColor::Red, FVector(1, 0, 0));
		bAnyRotate = true;
	}
	if ((Elements & ETransformGizmoSubElements::RotateAxisY) != ETransformGizmoSubElements::None)
	{
		NewActor->RotateY = MakeAxisRotateCircleFunc(FLinearColor::Green, FVector(0, 1, 0));
		bAnyRotate = true;
	}
	if ((Elements & ETransformGizmoSubElements::RotateAxisZ) != ETransformGizmoSubElements::None)
	{
		NewActor->RotateZ = MakeAxisRotateCircleFunc(FLinearColor::Blue, FVector(0, 0, 1));
		bAnyRotate = true;
	}


	// add a non-interactive view-aligned circle element, so the axes look like a sphere.
	if (bAnyRotate)
	{
		UGizmoCircleComponent* SphereEdge = NewObject<UGizmoCircleComponent>(NewActor);
		NewActor->AddInstanceComponent(SphereEdge);
		SphereEdge->AttachToComponent(NewActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		SphereEdge->Color = FLinearColor::Gray;
		SphereEdge->Thickness = 1.0f;
		SphereEdge->Radius = 120.0f;
		SphereEdge->bViewAligned = true;
		SphereEdge->RegisterComponent();
	}



	if ((Elements & ETransformGizmoSubElements::ScaleUniform) != ETransformGizmoSubElements::None)
	{
		float BoxSize = 14.0f;
		UGizmoBoxComponent* ScaleComponent = AddDefaultBoxComponent(World, NewActor, FLinearColor::Black,
			FVector(BoxSize/2, BoxSize/2, BoxSize/2), FVector(BoxSize, BoxSize, BoxSize));
		NewActor->UniformScale = ScaleComponent;
	}



	auto MakeAxisScaleFunc = [&](const FLinearColor& Color, const FVector& Axis0, const FVector& Axis1)
	{
		UGizmoRectangleComponent* ScaleComponent = AddDefaultRectangleComponent(World, NewActor,  Color, Axis0, Axis1);
		ScaleComponent->OffsetX = 140.0f; ScaleComponent->OffsetY = -10.0f;
		ScaleComponent->LengthX = 7.0f; ScaleComponent->LengthY = 20.0f;
		ScaleComponent->Thickness = GizmoLineThickness;
		ScaleComponent->NotifyExternalPropertyUpdates();
		ScaleComponent->SegmentFlags = 0x1 | 0x2 | 0x4; // | 0x8;
		return ScaleComponent;
	};
	if ((Elements & ETransformGizmoSubElements::ScaleAxisX) != ETransformGizmoSubElements::None)
	{
		NewActor->AxisScaleX = MakeAxisScaleFunc(FLinearColor::Red, FVector(1, 0, 0), FVector(0, 0, 1));
	}
	if ((Elements & ETransformGizmoSubElements::ScaleAxisY) != ETransformGizmoSubElements::None)
	{
		NewActor->AxisScaleY = MakeAxisScaleFunc(FLinearColor::Green, FVector(0, 1, 0), FVector(0, 0, 1));
	}
	if ((Elements & ETransformGizmoSubElements::ScaleAxisZ) != ETransformGizmoSubElements::None)
	{
		NewActor->AxisScaleZ = MakeAxisScaleFunc(FLinearColor::Blue, FVector(0, 0, 1), FVector(1, 0, 0));
	}


	auto MakePlaneScaleFunc = [&](const FLinearColor& Color, const FVector& Axis0, const FVector& Axis1)
	{
		UGizmoRectangleComponent* ScaleComponent = AddDefaultRectangleComponent(World, NewActor,  Color, Axis0, Axis1);
		ScaleComponent->OffsetX = ScaleComponent->OffsetY = 120.0f;
		ScaleComponent->LengthX = ScaleComponent->LengthY = 20.0f;
		ScaleComponent->Thickness = GizmoLineThickness;
		ScaleComponent->NotifyExternalPropertyUpdates();
		ScaleComponent->SegmentFlags = 0x2 | 0x4;
		return ScaleComponent;
	};
	if ((Elements & ETransformGizmoSubElements::ScalePlaneYZ) != ETransformGizmoSubElements::None)
	{
		NewActor->PlaneScaleYZ = MakePlaneScaleFunc(FLinearColor::Red, FVector(0, 1, 0), FVector(0, 0, 1));
	}
	if ((Elements & ETransformGizmoSubElements::ScalePlaneXZ) != ETransformGizmoSubElements::None)
	{
		NewActor->PlaneScaleXZ = MakePlaneScaleFunc(FLinearColor::Green, FVector(1, 0, 0), FVector(0, 0, 1));
	}
	if ((Elements & ETransformGizmoSubElements::ScalePlaneXY) != ETransformGizmoSubElements::None)
	{
		NewActor->PlaneScaleXY = MakePlaneScaleFunc(FLinearColor::Blue, FVector(1, 0, 0), FVector(0, 1, 0));
	}


	return NewActor;
}




ATransformGizmoActor* FTransformGizmoActorFactory::CreateNewGizmoActor(UWorld* World) const
{
	return ATransformGizmoActor::ConstructCustom3AxisGizmo(World, EnableElements);
}



UInteractiveGizmo* UTransformGizmoBuilder::BuildGizmo(const FToolBuilderState& SceneState) const
{
	UTransformGizmo* NewGizmo = NewObject<UTransformGizmo>(SceneState.GizmoManager);
	NewGizmo->SetWorld(SceneState.World);

	// use default gizmo actor if client has not given us a new builder
	UGizmoViewContext* GizmoViewContext = SceneState.ToolManager->GetContextObjectStore()->FindContext<UGizmoViewContext>();
	check(GizmoViewContext && GizmoViewContext->IsValidLowLevel());

	NewGizmo->SetGizmoActorBuilder( (GizmoActorBuilder) ? GizmoActorBuilder : MakeShared<FTransformGizmoActorFactory>(GizmoViewContext) );

	NewGizmo->SetSubGizmoBuilderIdentifiers(AxisPositionBuilderIdentifier, PlanePositionBuilderIdentifier, AxisAngleBuilderIdentifier);
	// override default hover function if proposed
	if (UpdateHoverFunction)
	{
		NewGizmo->SetUpdateHoverFunction(UpdateHoverFunction);
	}

	if (UpdateCoordSystemFunction)
	{
		NewGizmo->SetUpdateCoordSystemFunction(UpdateCoordSystemFunction);
	}

	return NewGizmo;
}



void UTransformGizmo::SetWorld(UWorld* WorldIn)
{
	this->World = WorldIn;
}


void UTransformGizmo::SetGizmoActorBuilder(TSharedPtr<FTransformGizmoActorFactory> Builder)
{
	GizmoActorBuilder = Builder;
}

void UTransformGizmo::SetSubGizmoBuilderIdentifiers(FString AxisPositionBuilderIdentifierIn, FString PlanePositionBuilderIdentifierIn, FString AxisAngleBuilderIdentifierIn)
{
	AxisPositionBuilderIdentifier = AxisPositionBuilderIdentifierIn;
	PlanePositionBuilderIdentifier = PlanePositionBuilderIdentifierIn;
	AxisAngleBuilderIdentifier = AxisAngleBuilderIdentifierIn;
}

void UTransformGizmo::SetUpdateHoverFunction(TFunction<void(UPrimitiveComponent*, bool)> HoverFunction)
{
	UpdateHoverFunction = HoverFunction;
}

void UTransformGizmo::SetUpdateCoordSystemFunction(TFunction<void(UPrimitiveComponent*, EToolContextCoordinateSystem)> CoordSysFunction)
{
	UpdateCoordSystemFunction = CoordSysFunction;
}

void UTransformGizmo::SetWorldAlignmentFunctions(
	TUniqueFunction<bool()>&& ShouldAlignTranslationIn,
	TUniqueFunction<bool(const FRay&, FVector&)>&& TranslationAlignmentRayCasterIn)
{
	// Save these so that later changes of gizmo target keep the settings.
	ShouldAlignDestination = MoveTemp(ShouldAlignTranslationIn);
	DestinationAlignmentRayCaster = MoveTemp(TranslationAlignmentRayCasterIn);

	// We allow this function to be called after Setup(), so modify any existing translation/rotation sub gizmos.
	// Unfortunately we keep all the sub gizmos in one list, and the scaling gizmos are differentiated from the
	// translation ones mainly in the components they use. So this ends up being a slightly messy set of checks,
	// but it didn't seem worth keeping a segregated list for something that will only happen once.
	for (UInteractiveGizmo* SubGizmo : this->ActiveGizmos)
	{
		if (UAxisPositionGizmo* CastGizmo = Cast<UAxisPositionGizmo>(SubGizmo))
		{
			if (UGizmoComponentHitTarget* CastHitTarget = Cast<UGizmoComponentHitTarget>(CastGizmo->HitTarget.GetObject()))
			{
				if (CastHitTarget->Component == GizmoActor->TranslateX
					|| CastHitTarget->Component == GizmoActor->TranslateY
					|| CastHitTarget->Component == GizmoActor->TranslateZ)
				{
					CastGizmo->ShouldUseCustomDestinationFunc = [this]() { return ShouldAlignDestination(); };
					CastGizmo->CustomDestinationFunc =
						[this](const UAxisPositionGizmo::FCustomDestinationParams& Params, FVector& OutputPoint) {
						return DestinationAlignmentRayCaster(*Params.WorldRay, OutputPoint);
						};
				}
			}
		}
		if (UPlanePositionGizmo* CastGizmo = Cast<UPlanePositionGizmo>(SubGizmo))
		{
			if (UGizmoComponentHitTarget* CastHitTarget = Cast<UGizmoComponentHitTarget>(CastGizmo->HitTarget.GetObject()))
			{
				if (CastHitTarget->Component == GizmoActor->TranslateXY
					|| CastHitTarget->Component == GizmoActor->TranslateXZ
					|| CastHitTarget->Component == GizmoActor->TranslateYZ)
				{
					CastGizmo->ShouldUseCustomDestinationFunc = [this]() { return ShouldAlignDestination(); };
					CastGizmo->CustomDestinationFunc =
						[this](const UPlanePositionGizmo::FCustomDestinationParams& Params, FVector& OutputPoint) {
						return DestinationAlignmentRayCaster(*Params.WorldRay, OutputPoint);
						};
				}
			}
		}
		if (UAxisAngleGizmo* CastGizmo = Cast<UAxisAngleGizmo>(SubGizmo))
		{
			CastGizmo->ShouldUseCustomDestinationFunc = [this]() { return ShouldAlignDestination(); };
			CastGizmo->CustomDestinationFunc =
				[this](const UAxisAngleGizmo::FCustomDestinationParams& Params, FVector& OutputPoint) {
				return DestinationAlignmentRayCaster(*Params.WorldRay, OutputPoint);
				};
		}
	}
}

void UTransformGizmo::SetDisallowNegativeScaling(bool bDisallow)
{
	if (bDisallowNegativeScaling != bDisallow)
	{
		bDisallowNegativeScaling = bDisallow;
		for (UInteractiveGizmo* SubGizmo : this->ActiveGizmos)
		{
			if (UAxisPositionGizmo* CastGizmo = Cast<UAxisPositionGizmo>(SubGizmo))
			{
				if (UGizmoAxisScaleParameterSource* ParamSource = Cast<UGizmoAxisScaleParameterSource>(CastGizmo->ParameterSource.GetObject()))
				{
					ParamSource->bClampToZero = bDisallow;
				}
			}
			if (UPlanePositionGizmo* CastGizmo = Cast<UPlanePositionGizmo>(SubGizmo))
			{
				if (UGizmoPlaneScaleParameterSource* ParamSource = Cast<UGizmoPlaneScaleParameterSource>(CastGizmo->ParameterSource.GetObject()))
				{
					ParamSource->bClampToZero = bDisallow;
				}
			}
		}
	}
}

void UTransformGizmo::SetIsNonUniformScaleAllowedFunction(TUniqueFunction<bool()>&& IsNonUniformScaleAllowedIn)
{
	IsNonUniformScaleAllowedFunc = MoveTemp(IsNonUniformScaleAllowedIn);
}

void UTransformGizmo::Setup()
{
	UInteractiveGizmo::Setup();

	UpdateHoverFunction = [](UPrimitiveComponent* Component, bool bHovering)
	{
		if (Cast<UGizmoBaseComponent>(Component) != nullptr)
		{
			Cast<UGizmoBaseComponent>(Component)->UpdateHoverState(bHovering);
		}
	};

	UpdateCoordSystemFunction = [](UPrimitiveComponent* Component, EToolContextCoordinateSystem CoordSystem)
	{
		if (Cast<UGizmoBaseComponent>(Component) != nullptr)
		{
			Cast<UGizmoBaseComponent>(Component)->UpdateWorldLocalState(CoordSystem == EToolContextCoordinateSystem::World);
		}
	};

	GizmoActor = GizmoActorBuilder->CreateNewGizmoActor(World);
}



void UTransformGizmo::Shutdown()
{
	ClearActiveTarget();

	if (GizmoActor)
	{
		GizmoActor->Destroy();
		GizmoActor = nullptr;
	}
}



void UTransformGizmo::UpdateCameraAxisSource()
{
	FViewCameraState CameraState;
	GetGizmoManager()->GetContextQueriesAPI()->GetCurrentViewState(CameraState);
	if (CameraAxisSource != nullptr && GizmoActor != nullptr)
	{
		CameraAxisSource->Origin = GizmoActor->GetTransform().GetLocation();
		CameraAxisSource->Direction = -CameraState.Forward();
		CameraAxisSource->TangentX = CameraState.Right();
		CameraAxisSource->TangentY = CameraState.Up();
	}
}


void UTransformGizmo::Tick(float DeltaTime)
{	
	if (bUseContextCoordinateSystem)
	{
		CurrentCoordinateSystem = GetGizmoManager()->GetContextQueriesAPI()->GetCurrentCoordinateSystem();
	}
	
	check(CurrentCoordinateSystem == EToolContextCoordinateSystem::World || CurrentCoordinateSystem == EToolContextCoordinateSystem::Local)
	bool bUseLocalAxes = (CurrentCoordinateSystem == EToolContextCoordinateSystem::Local);

	if (AxisXSource != nullptr && AxisYSource != nullptr && AxisZSource != nullptr)
	{
		AxisXSource->bLocalAxes = bUseLocalAxes;
		AxisYSource->bLocalAxes = bUseLocalAxes;
		AxisZSource->bLocalAxes = bUseLocalAxes;
	}
	if (UpdateCoordSystemFunction)
	{
		for (UPrimitiveComponent* Component : ActiveComponents)
		{
			UpdateCoordSystemFunction(Component, CurrentCoordinateSystem);
		}
	}

	// apply dynamic visibility filtering to sub-gizmos

	auto SetSubGizmoTypeVisibility = [this](TArray<FSubGizmoInfo>& GizmoInfos, bool bVisible)
		{
			for (FSubGizmoInfo& GizmoInfo : GizmoInfos)
			{
				if (GizmoInfo.Component.IsValid())
				{
					GizmoInfo.Component->SetVisibility(bVisible);
				}
			}
		};

	if (bUseContextGizmoMode)
	{
		ActiveGizmoMode = GetGizmoManager()->GetContextQueriesAPI()->GetCurrentTransformGizmoMode();
	}
	EToolContextTransformGizmoMode UseGizmoMode = ActiveGizmoMode;

	bool bShouldShowTranslation =
		(UseGizmoMode == EToolContextTransformGizmoMode::Combined || UseGizmoMode == EToolContextTransformGizmoMode::Translation);
	bool bShouldShowRotation =
		(UseGizmoMode == EToolContextTransformGizmoMode::Combined || UseGizmoMode == EToolContextTransformGizmoMode::Rotation);
	bool bShouldShowUniformScale =
		(UseGizmoMode == EToolContextTransformGizmoMode::Combined || UseGizmoMode == EToolContextTransformGizmoMode::Scale);
	bool bShouldShowNonUniformScale =
		(UseGizmoMode == EToolContextTransformGizmoMode::Combined || UseGizmoMode == EToolContextTransformGizmoMode::Scale)
		&& IsNonUniformScaleAllowedFunc();

	SetSubGizmoTypeVisibility(TranslationSubGizmos, bShouldShowTranslation);
	SetSubGizmoTypeVisibility(RotationSubGizmos, bShouldShowRotation);
	SetSubGizmoTypeVisibility(UniformScaleSubGizmos, bShouldShowUniformScale);
	SetSubGizmoTypeVisibility(NonUniformScaleSubGizmos, bShouldShowNonUniformScale);

	for (UPrimitiveComponent* Component : NonuniformScaleComponents)
	{
		Component->SetVisibility(bUseLocalAxes);
	}

	UpdateCameraAxisSource();
}



void UTransformGizmo::SetActiveTarget(UTransformProxy* Target, IToolContextTransactionProvider* TransactionProvider)
{
	if (ActiveTarget != nullptr)
	{
		ClearActiveTarget();
	}

	ActiveTarget = Target;

	// move gizmo to target location
	USceneComponent* GizmoComponent = GizmoActor->GetRootComponent();

	FTransform TargetTransform = Target->GetTransform();
	FTransform GizmoTransform = TargetTransform;
	GizmoTransform.SetScale3D(FVector(1, 1, 1));
	GizmoComponent->SetWorldTransform(GizmoTransform);

	// save current scale because gizmo is not scaled
	SeparateChildScale = TargetTransform.GetScale3D();

	UGizmoComponentWorldTransformSource* ComponentTransformSource =
		UGizmoComponentWorldTransformSource::Construct(GizmoComponent, this);
	FSeparateScaleProvider ScaleProvider = {
		[this]() { return this->SeparateChildScale; },
		[this](FVector Scale) { this->SeparateChildScale = Scale; }
	};
	ScaledTransformSource = UGizmoScaledTransformSource::Construct(ComponentTransformSource, ScaleProvider, this);

	// Target tracks location of GizmoComponent. Note that TransformUpdated is not called during undo/redo transactions!
	// We currently rely on the transaction system to undo/redo target object locations. This will not work during runtime...
	GizmoComponent->TransformUpdated.AddLambda(
		[this](USceneComponent* Component, EUpdateTransformFlags /*UpdateTransformFlags*/, ETeleportType /*Teleport*/) {
		//FTransform NewXForm = Component->GetComponentToWorld();
		//NewXForm.SetScale3D(this->CurTargetScale);
		FTransform NewXForm = ScaledTransformSource->GetTransform();
		this->ActiveTarget->SetTransform(NewXForm);
	});
	ScaledTransformSource->OnTransformChanged.AddLambda(
		[this](IGizmoTransformSource* Source)
	{
		FTransform NewXForm = ScaledTransformSource->GetTransform();
		this->ActiveTarget->SetTransform(NewXForm);
	});


	// This state target emits an explicit FChange that moves the GizmoActor root component during undo/redo.
	// It also opens/closes the Transaction that saves/restores the target object locations.
	if (TransactionProvider == nullptr)
	{
		TransactionProvider = GetGizmoManager();
	}
	StateTarget = UGizmoTransformChangeStateTarget::Construct(GizmoComponent,
		LOCTEXT("UTransformGizmoTransaction", "Transform"), TransactionProvider, this);
	StateTarget->DependentChangeSources.Add(MakeUnique<FTransformProxyChangeSource>(Target));
	StateTarget->ExternalDependentChangeSources.Add(this);

	CameraAxisSource = NewObject<UGizmoConstantFrameAxisSource>(this);

	// root component provides local X/Y/Z axis, identified by AxisIndex
	AxisXSource = UGizmoComponentAxisSource::Construct(GizmoComponent, 0, true, this);
	AxisYSource = UGizmoComponentAxisSource::Construct(GizmoComponent, 1, true, this);
	AxisZSource = UGizmoComponentAxisSource::Construct(GizmoComponent, 2, true, this);

	// todo should we hold onto these?
	if (GizmoActor->TranslateX != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddAxisTranslationGizmo(GizmoActor->TranslateX, GizmoComponent, AxisXSource, ScaledTransformSource, StateTarget, 0);
		ActiveComponents.Add(GizmoActor->TranslateX);
		TranslationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->TranslateX, NewGizmo });
	}
	if (GizmoActor->TranslateY != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddAxisTranslationGizmo(GizmoActor->TranslateY, GizmoComponent, AxisYSource, ScaledTransformSource, StateTarget, 1);
		ActiveComponents.Add(GizmoActor->TranslateY);
		TranslationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->TranslateY, NewGizmo });
	}
	if (GizmoActor->TranslateZ != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddAxisTranslationGizmo(GizmoActor->TranslateZ, GizmoComponent, AxisZSource, ScaledTransformSource, StateTarget, 2);
		ActiveComponents.Add(GizmoActor->TranslateZ);
		TranslationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->TranslateZ, NewGizmo });
	}


	if (GizmoActor->TranslateYZ != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddPlaneTranslationGizmo(GizmoActor->TranslateYZ, GizmoComponent, AxisXSource, ScaledTransformSource, StateTarget, 1, 2);
		ActiveComponents.Add(GizmoActor->TranslateYZ);
		TranslationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->TranslateYZ, NewGizmo });
	}
	if (GizmoActor->TranslateXZ != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddPlaneTranslationGizmo(GizmoActor->TranslateXZ, GizmoComponent, AxisYSource, ScaledTransformSource, StateTarget, 2, 0);
		ActiveComponents.Add(GizmoActor->TranslateXZ);
		TranslationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->TranslateXZ, NewGizmo });
	}
	if (GizmoActor->TranslateXY != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddPlaneTranslationGizmo(GizmoActor->TranslateXY, GizmoComponent, AxisZSource, ScaledTransformSource, StateTarget, 0, 1);
		ActiveComponents.Add(GizmoActor->TranslateXY);
		TranslationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->TranslateXY, NewGizmo });
	}

	if (GizmoActor->RotateX != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddAxisRotationGizmo(GizmoActor->RotateX, GizmoComponent, AxisXSource, ScaledTransformSource, StateTarget);
		ActiveComponents.Add(GizmoActor->RotateX);
		RotationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->RotateX, NewGizmo });
	}
	if (GizmoActor->RotateY != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddAxisRotationGizmo(GizmoActor->RotateY, GizmoComponent, AxisYSource, ScaledTransformSource, StateTarget);
		ActiveComponents.Add(GizmoActor->RotateY);
		RotationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->RotateY, NewGizmo });
	}
	if (GizmoActor->RotateZ != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddAxisRotationGizmo(GizmoActor->RotateZ, GizmoComponent, AxisZSource, ScaledTransformSource, StateTarget);
		ActiveComponents.Add(GizmoActor->RotateZ);
		RotationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->RotateZ, NewGizmo });
	}
	if (GizmoActor->RotationSphere != nullptr)
	{
		// no gizmo for the sphere currently
		ActiveComponents.Add(GizmoActor->RotationSphere);
		RotationSubGizmos.Add(FSubGizmoInfo{ GizmoActor->RotationSphere, nullptr });
	}

    // only need these if scaling enabled. Essentially these are just the unit axes, regardless
	// of what 3D axis is in use, we will tell the ParameterSource-to-3D-Scale mapper to
	// use the coordinate axes
	UnitAxisXSource = UGizmoComponentAxisSource::Construct(GizmoComponent, 0, false, this);
	UnitAxisYSource = UGizmoComponentAxisSource::Construct(GizmoComponent, 1, false, this);
	UnitAxisZSource = UGizmoComponentAxisSource::Construct(GizmoComponent, 2, false, this);

	if (GizmoActor->UniformScale != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddUniformScaleGizmo(GizmoActor->UniformScale, GizmoComponent, CameraAxisSource, CameraAxisSource, ScaledTransformSource, StateTarget);
		ActiveComponents.Add(GizmoActor->UniformScale);
		UniformScaleSubGizmos.Add(FSubGizmoInfo{ GizmoActor->UniformScale, NewGizmo });
	}

	if (GizmoActor->AxisScaleX != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddAxisScaleGizmo(GizmoActor->AxisScaleX, GizmoComponent, AxisXSource, UnitAxisXSource, ScaledTransformSource, StateTarget);
		ActiveComponents.Add(GizmoActor->AxisScaleX);
		NonUniformScaleSubGizmos.Add(FSubGizmoInfo{ GizmoActor->AxisScaleX, NewGizmo });
	}
	if (GizmoActor->AxisScaleY != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddAxisScaleGizmo(GizmoActor->AxisScaleY, GizmoComponent, AxisYSource, UnitAxisYSource, ScaledTransformSource, StateTarget);
		ActiveComponents.Add(GizmoActor->AxisScaleY);
		NonUniformScaleSubGizmos.Add(FSubGizmoInfo{ GizmoActor->AxisScaleY, NewGizmo });
	}
	if (GizmoActor->AxisScaleZ != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddAxisScaleGizmo(GizmoActor->AxisScaleZ, GizmoComponent, AxisZSource, UnitAxisZSource, ScaledTransformSource, StateTarget);
		ActiveComponents.Add(GizmoActor->AxisScaleZ);
		NonUniformScaleSubGizmos.Add(FSubGizmoInfo{ GizmoActor->AxisScaleZ, NewGizmo });
	}

	if (GizmoActor->PlaneScaleYZ != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddPlaneScaleGizmo(GizmoActor->PlaneScaleYZ, GizmoComponent, AxisXSource, UnitAxisXSource, ScaledTransformSource, StateTarget);
		ActiveComponents.Add(GizmoActor->PlaneScaleYZ);
		NonUniformScaleSubGizmos.Add(FSubGizmoInfo{ GizmoActor->PlaneScaleYZ, NewGizmo });
	}
	if (GizmoActor->PlaneScaleXZ != nullptr)
	{
		UPlanePositionGizmo* Gizmo = (UPlanePositionGizmo *)AddPlaneScaleGizmo(GizmoActor->PlaneScaleXZ, GizmoComponent, AxisYSource, UnitAxisYSource, ScaledTransformSource, StateTarget);
		Gizmo->bFlipX = true;		// unclear why this is necessary...possibly a handedness issue?
		ActiveComponents.Add(GizmoActor->PlaneScaleXZ);
		NonUniformScaleSubGizmos.Add(FSubGizmoInfo{ GizmoActor->PlaneScaleXZ, Gizmo });
	}
	if (GizmoActor->PlaneScaleXY != nullptr)
	{
		UInteractiveGizmo* NewGizmo = AddPlaneScaleGizmo(GizmoActor->PlaneScaleXY, GizmoComponent, AxisZSource, UnitAxisZSource, ScaledTransformSource, StateTarget);
		ActiveComponents.Add(GizmoActor->PlaneScaleXY);
		NonUniformScaleSubGizmos.Add(FSubGizmoInfo{ GizmoActor->PlaneScaleXY, NewGizmo });
	}

	OnSetActiveTarget.Broadcast(this, ActiveTarget);
}

FTransform UTransformGizmo::GetGizmoTransform() const
{
	USceneComponent* GizmoComponent = GizmoActor->GetRootComponent();
	return GizmoComponent->GetComponentTransform();
}

void UTransformGizmo::ReinitializeGizmoTransform(const FTransform& NewTransform, bool bKeepGizmoUnscaled)
{
	// To update the gizmo location without triggering any callbacks, we temporarily
	// store a copy of the callback list, detach them, reposition, and then reattach
	// the callbacks.
	USceneComponent* GizmoComponent = GizmoActor->GetRootComponent();
	auto temp = GizmoComponent->TransformUpdated;
	GizmoComponent->TransformUpdated.Clear();
	FTransform GizmoTransform = NewTransform;
	if (bKeepGizmoUnscaled)
	{
		GizmoTransform.SetScale3D(FVector(1, 1, 1));
	}
	GizmoComponent->SetWorldTransform(NewTransform);
	GizmoComponent->TransformUpdated = temp;

	// The underlying proxy has an existing way to reinitialize its transform without callbacks.
	bool bSavedSetPivotMode = ActiveTarget->bSetPivotMode;
	ActiveTarget->bSetPivotMode = true;
	ActiveTarget->SetTransform(NewTransform);
	ActiveTarget->bSetPivotMode = false;
}

void UTransformGizmo::SetNewGizmoTransform(const FTransform& NewTransform, bool bKeepGizmoUnscaled)
{
	check(ActiveTarget != nullptr);

	BeginTransformEditSequence();
	UpdateTransformDuringEditSequence(NewTransform, bKeepGizmoUnscaled);
	EndTransformEditSequence();
}

void UTransformGizmo::BeginTransformEditSequence()
{
	if (ensure(StateTarget))
	{
		StateTarget->BeginUpdate();
	}
}

void UTransformGizmo::EndTransformEditSequence()
{
	if (ensure(StateTarget))
	{
		StateTarget->EndUpdate();
	}
}

void UTransformGizmo::UpdateTransformDuringEditSequence(const FTransform& NewTransform, bool bKeepGizmoUnscaled)
{
	check(ActiveTarget != nullptr);

	USceneComponent* GizmoComponent = GizmoActor->GetRootComponent();
	FTransform GizmoTransform = NewTransform;
	if (bKeepGizmoUnscaled)
	{
		GizmoTransform.SetScale3D(FVector(1, 1, 1));
	}
	GizmoComponent->SetWorldTransform(GizmoTransform);
	ActiveTarget->SetTransform(NewTransform);
}

void UTransformGizmo::SetNewChildScale(const FVector& NewChildScale)
{
	//SeparateChildScale = NewChildScale;
	FTransform NewTransform = ActiveTarget->GetTransform();
	NewTransform.SetScale3D(NewChildScale);

	bool bSavedSetPivotMode = ActiveTarget->bSetPivotMode;
	ActiveTarget->bSetPivotMode = true;
	ActiveTarget->SetTransform(NewTransform);
	ActiveTarget->bSetPivotMode = bSavedSetPivotMode;
}


void UTransformGizmo::SetVisibility(bool bVisible)
{
	bool bPreviousVisibility = !GizmoActor->IsHidden();

	GizmoActor->SetActorHiddenInGame(bVisible == false);
#if WITH_EDITOR
	GizmoActor->SetIsTemporarilyHiddenInEditor(bVisible == false);
#endif
	if (bPreviousVisibility != bVisible)
	{
		OnVisibilityChanged.Broadcast(this, bVisible);
	}
}

void UTransformGizmo::SetDisplaySpaceTransform(TOptional<FTransform> TransformIn)
{
	if (DisplaySpaceTransform.IsSet() != TransformIn.IsSet()
		|| (TransformIn.IsSet() && !TransformIn.GetValue().Equals(DisplaySpaceTransform.GetValue())))
	{
		DisplaySpaceTransform = TransformIn;
		OnDisplaySpaceTransformChanged.Broadcast(this, TransformIn);
	}
}

ETransformGizmoSubElements UTransformGizmo::GetGizmoElements()
{
	using namespace TransformGizmoLocals;

	return GetSubElementFlagsFromActor(GizmoActor);
}


UInteractiveGizmo* UTransformGizmo::AddAxisTranslationGizmo(
	UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
	IGizmoAxisSource* AxisSource,
	IGizmoTransformSource* TransformSource,
	IGizmoStateTarget* StateTargetIn,
	int AxisIndex)
{
	// create axis-position gizmo, axis-position parameter will drive translation
	UAxisPositionGizmo* TranslateGizmo = Cast<UAxisPositionGizmo>(GetGizmoManager()->CreateGizmo(
		UInteractiveGizmoManager::DefaultAxisPositionBuilderIdentifier));
	check(TranslateGizmo);

	// axis source provides the translation axis
	TranslateGizmo->AxisSource = Cast<UObject>(AxisSource);

	// parameter source maps axis-parameter-change to translation of TransformSource's transform
	UGizmoAxisTranslationParameterSource* ParamSource = UGizmoAxisTranslationParameterSource::Construct(AxisSource, TransformSource, this);
	ParamSource->PositionConstraintFunction = [this](const FVector& Pos, FVector& Snapped) { return PositionSnapFunction(Pos, Snapped); };
	ParamSource->AxisDeltaConstraintFunction = [this, AxisIndex](double AxisDelta, double& SnappedAxisDelta) { return PositionAxisDeltaSnapFunction(AxisDelta, SnappedAxisDelta, AxisIndex); };
	TranslateGizmo->ParameterSource = ParamSource;

	// sub-component provides hit target
	UGizmoComponentHitTarget* HitTarget = UGizmoComponentHitTarget::Construct(AxisComponent, this);
	if (this->UpdateHoverFunction)
	{
		HitTarget->UpdateHoverFunction = [AxisComponent, this](bool bHovering) { this->UpdateHoverFunction(AxisComponent, bHovering); };
	}
	TranslateGizmo->HitTarget = HitTarget;

	TranslateGizmo->StateTarget = Cast<UObject>(StateTargetIn);

	TranslateGizmo->ShouldUseCustomDestinationFunc = [this]() { return ShouldAlignDestination(); };
	TranslateGizmo->CustomDestinationFunc =
		[this](const UAxisPositionGizmo::FCustomDestinationParams& Params, FVector& OutputPoint) {
		return DestinationAlignmentRayCaster(*Params.WorldRay, OutputPoint);
		};

	ActiveGizmos.Add(TranslateGizmo);
	return TranslateGizmo;
}



UInteractiveGizmo* UTransformGizmo::AddPlaneTranslationGizmo(
	UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
	IGizmoAxisSource* AxisSource,
	IGizmoTransformSource* TransformSource,
	IGizmoStateTarget* StateTargetIn,
	int XAxisIndex, int YAxisIndex)
{
	// create axis-position gizmo, axis-position parameter will drive translation
	UPlanePositionGizmo* TranslateGizmo = Cast<UPlanePositionGizmo>(GetGizmoManager()->CreateGizmo(
		UInteractiveGizmoManager::DefaultPlanePositionBuilderIdentifier));
	check(TranslateGizmo);

	// axis source provides the translation axis
	TranslateGizmo->AxisSource = Cast<UObject>(AxisSource);

	// parameter source maps axis-parameter-change to translation of TransformSource's transform
	UGizmoPlaneTranslationParameterSource* ParamSource = UGizmoPlaneTranslationParameterSource::Construct(AxisSource, TransformSource, this);
	ParamSource->PositionConstraintFunction = [this](const FVector& Pos, FVector& Snapped) { return PositionSnapFunction(Pos, Snapped); };
	ParamSource->AxisXDeltaConstraintFunction = [this, XAxisIndex](double AxisDelta, double& SnappedAxisDelta) { return PositionAxisDeltaSnapFunction(AxisDelta, SnappedAxisDelta, XAxisIndex); };
	ParamSource->AxisYDeltaConstraintFunction = [this, YAxisIndex](double AxisDelta, double& SnappedAxisDelta) { return PositionAxisDeltaSnapFunction(AxisDelta, SnappedAxisDelta, YAxisIndex); };
	TranslateGizmo->ParameterSource = ParamSource;

	// sub-component provides hit target
	UGizmoComponentHitTarget* HitTarget = UGizmoComponentHitTarget::Construct(AxisComponent, this);
	if (this->UpdateHoverFunction)
	{
		HitTarget->UpdateHoverFunction = [AxisComponent, this](bool bHovering) { this->UpdateHoverFunction(AxisComponent, bHovering); };
	}
	TranslateGizmo->HitTarget = HitTarget;

	TranslateGizmo->StateTarget = Cast<UObject>(StateTargetIn);

	TranslateGizmo->ShouldUseCustomDestinationFunc = [this]() { return ShouldAlignDestination(); };
	TranslateGizmo->CustomDestinationFunc =
		[this](const UPlanePositionGizmo::FCustomDestinationParams& Params, FVector& OutputPoint) {
		return DestinationAlignmentRayCaster(*Params.WorldRay, OutputPoint);
		};

	ActiveGizmos.Add(TranslateGizmo);
	return TranslateGizmo;
}





UInteractiveGizmo* UTransformGizmo::AddAxisRotationGizmo(
	UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
	IGizmoAxisSource* AxisSource,
	IGizmoTransformSource* TransformSource,
	IGizmoStateTarget* StateTargetIn)
{
	// create axis-angle gizmo, angle will drive axis-rotation
	UAxisAngleGizmo* RotateGizmo = Cast<UAxisAngleGizmo>(GetGizmoManager()->CreateGizmo(
		UInteractiveGizmoManager::DefaultAxisAngleBuilderIdentifier));
	check(RotateGizmo);

	// axis source provides the rotation axis
	RotateGizmo->AxisSource = Cast<UObject>(AxisSource);

	// parameter source maps angle-parameter-change to rotation of TransformSource's transform
	UGizmoAxisRotationParameterSource* AngleSource = UGizmoAxisRotationParameterSource::Construct(AxisSource, TransformSource, this);
	//AngleSource->RotationConstraintFunction = [this](const FQuat& DeltaRotation){ return RotationSnapFunction(DeltaRotation); };
	AngleSource->AngleDeltaConstraintFunction = [this](double AngleDelta, double& SnappedDelta) { return RotationAxisAngleSnapFunction(AngleDelta, SnappedDelta, 0); };
	RotateGizmo->AngleSource = AngleSource;

	// sub-component provides hit target
	UGizmoComponentHitTarget* HitTarget = UGizmoComponentHitTarget::Construct(AxisComponent, this);
	if (this->UpdateHoverFunction)
	{
		HitTarget->UpdateHoverFunction = [AxisComponent, this](bool bHovering) { this->UpdateHoverFunction(AxisComponent, bHovering); };
	}
	RotateGizmo->HitTarget = HitTarget;

	RotateGizmo->StateTarget = Cast<UObject>(StateTargetIn);

	RotateGizmo->ShouldUseCustomDestinationFunc = [this]() { return ShouldAlignDestination(); };
	RotateGizmo->CustomDestinationFunc =
		[this](const UAxisAngleGizmo::FCustomDestinationParams& Params, FVector& OutputPoint) {
		return DestinationAlignmentRayCaster(*Params.WorldRay, OutputPoint);
		};

	ActiveGizmos.Add(RotateGizmo);

	return RotateGizmo;
}



UInteractiveGizmo* UTransformGizmo::AddAxisScaleGizmo(
	UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
	IGizmoAxisSource* GizmoAxisSource, IGizmoAxisSource* ParameterAxisSource,
	IGizmoTransformSource* TransformSource,
	IGizmoStateTarget* StateTargetIn)
{
	// create axis-position gizmo, axis-position parameter will drive scale
	UAxisPositionGizmo* ScaleGizmo = Cast<UAxisPositionGizmo>(GetGizmoManager()->CreateGizmo(
		UInteractiveGizmoManager::DefaultAxisPositionBuilderIdentifier));
	ScaleGizmo->bEnableSignedAxis = true;
	check(ScaleGizmo);

	// axis source provides the translation axis
	ScaleGizmo->AxisSource = Cast<UObject>(GizmoAxisSource);

	// parameter source maps axis-parameter-change to translation of TransformSource's transform
	UGizmoAxisScaleParameterSource* ParamSource = UGizmoAxisScaleParameterSource::Construct(ParameterAxisSource, TransformSource, this);
	//ParamSource->PositionConstraintFunction = [this](const FVector& Pos, FVector& Snapped) { return PositionSnapFunction(Pos, Snapped); };
	ParamSource->bClampToZero = bDisallowNegativeScaling;
	ScaleGizmo->ParameterSource = ParamSource;

	// sub-component provides hit target
	UGizmoComponentHitTarget* HitTarget = UGizmoComponentHitTarget::Construct(AxisComponent, this);
	if (this->UpdateHoverFunction)
	{
		HitTarget->UpdateHoverFunction = [AxisComponent, this](bool bHovering) { this->UpdateHoverFunction(AxisComponent, bHovering); };
	}
	ScaleGizmo->HitTarget = HitTarget;

	ScaleGizmo->StateTarget = Cast<UObject>(StateTargetIn);

	ActiveGizmos.Add(ScaleGizmo);
	return ScaleGizmo;
}



UInteractiveGizmo* UTransformGizmo::AddPlaneScaleGizmo(
	UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
	IGizmoAxisSource* GizmoAxisSource, IGizmoAxisSource* ParameterAxisSource,
	IGizmoTransformSource* TransformSource,
	IGizmoStateTarget* StateTargetIn)
{
	// create axis-position gizmo, axis-position parameter will drive scale
	UPlanePositionGizmo* ScaleGizmo = Cast<UPlanePositionGizmo>(GetGizmoManager()->CreateGizmo(
		UInteractiveGizmoManager::DefaultPlanePositionBuilderIdentifier));
	ScaleGizmo->bEnableSignedAxis = true;
	check(ScaleGizmo);

	// axis source provides the translation axis
	ScaleGizmo->AxisSource = Cast<UObject>(GizmoAxisSource);

	// parameter source maps axis-parameter-change to translation of TransformSource's transform
	UGizmoPlaneScaleParameterSource* ParamSource = UGizmoPlaneScaleParameterSource::Construct(ParameterAxisSource, TransformSource, this);
	//ParamSource->PositionConstraintFunction = [this](const FVector& Pos, FVector& Snapped) { return PositionSnapFunction(Pos, Snapped); };
	ParamSource->bClampToZero = bDisallowNegativeScaling;
	ParamSource->bUseEqualScaling = true;
	ScaleGizmo->ParameterSource = ParamSource;

	// sub-component provides hit target
	UGizmoComponentHitTarget* HitTarget = UGizmoComponentHitTarget::Construct(AxisComponent, this);
	if (this->UpdateHoverFunction)
	{
		HitTarget->UpdateHoverFunction = [AxisComponent, this](bool bHovering) { this->UpdateHoverFunction(AxisComponent, bHovering); };
	}
	ScaleGizmo->HitTarget = HitTarget;

	ScaleGizmo->StateTarget = Cast<UObject>(StateTargetIn);

	ActiveGizmos.Add(ScaleGizmo);
	return ScaleGizmo;
}





UInteractiveGizmo* UTransformGizmo::AddUniformScaleGizmo(
	UPrimitiveComponent* ScaleComponent, USceneComponent* RootComponent,
	IGizmoAxisSource* GizmoAxisSource, IGizmoAxisSource* ParameterAxisSource,
	IGizmoTransformSource* TransformSource,
	IGizmoStateTarget* StateTargetIn)
{
	// create plane-position gizmo, plane-position parameter will drive scale
	UPlanePositionGizmo* ScaleGizmo = Cast<UPlanePositionGizmo>(GetGizmoManager()->CreateGizmo(
		UInteractiveGizmoManager::DefaultPlanePositionBuilderIdentifier));
	check(ScaleGizmo);

	// axis source provides the translation plane
	ScaleGizmo->AxisSource = Cast<UObject>(GizmoAxisSource);

	// parameter source maps axis-parameter-change to translation of TransformSource's transform
	UGizmoUniformScaleParameterSource* ParamSource = UGizmoUniformScaleParameterSource::Construct(ParameterAxisSource, TransformSource, this);
	//ParamSource->PositionConstraintFunction = [this](const FVector& Pos, FVector& Snapped) { return PositionSnapFunction(Pos, Snapped); };
	ScaleGizmo->ParameterSource = ParamSource;

	// sub-component provides hit target
	UGizmoComponentHitTarget* HitTarget = UGizmoComponentHitTarget::Construct(ScaleComponent, this);
	if (this->UpdateHoverFunction)
	{
		HitTarget->UpdateHoverFunction = [ScaleComponent, this](bool bHovering) { this->UpdateHoverFunction(ScaleComponent, bHovering); };
	}
	ScaleGizmo->HitTarget = HitTarget;

	ScaleGizmo->StateTarget = Cast<UObject>(StateTargetIn);

	ActiveGizmos.Add(ScaleGizmo);
	return ScaleGizmo;
}



void UTransformGizmo::ClearActiveTarget()
{
	OnAboutToClearActiveTarget.Broadcast(this, ActiveTarget);

	for (UInteractiveGizmo* Gizmo : ActiveGizmos)
	{
		GetGizmoManager()->DestroyGizmo(Gizmo);
	}
	ActiveGizmos.SetNum(0);
	ActiveComponents.SetNum(0);
	TranslationSubGizmos.SetNum(0);
	RotationSubGizmos.SetNum(0);
	UniformScaleSubGizmos.SetNum(0);
	NonuniformScaleComponents.SetNum(0);

	CameraAxisSource = nullptr;
	AxisXSource = nullptr;
	AxisYSource = nullptr;
	AxisZSource = nullptr;
	UnitAxisXSource = nullptr;
	UnitAxisYSource = nullptr;
	UnitAxisZSource = nullptr;
	StateTarget = nullptr;

	ActiveTarget = nullptr;
}




bool UTransformGizmo::PositionSnapFunction(const FVector& WorldPosition, FVector& SnappedPositionOut) const
{
	SnappedPositionOut = WorldPosition;

	// only snap if we want snapping obvs
	if (bSnapToWorldGrid == false)
	{
		return false;
	}

	// only snap to world grid when using world axes
	if (GetGizmoManager()->GetContextQueriesAPI()->GetCurrentCoordinateSystem() != EToolContextCoordinateSystem::World)
	{
		return false;
	}
    
	FSceneSnapQueryRequest Request;
	Request.RequestType = ESceneSnapQueryType::Position;
	Request.TargetTypes = ESceneSnapQueryTargetType::Grid;
	Request.Position = WorldPosition;
	if ( bGridSizeIsExplicit )
		{
			Request.GridSize = ExplicitGridSize;
		}
		TArray<FSceneSnapQueryResult> Results;
	if (GetGizmoManager()->GetContextQueriesAPI()->ExecuteSceneSnapQuery(Request, Results))
		{
		SnappedPositionOut = Results[0].Position;
			return true;
		};

	return false;
}

bool UTransformGizmo::PositionAxisDeltaSnapFunction(double AxisDelta, double& SnappedDeltaOut, int AxisIndex) const
{
	if (!bSnapToWorldGrid) return false;

	EToolContextCoordinateSystem CoordSystem = GetGizmoManager()->GetContextQueriesAPI()->GetCurrentCoordinateSystem();
	bool bUseRelativeSnapping = RelativeTranslationSnapping.IsEnabled() || (CoordSystem != EToolContextCoordinateSystem::World);
	if (!bUseRelativeSnapping)
	{
		return false;
	}

	FSceneSnapQueryRequest Request;
	Request.RequestType = ESceneSnapQueryType::Position;
	Request.TargetTypes = ESceneSnapQueryTargetType::Grid;
	if (bGridSizeIsExplicit)
	{
		Request.GridSize = ExplicitGridSize;
	}
	TArray<FSceneSnapQueryResult> Results;
	Results.Reserve(1);

	// this is a bit of a hack, since the snap query only snaps world points, and the grid may not be
	// uniform. A point on the specified X/Y/Z at the delta-distance is snapped, this is ideally
	// equivalent to actually computing a snap of the axis-delta
	Request.Position = FVector::ZeroVector;
	Request.Position[AxisIndex] = AxisDelta;
	if (GetGizmoManager()->GetContextQueriesAPI()->ExecuteSceneSnapQuery(Request, Results))
	{
		SnappedDeltaOut = Results[0].Position[AxisIndex];
		return true;
	};
	
	return false;
}

FQuat UTransformGizmo::RotationSnapFunction(const FQuat& DeltaRotation) const
{
	FQuat SnappedDeltaRotation = DeltaRotation;

	// only snap if we want snapping obvs
	if (bSnapToWorldRotGrid)
	{
		FSceneSnapQueryRequest Request;
		Request.RequestType   = ESceneSnapQueryType::Rotation;
		Request.TargetTypes   = ESceneSnapQueryTargetType::Grid;
		Request.DeltaRotation = DeltaRotation;
		if ( bRotationGridSizeIsExplicit )
		{
			Request.RotGridSize = ExplicitRotationGridSize;
		}
		TArray<FSceneSnapQueryResult> Results;
		if (GetGizmoManager()->GetContextQueriesAPI()->ExecuteSceneSnapQuery(Request, Results))
		{
			SnappedDeltaRotation = Results[0].DeltaRotation;
		};
	}
	return SnappedDeltaRotation;
}

void UTransformGizmo::BeginChange()
{
	ActiveChange = MakeUnique<FTransformGizmoTransformChange>();
	ActiveChange->ChildScaleBefore = SeparateChildScale;
}

TUniquePtr<FToolCommandChange> UTransformGizmo::EndChange()
{
	ActiveChange->ChildScaleAfter = SeparateChildScale;
	return MoveTemp(ActiveChange);
	ActiveChange = nullptr;
}

UObject* UTransformGizmo::GetChangeTarget()
{
	return this;
}

FText UTransformGizmo::GetChangeDescription()
{
	return LOCTEXT("TransformGizmoChangeDescription", "Transform Change");
}


void UTransformGizmo::ExternalSetChildScale(const FVector& NewScale)
{
	SeparateChildScale = NewScale;
}


void FTransformGizmoTransformChange::Apply(UObject* Object)
{
	UTransformGizmo* Gizmo = CastChecked<UTransformGizmo>(Object);
	Gizmo->ExternalSetChildScale(ChildScaleAfter);
}


void FTransformGizmoTransformChange::Revert(UObject* Object)
{
	UTransformGizmo* Gizmo = CastChecked<UTransformGizmo>(Object);
	Gizmo->ExternalSetChildScale(ChildScaleBefore);
}

FString FTransformGizmoTransformChange::ToString() const
{
	return FString(TEXT("TransformGizmo Change"));
}

bool UTransformGizmo::RotationAxisAngleSnapFunction(double AxisAngleDelta, double& SnappedAxisAngleDeltaOut, int AxisIndex) const
{
	if (!bSnapToWorldRotGrid) return false;

	FToolContextSnappingConfiguration SnappingConfig = GetGizmoManager()->GetContextQueriesAPI()->GetCurrentSnappingSettings();
	if (SnappingConfig.bEnableRotationGridSnapping)
	{
		double SnapDelta = SnappingConfig.RotationGridAngles.Yaw;		// could use AxisIndex here?
		if (bRotationGridSizeIsExplicit)
		{
			SnapDelta = ExplicitRotationGridSize.Yaw;
		}
		AxisAngleDelta *= 180.0 / 3.1415926535897932384626433832795;
		SnappedAxisAngleDeltaOut = GizmoMath::SnapToIncrement(AxisAngleDelta, SnapDelta);
		SnappedAxisAngleDeltaOut *= 3.1415926535897932384626433832795 / 180.0;
		return true;
	}

	return false;
}

const FString UTransformGizmoContextObject::DefaultAxisPositionBuilderIdentifier = TEXT("Util_StandardXFormAxisTranslationGizmo");
const FString UTransformGizmoContextObject::DefaultPlanePositionBuilderIdentifier = TEXT("Util_StandardXFormPlaneTranslationGizmo");
const FString UTransformGizmoContextObject::DefaultAxisAngleBuilderIdentifier = TEXT("Util_StandardXFormAxisRotationGizmo");
const FString UTransformGizmoContextObject::DefaultThreeAxisTransformBuilderIdentifier = TEXT("Util_DefaultThreeAxisTransformBuilderIdentifier");
const FString UTransformGizmoContextObject::CustomThreeAxisTransformBuilderIdentifier = TEXT("Util_CustomThreeAxisTransformBuilderIdentifier");
const FString UTransformGizmoContextObject::CustomRepositionableThreeAxisTransformBuilderIdentifier = TEXT("Util_CustomRepositionableThreeAxisTransformBuilderIdentifier");


void UTransformGizmoContextObject::RegisterGizmosWithManager(UInteractiveToolManager* ToolManager)
{
	if (ensure(!bDefaultGizmosRegistered) == false)
	{
		return;
	}

	UInteractiveGizmoManager* GizmoManager = ToolManager->GetPairedGizmoManager();
	ToolManager->GetContextObjectStore()->AddContextObject(this);

	UAxisPositionGizmoBuilder* AxisTranslationBuilder = NewObject<UAxisPositionGizmoBuilder>();
	GizmoManager->RegisterGizmoType(DefaultAxisPositionBuilderIdentifier, AxisTranslationBuilder);

	UPlanePositionGizmoBuilder* PlaneTranslationBuilder = NewObject<UPlanePositionGizmoBuilder>();
	GizmoManager->RegisterGizmoType(DefaultPlanePositionBuilderIdentifier, PlaneTranslationBuilder);

	UAxisAngleGizmoBuilder* AxisRotationBuilder = NewObject<UAxisAngleGizmoBuilder>();
	GizmoManager->RegisterGizmoType(DefaultAxisAngleBuilderIdentifier, AxisRotationBuilder);

	UTransformGizmoBuilder* TransformBuilder = NewObject<UTransformGizmoBuilder>();
	TransformBuilder->AxisPositionBuilderIdentifier = DefaultAxisPositionBuilderIdentifier;
	TransformBuilder->PlanePositionBuilderIdentifier = DefaultPlanePositionBuilderIdentifier;
	TransformBuilder->AxisAngleBuilderIdentifier = DefaultAxisAngleBuilderIdentifier;
	GizmoManager->RegisterGizmoType(DefaultThreeAxisTransformBuilderIdentifier, TransformBuilder);
	UGizmoViewContext* GizmoViewContext = ToolManager->GetContextObjectStore()->FindContext<UGizmoViewContext>();
	if (!GizmoViewContext)
	{
		GizmoViewContext = NewObject<UGizmoViewContext>();
		ToolManager->GetContextObjectStore()->AddContextObject(GizmoViewContext);
	}

	GizmoActorBuilder = MakeShared<FTransformGizmoActorFactory>(GizmoViewContext);
	
    UTransformGizmoBuilder* CustomThreeAxisBuilder = NewObject<UTransformGizmoBuilder>();
	CustomThreeAxisBuilder->AxisPositionBuilderIdentifier = DefaultAxisPositionBuilderIdentifier;
	CustomThreeAxisBuilder->PlanePositionBuilderIdentifier = DefaultPlanePositionBuilderIdentifier;
	CustomThreeAxisBuilder->AxisAngleBuilderIdentifier = DefaultAxisAngleBuilderIdentifier;
	CustomThreeAxisBuilder->GizmoActorBuilder = GizmoActorBuilder;
	GizmoManager->RegisterGizmoType(CustomThreeAxisTransformBuilderIdentifier, CustomThreeAxisBuilder);

	URepositionableTransformGizmoBuilder* CustomRepositionableThreeAxisBuilder = NewObject<URepositionableTransformGizmoBuilder>();
	CustomRepositionableThreeAxisBuilder->AxisPositionBuilderIdentifier = DefaultAxisPositionBuilderIdentifier;
	CustomRepositionableThreeAxisBuilder->PlanePositionBuilderIdentifier = DefaultPlanePositionBuilderIdentifier;
	CustomRepositionableThreeAxisBuilder->AxisAngleBuilderIdentifier = DefaultAxisAngleBuilderIdentifier;
	CustomRepositionableThreeAxisBuilder->GizmoActorBuilder = GizmoActorBuilder;
	GizmoManager->RegisterGizmoType(CustomRepositionableThreeAxisTransformBuilderIdentifier, CustomRepositionableThreeAxisBuilder);

	bDefaultGizmosRegistered = true;


}

void UTransformGizmoContextObject::DeregisterGizmosWithManager(UInteractiveToolManager* ToolManager)
{
	UInteractiveGizmoManager* GizmoManager = ToolManager->GetPairedGizmoManager();
	ToolManager->GetContextObjectStore()->RemoveContextObject(this);

	ensure(bDefaultGizmosRegistered);
	GizmoManager->DeregisterGizmoType(DefaultAxisPositionBuilderIdentifier);
	GizmoManager->DeregisterGizmoType(DefaultPlanePositionBuilderIdentifier);
	GizmoManager->DeregisterGizmoType(DefaultAxisAngleBuilderIdentifier);
	GizmoManager->DeregisterGizmoType(DefaultThreeAxisTransformBuilderIdentifier);
	GizmoManager->DeregisterGizmoType(CustomThreeAxisTransformBuilderIdentifier);
	GizmoManager->DeregisterGizmoType(CustomRepositionableThreeAxisTransformBuilderIdentifier);
	bDefaultGizmosRegistered = false;
}




bool UE::TransformGizmoUtil::RegisterTransformGizmoContextObject(UInteractiveToolsContext* ToolsContext)
{
	if (ensure(ToolsContext))
	{
		UTransformGizmoContextObject* Found = ToolsContext->ContextObjectStore->FindContext<UTransformGizmoContextObject>();
		if (Found == nullptr)
		{
			UTransformGizmoContextObject* GizmoHelper = NewObject<UTransformGizmoContextObject>(ToolsContext->ToolManager);
			if (ensure(GizmoHelper))
			{
				GizmoHelper->RegisterGizmosWithManager(ToolsContext->ToolManager);
				return true;
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	return false;
}


bool UE::TransformGizmoUtil::DeregisterTransformGizmoContextObject(UInteractiveToolsContext* ToolsContext)
{
	if (ensure(ToolsContext))
	{
		UTransformGizmoContextObject* Found = ToolsContext->ContextObjectStore->FindContext<UTransformGizmoContextObject>();
		if (Found != nullptr)
		{
			Found->DeregisterGizmosWithManager(ToolsContext->ToolManager);
			ToolsContext->ContextObjectStore->RemoveContextObject(Found);
		}
		return true;
	}
	return false;
}



UTransformGizmo* UTransformGizmoContextObject::Create3AxisTransformGizmo(UInteractiveGizmoManager* GizmoManager, void* Owner, const FString& InstanceIdentifier)
{
	if (ensure(bDefaultGizmosRegistered))
	{
		UInteractiveGizmo* NewGizmo = GizmoManager->CreateGizmo(DefaultThreeAxisTransformBuilderIdentifier, InstanceIdentifier, Owner);
		UTransformGizmo* CastGizmo = Cast<UTransformGizmo>(NewGizmo);
		if (ensure(CastGizmo))
		{
			OnGizmoCreated.Broadcast(CastGizmo);
		}
		return CastGizmo;
	}
	return nullptr;
}
UTransformGizmo* UE::TransformGizmoUtil::Create3AxisTransformGizmo(UInteractiveGizmoManager* GizmoManager, void* Owner, const FString& InstanceIdentifier)
{
	if (ensure(GizmoManager))
	{
		UTransformGizmoContextObject* UseThis = GizmoManager->GetContextObjectStore()->FindContext<UTransformGizmoContextObject>();
		if (ensure(UseThis))
		{
			return UseThis->Create3AxisTransformGizmo(GizmoManager, Owner, InstanceIdentifier);
		}
	}
	return nullptr;
}
UTransformGizmo* UE::TransformGizmoUtil::Create3AxisTransformGizmo(UInteractiveToolManager* ToolManager, void* Owner, const FString& InstanceIdentifier)
{
	if (ensure(ToolManager))
	{
		return Create3AxisTransformGizmo(ToolManager->GetPairedGizmoManager(), Owner, InstanceIdentifier);
	}
	return nullptr;
}

UTransformGizmo* UTransformGizmoContextObject::CreateCustomTransformGizmo(UInteractiveGizmoManager* GizmoManager, ETransformGizmoSubElements Elements, void* Owner, const FString& InstanceIdentifier)
{
	if (ensure(bDefaultGizmosRegistered))
	{
		GizmoActorBuilder->EnableElements = Elements;
		UInteractiveGizmo* NewGizmo = GizmoManager->CreateGizmo(CustomThreeAxisTransformBuilderIdentifier, InstanceIdentifier, Owner);
		UTransformGizmo* CastGizmo = Cast<UTransformGizmo>(NewGizmo);
		if (ensure(CastGizmo))
		{
			OnGizmoCreated.Broadcast(CastGizmo);
		}
		return CastGizmo;
	}
	return nullptr;
}
UTransformGizmo* UE::TransformGizmoUtil::CreateCustomTransformGizmo(UInteractiveGizmoManager* GizmoManager, ETransformGizmoSubElements Elements, void* Owner, const FString& InstanceIdentifier)
{
	if (ensure(GizmoManager))
	{
		UTransformGizmoContextObject* UseThis = GizmoManager->GetContextObjectStore()->FindContext<UTransformGizmoContextObject>();
		if (ensure(UseThis))
		{
			return UseThis->CreateCustomTransformGizmo(GizmoManager, Elements, Owner, InstanceIdentifier);
		}
	}
	return nullptr;
}
UTransformGizmo* UE::TransformGizmoUtil::CreateCustomTransformGizmo(UInteractiveToolManager* ToolManager, ETransformGizmoSubElements Elements, void* Owner, const FString& InstanceIdentifier)
{
	if (ensure(ToolManager))
	{
		return CreateCustomTransformGizmo(ToolManager->GetPairedGizmoManager(), Elements, Owner, InstanceIdentifier);
	}
	return nullptr;
}


UTransformGizmo* UTransformGizmoContextObject::CreateCustomRepositionableTransformGizmo(UInteractiveGizmoManager* GizmoManager, ETransformGizmoSubElements Elements, void* Owner, const FString& InstanceIdentifier)
{
	if (ensure(bDefaultGizmosRegistered))
	{
		GizmoActorBuilder->EnableElements = Elements;
		UInteractiveGizmo* NewGizmo = GizmoManager->CreateGizmo(CustomRepositionableThreeAxisTransformBuilderIdentifier, InstanceIdentifier, Owner);
		ensure(NewGizmo);
		UTransformGizmo* CastGizmo = Cast<UTransformGizmo>(NewGizmo);
		if (ensure(CastGizmo))
		{
			OnGizmoCreated.Broadcast(CastGizmo);
		}
		return CastGizmo;
	}
	return nullptr;
}
UTransformGizmo* UE::TransformGizmoUtil::CreateCustomRepositionableTransformGizmo(UInteractiveGizmoManager* GizmoManager, ETransformGizmoSubElements Elements, void* Owner, const FString& InstanceIdentifier)
{
	if (ensure(GizmoManager))
	{
		UTransformGizmoContextObject* UseThis = GizmoManager->GetContextObjectStore()->FindContext<UTransformGizmoContextObject>();
		if (ensure(UseThis))
		{
			return UseThis->CreateCustomRepositionableTransformGizmo(GizmoManager, Elements, Owner, InstanceIdentifier);
		}
	}
	return nullptr;
}
UTransformGizmo* UE::TransformGizmoUtil::CreateCustomRepositionableTransformGizmo(UInteractiveToolManager* ToolManager, ETransformGizmoSubElements Elements, void* Owner, const FString& InstanceIdentifier)
{
	if (ensure(ToolManager))
	{
		return CreateCustomRepositionableTransformGizmo(ToolManager->GetPairedGizmoManager(), Elements, Owner, InstanceIdentifier);
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
