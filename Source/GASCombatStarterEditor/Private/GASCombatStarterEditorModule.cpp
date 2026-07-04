// Copyright Fallen Signal Studios. All Rights Reserved.

#include "GASCombatStarterEditorModule.h"

DEFINE_LOG_CATEGORY(LogGASCombatStarterEditor);

#define LOCTEXT_NAMESPACE "FGASCombatStarterEditorModule"

void FGASCombatStarterEditorModule::StartupModule()
{
	UE_LOG(LogGASCombatStarterEditor, Log, TEXT("GASCombatStarterEditor module starting up."));

	// Intentionally minimal. Register asset type actions, detail customizations,
	// or editor toolbar extensions here as the plugin grows. Keeping this module
	// small avoids editor-only code accidentally leaking into runtime behavior.
}

void FGASCombatStarterEditorModule::ShutdownModule()
{
	UE_LOG(LogGASCombatStarterEditor, Log, TEXT("GASCombatStarterEditor module shutting down."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGASCombatStarterEditorModule, GASCombatStarterEditor)
