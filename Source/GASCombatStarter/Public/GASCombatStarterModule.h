// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

GASCOMBATSTARTER_API DECLARE_LOG_CATEGORY_EXTERN(LogGASCombatStarter, Log, All);

/**
 * Runtime module for the GAS Combat Starter plugin.
 *
 * Responsible for module lifecycle only. All gameplay logic lives in the
 * individual classes under Public/Private (AttributeSets, AbilitySystem,
 * Abilities, Components, Characters, Data).
 */
class FGASCombatStarterModule : public IModuleInterface
{
public:
	//~ Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface interface
};
