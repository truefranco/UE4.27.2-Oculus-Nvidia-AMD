// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Chaos/Array.h"
#include "Chaos/PBDBendingConstraintsBase.h"
#include "Chaos/PBDParticles.h"
#include "Chaos/ParticleRule.h"

namespace Chaos
{
class FPBDBendingConstraints : public FParticleRule, public FPBDBendingConstraintsBase
{
	typedef FPBDBendingConstraintsBase Base;
	using Base::MConstraints;

  public:
	FPBDBendingConstraints(const FDynamicParticles& InParticles, TArray<TVec4<int32>>&& Constraints, const FReal stiffness = (FReal)1.)
	    : Base(InParticles, MoveTemp(Constraints), stiffness) {}
	virtual ~FPBDBendingConstraints() {}

	void Apply(FPBDParticles& InParticles, const FReal Dt) const override //-V762
	{
		for (int i = 0; i < MConstraints.Num(); ++i)
		{
			const auto& Constraint = MConstraints[i];
			const int32 i1 = Constraint[0];
			const int32 i2 = Constraint[1];
			const int32 i3 = Constraint[2];
			const int32 i4 = Constraint[3];
			auto Grads = Base::GetGradients(InParticles, i);
			auto S = Base::GetScalingFactor(InParticles, i, Grads);
			InParticles.P(i1) -= S * InParticles.InvM(i1) * Grads[0];
			InParticles.P(i2) -= S * InParticles.InvM(i2) * Grads[1];
			InParticles.P(i3) -= S * InParticles.InvM(i3) * Grads[2];
			InParticles.P(i4) -= S * InParticles.InvM(i4) * Grads[3];
		}
	}
};

template<class T>
using TPBDBendingConstraints UE_DEPRECATED(4.27, "Deprecated. this class is to be deleted, use FPBDBendingConstraints instead") = FPBDBendingConstraints;
}
