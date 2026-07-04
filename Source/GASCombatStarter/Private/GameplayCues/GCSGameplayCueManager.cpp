// Copyright Fallen Signal Studios. All Rights Reserved.

#include "GameplayCues/GCSGameplayCueManager.h"
#include "GASCombatStarterModule.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Actor.h"

bool UGCSGameplayCueManager::ShouldAsyncLoadRuntimeObjectLibraries() const
{
	// See class comment: for a project-sized cue library, a synchronous load
	// at startup is effectively instant and removes an entire category of
	// "cue silently failed to fire because the async object library hadn't
	// finished loading yet" bugs. Larger projects with hundreds of cue
	// assets spread across many content paths should instead subclass
	// UGameplayCueManager directly (or override this to return true) so the
	// load can be spread across frames.
	return false;
}

TArray<FString> UGCSGameplayCueManager::GetValidGameplayCuePaths()
{
	// Start from the engine default -- whatever the host project has
	// configured via UAbilitySystemGlobals::GetGameplayCueNotifyPaths()
	// (Project Settings > Gameplay Ability System > Gameplay Cue Notify
	// Paths, which ultimately comes from DefaultGame.ini). We are additive on
	// top of this, not a replacement for it.
	TArray<FString> ValidPaths = Super::GetValidGameplayCuePaths();

	// Guarantee this plugin's own mounted content root is always treated as
	// a valid GameplayCueNotify source, even if a host project forgets (or
	// simply never bothers) to add "/GASCombatStarter" to its own
	// GameplayCueNotifyPaths config. This keeps UGCSGameplayCueNotify_HitImpact
	// Blueprint children (and any future GCS.* cues shipped in this plugin's
	// Content folder) discoverable out of the box.
	//
	// AddUnique avoids duplicate entries if the host project's config already
	// includes this path explicitly.
	static const FString GASCombatStarterContentPath = TEXT("/GASCombatStarter");
	ValidPaths.AddUnique(GASCombatStarterContentPath);

	return ValidPaths;
}

void UGCSGameplayCueManager::ExecuteGameplayCueOnActor(AActor* Target, FGameplayTag CueTag, const FGameplayCueParameters& Params)
{
	if (!Target)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSGameplayCueManager::ExecuteGameplayCueOnActor called with a null Target (CueTag: %s)."), *CueTag.ToString());
		return;
	}

	if (!CueTag.IsValid())
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSGameplayCueManager::ExecuteGameplayCueOnActor called with an invalid CueTag on target %s."), *Target->GetName());
		return;
	}

	// Resolve the target's ASC the same way the rest of GAS does (handles
	// actors that implement IAbilitySystemInterface directly, as well as
	// those that expose it indirectly, e.g. via a PlayerState).
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		// A cosmetic GameplayCue should never be a hard gameplay requirement.
		// Actors without an ASC (e.g. purely cosmetic props/destructibles)
		// simply can't route cues through the ASC pipeline -- log and return
		// rather than asserting or crashing.
		UE_LOG(LogGASCombatStarter, Warning, TEXT("UGCSGameplayCueManager::ExecuteGameplayCueOnActor: target %s has no AbilitySystemComponent; cue %s was not executed."), *Target->GetName(), *CueTag.ToString());
		return;
	}

	TargetASC->ExecuteGameplayCue(CueTag, Params);
}
