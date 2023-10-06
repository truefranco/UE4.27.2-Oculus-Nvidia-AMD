// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Chaos/Real.h"
#include "Chaos/Array.h"
namespace GeometryCollectionTest
{

	class BoxGeometry
	{

	public:
		BoxGeometry() {}
		~BoxGeometry() {}

		static const TArray<Chaos::FReal>	RawVertexArray;
		static const TArray<int32>			RawIndicesArray;
	};
}