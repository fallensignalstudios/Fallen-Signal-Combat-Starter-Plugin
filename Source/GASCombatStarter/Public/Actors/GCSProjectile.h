// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCSProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGCSOnProjectileImpact, AActor*, HitActor, const FHitResult&, Hit);

/**
 * AGCSProjectile
 *
 * Minimal, generic projectile actor used by UGCSAbility_Projectile. Handles
 * movement (via UProjectileMovementComponent) and collision detection only --
 * the actual damage GameplayEffect to apply, and instigator/source ASC, are
 * set by the spawning ability via InitializeProjectile() so this class has no
 * hard dependency on any specific ability class.
 *
 * Visuals (mesh, niagara trail, impact VFX) are intentionally left to
 * Blueprint subclasses -- this C++ base only guarantees function.
 */
UCLASS(Blueprintable)
class GASCOMBATSTARTER_API AGCSProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGCSProjectile();

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor interface

	/** Collision sphere used to detect impacts. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GCS|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	/** Drives the projectile's flight. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GCS|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	/** How long the projectile lives before self-destructing if it never hits anything. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Projectile")
	float MaxLifetime = 5.f;

	/**
	 * Called by the spawning ability immediately after SpawnActor to configure
	 * this projectile's damage effect, level, instigator, and speed. Must be
	 * called before the projectile starts moving in a meaningful way.
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|Projectile")
	void InitializeProjectile(AActor* InInstigator, TSubclassOf<UGameplayEffect> InDamageEffectClass, float InDamageEffectLevel, float InSpeed);

	UPROPERTY(BlueprintAssignable, Category = "GCS|Projectile|Events")
	FGCSOnProjectileImpact OnProjectileImpact;

protected:
	UFUNCTION()
	void OnCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Applies the configured damage effect to the hit actor's ASC, if it has one. */
	void ApplyDamageToHitActor(AActor* HitActor, const FHitResult& Hit);

	UPROPERTY(Replicated)
	TObjectPtr<AActor> ProjectileInstigator;

	UPROPERTY(Replicated)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(Replicated)
	float DamageEffectLevel = 1.f;

	/** True once this projectile has impacted something, to guard against double-hits. */
	bool bHasImpacted = false;
};
