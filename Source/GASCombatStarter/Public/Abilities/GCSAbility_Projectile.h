// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GCSGameplayAbility.h"
#include "GCSAbility_Projectile.generated.h"

class AGCSProjectile;
class UGameplayEffect;

/**
 * UGCSAbility_Projectile
 *
 * Base ranged attack ability: spawns a configurable AGCSProjectile-derived
 * actor from a named socket (or the actor's root if none is set), aimed along
 * the owner's aim/control rotation, and lets the projectile itself handle
 * flight and impact damage (see AGCSProjectile).
 *
 * Kept deliberately simple -- no homing, no charge-up, no multi-shot. Those
 * are exactly the kinds of things a client project should build on top by
 * subclassing this ability in Blueprint or C++.
 */
UCLASS(Abstract, Blueprintable)
class GASCOMBATSTARTER_API UGCSAbility_Projectile : public UGCSGameplayAbility
{
	GENERATED_BODY()

public:
	UGCSAbility_Projectile();

	//~ Begin UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~ End UGameplayAbility interface

	/** Projectile actor class to spawn. Must derive from AGCSProjectile. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Projectile")
	TSubclassOf<AGCSProjectile> ProjectileClass;

	/** Socket on the owner's mesh to spawn the projectile from. "None" uses the actor's root transform. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Projectile")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** Initial/max speed given to the projectile's movement component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Projectile")
	float ProjectileSpeed = 3000.f;

	/** GameplayEffect the projectile applies to whatever it hits. Should derive from UGCSEffect_DamageBase. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Projectile|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Level passed to the damage GameplayEffectSpec. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Projectile|Damage")
	float DamageEffectLevel = 1.f;

	/**
	 * Spawns and initializes the configured projectile. Blueprint-callable so
	 * child classes can trigger the spawn at a precise animation notify instead
	 * of immediately on activation.
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|Projectile")
	AGCSProjectile* SpawnProjectile();

protected:
	/** Computes the world-space spawn transform (location + aim rotation) for the projectile. */
	FTransform GetMuzzleTransform() const;
};
