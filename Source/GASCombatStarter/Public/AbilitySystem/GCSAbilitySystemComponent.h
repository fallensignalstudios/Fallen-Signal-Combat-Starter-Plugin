// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GCSAbilitySystemComponent.generated.h"

class UGCSGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGCSOnAbilityActivated, UGameplayAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGCSOnAbilityEnded, UGameplayAbility*, Ability, bool, bWasCancelled, bool, bWasSuccessful);

/**
 * UGCSAbilitySystemComponent
 *
 * Combat-focused extension of UAbilitySystemComponent. Adds convenience
 * Blueprint-callable functions for granting/removing abilities and querying
 * active abilities by tag, plus broadcast delegates for ability lifecycle
 * events that Blueprint (UI, VFX, audio) can bind to without touching C++.
 *
 * This class intentionally does not add gameplay rules of its own -- it is a
 * thin, reusable wrapper. Game-specific ability granting policy lives in
 * AGCSCharacterBase / UGCSCombatConfig.
 */
UCLASS(ClassGroup = (GASCombatStarter), meta = (BlueprintSpawnableComponent))
class GASCOMBATSTARTER_API UGCSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UGCSAbilitySystemComponent();

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	//~ End UActorComponent interface

	/**
	 * Grants each ability in DefaultAbilities to this component at the given level.
	 * Safe to call multiple times -- abilities already granted (matched by class)
	 * are skipped. Typically called once from AGCSCharacterBase::PossessedBy /
	 * BeginPlay after InitAbilityActorInfo.
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|Abilities")
	void InitializeDefaultAbilities(const TArray<TSubclassOf<UGCSGameplayAbility>>& DefaultAbilities, int32 AbilityLevel = 1);

	/** Grants a single ability class at the given level. Returns the resulting FGameplayAbilitySpecHandle. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Abilities")
	FGameplayAbilitySpecHandle GrantAbility(TSubclassOf<UGCSGameplayAbility> AbilityClass, int32 AbilityLevel = 1, bool bRemoveAfterActivation = false);

	/** Removes a previously granted ability by its spec handle. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Abilities")
	void RemoveAbility(FGameplayAbilitySpecHandle AbilityHandle);

	/** Removes all granted abilities matching the given class. */
	UFUNCTION(BlueprintCallable, Category = "GCS|Abilities")
	void RemoveAbilitiesByClass(TSubclassOf<UGCSGameplayAbility> AbilityClass);

	/** Returns all currently-granted ability specs whose ability tags contain the given tag. */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "GCS|Abilities")
	TArray<FGameplayAbilitySpecHandle> GetActiveAbilitiesByTag(FGameplayTag Tag) const;

	/** Broadcast whenever any ability on this component is activated. */
	UPROPERTY(BlueprintAssignable, Category = "GCS|Abilities|Events")
	FGCSOnAbilityActivated OnGCSAbilityActivated;

	/** Broadcast whenever any ability on this component ends. */
	UPROPERTY(BlueprintAssignable, Category = "GCS|Abilities|Events")
	FGCSOnAbilityEnded OnGCSAbilityEnded;

protected:
	//~ Begin UAbilitySystemComponent interface
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
	virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;
	//~ End UAbilitySystemComponent interface

	/** Tracks handles of abilities granted via InitializeDefaultAbilities, to avoid double-granting. */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> DefaultAbilityHandles;
};
