// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "ComponentSourceInterfaces.h"

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/Interface.h"
#include "ComponentSourceInterfaces.h"
#include "UObject/ObjectMacros.h"
#include "Containers/Array.h"
#include "Templates/Casts.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/EngineTypes.h"

#include "ToolContextInterfaces.generated.h"



// predeclarations so we don't have to include these in all tools
class UWorld;
class AActor;
class UActorComponent;
class FToolCommandChange;
class UPrimitiveComponent;
class UPackage;
class FPrimitiveDrawInterface;
class FSceneView;
class UInteractiveToolManager;
class UInteractiveGizmoManager;
class UTexture2D;
class UToolTargetManager;
class UObject;
class UClass;
struct FMeshDescription;
struct FSceneHitQueryRequest;


#if WITH_EDITOR
class HHitProxy;
#endif

struct INTERACTIVETOOLSFRAMEWORK_API FSceneQueryVisibilityFilter
{
	/** Optional: components to consider invisible even if they aren't. */
	const TArray<const UPrimitiveComponent*>* ComponentsToIgnore = nullptr;

	/** Optional: components to consider visible even if they aren't. */
	const TArray<const UPrimitiveComponent*>* InvisibleComponentsToInclude = nullptr;

	/** @return true if the Component is currently configured as visible (does not consider ComponentsToIgnore or InvisibleComponentsToInclude lists) */
	 bool IsVisible(const UPrimitiveComponent* Component) const;
};

/**
* Configuration variables for a USceneSnappingManager hit query request.
*/
struct INTERACTIVETOOLSFRAMEWORK_API FSceneHitQueryRequest
{
	/** scene query ray */
	FRay WorldRay;

	bool bWantHitGeometryInfo = false;

	FSceneQueryVisibilityFilter VisibilityFilter;
};

/**
* Computed result of a USceneSnappingManager hit query request
*/
struct INTERACTIVETOOLSFRAMEWORK_API FSceneHitQueryResult
{
	/** Actor that owns hit target */
	AActor* TargetActor = nullptr;
	/** Component that owns hit target */
	UPrimitiveComponent* TargetComponent = nullptr;

	/** hit position*/
	FVector Position = FVector::ZeroVector;
	/** hit normal */
	FVector Normal = FVector::ZAxisVector;

	/** integer ID of triangle that was hit */
	int HitTriIndex = -1;
	/** Vertices of triangle that was hit (for debugging, may not be set) */
	FVector TriVertices[3];

	FHitResult HitResult;

	void InitializeHitResult(const FSceneHitQueryRequest& FromRequest);
};

/**
 * FToolBuilderState is a bucket of state information that a ToolBuilder might need
 * to construct a Tool. This information comes from a level above the Tools framework,
 * and depends on the context we are in (Editor vs Runtime, for example).
 */
struct INTERACTIVETOOLSFRAMEWORK_API FToolBuilderState
{
	/** The current UWorld */
	UWorld* World = nullptr;
	/** The current ToolManager */
	UInteractiveToolManager* ToolManager = nullptr;
	/** The current TargetManager */
	UToolTargetManager* TargetManager = nullptr;
	/** The current GizmoManager */
	UInteractiveGizmoManager* GizmoManager = nullptr;

	/** Current selected Actors. May be empty or nullptr. */
	TArray<AActor*> SelectedActors;
	/** Current selected Components. May be empty or nullptr. */
	TArray<UActorComponent*> SelectedComponents;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FToolBuilderState() = default;
	FToolBuilderState(FToolBuilderState&&) = default;
	FToolBuilderState(const FToolBuilderState&) = default;
	FToolBuilderState& operator=(FToolBuilderState&&) = default;
	FToolBuilderState& operator=(const FToolBuilderState&) = default;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
};


/**
 * FViewCameraState is a bucket of state information that a Tool might
 * need to implement interactions that depend on the current scene view.
 */
struct INTERACTIVETOOLSFRAMEWORK_API FViewCameraState
{
	/** Current camera/head position */
	FVector Position;
	/** Current camera/head orientation */
	FQuat Orientation;
	/** Current Horizontal Field of View Angle in Degrees. Only relevant if bIsOrthographic is false. */
	float HorizontalFOVDegrees;
	/** Current width of viewport in world space coordinates. Only valid if bIsOrthographic is true. */
	float OrthoWorldCoordinateWidth;
	/** Current Aspect Ratio */
	float AspectRatio;
	/** Is current view an orthographic view */
	bool bIsOrthographic;
	/** Is current view a VR view */
	bool bIsVR;

	/** @return "right"/horizontal direction in camera plane */
	FVector Right() const { return Orientation.GetAxisY(); }
	/** @return "up"/vertical direction in camera plane */
	FVector Up() const { return Orientation.GetAxisZ(); }
	/** @return forward camera direction */
	FVector Forward() const { return Orientation.GetAxisX(); }

	/** @return scaling factor that should be applied to PDI thickness/size */
	float GetPDIScalingFactor() const { return HorizontalFOVDegrees / 90.0f; }

	/** @return FOV normalization factor that should be applied when comparing angles */
	float GetFOVAngleNormalizationFactor() const { return HorizontalFOVDegrees / 90.0f; }

};



/** Types of Snap Queries that a ToolsContext parent may support, that Tools may request */
UENUM()
enum class ESceneSnapQueryType
{
	/** snapping a position */
	Position = 1,
	Rotation = 2
};

/** Types of snap targets that a Tool may want to run snap queries against. */
UENUM()
enum class ESceneSnapQueryTargetType
{
	None = 0,
	/** Consider any mesh vertex */
	MeshVertex = 1,
	/** Consider any mesh edge */
	MeshEdge = 2,
	/** Grid Snapping */
	Grid = 4,

	All = MeshVertex | MeshEdge | Grid
};
ENUM_CLASS_FLAGS(ESceneSnapQueryTargetType);

/**
 * Configuration variables for a IToolsContextQueriesAPI snap query request.
 */
struct INTERACTIVETOOLSFRAMEWORK_API FSceneSnapQueryRequest
{
	/** What type of snap query geometry is this */
	ESceneSnapQueryType RequestType = ESceneSnapQueryType::Position;
	/** What does caller want to try to snap to */
	ESceneSnapQueryTargetType TargetTypes = ESceneSnapQueryTargetType::Grid;

	/** Optional explicitly specified position grid */
	TOptional<FVector> GridSize{};

	/** Optional explicitly specified rotation grid */
	TOptional<FRotator> RotGridSize{};

	/** Snap input position */
	FVector Position = FVector::ZeroVector;
	/**
	 *  When considering if one point is close enough to another point for snapping purposes, they
	 *  must deviate less than this number of degrees (in visual angle) to be considered an acceptable snap position.
	 */
	float VisualAngleThresholdDegrees = 15.0;

	/** Snap input direction */
	FVector Direction;
	/** Another direction must deviate less than this number of degrees from Direction to be considered an acceptable snap direction */
	float DirectionAngleThresholdDegrees;

	/** Snap input rotation delta */
	FQuat DeltaRotation = FQuat(EForceInit::ForceInitToZero);

	/** Optional: components to consider invisible even if they aren't. */
	const TArray<const UPrimitiveComponent*>* ComponentsToIgnore = nullptr;
	/** Optional: components to consider visible even if they aren't. */
	const TArray<const UPrimitiveComponent*>* InvisibleComponentsToInclude = nullptr;

};


/**
 * Computed result of a IToolsContextQueriesAPI snap query request
 */
struct INTERACTIVETOOLSFRAMEWORK_API FSceneSnapQueryResult
{
	/** Actor that owns snap target */
	AActor* TargetActor = nullptr;
	/** Component that owns snap target */
	UActorComponent* TargetComponent = nullptr;
	/** What kind of geometric element was snapped to */
	ESceneSnapQueryTargetType TargetType = ESceneSnapQueryTargetType::None;

	/** Snap position (may not be set depending on query types) */
	FVector Position;
	/** Snap normal (may not be set depending on query types) */
	FVector Normal;
	/** Snap direction (may not be set depending on query types) */
	FVector Direction;
	/** Snap rotation delta (may not be set depending on query types) */
	FQuat   DeltaRotation;

	/** Vertices of triangle that contains result (for debugging, may not be set) */
	FVector TriVertices[3];
	/** Vertex/Edge index we snapped to in triangle */
	int TriSnapIndex;

};



/** Types of standard materials that Tools may request from Context */
UENUM()
enum class EStandardToolContextMaterials
{
	/** White material that displays vertex colors set on mesh */
	VertexColorMaterial = 1
};


/** Types of coordinate systems that a Tool/Gizmo might use */
UENUM()
enum class EToolContextCoordinateSystem
{
	World = 0,
	Local = 1
};

/**
 * High-level configuration options for a standard 3D translate/rotate/scale Gizmo, like is commonly used in 3D DCC tools, game editors, etc
 * This is meant to be used to convey UI-level settings to Tools/Gizmos, eg like the W/E/R toggles for Rranslate/Rotate/Scale in Maya or UE Editor.
 * More granular control over precise gizmo elements is available through other mechanisms, eg the ETransformGizmoSubElements flags in UCombinedTransformGizmo
 */
UENUM()
enum class EToolContextTransformGizmoMode : uint8
{
	/** Hide all Gizmo sub-elements */
	NoGizmo = 0,
	/** Only Show Translation sub-elements */
	Translation = 1,
	/** Only Show Rotation sub-elements */
	Rotation = 2,
	/** Only Show Scale sub-elements */
	Scale = 3,
	/** Show all available Gizmo sub-elements */
	Combined = 8
};



/**
 * Snapping configuration settings/state
 */
struct FToolContextSnappingConfiguration
{
	/** Specify whether position snapping should be applied */
	bool bEnablePositionGridSnapping = false;
	/** Specify position grid spacing or cell dimensions */
	FVector PositionGridDimensions = FVector::ZeroVector;

	/** Specify whether rotation snapping should be applied */
	bool bEnableRotationGridSnapping = false;
	/** Specify rotation snapping step size */
	FRotator RotationGridAngles = FRotator::ZeroRotator;

	/** Specify whether Position snapping in World Coordinate System should be Absolute or Relative */
	bool bEnableAbsoluteWorldSnapping = false;
};

/**
 * Users of the Tools Framework need to implement IToolsContextQueriesAPI to provide
 * access to scene state information like the current UWorld, active USelections, etc.
 */
class IToolsContextQueriesAPI
{
public:
	virtual ~IToolsContextQueriesAPI() {}

	/**
	 * @return the UWorld currently being targetted by the ToolsContext, default location for new Actors/etc
	 */
	virtual UWorld* GetCurrentEditingWorld() const = 0;

	/**
	 * Collect up current-selection information for the current scene state (ie what is selected in Editor, etc)
	 * @param StateOut this structure is populated with available state information
	 */
	virtual void GetCurrentSelectionState(FToolBuilderState& StateOut) const = 0;


	/**
	 * Request information about current view state
	 * @param StateOut this structure is populated with available state information
	 */
	virtual void GetCurrentViewState(FViewCameraState& StateOut) const = 0;


	/**
	 * Request current external coordinate-system setting
	 */
	virtual EToolContextCoordinateSystem GetCurrentCoordinateSystem() const = 0;	

	/**
	 * Request current external Gizmo Mode setting. Defaulting this to Combined gizmo as this was the default behavior before UE-5.2.
	 */
	virtual EToolContextTransformGizmoMode GetCurrentTransformGizmoMode() const { return EToolContextTransformGizmoMode::Combined; }

	/**
	 * Request current external snapping settings. Defaults to no snapping.
	 */
	virtual FToolContextSnappingConfiguration GetCurrentSnappingSettings() const { return FToolContextSnappingConfiguration(); }
	/**
	 * Try to find Snap Targets in the scene that satisfy the Snap Query.
	 * @param Request snap query configuration
	 * @param Results list of potential snap results
	 * @return true if any valid snap target was found
	 * @warning implementations are not required (and may not be able) to support snapping
	 */
	virtual bool ExecuteSceneSnapQuery(const FSceneSnapQueryRequest& Request, TArray<FSceneSnapQueryResult>& Results ) const = 0;

	/**
	 * Many tools need standard types of materials that the user should provide (eg a vertex-color material, etc)
	 * @param MaterialType the type of material being requested
	 * @return Instance of material to use for this purpose
	 */
	virtual UMaterialInterface* GetStandardMaterial(EStandardToolContextMaterials MaterialType) const = 0;

#if WITH_EDITOR
	/**
	* When selecting, sometimes we need a hit proxy rather than a physics trace or other raycast.                                                                   
	*/
	virtual HHitProxy* GetHitProxy(int32 X, int32 Y) const = 0;
#endif
};




/** Level of severity of messages emitted by Tool framework */
UENUM()
enum class EToolMessageLevel
{
	/** Development message goes into development log */
	Internal = 0,
	/** User message should appear in user-facing log */
	UserMessage = 1,
	/** Notification message should be shown in a non-modal notification window */
	UserNotification = 2,
	/** Warning message should be shown in a non-modal notification window with panache */
	UserWarning = 3,
	/** Error message should be shown in a modal notification window */
	UserError = 4
};


/** Type of change we want to apply to a selection */
UENUM()
enum class ESelectedObjectsModificationType
{
	Replace = 0,
	Add = 1,
	Remove = 2,
	Clear = 3
};


/** Represents a change to a set of selected Actors and Components */
struct FSelectedOjectsChangeList
{
	/** How should this list be interpreted in the context of a larger selection set */
	ESelectedObjectsModificationType ModificationType;
	/** List of Actors */
	TArray<AActor*> Actors;
	/** List of Componets */
	TArray<UActorComponent*> Components;
};


/**
 * Users of the Tools Framework need to implement IToolsContextTransactionsAPI so that
 * the Tools have the ability to create Transactions and emit Changes. Note that this is
 * technically optional, but that undo/redo won't be supported without it.
 */
class IToolsContextTransactionsAPI
{
public:
	virtual ~IToolsContextTransactionsAPI() {}

	/**
	 * Request that context display message information.
	 * @param Message text of message
	 * @param Level severity level of message
	 */
	virtual void DisplayMessage(const FText& Message, EToolMessageLevel Level) = 0;

	/** 
	 * Forward an invalidation request from Tools framework, to cause repaint/etc. 
	 * This is not always necessary but in some situations (eg in Non-Realtime mode in Editor)
	 * a redraw will not happen every frame. 
	 * See UInputRouter for options to enable auto-invalidation.
	 */
	virtual void PostInvalidation() = 0;
	
	/**
	 * Begin a Transaction, whatever this means in the current Context. For example in the
	 * Editor it means open a GEditor Transaction. You must call EndUndoTransaction() after calling this.
	 * @param Description text description of the transaction that could be shown to user
	 */
	virtual void BeginUndoTransaction(const FText& Description) = 0;

	/**
	 * Complete the Transaction. Assumption is that Begin/End are called in pairs.
	 */
	virtual void EndUndoTransaction() = 0;

	/**
	 * Insert an FChange into the transaction history in the current Context. 
	 * This cannot be called between Begin/EndUndoTransaction, the FChange should be 
	 * automatically inserted into a Transaction.
	 * @param TargetObject The UObject this Change is applied to
	 * @param Change The Change implementation
	 * @param Description text description of the transaction that could be shown to user
	 */
	virtual void AppendChange(UObject* TargetObject, TUniquePtr<FToolCommandChange> Change, const FText& Description) = 0;



	/**
	 * Request a modification to the current selected objects
	 * @param SelectionChange desired modification to current selection
	 * @return true if the selection change could be applied
	 */
	virtual bool RequestSelectionChange(const FSelectedOjectsChangeList& SelectionChange) = 0;

};

UENUM()
enum class EViewInteractionState {
	None = 0,
	Hovered = 1,
	Focused = 2
};
ENUM_CLASS_FLAGS(EViewInteractionState);

/**
 * Users of the Tools Framework need to implement IToolsContextRenderAPI to allow
 * Tools, Indicators, and Gizmos to make low-level rendering calls for things like line drawing.
 * This API will be passed to eg UInteractiveTool::Render(), so access is only provided when it
 * makes sense to call the functions
 */
class IToolsContextRenderAPI
{
public:
	virtual ~IToolsContextRenderAPI() {}

	/** @return Current PDI */
	virtual FPrimitiveDrawInterface* GetPrimitiveDrawInterface() = 0;

	/** @return Current FSceneView */
	virtual const FSceneView* GetSceneView() = 0;

	/** @return Current Camera State for this Render API */
	virtual FViewCameraState GetCameraState() = 0;

	/** @return Current interaction state of the view to render */
	virtual EViewInteractionState GetViewInteractionState() = 0;
};


/**
 * FGeneratedStaticMeshAssetConfig is passed to IToolsContextAssetAPI::GenerateStaticMeshActor() to
 * provide the underlying mesh, materials, and configuration settings. Note that all implementations
 * may not use/respect all settings.
 */
struct  FGeneratedStaticMeshAssetConfig
{
	INTERACTIVETOOLSFRAMEWORK_API ~FGeneratedStaticMeshAssetConfig();

	TUniquePtr<FMeshDescription> MeshDescription;
	TArray<UMaterialInterface*> Materials;

	bool bEnableRecomputeNormals = false;
	bool bEnableRecomputeTangents = false;

	bool bEnablePhysics = true;
	bool bEnableComplexAsSimpleCollision = true;
};


/**
 * Users of the Tools Framework need to provide an IToolsContextAssetAPI implementation
 * that allows Packages and Assets to be created/saved. Note that this is not strictly
 * necessary, for example a trivial implementation could just store things in the Transient
 * package and not do any saving.
 */
class IToolsContextAssetAPI
{
public:
	virtual ~IToolsContextAssetAPI() {}

	/** 
	 * Get a path to save assets in that is relative to the given UWorld. 
	 */
	virtual FString GetWorldRelativeAssetRootPath(const UWorld* World) = 0;

	/** 
	 * Get a "currently-visible/selected" location to save assets in. For example the currently-visible path in the Editor Content Browser.
	 */
	virtual FString GetActiveAssetFolderPath() = 0;

	/** Allow the user to select a path and filename for an asset using a modal dialog */
	virtual FString InteractiveSelectAssetPath(const FString& DefaultAssetName, const FText& DialogTitleMessage) = 0;

	/**
	 * Creates a new package for an asset
	 * @param FolderPath path for new package
	 * @param AssetBaseName base name for asset
	 * @param UniqueAssetNameOut unique name in form of AssetBaseName##, where ## is a unqiue index
	 * @return new package
	 */
	virtual UPackage* MakeNewAssetPackage(const FString& FolderPath, const FString& AssetBaseName, FString& UniqueAssetNameOut) = 0;

	/** Request saving of asset to persistent storage via something like an interactive popup dialog */
	virtual void InteractiveSaveGeneratedAsset(UObject* Asset, UPackage* AssetPackage) = 0;

	/** Autosave asset to persistent storage */
	virtual void AutoSaveGeneratedAsset(UObject* Asset, UPackage* AssetPackage) = 0;

	/** Notify that asset has been created and is dirty */
	virtual void NotifyGeneratedAssetModified(UObject* Asset, UPackage* AssetPackage) = 0;



	/**
	 * Create a new UStaticMeshAsset for the provided mesh, then a new UStaticMeshComponent and AStaticMeshActor in the TargetWorld
	 * @param TargetWorld world to create new Actor in
	 * @param Transform transform to set on Actor
	 * @param ObjectBaseName a base name for the Asset and Actor. Unique name will be generated by appending a number, if a name collision occurs.
	 * @param AssetConfig configuration information for the new Asset
	 * @return new AStaticMeshActor with new Asset assigned to it's UStaticMeshComponent
	 * @warning this function may return null. In this case the asset creation failed and/or the user cancelled during the process, if it was interactive.
	 */
	virtual AActor* GenerateStaticMeshActor(
		UWorld* TargetWorld,
		FTransform Transform,
		FString ObjectBaseName,
		FGeneratedStaticMeshAssetConfig&& AssetConfig) = 0;


	/**
	 * Save a generated UTexture2D as an Asset.
	 * Assumption is that the texture was generated in code and is in the Transient package.
	 * @param GeneratedTexture Texture2D to save
	 * @param ObjectBaseName a base name for the Asset. Unique name will be generated by appending a number, if a name collision occurs.
	 * @param RelativeToAsset New texture will be saved at the same path as the UPackage for this UObject, which we assume to be an Asset (eg like a UStaticMesh)
	 */
	virtual bool SaveGeneratedTexture2D(
		UTexture2D* GeneratedTexture,
		FString ObjectBaseName,
		const UObject* RelativeToAsset)
	{
		check(false); return false;
	}
};

UCLASS()
class INTERACTIVETOOLSFRAMEWORK_API USceneSnappingManager : public UObject
{
	GENERATED_BODY()
public:

	/**
	* Try to find a Hit Object in the scene that satisfies the Hit Query
	* @param Request hit query configuration
	* @param Results hit query result
	* @return true if any valid hit target was found
	* @warning implementations are not required (and may not be able) to support hit testing
	*/
	virtual bool ExecuteSceneHitQuery(const FSceneHitQueryRequest& Request, FSceneHitQueryResult& ResultOut) const
	{
		return false;
	}

	/**
	* Try to find Snap Targets/Results in the scene that satisfy the Snap Query.
	* @param Request snap query configuration
	* @param Results list of potential snap results
	* @return true if any valid snap target/result was found
	* @warning implementations are not required (and may not be able) to support snapping
	*/
	virtual bool ExecuteSceneSnapQuery(const FSceneSnapQueryRequest& Request, TArray<FSceneSnapQueryResult>& ResultsOut) const
	{
		return false;
	}

public:
	/**
	 * @return existing USceneSnappingManager registered in context store via the ToolManager, or nullptr if not found
	 */
	static USceneSnappingManager* Find(UInteractiveToolManager* ToolManager);

	/**
	 * @return existing USceneSnappingManager registered in context store via the ToolManager, or nullptr if not found
	 */
	static USceneSnappingManager* Find(UInteractiveGizmoManager* GizmoManager);

};

/**
 * A context object store allows tools to get access to arbitrary objects which expose data or APIs to enable additional functionality.
  * Some example use cases of context objects:
  *   - A tool builder may disallow a particular tool if a needed API object is not present in the context store.
  *   - A tool may allow extra actions if it has access to a particular API object in the context store.
  *   - A tool may choose to initialize itself differently based on the presence of a selection-holding data object in the context store.
 */
UCLASS(Transient)
class INTERACTIVETOOLSFRAMEWORK_API UContextObjectStore : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Finds the first the context object of the given type. Can return a subclass of the given type.
	 * @returns the found context object, casted to the given type, or nullptr if none matches.
	 */
	template <typename TObjectType>
	TObjectType* FindContext() const
	{
		for (UObject* ContextObject : ContextObjects)
		{
			if (TObjectType* CastedObject = Cast<TObjectType>(ContextObject))
			{
				return CastedObject;
			}
		}

		if (UContextObjectStore* ParentStore = Cast<UContextObjectStore>(GetOuter()))
		{
			return ParentStore->FindContext<TObjectType>();
		}

		return nullptr;
	}

	/**
	 * Finds the first context object that derives from the given class.
	 * @returns the found context object, or nullptr if none matches.
	 */
	UObject* FindContextByClass(UClass* InClass) const;

	/**
	 * Adds a data object to the tool manager's set of shared data objects.
	 * @returns true if the addition is successful.
	 */
	bool AddContextObject(UObject* InContextObject);

	/**
	 * Removes a data object from the tool manager's set of shared data objects.
	 * @returns true if the removal is successful.
	 */
	bool RemoveContextObject(UObject* InContextObject);

	/**
	 * Removes any data objects from the tool manager's set of shared data objects that are of type @param InClass
	 * @returns true if any objects are removed.
	 */
	bool RemoveContextObjectsOfType(const UClass* InClass);

	template <class TObjectType>
	bool RemoveContextObjectsOfType()
	{
		return RemoveContextObjectsOfType(TObjectType::StaticClass());
	}

	/**
	 * Shuts down the context object store, releasing hold on any stored content objects.
	 */
	void Shutdown();

protected:
	UPROPERTY()
	TArray<UObject*> ContextObjects;
};

// UInterface for IInteractiveToolCameraFocusAPI
UINTERFACE(MinimalAPI)
class UInteractiveToolCameraFocusAPI : public UInterface
{
	GENERATED_BODY()
};

/**
 * IInteractiveToolCameraFocusAPI provides two functions that can be
 * used to extract "Focus" / "Region of Interest" information about an
 * active Tool:
 *
 * GetWorldSpaceFocusBox() - provides a bounding box for an "active region" if one is known.
 *   An example of using the FocusBox would be to center/zoom the camera in a 3D viewport
 *   onto this box when the user hits a hotkey (eg 'f' in the Editor).
 *   Should default to the entire active object, if no subregion is available.
 *
 * GetWorldSpaceFocusPoint() - provides a "Focus Point" at the cursor ray if one is known.
 *   This can be used to (eg) center the camera at the focus point.
 *
 * The above functions should not be called unless the corresponding SupportsX() function returns true.
 */
class IInteractiveToolCameraFocusAPI
{
	GENERATED_BODY()
public:

	/**
	 * @return true if the implementation can provide a Focus Box
	 */
	virtual bool SupportsWorldSpaceFocusBox() { return false; }

	/**
	 * @return the current Focus Box
	 */
	virtual FBox GetWorldSpaceFocusBox() { return FBox(); }

	/**
	 * @return true if the implementation can provide a Focus Point
	 */
	virtual bool SupportsWorldSpaceFocusPoint() { return false; }

	/**
	 * @param WorldRay 3D Ray that should be used to find the focus point, generally ray under cursor
	 * @param PointOut computed Focus Point
	 * @return true if a Focus Point was found, can return false if (eg) the ray missed the target objects
	 */
	virtual bool GetWorldSpaceFocusPoint(const FRay& WorldRay, FVector& PointOut) { return false; }


};







// UInterface for IInteractiveToolNestedAcceptCancelAPI
UINTERFACE(MinimalAPI)
class UInteractiveToolNestedAcceptCancelAPI : public UInterface
{
	GENERATED_BODY()
};

/**
 * IInteractiveToolNestedAcceptCancelAPI provides an API for a Tool to publish
 * intent and ability to Accept or Cancel sub-operations. For example in a Tool
 * that has an editable active Selection, we might want the Escape hotkey to
 * Clear any active selection, and then on a second press, to Cancel the Tool.
 * This API allows a Tool to say "I can consume a Cancel action", and similarly
 * for Accept (although this is much less common).
 */
class IInteractiveToolNestedAcceptCancelAPI
{
	GENERATED_BODY()
public:

	/**
	 * @return true if the implementor of this API may be able to consume a Cancel action
	 */
	virtual bool SupportsNestedCancelCommand() { return false; }

	/**
	 * @return true if the implementor of this API can currently consume a Cancel action
	 */
	virtual bool CanCurrentlyNestedCancel() { return false; }

	/**
	 * Called by Editor levels to tell the implementor (eg Tool) to execute a nested Cancel action
	 * @return true if the implementor consumed the Cancel action
	 */
	virtual bool ExecuteNestedCancelCommand() { return false; }



	/**
	 * @return true if the implementor of this API may be able to consume an Accept action
	 */
	virtual bool SupportsNestedAcceptCommand() { return false; }

	/**
	 * @return true if the implementor of this API can currently consume an Accept action
	 */
	virtual bool CanCurrentlyNestedAccept() { return false; }

	/**
	 * Called by Editor levels to tell the implementor (eg Tool) to execute a nested Accept action
	 * @return true if the implementor consumed the Accept action
	 */
	virtual bool ExecuteNestedAcceptCommand() { return false; }


};


// UInterface for IInteractiveToolExclusiveToolAPI
UINTERFACE(MinimalAPI)
class UInteractiveToolExclusiveToolAPI : public UInterface
{
	GENERATED_BODY()
};

/**
 * IInteractiveToolExclusiveToolAPI provides an API to inform the
 * ToolManager about tool exclusivity. An exclusive tool prevents other
 * tools from building & activating while the tool is active. This is
 * useful in scenarios where tools want to enforce an explicit Accept,
 * Cancel or Complete user input to exit the tool.
 */
class IInteractiveToolExclusiveToolAPI
{
	GENERATED_BODY()
};



// UInterface for IInteractiveToolEditorGizmoAPI
UINTERFACE(MinimalAPI)
class UInteractiveToolEditorGizmoAPI : public UInterface
{
	GENERATED_BODY()
};

/**
 * IInteractiveToolEditorGizmoAPI provides an API to indicate whether
 * the standard editor gizmos can be enabled while this tool is active.
 */
class IInteractiveToolEditorGizmoAPI
{
	GENERATED_BODY()
public:

	/**
	 * @return true if the tool implementing this API allows the editor gizmos to be enabled while the tool is active
	 */
	virtual bool GetAllowStandardEditorGizmos() { return false; }
};