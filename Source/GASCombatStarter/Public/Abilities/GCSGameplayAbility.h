// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GCSGameplayAbility.generated.h"

class UGCSCombatComponent;
class UGCSAbilitySystemComponent;
class AGCSCharacterBase;

/**
 * Determines how/when a UGCSGameplayAbility tries to activate.
 */
UENUM(BlueprintType)
enum class EGCSAbilityActivationPolicy : uint8
{
	/** Activated in response to a bound input action (see AGCSCharacterBase input bindings). */
	OnInput UMETA(DisplayName = "On Input"),

	/** Activated automatically the moment it is granted to the ability system component. */
	OnGranted UMETA(DisplayName = "On Granted"),

	/** Activated only in response to a GameplayEvent (e.g. sent via SendGameplayEventToActor). */
	OnEvent UMETA(DisplayName = "On Event")
};

/**
 * UGCSGameplayAbility
 *
 * Base class for all gameplay abilities in the GAS Combat Starter plugin.
 * Adds combat-oriented conveniences on top of the stock UGameplayAbility:
 *
 *   - EGCSAbilityActivationPolicy, so AGCSCharacterBase / input binding code
 *     can decide how to trigger abilities without hardcoding per-ability logic.
 *   - CooldownTag / CostTag configuration hooks (actual GameplayEffect classes
 *     for cooldown/cost are assigned per-ability in Blueprint, per standard GAS
 *     practice -- these tags are what UGameplayAbility::GetCooldownTags() and
 *     friends key off of if you build cooldown GEs using DynamicTags).
 *   - GetCombatComponent() / GetOwningCharacter() helpers so derived abilities
 *     (melee/projectile/block) don't repeat actor-info boilerplate.
 *   - Blueprint-implementable hooks for activation/end so designers can layer
 *     Blueprint logic (VFX, camera shake, sound) on top of C++ behavior
 *     without subclassing in C++.
 */
UCLASS(Abstract, HideCategories = Input, Blueprintable)
class GASCOMBATSTARTER_API UGCSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGCSGameplayAbility();

	/** How this ability decides to activate. See EGCSAbilityActivationPolicy. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Ability")
	EGCSAbilityActivationPolicy ActivationPolicy = EGCSAbilityActivationPolicy::OnInput;

	/**
	 * Gameplay tag identifying this ability's cooldown "channel". Convention:
	 * pair this with a GameplayEffect (assigned to CooldownGameplayEffectClass)
	 * that grants this tag for the cooldown duration. Exposed here so C++ and
	 * Blueprint can both query/react to it consistently (e.g. GCS.Cooldown.Melee).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Ability|Cooldown", meta = (Categories = "GCS.Cooldown"))
	FGameplayTag CooldownTag;

	/**
	 * Gameplay tag identifying this ability's resource cost "channel" (e.g. for
	 * UI/analytics hooks). The actual cost (Stamina/Energy) is still applied via
	 * CostGameplayEffectClass, per standard GAS convention -- this tag is metadata
	 * describing what kind of cost it is.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Ability|Cost", meta = (Categories = "GCS.Ability"))
	FGameplayTag CostTag;

	//~ Begin UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	//~ End UGameplayAbility interface

	/** Returns the UGCSCombatComponent on the avatar actor, or nullptr if not present. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GCS|Ability")
	UGCSCombatComponent* GetCombatComponent() const;

	/** Returns the avatar actor cast to AGCSCharacterBase, or nullptr if it isn't one. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GCS|Ability")
	AGCSCharacterBase* GetOwningCharacter() const;

	/** Returns the owning ability system component cast to UGCSAbilitySystemComponent, or nullptr. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GCS|Ability")
	UGCSAbilitySystemComponent* GetGCSAbilitySystemComponent() const;

protected:
	/**
	 * Blueprint hook fired at the top of ActivateAbility, after the C++ base
	 * implementation has run. Use this for designer-facing activation logic
	 * (montage play, VFX spawn, etc.) instead of overriding ActivateAbility in
	 * a C++ subclass when possible.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "GCS|Ability", meta = (DisplayName = "On Ability Activated"))
	void K2_OnAbilityActivated();

	/**
	 * Blueprint hook fired at the top of EndAbility, before the C++ base
	 * implementation runs Super::EndAbility. Use for cleanup VFX/state.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "GCS|Ability", meta = (DisplayName = "On Ability End"))
	void K2_OnAbilityEnd(bool bWasCancelled);
};
