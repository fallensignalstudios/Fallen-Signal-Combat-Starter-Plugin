// Copyright Fallen Signal Studios. All Rights Reserved.

#include "GASCombatStarterModule.h"
#include "AbilitySystemGlobals.h"

DEFINE_LOG_CATEGORY(LogGASCombatStarter);

#define LOCTEXT_NAMESPACE "FGASCombatStarterModule"

void FGASCombatStarterModule::StartupModule()
{
	UE_LOG(LogGASCombatStarter, Log, TEXT("GASCombatStarter module starting up."));

	// Ensure GAS global data is initialized. UAbilitySystemGlobals::InitGlobalData()
	// is safe to call more than once (it guards internally), so calling it here is a
	// harmless no-op if your project already initializes it elsewhere -- e.g. via
	// bInitGlobalDataOnGameStart=true in DefaultGame.ini (see GASCombatStarter.Build.cs
	// for the exact config snippet).
	UAbilitySystemGlobals::Get().InitGlobalData();
}

void FGASCombatStarterModule::ShutdownModule()
{
	UE_LOG(LogGASCombatStarter, Log, TEXT("GASCombatStarter module shutting down."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGASCombatStarterModule, GASCombatStarter)
