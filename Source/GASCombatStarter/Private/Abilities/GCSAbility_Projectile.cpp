// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Abilities/GCSAbility_Projectile.h"
#include "Actors/GCSProjectile.h"
#include "GASCombatStarterModule.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

UGCSAbility_Projectile::UGCSAbility_Projectile()
{
	ActivationPolicy = EGCSAbilityActivationPolicy::OnInput;
}

void UGCSAbility_Projectile::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		return;
	}

	// Default flow: spawn immediately on activation. For animation-driven ranged
	// attacks (e.g. spawn on a "release arrow" anim notify), override this in a
	// Blueprint child to instead wait for that notify before calling SpawnProjectile().
	SpawnProjectile();

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

FTransform UGCSAbility_Projectile::GetMuzzleTransform() const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return FTransform::Identity;
	}

	if (MuzzleSocketName != NAME_None)
	{
		if (const ACharacter* Character = Cast<ACharacter>(Avatar))
		{
			if (USkeletalMeshComponent* Mesh = Character->GetMesh())
			{
				if (Mesh->DoesSocketExist(MuzzleSocketName))
				{
					return Mesh->GetSocketTransform(MuzzleSocketName);
				}
			}
		}
	}

	// Fall back to aiming along the controller's view direction if available,
	// otherwise the actor's own forward vector.
	FRotator AimRotation = Avatar->GetActorRotation();
	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			AimRotation = Controller->GetControlRotation();
		}
	}

	return FTransform(AimRotation, Avatar->GetActorLocation());
}

AGCSProjectile* UGCSAbility_Projectile::SpawnProjectile()
{
	if (!ProjectileClass)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSAbility_Projectile::SpawnProjectile called with no ProjectileClass set on %s."), *GetName());
		return nullptr;
	}

	UWorld* World = GetWorld();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!World || !Avatar)
	{
		return nullptr;
	}

	if (!Avatar->HasAuthority())
	{
		// Projectile spawning that deals damage must be server-authoritative to
		// prevent client-side damage spoofing. If you want client-predicted
		// cosmetic projectiles, spawn a separate non-damaging FX-only actor
		// locally instead of relying on this function under prediction.
		return nullptr;
	}

	const FTransform SpawnTransform = GetMuzzleTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Avatar;
	SpawnParams.Instigator = Cast<APawn>(Avatar);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGCSProjectile* Projectile = World->SpawnActor<AGCSProjectile>(ProjectileClass, SpawnTransform, SpawnParams);
	if (Projectile)
	{
		Projectile->InitializeProjectile(Avatar, DamageEffectClass, DamageEffectLevel, ProjectileSpeed);
	}

	return Projectile;
}
