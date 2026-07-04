// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GCSDeathComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UGCSCombatComponent;
class UGCSCombatAttributeSet;
class USkeletalMeshComponent;
class UCapsuleComponent;
struct FActiveGameplayEffectHandle;

/**
 * Broadcast when death handling begins on this actor.
 * @param Killer  The actor that caused the death (may be nullptr).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGCSOnDeathStarted, AActor*, Killer);

/**
 * Broadcast when respawn handling begins on this actor.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCSOnRespawnStarted);

/**
 * UGCSDeathComponent
 *
 * Server-authoritative component responsible for driving the death and respawn
 * state machine for an Actor that owns an AbilitySystemComponent (typically
 * AGCSCharacterBase). This component works ALONGSIDE UGCSCombatComponent
 * (which continues to own combat state such as staggering) — it does not
 * replace or duplicate its responsibilities.
 *
 * Responsibilities:
 *   - Applying/removing the "Dead" GameplayEffect and GCS.State.Dead tag.
 *   - Optionally enabling ragdoll physics on death.
 *   - Optionally scheduling an automatic respawn after a delay.
 *   - Restoring attributes (Health/Shield) on respawn.
 *   - Replicating a simple bIsDead flag so clients can react cosmetically
 *     even without directly inspecting the ability system's tag container.
 *   - Forwarding lifecycle notifications to IGCSDeathInterface and to
 *     C++/Blueprint listeners via dynamic multicast delegates.
 *
 * Usage:
 *   Add this as a subobject on AGCSCharacterBase (or any Pawn with an ASC).
 *   Call HandleDeath() from the AttributeSet's PostGameplayEffectExecute
 *   (or equivalent) once Health has been clamped to <= 0. See
 *   GCSCombatAttributeSet_DeathPatch.h and GCSCharacterBase_DeathPatch.h for
 *   the exact wiring pattern.
 */
UCLASS(ClassGroup = (GCS), meta = (BlueprintSpawnableComponent))
class GASCOMBATSTARTER_API UGCSDeathComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UGCSDeathComponent();

	//~ Begin UActorComponent Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface

	/**
	 * Server-authoritative entry point for killing the owning actor.
	 *
	 * Steps performed:
	 *   1. Early-outs if already dead (prevents double-death).
	 *   2. Applies DeathGameplayEffectClass to the owner's ASC (grants the
	 *      Dead tag, strips other gameplay tags, blocks new ability
	 *      activation — all defined on the GE asset itself).
	 *   3. Adds DeadStateTag as a loose gameplay tag for systems that check
	 *      tags without caring about the underlying GE (e.g. AI, animation).
	 *   4. Sets bIsDead = true, which replicates and fires OnRep_IsDead on
	 *      clients for cosmetic reactions.
	 *   5. Optionally enables ragdoll (EnableRagdoll) if bAutoRagdoll is set.
	 *   6. Broadcasts OnDeathStarted and calls IGCSDeathInterface::OnDeath on
	 *      the owner, if implemented.
	 *   7. Optionally starts a respawn timer if RespawnDelay > 0.
	 *
	 * Safe to call multiple times — subsequent calls while already dead are ignored.
	 *
	 * @param Killer  The actor responsible for the kill (may be nullptr).
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|Death")
	void HandleDeath(AActor* Killer);

	/**
	 * Server-authoritative entry point for respawning the owning actor.
	 *
	 * Steps performed:
	 *   1. Early-outs if not currently dead.
	 *   2. Removes the DeathGameplayEffectClass (by class) from the owner's ASC.
	 *   3. Removes DeadStateTag loose tag.
	 *   4. Restores Health/Shield to their max values, either by applying
	 *      RespawnHealGameplayEffectClass (preferred, if set) or by directly
	 *      setting the attributes on the AttributeSet as a fallback.
	 *   5. Sets bIsDead = false, replicating to clients.
	 *   6. Clears any pending respawn timer.
	 *   7. Broadcasts OnRespawnStarted and calls IGCSDeathInterface::OnRespawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|Death")
	void HandleRespawn();

	/** @return True if the owner's ASC currently has DeadStateTag. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GCS|Death")
	bool IsDead() const;

	/**
	 * Enables ragdoll physics on the owner's mesh and disables capsule collision
	 * so the corpse can settle naturally without blocking further movement/traces.
	 * Also cancels all currently active gameplay abilities to prevent input or
	 * abilities from executing while dead.
	 *
	 * Safe to call directly (e.g. from a death montage notify) or automatically
	 * via bAutoRagdoll.
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|Death")
	void EnableRagdoll();

	/** Broadcast on the server and on all clients when death handling starts. */
	UPROPERTY(BlueprintAssignable, Category = "GCS|Death")
	FGCSOnDeathStarted OnDeathStarted;

	/** Broadcast on the server and on all clients when respawn handling starts. */
	UPROPERTY(BlueprintAssignable, Category = "GCS|Death")
	FGCSOnRespawnStarted OnRespawnStarted;

protected:

	/**
	 * GameplayEffect applied to the owner on death. This GE is expected to:
	 *   - Grant the GCS.State.Dead tag (as a granted tag, not just an asset tag).
	 *   - Remove/clear other relevant state tags (e.g. GCS.State.Staggered).
	 *   - Apply a GameplayTagCountContainer-based ability block (e.g. via a
	 *     "GCS.State.Dead" tag added to ActivationBlockedTags on abilities),
	 *     preventing new abilities from activating while dead.
	 * Assign a Blueprint GE subclass in the editor (e.g. GE_Death).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Death")
	TSubclassOf<UGameplayEffect> DeathGameplayEffectClass;

	/**
	 * Optional GameplayEffect applied on respawn to restore Health/Shield to
	 * max (e.g. via SetByCaller or Instant modifiers with "set to max" logic).
	 * If unset, HandleRespawn falls back to directly setting the attributes
	 * to their max values on the AttributeSet.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Death")
	TSubclassOf<UGameplayEffect> RespawnHealGameplayEffectClass;

	/** If true, HandleDeath automatically calls EnableRagdoll(). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GCS|Death")
	bool bAutoRagdoll = false;

	/**
	 * Seconds after death before HandleRespawn is automatically triggered.
	 * Set to 0 (or less) to disable automatic respawn — in that case some
	 * external system (game mode, respawn manager, etc.) is expected to call
	 * HandleRespawn() explicitly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GCS|Death", meta = (ClampMin = "0.0"))
	float RespawnDelay = 5.f;

	/** Gameplay tag representing the "dead" state. Defaults to GCS.State.Dead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GCS|Death")
	FGameplayTag DeadStateTag;

	/**
	 * Replicated cosmetic flag mirroring the death state. Game logic that
	 * needs authoritative state should prefer IsDead()/ASC tag checks; this
	 * flag exists so clients (including simulated proxies without full ASC
	 * tag replication awareness) can react to death/respawn transitions via
	 * OnRep_IsDead without polling gameplay tags every frame.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleInstanceOnly, BlueprintReadOnly, Category = "GCS|Death")
	bool bIsDead = false;

	/** Fired on clients (and locally on listen server) whenever bIsDead changes. */
	UFUNCTION()
	void OnRep_IsDead();

	/**
	 * Cosmetic reaction helper called from OnRep_IsDead. Plays/stops ragdoll
	 * and any other purely-visual reactions that must run on every machine
	 * (including simulated proxies), independent of the authoritative logic
	 * already executed on the server inside HandleDeath/HandleRespawn.
	 */
	void HandleCosmeticDeathStateChanged();

private:

	/** Resolves and caches the owner's AbilitySystemComponent. */
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;

	/** Resolves the owner's CombatAttributeSet, if present. */
	UGCSCombatAttributeSet* GetOwnerCombatAttributeSet() const;

	/** Resolves the owner's skeletal mesh component, if present. */
	USkeletalMeshComponent* GetOwnerMesh() const;

	/** Resolves the owner's capsule component, if present. */
	UCapsuleComponent* GetOwnerCapsule() const;

	/** Handle to the active death GameplayEffect, used to remove it precisely on respawn. */
	FActiveGameplayEffectHandle ActiveDeathEffectHandle;

	/** Timer handle used for the automatic respawn delay. */
	FTimerHandle RespawnTimerHandle;
};
