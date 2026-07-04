// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GCSGameplayAbility.h"
#include "GCSAbility_MeleeAttack.generated.h"

/** Shape used for the melee hit trace. */
UENUM(BlueprintType)
enum class EGCSMeleeTraceShape : uint8
{
	Sphere UMETA(DisplayName = "Sphere Sweep"),
	Box UMETA(DisplayName = "Box Sweep")
};

/**
 * UGCSAbility_MeleeAttack
 *
 * Base melee attack ability: performs a configurable shape sweep in front of
 * (or at a named socket on) the owning character, applies a damage
 * GameplayEffect to anything hit, and sends a GCS.Event.HitReact GameplayEvent
 * to each target so their hit-react ability/montage can trigger.
 *
 * Designed to be subclassed in Blueprint for combo variants -- expose the
 * trace/damage parameters as EditDefaultsOnly so each combo step can be its
 * own Blueprint child with different range, shape, and damage effect.
 *
 * This class does not play montages itself: montage playback is expected to
 * be handled either by an Ability Task in a Blueprint child, or by a
 * C++ subclass using UAbilityTask_PlayMontageAndWait. Keeping animation out of
 * this base class keeps it usable with any animation setup (root motion,
 * in-place, third person, first person, etc.).
 */
UCLASS(Abstract, Blueprintable)
class GASCOMBATSTARTER_API UGCSAbility_MeleeAttack : public UGCSGameplayAbility
{
	GENERATED_BODY()

public:
	UGCSAbility_MeleeAttack();

	//~ Begin UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~ End UGameplayAbility interface

	/** Shape of the melee hit trace. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Melee|Trace")
	EGCSMeleeTraceShape TraceShape = EGCSMeleeTraceShape::Sphere;

	/** Radius used when TraceShape == Sphere. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Melee|Trace", meta = (EditCondition = "TraceShape == EGCSMeleeTraceShape::Sphere"))
	float TraceRadius = 75.f;

	/** Half-extent used when TraceShape == Box. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Melee|Trace", meta = (EditCondition = "TraceShape == EGCSMeleeTraceShape::Box"))
	FVector TraceBoxHalfExtent = FVector(75.f, 50.f, 50.f);

	/** How far in front of the trace origin the sweep extends. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Melee|Trace")
	float TraceForwardDistance = 150.f;

	/** Optional socket on the owning mesh to originate the trace from. If "None", uses the actor's root/location. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Melee|Trace")
	FName TraceSocketName = NAME_None;

	/** Object types to trace against. Defaults to Pawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Melee|Trace")
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;

	/** If true, draws debug shapes for the trace (development builds only). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Melee|Debug")
	bool bDrawDebugTrace = false;

	/** GameplayEffect applied to each actor hit by the trace. Should derive from UGCSEffect_DamageBase. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Melee|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Level passed to the damage GameplayEffectSpec. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Melee|Damage")
	float DamageEffectLevel = 1.f;

	/**
	 * Performs the configured shape sweep from the owning avatar and returns all
	 * hit actors (deduplicated, excluding the instigator). Blueprint-callable so
	 * combo child classes can trigger the sweep at a specific animation notify.
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|Melee")
	TArray<AActor*> PerformMeleeTrace();

	/**
	 * Applies DamageEffectClass to each target and sends a GCS.Event.HitReact
	 * GameplayEvent to them. Called automatically by ActivateAbility's default
	 * flow, but exposed so Blueprint children can call it at a precise moment
	 * (e.g. from an anim notify) instead of immediately on activation.
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|Melee")
	void ApplyDamageToTargets(const TArray<AActor*>& Targets);

	/**
	 * Called for each individual target after the damage effect has been applied
	 * to it. BlueprintNativeEvent so C++ can provide default behavior (advancing
	 * the combo, recording hit time) while Blueprint children can extend or
	 * override presentation logic (spawn hit VFX, camera shake, etc.).
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "GCS|Melee")
	void OnHitConfirmed(AActor* Target, const FGameplayEffectSpecHandle& EffectSpecHandle);
	virtual void OnHitConfirmed_Implementation(AActor* Target, const FGameplayEffectSpecHandle& EffectSpecHandle);

protected:
	/** Computes the world-space origin and forward direction used for the trace. */
	void GetTraceTransform(FVector& OutOrigin, FVector& OutForward) const;
};
