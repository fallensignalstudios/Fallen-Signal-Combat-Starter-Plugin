// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GCSCombatConfig.generated.h"

class UGCSGameplayAbility;
class UGameplayEffect;

/**
 * UGCSCombatConfig
 *
 * Data-driven combat setup for a character archetype. Instead of hardcoding
 * default abilities/effects on every AGCSCharacterBase subclass, assign a
 * UGCSCombatConfig instance (e.g. DA_Combat_Grunt, DA_Combat_Boss,
 * DA_Combat_Player) and let the character read from it during
 * initialization. This keeps combat tuning in data assets that designers can
 * create/duplicate/tweak without touching C++ or even opening a Character
 * Blueprint.
 *
 * As a UPrimaryDataAsset, instances are discoverable via the Asset Manager
 * (PrimaryAssetType = "GCSCombatConfig"), which is convenient for async
 * loading combat configs by archetype ID at runtime (e.g. for spawning
 * enemies from a spawn table that only stores a PrimaryAssetId).
 */
UCLASS(BlueprintType)
class GASCOMBATSTARTER_API UGCSCombatConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//~ Begin UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UPrimaryDataAsset interface

	/** Abilities granted to the character on initialization. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Combat Config|Abilities")
	TArray<TSubclassOf<UGCSGameplayAbility>> DefaultAbilities;

	/**
	 * GameplayEffects applied once to the character on initialization -- typically
	 * used to set starting attribute values (Health/Energy/Stamina initial GE)
	 * distinct from the AttributeSet's C++ defaults, so different archetypes can
	 * have different starting stats from the same AttributeSet class.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Combat Config|Effects")
	TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;

	/** Default duration, in seconds, of the parry window opened by UGCSAbility_Block. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Combat Config|Tuning", meta = (ClampMin = "0.0", Units = "s"))
	float ParryWindowDuration = 0.25f;

	/** Default stagger duration, in seconds, applied when this archetype is hit-staggered. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Combat Config|Tuning", meta = (ClampMin = "0.0", Units = "s"))
	float StaggerDuration = 0.5f;

	/**
	 * Threshold (0.0 - 1.0) used by gameplay/UI code to decide what counts as a
	 * "critical" hit for presentation purposes (e.g. bigger damage numbers,
	 * special VFX) -- distinct from UGCSCombatAttributeSet::CritChance, which
	 * governs whether a crit occurs at all. This threshold is for reacting to
	 * a hit that did unusually large damage relative to the target's MaxHealth.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GCS|Combat Config|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalHitThreshold = 0.25f;
};
