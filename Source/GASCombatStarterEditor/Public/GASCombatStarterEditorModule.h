// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGASCombatStarterEditor, Log, All);

/**
 * Editor-only module for the GAS Combat Starter plugin.
 *
 * Kept intentionally minimal: this is a home for editor utilities such as
 * asset validation, thumbnail customization, or detail panel customizations
 * for GCS types (e.g. UGCSCombatConfig). No gameplay logic belongs here --
 * anything gameplay-relevant must live in the runtime module so it is
 * available in cooked/packaged builds.
 */
class FGASCombatStarterEditorModule : public IModuleInterface
{
public:
	//~ Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface interface
};
