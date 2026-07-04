// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GCSCharacterBase.generated.h"

class UGCSAbilitySystemComponent;
class UGCSCombatAttributeSet;
class UGCSCombatComponent;
class UGCSCombatConfig;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * AGCSCharacterBase
 *
 * Base character class integrating the full GAS Combat Starter stack:
 * UGCSAbilitySystemComponent, UGCSCombatAttributeSet, and UGCSCombatComponent.
 * Implements IAbilitySystemInterface so the ASC is discoverable by the rest of
 * GAS (targeting, GameplayCues, etc.).
 *
 * Handles the two standard GAS ActorInfo initialization paths:
 *   - PossessedBy (server): runs when the server possesses this pawn with a
 *     controller (AI or player). This is the authoritative InitAbilityActorInfo
 *     call and is also where DefaultCombatConfig is applied.
 *   - OnRep_PlayerState / AcknowledgePossession-equivalent (client): for
 *     player-controlled characters, the client needs its own
 *     InitAbilityActorInfo call once it has a valid PlayerState, since the ASC
 *     in this starter lives on the character (not the PlayerState) but Owner/
 *     Avatar actor info must still be (re)established on each client.
 *
 * NOTE ON ASC OWNERSHIP: This class places the ASC directly on the character
 * (common for action games where abilities don't need to persist across
 * character death/respawn). If your project needs abilities/attributes to
 * survive character death (typical for MOBA/RPG-style possession swapping),
 * move the ASC to PlayerState instead and adjust InitAbilityActorInfo calls
 * accordingly -- the rest of this plugin (attribute set, combat component,
 * abilities) works identically either way since they all fetch the ASC via
 * IAbilitySystemInterface rather than assuming it lives on the character.
 *
 * Enhanced Input: this class exposes IMC/IA properties and a virtual
 * SetupPlayerInputComponent override with clearly marked bind points.
 * Concrete input actions are intentionally left for the client project /
 * Blueprint subclass to assign, since input schemes vary heavily by game.
 */
UCLASS(Abstract, Blueprintable)
class GASCOMBATSTARTER_API AGCSCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGCSCharacterBase();

	//~ Begin AActor interface
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	//~ End AActor interface

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	/** Typed accessor for the ability system component. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GCS|Character")
	UGCSAbilitySystemComponent* GetGCSAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** Typed accessor for the combat attribute set. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GCS|Character")
	UGCSCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributeSet; }

	/** Typed accessor for the combat component. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GCS|Character")
	UGCSCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
	/** Ability System Component driving this character's abilities/effects/attributes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GCS|Character", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGCSAbilitySystemComponent> AbilitySystemComponent;

	/** Core combat attributes (Health, Shield, Energy, Stamina, AttackPower, etc.). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GCS|Character", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGCSCombatAttributeSet> CombatAttributeSet;

	/** Transient combat state (blocking, parry window, combo, stagger). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GCS|Character", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGCSCombatComponent> CombatComponent;

	/**
	 * Optional data asset describing this character's default abilities/effects
	 * and combat tuning. If set, InitializeFromCombatConfig() is called
	 * automatically from PossessedBy on the server. Leave unset if you'd rather
	 * grant abilities manually (e.g. from a GameMode or spawner).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Character|Config")
	TObjectPtr<UGCSCombatConfig> DefaultCombatConfig;

	// ----------------------------------------------------------------------
	// Enhanced Input -- assign concrete assets in your project/Blueprint subclass.
	// ----------------------------------------------------------------------

	/** Input Mapping Context to add for this character when possessed by a local player controller. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Character|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Priority used when adding DefaultMappingContext to the Enhanced Input subsystem. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Character|Input")
	int32 InputMappingPriority = 0;

	/** Input Action bound to GCS.Input.PrimaryAttack. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Character|Input")
	TObjectPtr<UInputAction> PrimaryAttackInputAction;

	/** Input Action bound to GCS.Input.Block. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Character|Input")
	TObjectPtr<UInputAction> BlockInputAction;

	/** Input Action bound to GCS.Input.Dodge. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Character|Input")
	TObjectPtr<UInputAction> DodgeInputAction;

	/**
	 * Shared entry point for both server and client actor-info initialization.
	 * Safe to call multiple times. Also grants DefaultCombatConfig's abilities
	 * when called with authority.
	 */
	virtual void InitAbilityActorInfo();

	/** Applies DefaultCombatConfig's abilities/effects to this character. Server-only; no-op if DefaultCombatConfig is unset. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Character|Config")
	virtual void InitializeFromCombatConfig();

	// ----------------------------------------------------------------------
	// Enhanced Input handlers. Bound in SetupPlayerInputComponent if the
	// corresponding Input Action is assigned. Override in Blueprint/C++
	// subclasses to change ability-triggering behavior (these defaults simply
	// try to activate abilities tagged with the matching GCS.Ability.* tag).
	// ----------------------------------------------------------------------

	UFUNCTION(BlueprintNativeEvent, Category = "GCS|Character|Input")
	void OnPrimaryAttackInput(const FInputActionValue& Value);
	virtual void OnPrimaryAttackInput_Implementation(const FInputActionValue& Value);

	UFUNCTION(BlueprintNativeEvent, Category = "GCS|Character|Input")
	void OnBlockInputStarted(const FInputActionValue& Value);
	virtual void OnBlockInputStarted_Implementation(const FInputActionValue& Value);

	UFUNCTION(BlueprintNativeEvent, Category = "GCS|Character|Input")
	void OnBlockInputCompleted(const FInputActionValue& Value);
	virtual void OnBlockInputCompleted_Implementation(const FInputActionValue& Value);

	UFUNCTION(BlueprintNativeEvent, Category = "GCS|Character|Input")
	void OnDodgeInput(const FInputActionValue& Value);
	virtual void OnDodgeInput_Implementation(const FInputActionValue& Value);
};
