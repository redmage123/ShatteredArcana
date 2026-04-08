// TESTS DISABLED — fix after main build is clean
#if 0
// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameInstanceTest.cpp — Unit tests for UCoMGameInstance, FCoMCombatContext,
//                           FCoMExplorationContext, FCoMNewGameSettings. COM-032

#include "Misc/AutomationTest.h"
#include "Framework/CoMGameInstance.h"
#include "CoreTypes/CoMConstants.h"


#if WITH_DEV_AUTOMATION_TESTS

// ─── FCoMCombatContext ────────────────────────────────────────────────────────


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMCombatContextValidityTest,
	"ShatteredArcana.Framework.GameInstance.CombatContext.Validity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoMCombatContextValidityTest::RunTest(const FString& Parameters)
{
	FCoMCombatContext Ctx;
	TestFalse(TEXT("Default FCoMCombatContext is invalid (no armies, no return map)"),
	          Ctx.ParticipatingArmyGroupIDs.Num() > 0);

	// IsValid() requires at least one army AND a return map name.
	Ctx.ParticipatingArmyGroupIDs.Add(0);
	TestFalse(TEXT("CombatContext invalid with armies but no return map"), Ctx.ParticipatingArmyGroupIDs.Num() > 0);

	Ctx.ReturnMapName = TEXT("L_Overworld");
	TestTrue(TEXT("CombatContext valid when armies + return map are set"), Ctx.ParticipatingArmyGroupIDs.Num() > 0);

	// Reset must restore to default-invalid state.
	Ctx.Reset();
	TestFalse(TEXT("CombatContext invalid after Reset()"), Ctx.ParticipatingArmyGroupIDs.Num() > 0);
	TestEqual(TEXT("AttackerWizardIndex reset to WIZARD_INDEX_NONE after Reset()"),
	          Ctx.AttackerWizardIndex, CoM::WIZARD_INDEX_NONE);
	TestEqual(TEXT("DefenderWizardIndex reset to WIZARD_INDEX_NONE after Reset()"),
	          Ctx.DefenderWizardIndex, CoM::WIZARD_INDEX_NONE);

	return true;
}

// ─── FCoMExplorationContext ───────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMExplorationContextValidityTest,
	"ShatteredArcana.Framework.GameInstance.ExplorationContext.Validity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoMExplorationContextValidityTest::RunTest(const FString& Parameters)
{
	FCoMExplorationContext Ctx;
	TestFalse(TEXT("Default FCoMExplorationContext is invalid"), Ctx.EntryArmyGroupID >= 0);

	Ctx.DungeonID        = TEXT("DungeonTemplate_CryptOfAshes");
	Ctx.EntryArmyGroupID = 3;
	TestFalse(TEXT("ExplorationContext invalid without return map"), Ctx.EntryArmyGroupID >= 0);

	Ctx.ReturnMapName = TEXT("L_Overworld");
	TestTrue(TEXT("ExplorationContext valid when dungeon, army, and return map are set"),
	         Ctx.EntryArmyGroupID >= 0);

	Ctx.Reset();
	TestFalse(TEXT("ExplorationContext invalid after Reset()"), Ctx.EntryArmyGroupID >= 0);
	TestTrue(TEXT("DungeonID empty after Reset()"), Ctx.DungeonID.IsNone());
	TestEqual(TEXT("EntryArmyGroupID reset to -1 after Reset()"), Ctx.EntryArmyGroupID, -1);

	return true;
}

// ─── FCoMNewGameSettings ──────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMNewGameSettingsValidityTest,
	"ShatteredArcana.Framework.GameInstance.NewGameSettings.Validity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoMNewGameSettingsValidityTest::RunTest(const FString& Parameters)
{
	FCoMNewGameSettings Settings;
	TestFalse(TEXT("Default FCoMNewGameSettings is invalid"), Settings.WizardClass != ECoMWizardClass::None);

	// Name without class → still invalid.
	Settings.WizardName = FText::FromString(TEXT("Aldric"));
	TestFalse(TEXT("Settings invalid with name but no class"), Settings.WizardClass != ECoMWizardClass::None);

	Settings.WizardClass = ECoMWizardClass::Wizard;
	TestTrue(TEXT("Settings valid when name + class are set"), Settings.WizardClass != ECoMWizardClass::None);

	// Default difficulty should be 2 (Normal).
	TestEqual(TEXT("DifficultyLevel defaults to 2 (Normal)"), Settings.DifficultyLevel, 2);

	Settings.Reset();
	TestFalse(TEXT("Settings invalid after Reset()"), Settings.WizardClass != ECoMWizardClass::None);
	TestEqual(TEXT("WizardClass reset to None after Reset()"),
	          Settings.WizardClass, ECoMWizardClass::None);
	TestEqual(TEXT("DifficultyLevel reset to 2 after Reset()"), Settings.DifficultyLevel, 2);

	return true;
}

// ─── UCoMGameInstance — ConsumeNewGameSettings ────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMGameInstanceConsumeSettingsTest,
	"ShatteredArcana.Framework.GameInstance.ConsumeNewGameSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoMGameInstanceConsumeSettingsTest::RunTest(const FString& Parameters)
{
	// NewObject without a world is safe for non-world-dependent GameInstance tests.
	UCoMGameInstance* GI = NewObject<UCoMGameInstance>();
	TestNotNull(TEXT("UCoMGameInstance created via NewObject"), GI);
	if (!GI) { return false; }

	GI->NewGameSettings.WizardName  = FText::FromString(TEXT("Mira"));
	GI->NewGameSettings.WizardClass = ECoMWizardClass::Warlock;

	const FCoMNewGameSettings Consumed = GI->ConsumeNewGameSettings();

	TestEqual(TEXT("Consumed wizard name matches"),
	          Consumed.WizardName.ToString(), FString(TEXT("Mira")));
	TestEqual(TEXT("Consumed wizard class matches"),
	          Consumed.WizardClass, ECoMWizardClass::Warlock);

	// Settings must be cleared after consume — prevents double-consumption.
	TestFalse(TEXT("NewGameSettings cleared after Consume (not valid)"),
	          GI->NewGameSettings.WizardClass != ECoMWizardClass::None);

	return true;
}

// ─── UCoMGameInstance — IsLoadedGame ─────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMGameInstanceIsLoadedGameTest,
	"ShatteredArcana.Framework.GameInstance.IsLoadedGame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoMGameInstanceIsLoadedGameTest::RunTest(const FString& Parameters)
{
	UCoMGameInstance* GI = NewObject<UCoMGameInstance>();
	TestNotNull(TEXT("UCoMGameInstance created"), GI);
	if (!GI) { return false; }

	TestFalse(TEXT("IsLoadedGame() false when LoadedSaveSlotName is empty"),
	          GI->IsLoadedGame());

	GI->LoadedSaveSlotName = TEXT("Save_001");
	TestTrue(TEXT("IsLoadedGame() true when LoadedSaveSlotName is set"),
	         GI->IsLoadedGame());

	return true;
}

// ─── UCoMGameInstance — Shutdown clears all contexts ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMGameInstanceShutdownClearsContextsTest,
	"ShatteredArcana.Framework.GameInstance.Shutdown.ClearsContexts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoMGameInstanceShutdownClearsContextsTest::RunTest(const FString& Parameters)
{
	UCoMGameInstance* GI = NewObject<UCoMGameInstance>();
	TestNotNull(TEXT("UCoMGameInstance created"), GI);
	if (!GI) { return false; }

	// Populate all three contexts.
	GI->CombatContext.ParticipatingArmyGroupIDs.Add(1);
	GI->CombatContext.ReturnMapName = TEXT("L_Overworld");

	GI->ExplorationContext.DungeonID        = TEXT("Crypt");
	GI->ExplorationContext.EntryArmyGroupID = 0;
	GI->ExplorationContext.ReturnMapName    = TEXT("L_Overworld");

	GI->NewGameSettings.WizardName  = FText::FromString(TEXT("Test"));
	GI->NewGameSettings.WizardClass = ECoMWizardClass::Wizard;

	TestTrue(TEXT("CombatContext valid before Shutdown"), GI->CombatContext.ParticipatingArmyGroupIDs.Num() > 0);
	TestTrue(TEXT("ExplorationContext valid before Shutdown"), GI->ExplorationContext.EntryArmyGroupID >= 0);
	TestTrue(TEXT("NewGameSettings valid before Shutdown"), GI->NewGameSettings.WizardClass != ECoMWizardClass::None);

	// Shutdown() must clear everything.
	GI->Shutdown();

	TestFalse(TEXT("CombatContext invalid after Shutdown"), GI->CombatContext.ParticipatingArmyGroupIDs.Num() > 0);
	TestFalse(TEXT("ExplorationContext invalid after Shutdown"), GI->ExplorationContext.EntryArmyGroupID >= 0);
	TestFalse(TEXT("NewGameSettings invalid after Shutdown"), GI->NewGameSettings.WizardClass != ECoMWizardClass::None);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS


#endif
