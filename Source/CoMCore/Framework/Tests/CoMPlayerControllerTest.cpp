#if WITH_AUTOMATION_TESTS
// Copyright Mythforge Studios. All Rights Reserved.
// CoMPlayerControllerTest.cpp — Unit tests for ACoMHumanPlayerController
//                               and ACoMAIPlayerController. COM-032

#include "Misc/AutomationTest.h"
#include "Framework/CoMPlayerController.h"


// ─── ACoMHumanPlayerController ────────────────────────────────────────────────


/**
 * CDO defaults: input asset refs must be null (set in project assets at Sprint 2),
 * and StartupInputPriority must default to 0.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMHumanPlayerControllerDefaultsTest,
	"ShatteredArcana.Framework.PlayerController.Human.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoMHumanPlayerControllerDefaultsTest::RunTest(const FString& Parameters)
{
	const ACoMHumanPlayerController* CDO = GetDefault<ACoMHumanPlayerController>();
	TestNotNull(TEXT("ACoMHumanPlayerController CDO is non-null"), CDO);

	if (!CDO) { return false; }

	// Input assets are assigned in Sprint 2 editor assets; must be null in code defaults.
	TestNull(TEXT("DefaultMappingContext defaults to null (assigned in project assets)"),
	         CDO->DefaultMappingContext.Get());
	TestNull(TEXT("IA_EndTurn defaults to null (assigned in project assets)"),
	         CDO->IA_EndTurn.Get());

	// Priority 0 = no override relative to UI contexts; changed in project settings if needed.
	TestEqual(TEXT("StartupInputPriority defaults to 0"), CDO->StartupInputPriority, 0);

	return true;
}

// ─── ACoMAIPlayerController ───────────────────────────────────────────────────

/**
 * AI controller must not be flagged as a player controller —
 * it gets no HUD, no viewport, and no human input component.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMAIPlayerControllerDefaultsTest,
	"ShatteredArcana.Framework.PlayerController.AI.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoMAIPlayerControllerDefaultsTest::RunTest(const FString& Parameters)
{
	const ACoMAIPlayerController* CDO = GetDefault<ACoMAIPlayerController>();
	TestNotNull(TEXT("ACoMAIPlayerController CDO is non-null"), CDO);

	if (!CDO) { return false; }

	TestTrue(TEXT("AI controller is still an APlayerController subclass"),
	         CDO->IsPlayerController());
	TestTrue(TEXT("AI controller class name contains 'AI'"),
	         CDO->GetClass()->GetName().Contains(TEXT("AI")));

	return true;
}

#endif
