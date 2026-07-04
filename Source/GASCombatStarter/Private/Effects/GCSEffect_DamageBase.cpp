// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Effects/GCSEffect_DamageBase.h"

UGCSEffect_DamageBase::UGCSEffect_DamageBase()
{
	// Instant is the correct default for the vast majority of "deal damage now"
	// effects. Duration-over-time damage effects should override this per-asset
	// in the Blueprint GE (set DurationPolicy to Duration/Infinite and configure
	// a Period for periodic execution) -- see the class comment in the header
	// for the full authoring workflow.
	DurationPolicy = EGameplayEffectDurationType::Instant;
}
