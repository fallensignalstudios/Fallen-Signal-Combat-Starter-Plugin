// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Components/GCSDeathComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include "Interfaces/IGCSDeathInterface.h"
#include "AttributeSets/GCSCombatAttributeSet.h"
// Note: UGCSCombatComponent is not directly referenced by this component.
// UGCSDeathComponent is intentionally decoupled from UGCSCombatComponent's
// internals -- it only relies on the shared AbilitySystemComponent and
// AttributeSet, so no include of GCSCombatComponent.h is required here.

UGCSDeathComponent::UGCSDeathComponent()
{
	// This component only drives state changes in response to gameplay events
	// (attribute changes, explicit calls) — it does not need to Tick.
	PrimaryComponentTick.bCanEverTick = false;

	// Must replicate so that bIsDead reaches simulated proxies for cosmetic reactions.
	SetIsReplicatedByDefault(true);

	// Default the state tag to GCS.State.Dead. Using TryFind avoids a hard
	// dependency/crash if the tag has not been created yet in a fresh project;
	// designers can still override this in the editor per-instance.
	DeadStateTag = FGameplayTag::RequestGameplayTag(FName("GCS.State.Dead"), /*ErrorIfNotFound=*/false);
}

void UGCSDeathComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGCSDeathComponent, bIsDead);
}

void UGCSDeathComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGCSDeathComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Make sure we don't fire a respawn on a component/actor that is being destroyed.
	if (GetOwner())
	{
		GetOwner()->GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* UGCSDeathComponent::GetOwnerAbilitySystemComponent() const
{
	if (AActor* Owner = GetOwner())
	{
		// UAbilitySystemBlueprintLibrary handles both the common cases of the
		// ASC living directly on the owner (Character) or on a separate
		// PlayerState (common for GAS setups with possession-based respawn).
		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	}
	return nullptr;
}

UGCSCombatAttributeSet* UGCSDeathComponent::GetOwnerCombatAttributeSet() const
{
	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent())
	{
		return const_cast<UGCSCombatAttributeSet*>(ASC->GetSet<UGCSCombatAttributeSet>());
	}
	return nullptr;
}

USkeletalMeshComponent* UGCSDeathComponent::GetOwnerMesh() const
{
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		return OwnerCharacter->GetMesh();
	}

	// Fallback: search for any skeletal mesh component on non-Character owners.
	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
	return nullptr;
}

UCapsuleComponent* UGCSDeathComponent::GetOwnerCapsule() const
{
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		return OwnerCharacter->GetCapsuleComponent();
	}
	return nullptr;
}

void UGCSDeathComponent::HandleDeath(AActor* Killer)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Server-authoritative guard: death should only ever be driven by the server.
	// Clients may call this (e.g. via a replicated RPC elsewhere) but we refuse
	// to run the authoritative logic locally on a non-authority instance.
	if (Owner->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	// Prevent double-death (e.g. simultaneous damage instances both reducing
	// Health to <= 0 in the same frame before the Dead tag is applied).
	if (IsDead())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();

	// 1. Apply the Death GameplayEffect, which is responsible for granting the
	//    Dead tag via the GE asset's "Granted Tags", stripping other transient
	//    state tags, and blocking new ability activation via tag requirements
	//    configured on individual abilities (ActivationBlockedTags).
	if (ASC && DeathGameplayEffectClass)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddInstigator(Killer, Killer);

		const FGameplayEffectSpecHandle DeathSpecHandle = ASC->MakeOutgoingSpec(DeathGameplayEffectClass, /*Level=*/1.f, EffectContext);
		if (DeathSpecHandle.IsValid())
		{
			const FActiveGameplayEffectHandle AppliedHandle = ASC->ApplyGameplayEffectSpecToSelf(*DeathSpecHandle.Data.Get());
			if (AppliedHandle.IsValid())
			{
				ActiveDeathEffectHandle = AppliedHandle;
			}
		}
	}

	// 2. Add the Dead tag as a loose tag as well. Loose tags are useful for
	//    systems (AI perception, animation blueprints, UI) that want to check
	//    "is dead" without caring whether it came from a GE or elsewhere, and
	//    ensures the tag is present even if DeathGameplayEffectClass is unset.
	if (ASC && DeadStateTag.IsValid())
	{
		ASC->AddLooseGameplayTag(DeadStateTag);
		// Replicate the loose tag to simulated proxies so tag-based checks
		// (e.g. GameplayTagQuery in Blueprints) are consistent across the network.
		ASC->SetTagMapCount(DeadStateTag, ASC->GetTagCount(DeadStateTag));
	}

	// 3. Update the replicated cosmetic flag. This will call OnRep_IsDead on
	//    clients automatically; call the shared handler directly here too so
	//    the server/listen-server host also reacts immediately (OnRep never
	//    fires locally on the server).
	bIsDead = true;
	HandleCosmeticDeathStateChanged();

	// 4. Optionally ragdoll immediately.
	if (bAutoRagdoll)
	{
		EnableRagdoll();
	}

	// 5. Broadcast to native/Blueprint listeners (e.g. GameMode kill tracking,
	//    UI kill feed, AI squad notifications).
	OnDeathStarted.Broadcast(Killer);

	// 6. Notify the owner via the death interface, if implemented. This is
	//    where per-character reactions (montages, disabling input, etc.) live.
	if (Owner->GetClass()->ImplementsInterface(UGCSDeathInterface::StaticClass()))
	{
		IGCSDeathInterface::Execute_OnDeath(Owner, Killer);
	}

	// 7. Schedule automatic respawn if configured.
	if (RespawnDelay > 0.f)
	{
		Owner->GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
		Owner->GetWorldTimerManager().SetTimer(
			RespawnTimerHandle,
			this,
			&UGCSDeathComponent::HandleRespawn,
			RespawnDelay,
			/*bLoop=*/false);
	}
}

void UGCSDeathComponent::HandleRespawn()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (Owner->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (!IsDead())
	{
		// Nothing to respawn from.
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();

	// 1. Remove the Death GameplayEffect. Prefer removing by the exact handle
	//    we stored (precise), falling back to removing all active instances of
	//    the class in case the handle was invalidated (e.g. GE expired/was
	//    removed by another system already).
	if (ASC)
	{
		if (ActiveDeathEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveDeathEffectHandle);
			ActiveDeathEffectHandle.Invalidate();
		}
		else if (DeathGameplayEffectClass)
		{
			ASC->RemoveActiveGameplayEffectBySourceEffect(DeathGameplayEffectClass, ASC);
		}
	}

	// 2. Remove the loose Dead tag that mirrors the GE-granted tag.
	if (ASC && DeadStateTag.IsValid())
	{
		ASC->RemoveLooseGameplayTag(DeadStateTag);
	}

	// 3. Restore Health/Shield to max. Prefer a dedicated "respawn heal" GE so
	//    designers can tune restoration (instant vs. over time, VFX cues via
	//    GameplayCues, etc.) entirely in data. Fall back to setting the
	//    attributes directly if no GE is configured, guaranteeing the
	//    character never respawns dead due to missing content.
	UGCSCombatAttributeSet* CombatAttributeSet = GetOwnerCombatAttributeSet();
	bool bRestoredViaEffect = false;

	if (ASC && RespawnHealGameplayEffectClass)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		const FGameplayEffectSpecHandle HealSpecHandle = ASC->MakeOutgoingSpec(RespawnHealGameplayEffectClass, /*Level=*/1.f, EffectContext);
		if (HealSpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*HealSpecHandle.Data.Get());
			bRestoredViaEffect = true;
		}
	}

	if (!bRestoredViaEffect && CombatAttributeSet)
	{
		// Direct attribute restoration fallback. SetHealth/SetShield use the
		// attribute's base-value setter (bypassing GameplayEffect execution),
		// which is acceptable here since respawn is a server-authoritative,
		// non-gameplay-effect-driven reset of character state.
		CombatAttributeSet->SetHealth(CombatAttributeSet->GetMaxHealth());
		CombatAttributeSet->SetShield(CombatAttributeSet->GetMaxShield());
	}

	// 4. Flip the replicated cosmetic flag and react locally.
	bIsDead = false;
	HandleCosmeticDeathStateChanged();

	// 5. Clear any pending respawn timer (covers the case where HandleRespawn
	//    was called manually/early, before the timer elapsed).
	Owner->GetWorldTimerManager().ClearTimer(RespawnTimerHandle);

	// 6. Broadcast to native/Blueprint listeners.
	OnRespawnStarted.Broadcast();

	// 7. Notify the owner via the death interface, if implemented.
	if (Owner->GetClass()->ImplementsInterface(UGCSDeathInterface::StaticClass()))
	{
		IGCSDeathInterface::Execute_OnRespawn(Owner);
	}
}

bool UGCSDeathComponent::IsDead() const
{
	if (const UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent())
	{
		return DeadStateTag.IsValid() && ASC->HasMatchingGameplayTag(DeadStateTag);
	}

	// If there is no ASC available (e.g. called too early in initialization),
	// fall back to the replicated cosmetic flag so callers still get a sane answer.
	return bIsDead;
}

void UGCSDeathComponent::EnableRagdoll()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Disable capsule collision first so the corpse does not continue to
	// block player movement, projectiles, or navigation queries.
	if (UCapsuleComponent* Capsule = GetOwnerCapsule())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Enable physics simulation on the mesh so it ragdolls naturally.
	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
		Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		Mesh->SetSimulatePhysics(true);
		Mesh->WakeAllRigidBodies();

		if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
		{
			// Detach movement from the mesh so the CharacterMovementComponent
			// stops trying to correct the now physically-simulated mesh pose.
			if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
			{
				MovementComponent->DisableMovement();
				MovementComponent->StopMovementImmediately();
			}
		}
	}

	// Cancel all active abilities so no ability logic (montages, timers,
	// input-bound tasks) continues to run on a dead/ragdolled character.
	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();
	}
}

void UGCSDeathComponent::OnRep_IsDead()
{
	// This only ever runs on non-authoritative machines (clients). The server
	// already ran HandleCosmeticDeathStateChanged() directly from
	// HandleDeath/HandleRespawn, so we simply mirror the same cosmetic reaction here.
	HandleCosmeticDeathStateChanged();
}

void UGCSDeathComponent::HandleCosmeticDeathStateChanged()
{
	// Centralized place for reactions that must run identically on the server
	// (host) and all clients purely from the replicated bIsDead flag, without
	// re-running authoritative logic (GE application, attribute changes, etc.)
	// which must only ever happen once, on the server.
	if (bIsDead)
	{
		if (bAutoRagdoll)
		{
			// Note: on the server this is also called from HandleDeath(); calling
			// it again here is harmless/idempotent since it just re-applies the
			// same collision/physics state. On clients, this is the only place
			// ragdoll gets triggered.
			EnableRagdoll();
		}
	}
	else
	{
		// Restore collision/physics on respawn so the character is fully
		// playable again on every machine.
		if (UCapsuleComponent* Capsule = GetOwnerCapsule())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}

		if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
		{
			Mesh->SetSimulatePhysics(false);
			Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));

			if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
			{
				Mesh->AttachToComponent(OwnerCharacter->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);

				if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
				{
					MovementComponent->SetMovementMode(MOVE_Walking);
				}
			}
		}
	}
}
