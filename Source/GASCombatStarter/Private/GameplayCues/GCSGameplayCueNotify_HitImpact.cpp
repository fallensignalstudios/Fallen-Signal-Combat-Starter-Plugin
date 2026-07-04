// Copyright Fallen Signal Studios. All Rights Reserved.

#include "GameplayCues/GCSGameplayCueNotify_HitImpact.h"
#include "GCSNativeGameplayTags.h"
#include "GASCombatStarterModule.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerCameraManager.h"
#include "GameFramework/Actor.h"

// NOTE ON MODULE DEPENDENCIES:
// UNiagaraFunctionLibrary::SpawnSystemAtLocation requires the "Niagara" module.
// Add it to GASCombatStarter.Build.cs's PublicDependencyModuleNames (or
// PrivateDependencyModuleNames, since only .cpp files here call into it):
//
//     PrivateDependencyModuleNames.AddRange(new string[]
//     {
//         "Slate",
//         "SlateCore",
//         "NetCore",
//         "Niagara"   // <-- required for UNiagaraFunctionLibrary::SpawnSystemAtLocation
//     });
//
// Without this, the #include "NiagaraFunctionLibrary.h" below will fail to
// resolve/link even though the header can often still be found on the
// include path via other plugins' transitive dependencies.

UGCSGameplayCueNotify_HitImpact::UGCSGameplayCueNotify_HitImpact()
{
	// Bind this Notify class to its cue tag. Concrete Blueprint children (or
	// this class itself, if used directly as a native-only cue) should have
	// GameplayCueTag == GameplayCue.GCS.HitImpact -- setting it here on the
	// CDO means every child starts pre-wired to the correct tag without
	// requiring per-asset configuration, while still being overridable in the
	// off chance a project wants to repurpose this class under a different tag.
	GameplayCueTag = GCSGameplayTags::GameplayCue_HitImpact;

	// Burst cue: no ongoing actor/duration semantics needed, so leave
	// IsOverride / bAutoDestroyOnRemove etc. at UGameplayCueNotify_Static's
	// defaults. All of this cue's work happens in OnExecute_Implementation.
}

bool UGCSGameplayCueNotify_HitImpact::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// UGameplayCueNotify_Static::OnExecute_Implementation has no meaningful
	// base behavior to chain to; this cue is fully self-contained.

	if (!MyTarget || !MyTarget->GetWorld())
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSGameplayCueNotify_HitImpact::OnExecute_Implementation called with a null/worldless MyTarget."));
		return false;
	}

	UWorld* World = MyTarget->GetWorld();

	// Prefer the cue's own captured hit location (works even if MyTarget has
	// since moved/died by the time the cue executes); fall back to the
	// target's current actor location if no location was captured by the
	// caller (Parameters.Location defaults to ZeroVector when unset, which we
	// treat as "not provided").
	const FVector ImpactLocation = !Parameters.Location.IsZero() ? Parameters.Location : MyTarget->GetActorLocation();
	const FRotator ImpactRotation = Parameters.Normal.IsNearlyZero() ? FRotator::ZeroRotator : Parameters.Normal.Rotation();

	// ------------------------------------------------------------------
	// Impact VFX
	// ------------------------------------------------------------------
	if (ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			ImpactVFX,
			ImpactLocation,
			ImpactRotation,
			FVector(1.f),   // Scale
			true,           // bAutoDestroy
			true,           // bAutoActivate
			ENCPoolMethod::AutoRelease // Pool the component for performance -- appropriate for a frequently-recurring burst effect like hit impacts.
		);
	}

	// ------------------------------------------------------------------
	// Impact sound
	// ------------------------------------------------------------------
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, ImpactSound, ImpactLocation, ImpactRotation);
	}

	// ------------------------------------------------------------------
	// Camera shake (applied to the instigator's local player camera, if any)
	// ------------------------------------------------------------------
	if (HitCameraShake)
	{
		// EffectContext.GetInstigator() is the actor that caused the
		// GameplayEffect/cue (the attacker), which is who we want screen
		// shake applied to -- shaking the victim's camera on every hit taken
		// reads as janky/disorienting rather than impactful.
		AActor* InstigatorActor = Parameters.EffectContext.GetInstigator();
		if (!InstigatorActor)
		{
			// Fall back to the original instigator recorded directly on the
			// cue parameters, if the effect context didn't carry one.
			InstigatorActor = Parameters.GetInstigator();
		}

		if (APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
		{
			if (APlayerController* PC = Cast<APlayerController>(InstigatorPawn->GetController()))
			{
				if (PC->IsLocalController() && PC->PlayerCameraManager)
				{
					PC->PlayerCameraManager->StartCameraShake(HitCameraShake, CameraShakeScale);
				}
			}
		}
	}

	// ------------------------------------------------------------------
	// Critical-hit companion cue
	// ------------------------------------------------------------------
	// PATTERN: rather than overloading this single cue with crit-specific
	// VFX/SFX toggles, we broadcast a SEPARATE cue tag (GameplayCue.GCS.CriticalHit)
	// when the magnitude clears CritThreshold. This keeps "always happens on
	// hit" feedback (this cue) decoupled from "only happens on crit" feedback
	// (a second UGCSGameplayCueNotify_Static-derived Blueprint bound to
	// GameplayCue.GCS.CriticalHit, e.g. with punchier VFX/a stinger sound),
	// and lets designers add/replace crit feedback without touching this
	// class at all. Parameters.RawMagnitude is expected to be populated by
	// the call site (e.g. from the damage GameplayEffectSpec's computed
	// magnitude -- see UGCSExecCalc_Damage) before ExecuteGameplayCue is called.
	if (Parameters.RawMagnitude > CritThreshold)
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyTarget))
		{
			TargetASC->ExecuteGameplayCue(GCSGameplayTags::GameplayCue_CriticalHit, Parameters);
		}
	}

	return true;
}
