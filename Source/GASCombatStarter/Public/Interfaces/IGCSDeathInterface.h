// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IGCSDeathInterface.generated.h"

/**
 * UGCSDeathInterface
 *
 * UInterface boilerplate required by Unreal's reflection system so that
 * IGCSDeathInterface can be implemented by both native (C++) and Blueprint classes.
 * This class carries no logic of its own — see IGCSDeathInterface below.
 */
UINTERFACE(BlueprintType, MinimalAPI)
class UGCSDeathInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * IGCSDeathInterface
 *
 * Pure interface implemented by any Actor that can die and be respawned
 * (typically AGCSCharacterBase). Decouples death-reactive gameplay/cosmetic
 * logic (ragdoll, animation, UI, AI notification, etc.) from the component
 * that drives the death state machine (UGCSDeathComponent).
 *
 * All functions are BlueprintNativeEvent so that:
 *   - C++ classes can provide a default native implementation (via the
 *     _Implementation suffix) that Blueprint subclasses may still override.
 *   - Designers can implement/extend death reactions entirely in Blueprint
 *     without touching C++.
 */
class GASCOMBATSTARTER_API IGCSDeathInterface
{
	GENERATED_BODY()

public:

	/**
	 * Called when this actor's Health has reached 0 and the death sequence
	 * has been initiated by UGCSDeathComponent::HandleDeath.
	 *
	 * Implementations typically:
	 *   - Play death animations/montages or enable ragdoll physics.
	 *   - Disable input, movement, and combat abilities.
	 *   - Notify AI controllers, UI (kill feed, health bars), and game modes.
	 *
	 * @param Killer  The actor responsible for the killing blow. May be nullptr
	 *                if the death was not attributable to a specific instigator
	 *                (e.g. environmental damage, scripted death).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GCS|Death")
	void OnDeath(AActor* Killer);

	/**
	 * Called when this actor has been respawned by UGCSDeathComponent::HandleRespawn.
	 *
	 * Implementations typically:
	 *   - Restore mesh/capsule collision and disable ragdoll physics.
	 *   - Re-enable input, movement, and combat abilities.
	 *   - Teleport to a spawn point, reset visual state, play a respawn effect.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GCS|Death")
	void OnRespawn();

	/**
	 * @return True if this actor is currently in the dead state
	 * (i.e. the ability system component has the GCS.State.Dead gameplay tag).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GCS|Death")
	bool IsDead() const;
};
