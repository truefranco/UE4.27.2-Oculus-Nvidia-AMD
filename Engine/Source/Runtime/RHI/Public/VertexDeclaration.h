
#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Containers/Set.h"
#include "CoreTypes.h"
#include "RHIDefinitions.h"

/** Info for supporting the vertex element types */
class FVertexElementTypeSupportInfo
{
public:
	FVertexElementTypeSupportInfo() { for (int32 i = 0; i < VET_MAX; i++) ElementCaps[i] = true; }
	FORCEINLINE bool IsSupported(EVertexElementType ElementType) { return ElementCaps[ElementType]; }
	FORCEINLINE void SetSupported(EVertexElementType ElementType, bool bIsSupported) { ElementCaps[ElementType] = bIsSupported; }
private:
	/** cap bit set for each VET. One-to-one mapping based on EVertexElementType */
	bool ElementCaps[VET_MAX];
};

struct FVertexElement
{
	uint8 StreamIndex;
	uint8 Offset;
	TEnumAsByte<EVertexElementType> Type;
	uint8 AttributeIndex;
	uint16 Stride;
	/**
	 * Whether to use instance index or vertex index to consume the element.
	 * eg if bUseInstanceIndex is 0, the element will be repeated for every instance.
	 */
	uint16 bUseInstanceIndex;

	FVertexElement() {}
	FVertexElement(uint8 InStreamIndex, uint8 InOffset, EVertexElementType InType, uint8 InAttributeIndex, uint16 InStride, bool bInUseInstanceIndex = false) :
		StreamIndex(InStreamIndex),
		Offset(InOffset),
		Type(InType),
		AttributeIndex(InAttributeIndex),
		Stride(InStride),
		bUseInstanceIndex(bInUseInstanceIndex)
	{}
	/**
	* Suppress the compiler generated assignment operator so that padding won't be copied.
	* This is necessary to get expected results for code that zeros, assigns and then CRC's the whole struct.
	*/
	void operator=(const FVertexElement& Other)
	{
		StreamIndex = Other.StreamIndex;
		Offset = Other.Offset;
		Type = Other.Type;
		AttributeIndex = Other.AttributeIndex;
		Stride = Other.Stride;
		bUseInstanceIndex = Other.bUseInstanceIndex;
	}

	friend FArchive& operator<<(FArchive& Ar, FVertexElement& Element)
	{
		Ar << Element.StreamIndex;
		Ar << Element.Offset;
		Ar << Element.Type;
		Ar << Element.AttributeIndex;
		Ar << Element.Stride;
		Ar << Element.bUseInstanceIndex;
		return Ar;
	}

	/**
	* This operator is needed by GVertexDeclarationCache in PipelineStateCache.cpp
	*/
	RHI_API friend bool operator== (const FVertexElement& Lhs, const FVertexElement& Rhs);

	RHI_API FString ToString() const;
	RHI_API void FromString(const FString& Src);
	RHI_API void FromString(const FStringView& Src);
};

struct FVertexDeclarationElementList : public TArray<FVertexElement, TFixedAllocator<MaxVertexElementCount> >
{
	FVertexDeclarationElementList()
	{
	}

	/**
	 * Initializer list constructor
	 */
	FVertexDeclarationElementList(std::initializer_list<FVertexElement> InitList) :
		TArray<FVertexElement, TFixedAllocator<MaxVertexElementCount> >(InitList)
	{
	}

	void SortList()
	{
		Sort([](FVertexElement const& A, FVertexElement const& B)
		{
			if (A.StreamIndex < B.StreamIndex)
			{
				return true;
			}
			if (A.StreamIndex > B.StreamIndex)
			{
				return false;
			}
			if (A.Offset < B.Offset)
			{
				return true;
			}
			if (A.Offset > B.Offset)
			{
				return false;
			}
			if (A.AttributeIndex < B.AttributeIndex)
			{
				return true;
			}
			return false;
		});
	}
};

inline uint32 GetTypeHash(const FVertexElement& Element)
{
	return GetTypeHash(Element.StreamIndex) ^
		GetTypeHash(Element.Offset) ^
		GetTypeHash(Element.Type) ^
		GetTypeHash(Element.AttributeIndex) ^
		GetTypeHash(Element.Stride) ^
		GetTypeHash(Element.bUseInstanceIndex);
}

inline uint32 GetTypeHash(const FVertexDeclarationElementList& Elements)
{
	// Because FVertexDeclarationElementList is kept sorted, we can just run
	// HashCombine without worrying the order changing the hash value
	// We don't want to call MemCrc32 because address alignment can change
	// the hash value
	uint32 Hash = 0;
	for (const FVertexElement& Element : Elements)
	{
		Hash = HashCombine(Hash, GetTypeHash(Element));
	}
	return Hash;
}

// this operator is needed by GVertexDeclarationCache (PipelineStateCache.cpp Line 1527)
inline bool operator== (const FVertexElement& Lhs, const FVertexElement& Rhs)
{
	return Lhs.StreamIndex == Rhs.StreamIndex &&
		Lhs.Offset == Rhs.Offset &&
		Lhs.Type == Rhs.Type &&
		Lhs.AttributeIndex == Rhs.AttributeIndex &&
		Lhs.Stride == Rhs.Stride &&
		Lhs.bUseInstanceIndex == Rhs.bUseInstanceIndex;
}