// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Actors/GCSProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GASCombatStarterModule.h"

AGCSProjectile::AGCSProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(15.f);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AGCSProjectile::OnCollisionOverlap);
	RootComponent = CollisionComponent;

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->InitialSpeed = 3000.f;
	ProjectileMovementComponent->MaxSpeed = 3000.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
}

void AGCSProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(MaxLifetime);
}

void AGCSProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGCSProjectile, ProjectileInstigator);
	DOREPLIFETIME(AGCSProjectile, DamageEffectClass);
	DOREPLIFETIME(AGCSProjectile, DamageEffectLevel);
}

void AGCSProjectile::InitializeProjectile(AActor* InInstigator, TSubclassOf<UGameplayEffect> InDamageEffectClass, float InDamageEffectLevel, float InSpeed)
{
	ProjectileInstigator = InInstigator;
	DamageEffectClass = InDamageEffectClass;
	DamageEffectLevel = InDamageEffectLevel;

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->InitialSpeed = InSpeed;
		ProjectileMovementComponent->MaxSpeed = InSpeed;
		ProjectileMovementComponent->Velocity = GetActorForwardVector() * InSpeed;
	}

	if (InInstigator)
	{
		// Ignore collision with the instigator so the projectile doesn't immediately
		// self-collide when spawned at/near the owning character.
		CollisionComponent->IgnoreActorWhenMoving(InInstigator, true);
	}
}

void AGCSProjectile::OnCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasImpacted)
	{
		return;
	}

	if (!OtherActor || OtherActor == this || OtherActor == ProjectileInstigator)
	{
		return;
	}

	bHasImpacted = true;

	ApplyDamageToHitActor(OtherActor, SweepResult);
	OnProjectileImpact.Broadcast(OtherActor, SweepResult);

	// Server owns projectile lifetime authority; clients simply stop simulating
	// visually when the actor is destroyed via replication.
	if (HasAuthority())
	{
		Destroy();
	}
}

void AGCSProjectile::ApplyDamageToHitActor(AActor* HitActor, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		// Damage application must be server-authoritative. Clients only get visual
		// feedback via OnProjectileImpact.
		return;
	}

	if (!DamageEffectClass || !HitActor)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = ProjectileInstigator
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ProjectileInstigator)
		: nullptr;
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

	if (!SourceASC || !TargetASC)
	{
		UE_LOG(LogGASCombatStarter, Verbose, TEXT("AGCSProjectile: missing ASC on source or target, skipping damage application."));
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(ProjectileInstigator, this);
	ContextHandle.AddHitResult(Hit);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, DamageEffectLevel, ContextHandle);
	if (SpecHandle.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}
