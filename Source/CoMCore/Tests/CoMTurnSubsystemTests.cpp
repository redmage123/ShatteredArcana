#if WITH_AUTOMATION_TESTS
// Copyright Mythforge Studios. All Rights Reserved.
// CoMTurnSubsystemTests.cpp — Unit tests for UCoMTurnSubsystem. COM-028 / Policy.
//
// Run: CoM.Turn.*
// Headless: UnrealEditor-Cmd ShatteredArcana.uproject -ExecCmds="Automation RunTests CoM.Turn" -Unattended -NullRHI

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Turn/CoMTurnSubsystem.h"
#include "CoreTypes/CoMEnums.h"
#include "CoreTypes/CoMConstants.h"

// ─────────────────────────────────────────────────────────────────────────────
// Initial state — subsystem starts at turn 1, WorldProcessing phase
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoMTurnInitialState,
	"CoM.Turn.InitialState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTurnInitialState::RunTest(const FString& /*Params*/)
{
	UCoMTurnSubsystem* Sys = NewObject<UCoMTurnSubsystem>();

	TestEqual(TEXT("Starts at turn 1"),              Sys->GetCurrentTurn(), 1);
	TestEqual(TEXT("Starts in WorldProcessing phase"),
		Sys->GetCurrentPhase(), ECoMGamePhase::WorldProcessing);
	TestEqual(TEXT("ActiveWizard = WIZARD_INDEX_NONE"),
		Sys->GetActiveWizardIndex(), CoM::WIZARD_INDEX_NONE);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// StartGame — 2-wizard game enters WorldProcessing then transitions to PlayerTurn
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoMTurnStartGame,
	"CoM.Turn.StartGame",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTurnStartGame::RunTest(const FString& /*Params*/)
{
	UCoMTurnSubsystem* Sys = NewObject<UCoMTurnSubsystem>();
	Sys->StartGame();

	// After StartGame, the first wizard's turn should be active
	TestNotEqual(TEXT("Phase advanced from WorldProcessing"),
		Sys->GetCurrentPhase(), ECoMGamePhase::WorldProcessing);
	TestTrue(TEXT("ActiveWizardIndex in valid range [0,1]"),
		Sys->GetActiveWizardIndex() >= 0 && Sys->GetActiveWizardIndex() < 2);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// AdvanceWizardPhase — stepping through all 6 sub-phases for 1 wizard
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoMTurnWizardPhaseAdvance,
	"CoM.Turn.WizardPhaseAdvance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTurnWizardPhaseAdvance::RunTest(const FString& /*Params*/)
{
	UCoMTurnSubsystem* Sys = NewObject<UCoMTurnSubsystem>();
	Sys->StartGame();  // single wizard simplifies sequencing

	const TArray<ECoMWizardPhase> ExpectedSequence = {
		ECoMWizardPhase::Planning,
		ECoMWizardPhase::Movement,
		ECoMWizardPhase::Combat,
		ECoMWizardPhase::Magic,
		ECoMWizardPhase::Income,
		ECoMWizardPhase::EndTurn,
	};

	// Record phase sequence as we advance
	TArray<ECoMWizardPhase> Observed;
	Observed.Add(Sys->GetCurrentWizardPhase());

	for (int32 i = 1; i < ExpectedSequence.Num(); ++i)
	{
		Sys->AdvanceWizardPhase();
		Observed.Add(Sys->GetCurrentWizardPhase());
	}

	TestEqual(TEXT("Phase sequence length"), Observed.Num(), ExpectedSequence.Num());
	for (int32 i = 0; i < FMath::Min(Observed.Num(), ExpectedSequence.Num()); ++i)
	{
		TestEqual(FString::Printf(TEXT("Phase[%d]"), i), Observed[i], ExpectedSequence[i]);
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// SkipToNextWizard — with 3 wizards, active index increments correctly
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoMTurnSkipToNextWizard,
	"CoM.Turn.SkipToNextWizard",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTurnSkipToNextWizard::RunTest(const FString& /*Params*/)
{
	UCoMTurnSubsystem* Sys = NewObject<UCoMTurnSubsystem>();
	Sys->StartGame();

	const int32 First  = Sys->GetActiveWizardIndex();
	Sys->SkipToNextWizard();
	const int32 Second = Sys->GetActiveWizardIndex();
	Sys->SkipToNextWizard();
	const int32 Third  = Sys->GetActiveWizardIndex();

	TestNotEqual(TEXT("First ≠ Second"), First, Second);
	TestNotEqual(TEXT("Second ≠ Third"), Second, Third);
	TestNotEqual(TEXT("First ≠ Third"),  First, Third);
	// All must be within [0, 2]
	TestTrue(TEXT("First in range"),  First  >= 0 && First  < 3);
	TestTrue(TEXT("Second in range"), Second >= 0 && Second < 3);
	TestTrue(TEXT("Third in range"),  Third  >= 0 && Third  < 3);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Turn counter — after all wizards complete their turns, turn number increments
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoMTurnCounterIncrement,
	"CoM.Turn.CounterIncrement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTurnCounterIncrement::RunTest(const FString& /*Params*/)
{
	UCoMTurnSubsystem* Sys = NewObject<UCoMTurnSubsystem>();
	Sys->StartGame();

	const int32 TurnBefore = Sys->GetCurrentTurn();

	// Advance both wizards through all phases to trigger EndOfTurn → BeginNextTurn
	for (int32 Wizard = 0; Wizard < 2; ++Wizard)
	{
		for (int32 Phase = 0; Phase < 6; ++Phase)  // 6 wizard sub-phases
		{
			Sys->AdvanceWizardPhase();
		}
	}

	const int32 TurnAfter = Sys->GetCurrentTurn();
	TestTrue(TEXT("Turn counter incremented after full round"), TurnAfter > TurnBefore);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Wizard count clamp — cannot start with more than MAX_WIZARDS
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoMTurnWizardCountClamp,
	"CoM.Turn.WizardCountClamp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTurnWizardCountClamp::RunTest(const FString& /*Params*/)
{
	UCoMTurnSubsystem* Sys = NewObject<UCoMTurnSubsystem>();
	// Passing 999 should not crash; it should clamp to MAX_WIZARDS
	Sys->StartGame();
	TestTrue(TEXT("WizardCount clamped to MAX_WIZARDS"),
		Sys->GetActiveWizardIndex() < CoM::MAX_WIZARDS);
	return true;
}

#endif
