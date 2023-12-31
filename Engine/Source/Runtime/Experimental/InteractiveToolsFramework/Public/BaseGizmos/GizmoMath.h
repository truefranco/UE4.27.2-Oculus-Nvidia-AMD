// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseGizmos/GizmoInterfaces.h"

/**
 * GizmoMath functions implement various vector math/geometry operations
 * required to implement various standard Gizmos. Although some of these
 * are standard queries, in the context of user interface Gizmos they may
 * have behaviors (like assumptions about valid ranges or error-handling)
 * that would not be suitable for general-purpose math code.
 */
namespace GizmoMath
{
	/**
	 * Project a point onto the line defined by LineOrigin and LineDirection
	 * @return projected point
	 */
	INTERACTIVETOOLSFRAMEWORK_API FVector ProjectPointOntoLine(
		const FVector& Point,
		const FVector& LineOrigin, const FVector& LineDirection);

	/**
	 * Find the nearest point to QueryPoint on the line defined by LineOrigin and LineDirection.
	 * Returns this point and the associated line-equation parameter
	 */
	INTERACTIVETOOLSFRAMEWORK_API void NearestPointOnLine(
		const FVector& LineOrigin, const FVector& LineDirection,
		const FVector& QueryPoint,
		FVector& NearestPointOut, float& LineParameterOut);

	/**
	 * Find the closest pair of points on a 3D line and 3D ray.
	 * Returns the points and line-equation parameters on both the line and ray.
	 */
	INTERACTIVETOOLSFRAMEWORK_API void NearestPointOnLineToRay(
		const FVector& LineOrigin, const FVector& LineDirection,
		const FVector& RayOrigin, const FVector& RayDirection,
		FVector& NearestLinePointOut, float& LineParameterOut,
		FVector& NearestRayPointOut, float& RayParameterOut);

	/**
	 * Find the intersection of the ray defined by RayOrigin and RayDirection
	 * with the plane defined by PlaneOrigin and PlaneNormal.
	 * Returns intersection success/failure in bIntersectsOut and the intersection point in PlaneIntersectionPointOut
	 */
	INTERACTIVETOOLSFRAMEWORK_API void RayPlaneIntersectionPoint(
		const FVector& PlaneOrigin, const FVector& PlaneNormal,
		const FVector& RayOrigin, const FVector& RayDirection,
		bool& bIntersectsOut, FVector& PlaneIntersectionPointOut);

	/**
	 * Find the intersection of the ray defined by RayOrigin and RayDirection
	 * with the sphere defined by SphereOrigin and SphereRadius.
	 * Returns intersection success/failure in bIntersectsOut and the intersection point in SphereIntersectionPointOut
	 */
	INTERACTIVETOOLSFRAMEWORK_API void RaySphereIntersection(
		const FVector& SphereOrigin, const float SphereRadius,
		const FVector& RayOrigin, const FVector& RayDirection,
		bool& bIntersectsOut, FVector& SphereIntersectionPointOut);

	/**
	 * Find the closest point to QueryPoint that is on the circle
	 * defined by CircleOrigin, CircleNormal, and CircleRadius.
	 */
	INTERACTIVETOOLSFRAMEWORK_API void ClosetPointOnCircle(
		const FVector& QueryPoint, 
		const FVector& CircleOrigin, const FVector& CircleNormal, float CircleRadius,
		FVector& ClosestPointOut);

	/**
	 * Construct two mutually-perpendicular unit vectors BasisAxis1Out and BasisAxis2Out
	 * which are orthogonal to PlaneNormal
	 */
	INTERACTIVETOOLSFRAMEWORK_API void MakeNormalPlaneBasis(
		const FVector& PlaneNormal,
		FVector& BasisAxis1Out, FVector& BasisAxis2Out);

	/**
	 * Project the input Point into the plane defined by PlaneOrigin and PlaneNormal,
	 * and then measure it's signed angle relative to the in-plane X and Y axes
	 * defined by PlaneAxis1 and PlaneAxis2
	 * @return the signed angle
	 */
	INTERACTIVETOOLSFRAMEWORK_API float ComputeAngleInPlane(
		const FVector& Point,
		const FVector& PlaneOrigin, const FVector& PlaneNormal,
		const FVector& PlaneAxis1, const FVector& PlaneAxis2);


	/**
	 * Project the input Point into the plane defined by PlaneOrigin and PlaneNormal,
	 * and then calculate it's UV coordinates in the space defined by the plane 
	 * axes PlaneAxis1 and PlaneAxis2
	 * @return the UV coordinates
	 */
	INTERACTIVETOOLSFRAMEWORK_API FVector2D ComputeCoordinatesInPlane(
		const FVector& Point,
		const FVector& PlaneOrigin, const FVector& PlaneNormal,
		const FVector& PlaneAxis1, const FVector& PlaneAxis2);

	/**
	 * Project the input Point onto the plane defined by PlaneOrigin and PlaneNormal,
	 * @return Nearest position to Point lying in the plane
	 */
	INTERACTIVETOOLSFRAMEWORK_API FVector ProjectPointOntoPlane(
		const FVector& Point,
		const FVector& PlaneOrigin, const FVector& PlaneNormal);


	/**
	 * Round Value to nearest step of Increment
	 * @return snapped/rounded value
	 */
	template <typename RealType>
	INTERACTIVETOOLSFRAMEWORK_API RealType SnapToIncrement(RealType Value, RealType Increment);
}