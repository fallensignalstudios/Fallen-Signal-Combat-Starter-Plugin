// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GCSCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCSOnBlockStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCSOnBlockEnd);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGCSOnParrySuccess, AActor*, Attacker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGCSOnComboAdvance, int32, NewComboIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCSOnComboReset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGCSOnStaggerBegin, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCSOnStaggerEnd);

/**
 * UGCSCombatComponent
 *
 * Lightweight actor component that tracks transient combat state which does
 * not belong on the AttributeSet (attributes are for numeric, GE-modifiable
 * values -- this component is for booleans/timers/indices that combat
 * abilities and animation/UI logic need to read and react to).
 *
 * Deliberately NOT a GAS AttributeSet or AbilitySystemComponent -- this is a
 * plain UActorComponent so it can be added to any actor (players, AI, bosses)
 * regardless of their ability setup, and so designers can inspect/tweak its
 * replicated state directly in the Blueprint details panel / debugger.
 *
 * Abilities (UGCSAbility_Block, UGCSAbility_MeleeAttack, etc.) read and write
 * this component's state via UGCSGameplayAbility::GetCombatComponent().
 */
UCLASS(ClassGroup = (GASCombatStarter), meta = (BlueprintSpawnableComponent))
class GASCOMBATSTARTER_API UGCSCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGCSCombatComponent();

	//~ Begin UActorComponent interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	//~ End UActorComponent interface

	// ----------------------------------------------------------------------
	// Blocking
	// ----------------------------------------------------------------------

	/** True while the owner is actively blocking (see UGCSAbility_Block). */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsBlocking, Category = "GCS|Combat|Block")
	bool bIsBlocking = false;

	/** Begins blocking. Server-authoritative; call from UGCSAbility_Block. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Block")
	void StartBlocking();

	/** Ends blocking. Server-authoritative; call from UGCSAbility_Block. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Block")
	void StopBlocking();

	// ----------------------------------------------------------------------
	// Parry window
	// ----------------------------------------------------------------------

	/** True while the owner is within an active parry window. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsInParryWindow, Category = "GCS|Combat|Parry")
	bool bIsInParryWindow = false;

	/** Opens a parry window for the given duration (seconds). Server-authoritative. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Parry")
	void BeginParryWindow(float Duration);

	/** Closes the parry window immediately (called automatically when the timer expires). */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Parry")
	void EndParryWindow();

	/**
	 * Called by the damage pipeline when an attack lands on this actor while
	 * bIsInParryWindow is true. Broadcasts OnParrySuccess and closes the window.
	 * Returns true if the parry was consumed (i.e. the window was open).
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Parry")
	bool TryConsumeParry(AActor* Attacker);

	// ----------------------------------------------------------------------
	// Combo tracking
	// ----------------------------------------------------------------------

	/** Current index into the owner's melee combo chain. 0 = no combo in progress. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentComboIndex, Category = "GCS|Combat|Combo")
	int32 CurrentComboIndex = 0;

	/** Advances the combo by one step and broadcasts OnComboAdvance. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Combo")
	void AdvanceCombo();

	/** Resets the combo back to 0 and broadcasts OnComboReset. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Combo")
	void ResetCombo();

	// ----------------------------------------------------------------------
	// Hit timing / stagger
	// ----------------------------------------------------------------------

	/** World time (GetWorld()->GetTimeSeconds()) of the last confirmed hit taken by this actor. */
	UPROPERTY(BlueprintReadOnly, Category = "GCS|Combat|Hits")
	float LastHitTime = -1.f;

	/** True while the owner is in a hit-stagger state (typically blocks new ability activation). */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsStaggered, Category = "GCS|Combat|Stagger")
	bool bIsStaggered = false;

	/** Records LastHitTime = now. Call from damage-handling code (e.g. AttributeSet or character). */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Hits")
	void RecordHitTaken();

	/** Begins a stagger state for the given duration; auto-clears via timer. Server-authoritative. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Stagger")
	void BeginStagger(float Duration);

	/** Ends the stagger state immediately. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Stagger")
	void EndStagger();

	// ----------------------------------------------------------------------
	// Delegates
	// ----------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "GCS|Combat|Events")
	FGCSOnBlockStart OnBlockStart;

	UPROPERTY(BlueprintAssignable, Category = "GCS|Combat|Events")
	FGCSOnBlockEnd OnBlockEnd;

	UPROPERTY(BlueprintAssignable, Category = "GCS|Combat|Events")
	FGCSOnParrySuccess OnParrySuccess;

	UPROPERTY(BlueprintAssignable, Category = "GCS|Combat|Events")
	FGCSOnComboAdvance OnComboAdvance;

	UPROPERTY(BlueprintAssignable, Category = "GCS|Combat|Events")
	FGCSOnComboReset OnComboReset;

	UPROPERTY(BlueprintAssignable, Category = "GCS|Combat|Events")
	FGCSOnStaggerBegin OnStaggerBegin;

	UPROPERTY(BlueprintAssignable, Category = "GCS|Combat|Events")
	FGCSOnStaggerEnd OnStaggerEnd;

protected:
	UFUNCTION()
	void OnRep_IsBlocking();

	UFUNCTION()
	void OnRep_IsInParryWindow();

	UFUNCTION()
	void OnRep_CurrentComboIndex();

	UFUNCTION()
	void OnRep_IsStaggered();

	/** Timer handle for auto-closing the parry window. */
	FTimerHandle ParryWindowTimerHandle;

	/** Timer handle for auto-clearing stagger. */
	FTimerHandle StaggerTimerHandle;
};
