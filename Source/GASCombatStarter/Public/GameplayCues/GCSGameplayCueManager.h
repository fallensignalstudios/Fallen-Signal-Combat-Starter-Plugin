// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueManager.h"
#include "GCSGameplayCueManager.generated.h"

/**
 * UGCSGameplayCueManager
 *
 * Lightweight UGameplayCueManager subclass tuned for small/medium-sized
 * projects that don't need the engine's async GameplayCue runtime object
 * library loading (which is designed for AAA-scale projects with hundreds of
 * cue assets spread across many paths, where blocking the game thread to
 * load them all synchronously at startup would be unacceptable).
 *
 * -----------------------------------------------------------------------
 * REQUIRED PROJECT SETUP -- this class does nothing until it is actually
 * selected as the engine's GameplayCueManager. Add the following to your
 * PROJECT's (not the plugin's) Config/DefaultGame.ini:
 *
 *     [/Script/GameplayAbilities.AbilitySystemGlobals]
 *     GameplayCueManagerClass=/Script/GASCombatStarter.GCSGameplayCueManager
 *
 * Without this, UAbilitySystemGlobals falls back to the stock
 * UGameplayCueManager, and none of the behavior below (sync loading, scoped
 * valid-path filtering) takes effect -- your cues will still fire (the stock
 * manager works fine), you just won't get this class's small-project tuning.
 * -----------------------------------------------------------------------
 *
 * What this class changes vs. the engine default:
 *
 *   - ShouldAsyncLoadRuntimeObjectLibraries() returns false, forcing the
 *     GameplayCue runtime object library (the list of all
 *     UGameplayCueNotify_Static/Actor assets discovered under the configured
 *     scan paths) to load synchronously during engine init instead of over
 *     several frames via the streaming manager. For a small number of cue
 *     assets (typical of a combat-starter-sized project) the synchronous
 *     load is effectively instant, and it sidesteps an entire class of bugs
 *     where a GameplayCue is executed before its async load has completed
 *     (silently doing nothing, with no error, the first time it's triggered
 *     after a level load).
 *
 *   - GetValidGameplayCuePaths() narrows the set of content paths the cue
 *     manager will treat as valid sources of GameplayCueNotify assets to
 *     this plugin's own content directory (GASCombatStarter/), in addition
 *     to whatever UAbilitySystemGlobals::GetGameplayCueNotifyPaths() already
 *     reports from project config (Project Settings > Gameplay Ability
 *     System > Gameplay Cue Notify Paths). This keeps this manager's
 *     validation/scan scope from accidentally picking up unrelated cue
 *     content in host projects that also use other GAS-based plugins/cue
 *     sets. NOTE: this does not replace configuring
 *     GameplayCueNotifyPaths in DefaultGame.ini -- it is additive scoping on
 *     top of it (see the .cpp for exactly how the paths are merged).
 *
 *   - ExecuteGameplayCueOnActor() is a static convenience wrapper for firing
 *     a cue by tag on an arbitrary actor without having to look up its
 *     UAbilitySystemComponent (or handle the case where it doesn't have one)
 *     at every call site.
 */
UCLASS()
class GASCOMBATSTARTER_API UGCSGameplayCueManager : public UGameplayCueManager
{
	GENERATED_BODY()

public:
	//~ Begin UGameplayCueManager interface

	/**
	 * Forces synchronous loading of the GameplayCue runtime object library.
	 * See the class comment for why this is the right tradeoff for small
	 * projects: it trades a (typically negligible) blocking load at startup
	 * for eliminating an entire class of "cue silently didn't fire because
	 * it hadn't finished async-loading yet" timing bugs.
	 */
	virtual bool ShouldAsyncLoadRuntimeObjectLibraries() const override;

	/**
	 * Returns the list of content-browser paths this manager considers valid
	 * locations for GameplayCueNotify assets. The base implementation
	 * (UGameplayCueManager::GetValidGameplayCuePaths) simply returns
	 * GetAlwaysLoadedGameplayCuePaths(), which is driven by
	 * UAbilitySystemGlobals::GetGameplayCueNotifyPaths() (i.e. whatever the
	 * host project configured in DefaultGame.ini). This override keeps that
	 * full behavior but guarantees "/GASCombatStarter" (this plugin's own
	 * mounted content root) is always included, so the plugin's own cues
	 * (e.g. UGCSGameplayCueNotify_HitImpact Blueprint children) are always
	 * discovered even in a host project that hasn't explicitly added this
	 * plugin's content path to its GameplayCueNotifyPaths config.
	 */
	virtual TArray<FString> GetValidGameplayCuePaths() override;

	//~ End UGameplayCueManager interface

	/**
	 * Convenience wrapper around UAbilitySystemComponent::ExecuteGameplayCue
	 * that resolves Target's ASC for you (via
	 * UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent, which also
	 * handles actors that implement IAbilitySystemInterface indirectly, e.g.
	 * through a PlayerState). Safe to call on an actor with no ASC at all --
	 * it simply no-ops (with a log warning) in that case, since a cosmetic
	 * cue should never be a hard requirement for gameplay code to function.
	 *
	 * @param Target  The actor to execute the cue "on" (drives cue actor
	 *                spawning/attachment for Actor-type cues, and is passed
	 *                through as the cue's target for Static cues).
	 * @param CueTag  The GameplayCue.* tag identifying which cue to run.
	 *                Must be rooted at "GameplayCue." (see the patch note in
	 *                GCSGameplayCueNotify_HitImpact.h for why).
	 * @param Params  Cue parameters (hit location/normal, magnitude,
	 *                instigator/effect context, etc.) forwarded to the cue.
	 */
	UFUNCTION(BlueprintCallable, Category = "GCS|GameplayCue", meta = (AutoCreateRefTerm = "Params"))
	static void ExecuteGameplayCueOnActor(AActor* Target, FGameplayTag CueTag, const FGameplayCueParameters& Params);
};
