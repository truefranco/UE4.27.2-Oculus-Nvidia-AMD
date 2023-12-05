// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/Ray.h"
#include "VectorTypes.h"
#include "RayTypes.h"
#include "Quaternion.h"

/**
 * TTransform3 is a double/float templated version of standard UE FTransform.
 */
template<typename RealType>
class TTransform3
{
protected:
	TQuaternion<RealType> Rotation;
	FVector3<RealType> Translation;
	FVector3<RealType> Scale3D;

public:

	TTransform3()
	{
		Rotation = TQuaternion<RealType>::Identity();
		Translation = FVector3<RealType>::Zero();
		Scale3D = FVector3<RealType>::One();
	}

	TTransform3(const TQuaternion<RealType>& RotationIn, const FVector3<RealType>& TranslationIn, const FVector3<RealType>& ScaleIn)
	{
		Rotation = RotationIn;
		Translation = TranslationIn;
		Scale3D = ScaleIn;
	}

	explicit TTransform3(const TQuaternion<RealType>& RotationIn, const FVector3<RealType>& TranslationIn)
	{
		Rotation = RotationIn;
		Translation = TranslationIn;
		Scale3D = FVector3<RealType>::One();
	}

	explicit TTransform3(const FVector3<RealType>& TranslationIn)
	{
		Rotation = TQuaternion<RealType>::Identity();
		Translation = TranslationIn;
		Scale3D = FVector3<RealType>::One();
	}

	explicit TTransform3(const FTransform& Transform)
	{
		Rotation = TQuaternion<RealType>(Transform.GetRotation());
		Translation = FVector3<RealType>(Transform.GetTranslation());
		Scale3D = FVector3<RealType>(Transform.GetScale3D());
	}

	/**
	 * @return identity transform, IE no rotation, zero origin, unit scale
	 */
	static TTransform3<RealType> Identity()
	{
		return TTransform3<RealType>(TQuaternion<RealType>::Identity(), FVector3<RealType>::Zero(), FVector3<RealType>::One());
	}

	/**
	 * @return this transform converted to FTransform 
	 */
	explicit operator FTransform() const
	{
		return FTransform((FQuat)Rotation, (FVector)Translation, (FVector)Scale3D);
	}


	/**
	 * @return Rotation portion of Transform, as Quaternion
	 */
	const TQuaternion<RealType>& GetRotation() const 
	{ 
		return Rotation; 
	}

	/** 
	 * Set Rotation portion of Transform 
	 */
	void SetRotation(const TQuaternion<RealType>& RotationIn)
	{
		Rotation = RotationIn;
	}

	/**
	 * @return Translation portion of transform
	 */
	const FVector3<RealType>& GetTranslation() const
	{
		return Translation;
	}

	/**
	 * set Translation portion of transform
	 */
	void SetTranslation(const FVector3<RealType>& TranslationIn)
	{
		Translation = TranslationIn;
	}

	/**
	 * @return Scale portion of transform
	 */
	const FVector3<RealType>& GetScale() const
	{
		return Scale3D;
	}

	/**
	 * set Scale portion of transform
	 */
	void SetScale(const FVector3<RealType>& ScaleIn)
	{
		Scale3D = ScaleIn;
	}

	RealType GetDeterminant() const
	{
		return Scale3D.X * Scale3D.Y * Scale3D.Z;
	}

	/**
	 * @return true if scale is nonuniform, within tolerance
	 */
	bool HasNonUniformScale(RealType Tolerance = TMathUtil<RealType>::ZeroTolerance) const
	{
		return (TMathUtil<RealType>::Abs(Scale3D.X - Scale3D.Y) > Tolerance)
			|| (TMathUtil<RealType>::Abs(Scale3D.X - Scale3D.Z) > Tolerance)
			|| (TMathUtil<RealType>::Abs(Scale3D.Y - Scale3D.Z) > Tolerance);
	}

	/**
	 * @return inverse of this transform
	 */
	TTransform3<RealType> Inverse() const
	{
		TQuaternion<RealType> InvRotation = Rotation.Inverse();
		FVector3<RealType> InvScale3D = GetSafeScaleReciprocal(Scale3D);
		FVector3<RealType> InvTranslation = InvRotation * (InvScale3D * -Translation);
		return TTransform3<RealType>(InvRotation, InvTranslation, InvScale3D);
	}


	/**
	 * @return input point with QST transformation applied, ie QST(P) = Rotate(Scale*P) + Translate
	 */
	FVector3<RealType> TransformPosition(const FVector3<RealType>& P) const
	{
		//Transform using QST is following
		//QST(P) = Q.Rotate(S*P) + T where Q = quaternion, S = scale, T = translation
		return Rotation * (Scale3D*P) + Translation;
	}

	/**
	 * @return input vector with QS transformation applied, ie QS(V) = Rotate(Scale*V)
	 */
	FVector3<RealType> TransformVector(const FVector3<RealType>& V) const
	{
		return Rotation * (Scale3D*V);
	}

	/**
	 * @return input vector with Q transformation applied, ie Q(V) = Rotate(V)
	 */
	FVector3<RealType> TransformVectorNoScale(const FVector3<RealType>& V) const
	{
		return Rotation * V;
	}

	/**
	 * Surface Normals are special, their transform is Rotate( Normalize( (1/Scale) * Normal) ) ).
	 * However 1/Scale requires special handling in case any component is near-zero.
	 * @return input surface normal with transform applied.
	 */
	FVector3<RealType> TransformNormal(const FVector3<RealType>& Normal) const
	{
		// transform normal by a safe inverse scale + normalize, and a standard rotation
		const FVector3<RealType>& S = Scale3D;
		RealType DetSign = FMathd::SignNonZero(S.X * S.Y * S.Z); // we only need to multiply by the sign of the determinant, rather than divide by it, since we normalize later anyway
		FVector3<RealType> SafeInvS(S.Y*S.Z*DetSign, S.X*S.Z*DetSign, S.X*S.Y*DetSign);
		return TransformVectorNoScale( (SafeInvS*Normal).Normalized() );
	}


	/**
	 * @return input vector with inverse-QST transformation applied, ie QSTinv(P) = InverseScale(InverseRotate(P - Translate))
	 */
	FVector3<RealType> InverseTransformPosition(const FVector3<RealType> &P) const
	{
		return GetSafeScaleReciprocal(Scale3D) * Rotation.InverseMultiply(P - Translation);
	}

	/**
	 * @return input vector with inverse-QS transformation applied, ie QSinv(V) = InverseScale(InverseRotate(V))
	 */
	FVector3<RealType> InverseTransformVector(const FVector3<RealType> &V) const
	{
		return GetSafeScaleReciprocal(Scale3D) * Rotation.InverseMultiply(V);
	}


	/**
	 * @return input vector with inverse-Q transformation applied, ie Qinv(V) = InverseRotate(V)
	 */
	FVector3<RealType> InverseTransformVectorNoScale(const FVector3<RealType> &V) const
	{
		return Rotation.InverseMultiply(V);
	}


	/**
	 * Surface Normals are special, their inverse transform is InverseRotate( Normalize(Scale * Normal) ) )
	 * @return input surface normal with inverse transform applied.
	 */
	FVector3<RealType> InverseTransformNormal(const FVector3<RealType>& Normal) const
	{
		return InverseTransformVectorNoScale( (Scale3D*Normal).Normalized() );
	}
	TRay3<RealType> TransformRay(const TRay3<RealType>& Ray) const
	{
		FVector3<RealType> Origin = TransformPosition(Ray.Origin);
		FVector3<RealType> Direction = TransformVector(Ray.Direction).Normalized();
		return TRay3<RealType>(Origin, Direction);
	}


	TRay3<RealType> InverseTransformRay(const TRay3<RealType>& Ray) const
	{
		FVector3<RealType> InvOrigin = InverseTransformPosition(Ray.Origin);
		FVector3<RealType> InvDirection = InverseTransformVector(Ray.Direction).Normalized();
		return TRay3<RealType>(InvOrigin, InvDirection);
	}


	/**
	 * Clamp all scale components to a minimum value. Sign of scale components is preserved.
	 * This is used to remove uninvertible zero/near-zero scaling.
	 */
	void ClampMinimumScale(RealType MinimumScale = TMathUtil<RealType>::ZeroTolerance)
	{
		for (int j = 0; j < 3; ++j)
		{
			RealType Value = Scale3D[j];
			if (TMathUtil<RealType>::Abs(Value) < MinimumScale)
			{
				Value = MinimumScale * TMathUtil<RealType>::SignNonZero(Value);
				Scale3D[j] = Value;
			}
		}
	}

	

	static FVector3<RealType> GetSafeScaleReciprocal(const FVector3<RealType>& InScale, RealType Tolerance = TMathUtil<RealType>::ZeroTolerance)
	{
		FVector3<RealType> SafeReciprocalScale;
		if (TMathUtil<RealType>::Abs(InScale.X) <= Tolerance)
		{
			SafeReciprocalScale.X = (RealType)0;
		}
		else
		{
			SafeReciprocalScale.X = (RealType)1 / InScale.X;
		}

		if (TMathUtil<RealType>::Abs(InScale.Y) <= Tolerance)
		{
			SafeReciprocalScale.Y = (RealType)0;
		}
		else
		{
			SafeReciprocalScale.Y = (RealType)1 / InScale.Y;
		}

		if (TMathUtil<RealType>::Abs(InScale.Z) <= Tolerance)
		{
			SafeReciprocalScale.Z = (RealType)0;
		}
		else
		{
			SafeReciprocalScale.Z = (RealType)1 / InScale.Z;
		}

		return SafeReciprocalScale;
	}




	// vector-type-conversion variants. This allows applying a float transform to double vector and vice-versa.
	// Whether this should be allowed is debatable. However in practice it is extremely rare to convert an
	// entire float transform to a double transform in order to apply to a double vector, which is the only
	// case where this conversion is an issue

	template<typename RealType2>
	FVector3<RealType2> TransformPosition(const FVector3<RealType2>& P) const
	{
		return FVector3<RealType2>(TransformPosition(FVector3<RealType>(P)));
	}
	template<typename RealType2>
	FVector3<RealType2> TransformVector(const FVector3<RealType2>& V) const
	{
		return FVector3<RealType2>(TransformVector(FVector3<RealType>(V)));
	}
	template<typename RealType2>
	FVector3<RealType2> TransformVectorNoScale(const FVector3<RealType2>& V) const
	{
		return FVector3<RealType2>(TransformVectorNoScale(FVector3<RealType>(V)));
	}
	template<typename RealType2>
	FVector3<RealType2> TransformNormal(const FVector3<RealType2>& V) const
	{
		return FVector3<RealType2>(TransformNormal(FVector3<RealType>(V)));
	}
	template<typename RealType2>
	FVector3<RealType2> InverseTransformPosition(const FVector3<RealType2>& P) const
	{
		return FVector3<RealType2>(InverseTransformPosition(FVector3<RealType>(P)));
	}
	template<typename RealType2>
	FVector3<RealType2> InverseTransformVector(const FVector3<RealType2>& V) const
	{
		return FVector3<RealType2>(InverseTransformVector(FVector3<RealType>(V)));
	}
	template<typename RealType2>
	FVector3<RealType2> InverseTransformVectorNoScale(const FVector3<RealType2>& V) const
	{
		return FVector3<RealType2>(InverseTransformVectorNoScale(FVector3<RealType>(V)));
	}
	template<typename RealType2>
	FVector3<RealType2> InverseTransformNormal(const FVector3<RealType2>& V) const
	{
		return FVector3<RealType2>(InverseTransformNormal(FVector3<RealType>(V)));
	}

};
typedef TTransform3<float> FTransform3f;
typedef TTransform3<double> FTransform3d;



/**
 * TTransformSequence3 represents a sequence of 3D transforms.
 */
template<typename RealType>
class TTransformSequence3
{
protected:
	TArray<TTransform3<RealType>, TInlineAllocator<2>> Transforms;

public:

	/**
	 * Add Transform to the end of the sequence, ie Seq(P) becomes NewTransform * Seq(P)
	 */
	void Append(const TTransform3<RealType>& Transform)
	{
		Transforms.Add(Transform);
	}

	/**
	 * Add Transform to the end of the sequence, ie Seq(P) becomes NewTransform * Seq(P)
	 */
	void Append(const FTransform& Transform)
	{
		Transforms.Add(TTransform3<RealType>(Transform));
	}

	/**
	 * Add all transforms in given sequence to the end of this sequence, ie Seq(P) becomes SequenceToAppend * Seq(P)
	 */
	void Append(const TTransformSequence3<RealType>& SequenceToAppend)
	{
		for (const TTransform3<RealType>& Transform : SequenceToAppend.Transforms)
		{
			Append(Transform);
		}
	}

	/**
	 * @return number of transforms in the sequence
	 */
	int32 Num() const { return Transforms.Num(); }

	/**
	 * @return transforms in the sequence
	 */
	const TArray<TTransform3<RealType>, TInlineAllocator<2>>& GetTransforms() const { return Transforms; }


	/**
	 * @return true if any transform in the sequence has nonuniform scaling
	 */
	bool HasNonUniformScale(RealType Tolerance = TMathUtil<RealType>::ZeroTolerance) const
	{
		for (const TTransform3<RealType>& Transform : Transforms)
		{
			if (Transform.HasNonUniformScale(Tolerance))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * @return Cumulative scale across Transforms.
	 */
	FVector3<RealType> GetAccumulatedScale() const
	{
		FVector3<RealType> FinalScale = FVector3<RealType>::One();
		for (const TTransform3<RealType>& Transform : Transforms)
		{
			FinalScale = FinalScale * Transform.GetScale();
		}
		return FinalScale;
	}

	/**
	 * @return Whether the sequence will invert a shape (by negative scaling) when applied
	 */
	bool WillInvert() const
	{
		RealType Det = 1;
		for (const TTransform3<RealType>& Transform : Transforms)
		{
			Det *= Transform.GetDeterminant();
		}
		return Det < 0;
	}

	/**
	 * Set scales of all transforms to (1,1,1)
	 */
	void ClearScales()
	{
		for (TTransform3<RealType>& Transform : Transforms)
		{
			Transform.SetScale(FVector3<RealType>::One());
		}
	}

	/**
	 * @return point P with transform sequence applied
	 */
	FVector3<RealType> TransformPosition(FVector3<RealType> P) const
	{
		for (const TTransform3<RealType>& Transform : Transforms)
		{
			P = Transform.TransformPosition(P);
		}
		return P;
	}

	/**
	 * @return point P with inverse transform sequence applied
	 */
	FVector3<RealType> InverseTransformPosition(FVector3<RealType> P) const
	{
		int32 N = Transforms.Num();
		for (int32 k = N - 1; k >= 0; k--)
		{
			P = Transforms[k].InverseTransformPosition(P);
		}
		return P;
	}

	/**
	 * @return Vector V with transform sequence applied
	 */
	FVector3<RealType> TransformVector(FVector3<RealType> V) const
	{
		for (const TTransform3<RealType>& Transform : Transforms)
		{
			V = Transform.TransformVector(V);
		}
		return V;
	}


	/**
	 * @return Normal with transform sequence applied
	 */
	FVector3<RealType> TransformNormal(FVector3<RealType> Normal) const
	{
		for (const TTransform3<RealType>& Transform : Transforms)
		{
			Normal = Transform.TransformNormal(Normal);
		}
		return Normal;
	}

	/**
	 * @return true if each Transform in this sequence is equivalent to each transform of another sequence under the given test
	 */
	template<typename TransformsEquivalentFunc>
	bool IsEquivalent(const TTransformSequence3<RealType>& OtherSeq, TransformsEquivalentFunc TransformsTest) const
	{
		int32 N = Transforms.Num();
		if (N == OtherSeq.Transforms.Num())
		{
			bool bAllTransformsEqual = true;
			for (int32 k = 0; k < N && bAllTransformsEqual; ++k)
			{
				bAllTransformsEqual = bAllTransformsEqual && TransformsTest(Transforms[k], OtherSeq.Transforms[k]);
			}
			return bAllTransformsEqual;
		}
		return false;
	}
};

typedef TTransformSequence3<float> FTransformSequence3f;
typedef TTransformSequence3<double> FTransformSequence3d;