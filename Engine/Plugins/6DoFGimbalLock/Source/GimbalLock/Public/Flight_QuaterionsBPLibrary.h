// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Flight_QuaterionsBPLibrary.generated.h"

/*
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu.
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/
UCLASS()
class UFlight_QuaterionsBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	// Convert Euler Rotations To Quaternions
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Euler To Quaternion", Keywords = "rotation, quaterion"), Category = "Quaternion Rotation")
	static FQuat Euler_To_Quaternion(FRotator Current_Rotation);

	// Function to set world rotation of scene component to input quaternion rotation
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set World Rotation (Quaterion)", Keywords = "rotation, quaternion"), Category = "Quaternion Rotation")
	static void SetWorldRotationQuat(USceneComponent* SceneComponent, const FQuat& Desired_Rotation);

	// Function to set relative rotation of scene component to input quaternion rotation
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Relative Rotation (Quaterion)", Keywords = "rotation, quaternion"), Category = "Quaternion Rotation")
	static void SetRelativeRotationQuat(USceneComponent* SceneComponent, const FQuat& Desired_Rotation);

	// Function to add delta rotation to current local rotation of scene component
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Local Rotation (Quaterion)", Keywords = "rotation, quaternion"), Category = "Quaternion Rotation")
	static void AddLocalRotationQuat(USceneComponent* SceneComponent, const FQuat& Delta_Rotation);

	// Function to set world rotation of Actor to input quaternion rotation
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Actor World Rotation (Quaternion)", Keywords = "rotation, quaternion"), Category = "Quaternion Rotation")
	static void SetActorWorldRotationQuat(AActor* Actor, const FQuat& Desired_Rotation);

	// Function to set relative rotation of Actor to input quaternion rotation
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Actor Relative Rotation (Quaternion)", Keywords = "rotation, quaternion"), Category = "Quaternion Rotation")
	static void SetActorRelativeRotationQuat(AActor* Actor, const FQuat& Desired_Rotation);

	// Function to add delta rotation to current local rotation of Actor
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Actor Local Rotation (Quaternion)", Keywords = "rotation, quaternion"), Category = "Quaternion Rotation")
	static void AddActorLocalRotationQuat(AActor* Actor, const FQuat& Delta_Rotation);
};
