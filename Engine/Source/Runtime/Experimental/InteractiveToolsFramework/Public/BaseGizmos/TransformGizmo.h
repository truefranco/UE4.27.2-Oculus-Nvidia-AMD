// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveGizmo.h"
#include "InteractiveGizmoBuilder.h"
#include "InteractiveToolObjects.h"
#include "SceneView.h"
#include "InteractiveToolChange.h"
#include "BaseGizmos/GizmoActor.h"
#include "BaseGizmos/TransformProxy.h"
#include "TransformGizmo.generated.h"

class UInteractiveGizmoManager;
class UInteractiveToolsContext;
class UInteractiveToolManager;
class UGizmoViewContext;
class IGizmoAxisSource;
class IGizmoTransformSource;
class IGizmoStateTarget;
class UGizmoConstantFrameAxisSource;
class UGizmoComponentAxisSource;
class UGizmoTransformChangeStateTarget;
class UGizmoScaledTransformSource;
class FTransformGizmoTransformChange;

/**
 * ATransformGizmoActor is an Actor type intended to be used with UTransformGizmo,
 * as the in-scene visual representation of the Gizmo.
 * 
 * FTransformGizmoActorFactory returns an instance of this Actor type (or a subclass), and based on
 * which Translate and Rotate UProperties are initialized, will associate those Components
 * with UInteractiveGizmo's that implement Axis Translation, Plane Translation, and Axis Rotation.
 * 
 * If a particular sub-Gizmo is not required, simply set that FProperty to null.
 * 
 * The static factory method ::ConstructDefault3AxisGizmo() creates and initializes an 
 * Actor suitable for use in a standard 3-axis Transformation Gizmo.
 */
UCLASS(Transient)
class INTERACTIVETOOLSFRAMEWORK_API ATransformGizmoActor : public AGizmoActor
{
	GENERATED_BODY()
public:

	ATransformGizmoActor();

public:
	//
	// Translation Components
	//

	/** X Axis Translation Component */
	UPROPERTY()
	UPrimitiveComponent* TranslateX;

	/** Y Axis Translation Component */
	UPROPERTY()
	UPrimitiveComponent* TranslateY;

	/** Z Axis Translation Component */
	UPROPERTY()
	UPrimitiveComponent* TranslateZ;


	/** YZ Plane Translation Component */
	UPROPERTY()
	UPrimitiveComponent* TranslateYZ;

	/** XZ Plane Translation Component */
	UPROPERTY()
	UPrimitiveComponent* TranslateXZ;

	/** XY Plane Translation Component */
	UPROPERTY()
	UPrimitiveComponent* TranslateXY;

	//
	// Rotation Components
	//

	/** X Axis Rotation Component */
	UPROPERTY()
	UPrimitiveComponent* RotateX;

	/** Y Axis Rotation Component */
	UPROPERTY()
	UPrimitiveComponent* RotateY;

	/** Z Axis Rotation Component */
	UPROPERTY()
	UPrimitiveComponent* RotateZ;

	/** Z Axis Rotation Component */
	UPROPERTY()
	UPrimitiveComponent* RotationSphere;

	//
	// Scaling Components
	//

	/** Uniform Scale Component */
	UPROPERTY()
	UPrimitiveComponent* UniformScale;


	/** X Axis Scale Component */
	UPROPERTY()
	UPrimitiveComponent* AxisScaleX;

	/** Y Axis Scale Component */
	UPROPERTY()
	UPrimitiveComponent* AxisScaleY;

	/** Z Axis Scale Component */
	UPROPERTY()
	UPrimitiveComponent* AxisScaleZ;


	/** YZ Plane Scale Component */
	UPROPERTY()
	UPrimitiveComponent* PlaneScaleYZ;

	/** XZ Plane Scale Component */
	UPROPERTY()
	UPrimitiveComponent* PlaneScaleXZ;

	/** XY Plane Scale Component */
	UPROPERTY()
	UPrimitiveComponent* PlaneScaleXY;



public:
	/**
	 * Create a new instance of ATransformGizmoActor and populate the various
	 * sub-components with standard GizmoXComponent instances suitable for a 3-axis transformer Gizmo
	 */
	static ATransformGizmoActor* ConstructDefault3AxisGizmo(
		UWorld* World
	);

	/**
	 * Create a new instance of ATransformGizmoActor. Populate the sub-components 
	 * specified by Elements with standard GizmoXComponent instances suitable for a 3-axis transformer Gizmo
	 */
	static ATransformGizmoActor* ConstructCustom3AxisGizmo(
		UWorld* World,
		ETransformGizmoSubElements Elements
	);
};





/**
 * FTransformGizmoActorFactory creates new instances of ATransformGizmoActor which
 * are used by UTransformGizmo to implement 3D transformation Gizmos. 
 * An instance of FTransformGizmoActorFactory is passed to UTransformGizmo
 * (by way of UTransformGizmoBuilder), which then calls CreateNewGizmoActor()
 * to spawn new Gizmo Actors.
 * 
 * By default CreateNewGizmoActor() returns a default Gizmo Actor suitable for
 * a three-axis transformation Gizmo, override this function to customize
 * the Actor sub-elements.
 */
class INTERACTIVETOOLSFRAMEWORK_API FTransformGizmoActorFactory
{
public:

	FTransformGizmoActorFactory(UGizmoViewContext* GizmoViewContextIn)
		: GizmoViewContext(GizmoViewContextIn)
	{
	}
	
	/** Only these members of the ATransformGizmoActor gizmo will be initialized */
	ETransformGizmoSubElements EnableElements =
		ETransformGizmoSubElements::TranslateAllAxes |
		ETransformGizmoSubElements::TranslateAllPlanes |
		ETransformGizmoSubElements::RotateAllAxes |
		ETransformGizmoSubElements::ScaleAllAxes |
		ETransformGizmoSubElements::ScaleAllPlanes | 
		ETransformGizmoSubElements::ScaleUniform;

	/**
	 * @param World the UWorld to create the new Actor in
	 * @return new ATransformGizmoActor instance with members initialized with Components suitable for a transformation Gizmo
	 */
	virtual ATransformGizmoActor* CreateNewGizmoActor(UWorld* World) const;

protected:
	/**
	 * The default gizmos that we use need to have the current view information stored for them via
	 * the ITF context store so that they can figure out how big they are for hit testing, so this
	 * pointer needs to be set (and kept alive elsewhere) for the actor factory to work properly.
	 */
	UGizmoViewContext* GizmoViewContext = nullptr;
};

UCLASS()
class INTERACTIVETOOLSFRAMEWORK_API UTransformGizmoBuilder : public UInteractiveGizmoBuilder
{
	GENERATED_BODY()

public:

	/**
	 * strings identifing GizmoBuilders already registered with GizmoManager. These builders will be used
	 * to spawn the various sub-gizmos
	 */
	FString AxisPositionBuilderIdentifier;
	FString PlanePositionBuilderIdentifier;
	FString AxisAngleBuilderIdentifier;
	/**
	 * If set, this Actor Builder will be passed to UTransformGizmo instances.
	 * Otherwise new instances of the base FTransformGizmoActorFactory are created internally.
	 */
	TSharedPtr<FTransformGizmoActorFactory> GizmoActorBuilder;

	/**
	 * If set, this hover function will be passed to UTransformGizmo instances to use instead of the default.
	 * Hover is complicated for UTransformGizmo because all it knows about the different gizmo scene elements
	 * is that they are UPrimitiveComponent (coming from the ATransformGizmoActor). The default hover
	 * function implementation is to try casting to UGizmoBaseComponent and calling ::UpdateHoverState().
	 * If you are using different Components that do not subclass UGizmoBaseComponent, and you want hover to 
	 * work, you will need to provide a different hover update function.
	 */
	TFunction<void(UPrimitiveComponent*, bool)> UpdateHoverFunction;

	/**
	 * If set, this coord-system function will be passed to UTransformGizmo instances to use instead
	 * of the default UpdateCoordSystemFunction. By default the UTransformGizmo will query the external Context
	 * to ask whether it should be using world or local coordinate system. Then the default UpdateCoordSystemFunction
	 * will try casting to UGizmoBaseCmponent and passing that info on via UpdateWorldLocalState();
	 * If you are using different Components that do not subclass UGizmoBaseComponent, and you want the coord system
	 * to be configurable, you will need to provide a different update function.
	 */
	TFunction<void(UPrimitiveComponent*, EToolContextCoordinateSystem)> UpdateCoordSystemFunction;


	virtual UInteractiveGizmo* BuildGizmo(const FToolBuilderState& SceneState) const override;
};

/**
 * FToolContextOptionalToggle is used to store a boolean flag where the value of
 * the boolean may either be set directly, or it may be set by querying some external context.
 * This struct does not directly do anything, it just wraps up the multiple flags/states
 * needed to provide such functionality
 */
struct FToolContextOptionalToggle
{
	bool bEnabledDirectly = false;
	bool bEnabledInContext = false;
	bool bInheritFromContext = false;

	FToolContextOptionalToggle() {}
	FToolContextOptionalToggle(bool bEnabled, bool bSetInheritFromContext)
	{
		bEnabledDirectly = bEnabled;
		bInheritFromContext = bSetInheritFromContext;
	}

	void UpdateContextValue(bool bNewValue) { bEnabledInContext = bNewValue; }
	bool InheritFromContext() const { return bInheritFromContext; }

	/** @return true if Toggle is currently set to Enabled/On, under the current configuration */
	bool IsEnabled() const { return (bInheritFromContext) ? bEnabledInContext : bEnabledDirectly; }
};

/**
 * UTransformGizmo provides standard Transformation Gizmo interactions,
 * applied to a UTransformProxy target object. By default the Gizmo will be
 * a standard XYZ translate/rotate Gizmo (axis and plane translation).
 * 
 * The in-scene representation of the Gizmo is a ATransformGizmoActor (or subclass).
 * This Actor has FProperty members for the various sub-widgets, each as a separate Component.
 * Any particular sub-widget of the Gizmo can be disabled by setting the respective
 * Actor Component to null. 
 * 
 * So, to create non-standard variants of the Transform Gizmo, set a new GizmoActorBuilder 
 * in the UTransformGizmoBuilder registered with the GizmoManager. Return
 * a suitably-configured GizmoActor and everything else will be handled automatically.
 * 
 */
UCLASS()
class INTERACTIVETOOLSFRAMEWORK_API UTransformGizmo : public UInteractiveGizmo, public IToolCommandChangeSource
{
	GENERATED_BODY()

public:

	virtual void SetWorld(UWorld* World);
	virtual void SetGizmoActorBuilder(TSharedPtr<FTransformGizmoActorFactory> Builder);
	virtual void SetSubGizmoBuilderIdentifiers(FString AxisPositionBuilderIdentifier, FString PlanePositionBuilderIdentifier, FString AxisAngleBuilderIdentifier);
	virtual void SetUpdateHoverFunction(TFunction<void(UPrimitiveComponent*, bool)> HoverFunction);
	virtual void SetUpdateCoordSystemFunction(TFunction<void(UPrimitiveComponent*, EToolContextCoordinateSystem)> CoordSysFunction);

	/**
	 * If used, binds alignment functions to the sub gizmos that they can use to align to geometry in the scene.
	 * Specifically, translation and rotation gizmos will check ShouldAlignDestination() to see if they should
	 * use the custom ray caster (this allows the behavior to respond to modifier key presses, for instance),
	 * and then use DestinationAlignmentRayCaster() to find a point to align to.
	 * Subgizmos align to the point in different ways, usually by projecting onto the axis or plane that they
	 * operate in.
	 */
	virtual void SetWorldAlignmentFunctions(
		TUniqueFunction<bool()>&& ShouldAlignDestination,
		TUniqueFunction<bool(const FRay&, FVector&)>&& DestinationAlignmentRayCaster
	);

	/**
	 * By default, non-uniform scaling handles appear (assuming they exist in the gizmo to begin with),
	 * when CurrentCoordinateSystem == EToolContextCoordinateSystem::Local, since components can only be
	 * locally scaled. However, this can be changed to a custom check here, perhaps to hide them in extra
	 * conditions or to always show them (if the gizmo is not scaling a component).
	 */
	virtual void SetIsNonUniformScaleAllowedFunction(
		TUniqueFunction<bool()>&& IsNonUniformScaleAllowed
	);

	/**
	 * Exposes the return value of the current IsNonUniformScaleAllowed function so that, for instance,
	 * numerical UI can react appropriately.
	 */
	virtual bool IsNonUniformScaleAllowed() const { return IsNonUniformScaleAllowedFunc(); }

	/**
	 * By default, the nonuniform scale components can scale negatively. However, they can be made to clamp
	 * to zero instead by passing true here. This is useful for using the gizmo to flatten geometry.
	 *
	 * TODO: Should this affect uniform scaling too?
	 */
	virtual void SetDisallowNegativeScaling(bool bDisallow);

	// UInteractiveGizmo overrides
	virtual void Setup() override;
	virtual void Shutdown() override;
	virtual void Tick(float DeltaTime) override;

     /**
	 * Set the active target object for the Gizmo
	 * @param Target active target
	 * @param TransactionProvider optional IToolContextTransactionProvider implementation to use - by default uses GizmoManager
	 */
	virtual void SetActiveTarget(UTransformProxy* Target, IToolContextTransactionProvider* TransactionProvider = nullptr);

	/**
	 * Clear the active target object for the Gizmo
	 */
	virtual void ClearActiveTarget();

	/** The active target object for the Gizmo */
	UPROPERTY()
	UTransformProxy* ActiveTarget;

	/**
	 * @return the internal GizmoActor used by the Gizmo
	 */
	ATransformGizmoActor* GetGizmoActor() const { return GizmoActor; }

	/**
	 * @return current transform of Gizmo
	 */
	FTransform GetGizmoTransform() const;

	/**
	 * Repositions the gizmo without issuing undo/redo changes, triggering callbacks, 
	 * or moving any components. Useful for resetting the gizmo to a new location without
	 * it being viewed as a gizmo manipulation.
	 */
	void ReinitializeGizmoTransform(const FTransform& NewTransform, bool bKeepGizmoUnscaled = true);

	/**
	 * Set a new position for the Gizmo. This is done via the same mechanisms as the sub-gizmos,
	 * so it generates the same Change/Modify() events, and hence works with Undo/Redo
	 */
	virtual void SetNewGizmoTransform(const FTransform& NewTransform, bool bKeepGizmoUnscaled = true);

	/**
	 * Called at the start of a sequence of gizmo transform edits, for instance while dragging or
	 * manipulating the gizmo numerical UI.
	 */
	virtual void BeginTransformEditSequence();

	/**
	 * Called at the end of a sequence of gizmo transform edits.
	 */
	virtual void EndTransformEditSequence();

	/**
	 * Updates the gizmo transform between Begin/EndTransformeditSequence calls.
	 */
	void UpdateTransformDuringEditSequence(const FTransform& NewTransform, bool bKeepGizmoUnscaled = true);
	/**
	 * Explicitly set the child scale. Mainly useful to "reset" the child scale to (1,1,1) when re-using Gizmo across multiple transform actions.
	 * @warning does not generate change/modify events!!
	 */
	virtual void SetNewChildScale(const FVector& NewChildScale);

	/**
	 * Set visibility for this Gizmo
	 */
	virtual void SetVisibility(bool bVisible);

	/**
	 * @return true if Gizmo is visible
	 */
	virtual bool IsVisible()
	{
		return GizmoActor && !GizmoActor->IsHidden();
	}
	/**
	 * If true, then when using world frame, Axis and Plane translation snap to the world grid via the ContextQueriesAPI (in PositionSnapFunction)
	 */
	UPROPERTY()
	bool bSnapToWorldGrid = true;

	/**
	 * Specify whether Relative snapping for Translations should be used in World frame mode. Relative snapping is always used in Local mode.
	 */
	FToolContextOptionalToggle RelativeTranslationSnapping = FToolContextOptionalToggle(true, true);
	/**
	 * Optional grid size which overrides the Context Grid
	 */
	UPROPERTY()
	bool bGridSizeIsExplicit = false;
	UPROPERTY()
	FVector ExplicitGridSize;

	/**
	 * Optional grid size which overrides the Context Rotation Grid
	 */
	UPROPERTY()
	bool bRotationGridSizeIsExplicit = false;
	UPROPERTY()
	FRotator ExplicitRotationGridSize;

	/**
	 * If true, then when using world frame, Axis and Plane translation snap to the world grid via the ContextQueriesAPI (in RotationSnapFunction)
	 */
	UPROPERTY()
	bool bSnapToWorldRotGrid = false;

	/**
	 * Whether to use the World/Local coordinate system provided by the context via the ContextyQueriesAPI.
	 */
	UPROPERTY()
	bool bUseContextCoordinateSystem = true;

	/**
	 * Current coordinate system in use. If bUseContextCoordinateSystem is true, this value will be updated internally every Tick()
	 * by quering the ContextyQueriesAPI, otherwise the default is Local and the client can change it as necessary
	 */
	UPROPERTY()
	EToolContextCoordinateSystem CurrentCoordinateSystem = EToolContextCoordinateSystem::Local;

	/**
	 * Whether to use the Gizmo Mode provided by the context via the ContextyQueriesAPI.
	 */
	UPROPERTY()
	bool bUseContextGizmoMode = true;

	/**
	 * Current dynamic sub-widget visibility mode to use (eg Translate-Only, Scale-Only, Combined, etc)
	 * If bUseContextGizmoMode is true, this value will be updated internally every Tick()
	 * by quering the ContextyQueriesAPI, otherwise the default is Combined and the client can change it as necessary
	 */
	UPROPERTY()
	EToolContextTransformGizmoMode ActiveGizmoMode = EToolContextTransformGizmoMode::Combined;

	/**
	 * Gets the elements that this gizmo was initialized with. Note that this may not account for individual
	 * element visibility- for instance the scaling component may not be visible if IsNonUniformScaleAllowed() is false.
	 */
	ETransformGizmoSubElements GetGizmoElements();


	/**
	 * The DisplaySpaceTransform is not used by the gizmo itself, but can be used by external adapters that might
	 * display gizmo values, to give values relative to this transform rather than relative to world origin and axes.
	 *
	 * For example a numerical UI for a two-axis gizmo that is not in a world XY/YZ/XZ plane cannot use the global
	 * axes for setting the absolute position of the plane if it wants the gizmo to remain in that plane; instead, the
	 * DisplaySpaceTransform can give a space in which X and Y values keep the gizmo in the plane.
	 *
	 * Note that this is an optional feature, as it would require tools to keep this transform up to date if they
	 * want the UI to use it, so tools could just leave it unset.
	 */
	void SetDisplaySpaceTransform(TOptional<FTransform> TransformIn);
	const TOptional<FTransform>& GetDisplaySpaceTransform() { return DisplaySpaceTransform; }

	/**
	 * Broadcast at the end of a SetDisplaySpaceTransform call that changes the display space transform.
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDisplaySpaceTransformChanged, UTransformGizmo*, TOptional<FTransform>);
	FOnDisplaySpaceTransformChanged OnDisplaySpaceTransformChanged;

	/**
	 * Broadcast at the end of a SetActiveTarget call. Using this, an adapter such as a numerical UI widget can
	 * bind to the gizmo at construction and still be able to initialize using the transform proxy once that is set.
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSetActiveTarget, UTransformGizmo*, UTransformProxy*);
	FOnSetActiveTarget OnSetActiveTarget;

	/**
	 * Broadcast at the beginning of a ClearActiveTarget call, when the ActiveTarget (if present) is not yet
	 * disconnected. Gives things a chance to unbind from it.
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnClearActiveTarget, UTransformGizmo*, UTransformProxy*);
	FOnClearActiveTarget OnAboutToClearActiveTarget;

	/** Broadcast at the end of a SetVisibility call if the visibility changes. */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnVisibilityChanged, UTransformGizmo*, bool);
	FOnVisibilityChanged OnVisibilityChanged;

protected:
	TSharedPtr<FTransformGizmoActorFactory> GizmoActorBuilder;

	FString AxisPositionBuilderIdentifier;
	FString PlanePositionBuilderIdentifier;
	FString AxisAngleBuilderIdentifier;

	// This function is called on each active GizmoActor Component to update it's hover state.
	// If the Component is not a UGizmoBaseCmponent, the client needs to provide a different implementation
	// of this function via the ToolBuilder
	TFunction<void(UPrimitiveComponent*, bool)> UpdateHoverFunction;

	// This function is called on each active GizmoActor Component to update it's coordinate system (eg world/local).
	// If the Component is not a UGizmoBaseCmponent, the client needs to provide a different implementation
	// of this function via the ToolBuilder
	TFunction<void(UPrimitiveComponent*, EToolContextCoordinateSystem)> UpdateCoordSystemFunction;

	/** List of current-active child components */
	UPROPERTY()
	TArray<UPrimitiveComponent*> ActiveComponents;

	/** 
	 * List of nonuniform scale components. Subset of of ActiveComponents. These are tracked separately so they can
	 * be hidden when Gizmo is not configured to use local axes, because UE only supports local nonuniform scaling
	 * on Components
	 */
	UPROPERTY()
	TArray<UPrimitiveComponent*> NonuniformScaleComponents;

	/** list of currently-active child gizmos */
	UPROPERTY()
	TArray<UInteractiveGizmo*> ActiveGizmos;

	/**
	 * FSubGizmoInfo stores the (UPrimitiveComponent,UInteractiveGizmo) pair for a sub-element of the widget.
	 * The ActiveComponents and ActiveGizmos arrays keep those items alive, so this is redundant information, but useful for filtering/etc
	 */
	struct FSubGizmoInfo
	{
		// note: either of these may be invalid
		TWeakObjectPtr<UPrimitiveComponent> Component;
		TWeakObjectPtr<UInteractiveGizmo> Gizmo;
	};
	TArray<FSubGizmoInfo> TranslationSubGizmos;
	TArray<FSubGizmoInfo> RotationSubGizmos;
	TArray<FSubGizmoInfo> UniformScaleSubGizmos;
	TArray<FSubGizmoInfo> NonUniformScaleSubGizmos;

	/** GizmoActors will be spawned in this World */
	UWorld* World;

	/** Current active GizmoActor that was spawned by this Gizmo. Will be destroyed when Gizmo is. */
	ATransformGizmoActor* GizmoActor;

	//
	// Axis Sources
	//


	/** Axis that points towards camera, X/Y plane tangents aligned to right/up. Shared across Gizmos, and created internally during SetActiveTarget() */
	UPROPERTY()
	UGizmoConstantFrameAxisSource* CameraAxisSource;

	// internal function that updates CameraAxisSource by getting current view state from GizmoManager
	void UpdateCameraAxisSource();


	/** X-axis source is shared across Gizmos, and created internally during SetActiveTarget() */
	UPROPERTY()
	UGizmoComponentAxisSource* AxisXSource;

	/** Y-axis source is shared across Gizmos, and created internally during SetActiveTarget() */
	UPROPERTY()
	UGizmoComponentAxisSource* AxisYSource;

	/** Z-axis source is shared across Gizmos, and created internally during SetActiveTarget() */
	UPROPERTY()
	UGizmoComponentAxisSource* AxisZSource;

	//
	// Scaling support. 
	// UE Components only support scaling in local coordinates, so we have to create separate sources for that.
	//

	/** Local X-axis source (ie 1,0,0) is shared across Scale Gizmos, and created internally during SetActiveTarget() */
	UPROPERTY()
	UGizmoComponentAxisSource* UnitAxisXSource;

	/** Y-axis source (ie 0,1,0) is shared across Scale Gizmos, and created internally during SetActiveTarget() */
	UPROPERTY()
	UGizmoComponentAxisSource* UnitAxisYSource;

	/** Z-axis source (ie 0,0,1) is shared across Scale Gizmos, and created internally during SetActiveTarget() */
	UPROPERTY()
	UGizmoComponentAxisSource* UnitAxisZSource;


	//
	// Other Gizmo Components
	//


	/** 
	 * State target is shared across gizmos, and created internally during SetActiveTarget(). 
	 * Several FChange providers are registered with this StateTarget, including the UTransformGizmo
	 * itself (IToolCommandChangeSource implementation above is called)
	 */
	UPROPERTY()
	UGizmoTransformChangeStateTarget* StateTarget;


	/**
	 * This TransformSource wraps a UGizmoComponentWorldTransformSource that is on the Gizmo Actor directly.
	 * It tracks the scaling separately (SeparateChildScale is provided as the storage for the scaling).
	 * This allows the various scaling handles to update the Transform without actually scaling the Gizmo itself.
	 */
	UPROPERTY()
	UGizmoScaledTransformSource* ScaledTransformSource;

	/**
	 * Scaling on the active target UTransformProxy is stored here. Undo/Redo events update this value via an FTransformGizmoTransformChange.
	 */
	FVector SeparateChildScale = FVector(1,1,1);

	/**
	 * This function is called by FTransformGizmoTransformChange to update SeparateChildScale on Undo/Redo.
	 */
	void ExternalSetChildScale(const FVector& NewScale);

	friend class FTransformGizmoTransformChange;
	/**
	 * Ongoing transform change is tracked here, initialized in the IToolCommandChangeSource implementation.
	 * Note that currently we only handle the SeparateChildScale this way. The transform changes on the target
	 * objects are dealt with using the editor Transaction system (and on the Proxy using a FTransformProxyChangeSource attached to the StateTarget).
	 * The Transaction system is not available at runtime, we should track the entire change in the FChange and update the proxy
	 */
	TUniquePtr<FTransformGizmoTransformChange> ActiveChange;

	/**
	 * These are used to let the translation subgizmos use raycasts into the scene to align the gizmo with scene geometry.
	 * See comment for SetWorldAlignmentFunctions().
	 */
	TUniqueFunction<bool()> ShouldAlignDestination = []() { return false; };
	TUniqueFunction<bool(const FRay&, FVector&)> DestinationAlignmentRayCaster = [](const FRay&, FVector&) {return false; };

	TUniqueFunction<bool()> IsNonUniformScaleAllowedFunc = [this]() { return CurrentCoordinateSystem == EToolContextCoordinateSystem::Local; };

	bool bDisallowNegativeScaling = false;

	// See comment for SetDisplaySpaceTransform;
	TOptional<FTransform> DisplaySpaceTransform;

public:
	// IToolCommandChangeSource implementation, used to initialize and emit ActiveChange
	virtual void BeginChange();
	virtual TUniquePtr<FToolCommandChange> EndChange();
	virtual UObject* GetChangeTarget();
	virtual FText GetChangeDescription();


protected:


	/** @return a new instance of the standard axis-translation Gizmo */
	virtual UInteractiveGizmo* AddAxisTranslationGizmo(
		UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
		IGizmoAxisSource* AxisSource,
		IGizmoTransformSource* TransformSource, 
		IGizmoStateTarget* StateTarget,
		int AxisIndex);

	/** @return a new instance of the standard plane-translation Gizmo */
	virtual UInteractiveGizmo* AddPlaneTranslationGizmo(
		UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
		IGizmoAxisSource* AxisSource,
		IGizmoTransformSource* TransformSource,
		IGizmoStateTarget* StateTarget,
		int XAxisIndex, int YAxisIndex);

	/** @return a new instance of the standard axis-rotation Gizmo */
	virtual UInteractiveGizmo* AddAxisRotationGizmo(
		UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
		IGizmoAxisSource* AxisSource,
		IGizmoTransformSource* TransformSource,
		IGizmoStateTarget* StateTarget);

	/** @return a new instance of the standard axis-scaling Gizmo */
	virtual UInteractiveGizmo* AddAxisScaleGizmo(
		UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
		IGizmoAxisSource* GizmoAxisSource, IGizmoAxisSource* ParameterAxisSource,
		IGizmoTransformSource* TransformSource,
		IGizmoStateTarget* StateTarget);

	/** @return a new instance of the standard plane-scaling Gizmo */
	virtual UInteractiveGizmo* AddPlaneScaleGizmo(
		UPrimitiveComponent* AxisComponent, USceneComponent* RootComponent,
		IGizmoAxisSource* GizmoAxisSource, IGizmoAxisSource* ParameterAxisSource,
		IGizmoTransformSource* TransformSource,
		IGizmoStateTarget* StateTarget);

	/** @return a new instance of the standard plane-scaling Gizmo */
	virtual UInteractiveGizmo* AddUniformScaleGizmo(
		UPrimitiveComponent* ScaleComponent, USceneComponent* RootComponent,
		IGizmoAxisSource* GizmoAxisSource, IGizmoAxisSource* ParameterAxisSource,
		IGizmoTransformSource* TransformSource,
		IGizmoStateTarget* StateTarget);

	// Axis and Plane TransformSources use this function to execute worldgrid snap queries
	bool PositionSnapFunction(const FVector& WorldPosition, FVector& SnappedPositionOut) const;
	bool PositionAxisDeltaSnapFunction(double AxisDelta, double& SnappedDeltaOut, int AxisIndex) const;
	FQuat RotationSnapFunction(const FQuat& DeltaRotation) const;
	bool RotationAxisAngleSnapFunction(double AxisAngleDelta, double& SnappedAxisAngleDeltaOut, int AxisIndex) const;

};

/**
 * FChange for a UTransformGizmo that applies transform change.
 * Currently only handles the scaling part of a transform.
 */
class INTERACTIVETOOLSFRAMEWORK_API FTransformGizmoTransformChange : public FToolCommandChange
{
public:
	FVector ChildScaleBefore;
	FVector ChildScaleAfter;

	/** Makes the change to the object */
	virtual void Apply(UObject* Object) override;

	/** Reverts change to the object */
	virtual void Revert(UObject* Object) override;

	/** Describes this change (for debugging) */
	virtual FString ToString() const override;
};

UCLASS(MinimalAPI)
class UGizmoViewContext : public UObject
{
	GENERATED_BODY()
public:

	// Wrapping class for the matrices so that they can be accessed in the same way
	// that they are accessed in FSceneView.
	class FMatrices
	{
	public:
		void ResetFromSceneView(const FSceneView& SceneView)
		{
			ViewMatrix = SceneView.ViewMatrices.GetViewMatrix();
			InvViewMatrix = SceneView.ViewMatrices.GetInvViewMatrix();
			ViewProjectionMatrix = SceneView.ViewMatrices.GetViewProjectionMatrix();
		}

		const FMatrix& GetViewMatrix() const { return ViewMatrix; }
		const FMatrix& GetInvViewMatrix() const { return InvViewMatrix; }
		const FMatrix& GetViewProjectionMatrix() const { return ViewProjectionMatrix; }

	protected:
		FMatrix ViewMatrix;
		FMatrix InvViewMatrix;
		FMatrix ViewProjectionMatrix;
	};

	// Use this to reinitialize the object each frame for the hovered viewport.
	void ResetFromSceneView(const FSceneView& SceneView)
	{
		UnscaledViewRect = SceneView.UnscaledViewRect;
		ViewMatrices.ResetFromSceneView(SceneView);
		bIsPerspectiveProjection = SceneView.IsPerspectiveProjection();
		ViewLocation = SceneView.ViewLocation;
	}

	// FSceneView-like functions/properties:
	FVector GetViewRight() const { return ViewMatrices.GetViewMatrix().GetColumn(0); }
	FVector GetViewUp() const { return ViewMatrices.GetViewMatrix().GetColumn(1); }
	FVector GetViewDirection() const { return ViewMatrices.GetViewMatrix().GetColumn(2); }

	// As a function just for similarity with FSceneView
	bool IsPerspectiveProjection() const { return bIsPerspectiveProjection; }

	FVector4 WorldToScreen(const FVector& WorldPoint) const
	{
		return ViewMatrices.GetViewProjectionMatrix().TransformFVector4(FVector4(WorldPoint, 1));
	}

	bool ScreenToPixel(const FVector4& ScreenPoint, FVector2D& OutPixelLocation) const
	{
		// implementation copied from FSceneView::ScreenToPixel() 

		if (ScreenPoint.W != 0.0f)
		{
			//Reverse the W in the case it is negative, this allow to manipulate a manipulator in the same direction when the camera is really close to the manipulator.
			double InvW = (ScreenPoint.W > 0.0 ? 1.0 : -1.0) / ScreenPoint.W;
			double Y = (GProjectionSignY > 0.0) ? ScreenPoint.Y : 1.0 - ScreenPoint.Y;
			OutPixelLocation = FVector2D(
				UnscaledViewRect.Min.X + (0.5 + ScreenPoint.X * 0.5 * InvW) * UnscaledViewRect.Width(),
				UnscaledViewRect.Min.Y + (0.5 - Y * 0.5 * InvW) * UnscaledViewRect.Height()
			);
			return true;
		}
		else
		{
			return false;
		}
	}

	FMatrices ViewMatrices;
	FIntRect UnscaledViewRect;
	FVector	ViewLocation;

protected:
	bool bIsPerspectiveProjection;
};

namespace UE
{
	namespace TransformGizmoUtil
	{
		//
		// The functions below are helper functions that simplify usage of a UCombinedTransformGizmoContextObject 
		// that is registered as a ContextStoreObject in an InteractiveToolsContext
		//

		/**
		 * If one does not already exist, create a new instance of UCombinedTransformGizmoContextObject and add it to the
		 * ToolsContext's ContextObjectStore
		 * @return true if the ContextObjectStore now has a UCombinedTransformGizmoContextObject (whether it already existed, or was created)
		 */
		INTERACTIVETOOLSFRAMEWORK_API bool RegisterTransformGizmoContextObject(UInteractiveToolsContext* ToolsContext);

		/**
		 * Remove any existing UCombinedTransformGizmoContextObject from the ToolsContext's ContextObjectStore
		 * @return true if the ContextObjectStore no longer has a UCombinedTransformGizmoContextObject (whether it was removed, or did not exist)
		 */
		INTERACTIVETOOLSFRAMEWORK_API bool DeregisterTransformGizmoContextObject(UInteractiveToolsContext* ToolsContext);


		/**
		 * Spawn a new standard 3-axis Transform gizmo (see UCombinedTransformGizmoContextObject::Create3AxisTransformGizmo for details)
		 * GizmoManager's ToolsContext must have a UCombinedTransformGizmoContextObject registered (see UCombinedTransformGizmoContextObject for details)
		 */
		INTERACTIVETOOLSFRAMEWORK_API UTransformGizmo* Create3AxisTransformGizmo(UInteractiveGizmoManager* GizmoManager, void* Owner = nullptr, const FString& InstanceIdentifier = FString());
		/**
		 * Spawn a new standard 3-axis Transform gizmo (see UCombinedTransformGizmoContextObject::Create3AxisTransformGizmo for details)
		 * ToolManager's ToolsContext must have a UCombinedTransformGizmoContextObject registered (see UCombinedTransformGizmoContextObject for details)
		 */
		INTERACTIVETOOLSFRAMEWORK_API UTransformGizmo* Create3AxisTransformGizmo(UInteractiveToolManager* ToolManager, void* Owner = nullptr, const FString& InstanceIdentifier = FString());


		/**
		 * Spawn a new custom Transform gizmo (see UCombinedTransformGizmoContextObject::CreateCustomTransformGizmo for details)
		 * GizmoManager's ToolsContext must have a UCombinedTransformGizmoContextObject registered (see UCombinedTransformGizmoContextObject for details)
		 */
		INTERACTIVETOOLSFRAMEWORK_API UTransformGizmo* CreateCustomTransformGizmo(UInteractiveGizmoManager* GizmoManager, ETransformGizmoSubElements Elements, void* Owner = nullptr, const FString& InstanceIdentifier = FString());
		/**
		 * Spawn a new custom Transform gizmo (see UCombinedTransformGizmoContextObject::CreateCustomTransformGizmo for details)
		 * ToolManager's ToolsContext must have a UCombinedTransformGizmoContextObject registered (see UCombinedTransformGizmoContextObject for details)
		 */
		INTERACTIVETOOLSFRAMEWORK_API UTransformGizmo* CreateCustomTransformGizmo(UInteractiveToolManager* ToolManager, ETransformGizmoSubElements Elements, void* Owner = nullptr, const FString& InstanceIdentifier = FString());


		/**
		 * Spawn a new custom Transform gizmo (see UCombinedTransformGizmoContextObject::CreateCustomTransformGizmo for details)
		 * GizmoManager's ToolsContext must have a UCombinedTransformGizmoContextObject registered (see UCombinedTransformGizmoContextObject for details)
		 */
		INTERACTIVETOOLSFRAMEWORK_API UTransformGizmo* CreateCustomRepositionableTransformGizmo(UInteractiveGizmoManager* GizmoManager, ETransformGizmoSubElements Elements, void* Owner = nullptr, const FString& InstanceIdentifier = FString());
		/**
		 * Spawn a new custom Transform gizmo (see UCombinedTransformGizmoContextObject::CreateCustomRepositionableTransformGizmo for details)
		 * ToolManager's ToolsContext must have a UCombinedTransformGizmoContextObject registered (see UCombinedTransformGizmoContextObject for details)
		 */
		INTERACTIVETOOLSFRAMEWORK_API UTransformGizmo* CreateCustomRepositionableTransformGizmo(UInteractiveToolManager* ToolManager, ETransformGizmoSubElements Elements, void* Owner = nullptr, const FString& InstanceIdentifier = FString());
	}
}


/**
 * UCombinedTransformGizmoContextObject is a utility object that registers a set of Gizmo Builders
 * for UCombinedTransformGizmo and variants. The intended usage is to call RegisterGizmosWithManager(),
 * and then the UCombinedTransformGizmoContextObject will register itself as a ContextObject in the
 * InteractiveToolsContext's ContextObjectStore. Then the Create3AxisTransformGizmo()/etc functions
 * will spawn different variants of UCombinedTransformGizmo. The above UE::TransformGizmoUtil:: functions
 * will look up the UCombinedTransformGizmoContextObject instance in the ContextObjectStore and then
 * call the associated function below.
 */
UCLASS(Transient)
class INTERACTIVETOOLSFRAMEWORK_API UTransformGizmoContextObject : public UObject
{
	GENERATED_BODY()
public:

public:
	// builder identifiers for default gizmo types. Perhaps should have an API for this...
	static  const FString DefaultAxisPositionBuilderIdentifier;
	static  const FString DefaultPlanePositionBuilderIdentifier;
	static  const FString DefaultAxisAngleBuilderIdentifier;
	static  const FString DefaultThreeAxisTransformBuilderIdentifier;
	static  const FString CustomThreeAxisTransformBuilderIdentifier;
	static  const FString CustomRepositionableThreeAxisTransformBuilderIdentifier;

	void RegisterGizmosWithManager(UInteractiveToolManager* ToolManager);
	void DeregisterGizmosWithManager(UInteractiveToolManager* ToolManager);

	/**
	 * Activate a new instance of the default 3-axis transformation Gizmo. RegisterDefaultGizmos() must have been called first.
	 * @param Owner optional void pointer to whatever "owns" this Gizmo. Allows Gizmo to later be deleted using DestroyAllGizmosByOwner()
	 * @param InstanceIdentifier optional client-defined *unique* string that can be used to locate this instance
	 * @return new Gizmo instance that has been created and initialized
	 */
	virtual UTransformGizmo* Create3AxisTransformGizmo(UInteractiveGizmoManager* GizmoManager, void* Owner = nullptr, const FString& InstanceIdentifier = FString());

	/**
	 * Activate a new customized instance of the default 3-axis transformation Gizmo, with only certain elements included. RegisterDefaultGizmos() must have been called first.
	 * @param Elements flags that indicate which standard gizmo sub-elements should be included
	 * @param Owner optional void pointer to whatever "owns" this Gizmo. Allows Gizmo to later be deleted using DestroyAllGizmosByOwner()
	 * @param InstanceIdentifier optional client-defined *unique* string that can be used to locate this instance
	 * @return new Gizmo instance that has been created and initialized
	 */
	virtual UTransformGizmo* CreateCustomTransformGizmo(UInteractiveGizmoManager* GizmoManager, ETransformGizmoSubElements Elements, void* Owner = nullptr, const FString& InstanceIdentifier = FString());

	/**
	 * Variant of CreateCustomTransformGizmo that creates a URepositionableTransformGizmo, which is an extension to UCombinedTransformGizmo that
	 * supports various snapping interactions
	 */
	virtual UTransformGizmo* CreateCustomRepositionableTransformGizmo(UInteractiveGizmoManager* GizmoManager, ETransformGizmoSubElements Elements, void* Owner = nullptr, const FString& InstanceIdentifier = FString());

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGizmoCreated, UTransformGizmo*);
	FOnGizmoCreated OnGizmoCreated;

protected:
	TSharedPtr<FTransformGizmoActorFactory> GizmoActorBuilder;
	bool bDefaultGizmosRegistered = false;
};