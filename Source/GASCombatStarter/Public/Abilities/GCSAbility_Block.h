// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GCSGameplayAbility.h"
#include "GCSAbility_Block.generated.h"

class UGameplayEffect;

/**
 * UGCSAbility_Block
 *
 * Toggle-hold block ability. On activation: applies GCS.State.Blocking (via
 * BlockStateEffectClass, which should be an infinite/duration GE that grants
 * that tag and any damage-mitigation modifiers), and opens a parry window at
 * the start of the block for ParryWindowDuration seconds. If an attack lands
 * on the owner while the parry window is open (see
 * UGCSCombatComponent::TryConsumeParry, called from your damage pipeline),
 * this ability sends a GCS.Event.ParrySuccess GameplayEvent so a counterattack
 * ability can react.
 *
 * Expected usage: bind to an input action with hold semantics. Call
 * StopBlocking (via input-released -> CancelAbility, or an explicit Blueprint
 * call) to end the block state. This ability does not auto-end itself; it
 * remains active for the duration of the hold.
 */
UCLASS(Abstract, Blueprintable)
class GASCOMBATSTARTER_API UGCSAbility_Block : public UGCSGameplayAbility
{
	GENERATED_BODY()

public:
	UGCSAbility_Block();

	//~ Begin UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	//~ End UGameplayAbility interface

	/**
	 * Duration/infinite GameplayEffect applied while blocking. Should grant
	 * GCS.State.Blocking and apply incoming-damage mitigation modifiers (e.g. a
	 * percentage damage reduction executed in your damage calculation).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Block")
	TSubclassOf<UGameplayEffect> BlockStateEffectClass;

	/** How long, in seconds, after starting a block the parry window stays open. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Block|Parry")
	float ParryWindowDuration = 0.25f;

protected:
	/** Handle to the active BlockStateEffectClass application, so it can be removed on EndAbility. */
	UPROPERTY()
	FActiveGameplayEffectHandle BlockStateEffectHandle;

	/** Applies BlockStateEffectClass to self and updates UGCSCombatComponent::bIsBlocking. */
	void ApplyBlockState();

	/** Removes the block state effect and clears UGCSCombatComponent::bIsBlocking. */
	void RemoveBlockState();
};
