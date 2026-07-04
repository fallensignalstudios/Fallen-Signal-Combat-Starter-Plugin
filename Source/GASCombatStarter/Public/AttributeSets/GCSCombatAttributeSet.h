// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GCSCombatAttributeSet.generated.h"

// Standard GAS accessor macro block. Generates Get/Set/Init functions plus the
// FGameplayAttribute static getter for each attribute listed via the macro below.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * UGCSCombatAttributeSet
 *
 * Core combat attributes shared by any actor that can fight: player characters,
 * AI, bosses, or destructible combat props. Intentionally generic -- this is not
 * an RPG stat sheet. If your game needs class-specific stats (e.g. Strength,
 * Intelligence), add a second attribute set alongside this one rather than
 * bloating this class.
 *
 * All attributes use FGameplayAttributeData and are replicated with OnRep
 * notifies, following the standard GAS replication pattern (see Epic's
 * ActionRPG / Lyra samples). Clamping (e.g. Health never exceeding MaxHealth)
 * should be implemented in PreAttributeChange / PostGameplayEffectExecute in
 * the .cpp -- see the comments there for extension points.
 */
UCLASS()
class GASCOMBATSTARTER_API UGCSCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGCSCombatAttributeSet();

	//~ Begin UAttributeSet interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	//~ End UAttributeSet interface

	// ----------------------------------------------------------------------
	// Health
	// ----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "GCS|Attributes|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "GCS|Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, MaxHealth)

	// ----------------------------------------------------------------------
	// Shield (damage absorbed before Health, e.g. energy shields)
	// ----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "GCS|Attributes|Shield")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, Shield)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxShield, Category = "GCS|Attributes|Shield")
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, MaxShield)

	// ----------------------------------------------------------------------
	// Energy (resource for special/ranged abilities)
	// ----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Energy, Category = "GCS|Attributes|Energy")
	FGameplayAttributeData Energy;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, Energy)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxEnergy, Category = "GCS|Attributes|Energy")
	FGameplayAttributeData MaxEnergy;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, MaxEnergy)

	// ----------------------------------------------------------------------
	// Stamina (resource for melee/dodge/block actions)
	// ----------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "GCS|Attributes|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "GCS|Attributes|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, MaxStamina)

	// ----------------------------------------------------------------------
	// Combat modifiers
	// ----------------------------------------------------------------------

	/** Flat/scalar attack power. Used by damage GameplayEffect calculations as a coefficient. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "GCS|Attributes|Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, AttackPower)

	/** Flat/scalar defense power. Used by damage GameplayEffect calculations to mitigate incoming damage. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DefensePower, Category = "GCS|Attributes|Combat")
	FGameplayAttributeData DefensePower;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, DefensePower)

	/** Chance (0.0 - 1.0) that an attack from this actor is a critical hit. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritChance, Category = "GCS|Attributes|Combat")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, CritChance)

	/** Damage multiplier applied when a critical hit occurs (e.g. 2.0 = double damage). */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritMultiplier, Category = "GCS|Attributes|Combat")
	FGameplayAttributeData CritMultiplier;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, CritMultiplier)

	// ----------------------------------------------------------------------
	// Meta attribute: incoming damage
	// ----------------------------------------------------------------------

	/**
	 * Not replicated. This is a "meta attribute" used purely as an execution
	 * output target for damage GameplayEffects (see UGCSEffect_DamageBase).
	 * A GameplayEffectExecutionCalculation writes the final computed damage
	 * here, and PostGameplayEffectExecute converts it into Shield/Health loss.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GCS|Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UGCSCombatAttributeSet, IncomingDamage)

protected:
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Shield(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxShield(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Energy(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxEnergy(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_AttackPower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_DefensePower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_CritChance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_CritMultiplier(const FGameplayAttributeData& OldValue);
};
