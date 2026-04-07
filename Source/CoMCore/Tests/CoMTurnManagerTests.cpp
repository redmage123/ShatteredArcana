// Copyright Shattered Arcana. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CoMTurnManager.h"
#include "CoMFogOfWarSubsystem.h"

// ---------------------------------------------------------------------------
// Shared helper: stand up a TurnManager without a full GameInstance.
// Uses NewObject<> directly (same pattern as CoMLeyPortalSubsystemTests).
// ---------------------------------------------------------------------------
namespace CoMTurnManagerTestHelpers
{
	struct FTestContext
	{
		UCoMTurnManager*       TM  = nullptr;
		UCoMFogOfWarSubsystem* FoW = nullptr;

		FTestContext()
		{
			TM  = NewObject<UCoMTurnManager>();
			FoW = NewObject<UCoMFogOfWarSubsystem>();
		}
	};
}

// ---------------------------------------------------------------------------
// 1. InitializeGame — wizards created, turn starts at 1
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTurnManagerInitTest,
	"CoM.TurnManager.InitializeGame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMTurnManagerInitTest::RunTest(const FString& Parameters)
{
	CoMTurnManagerTestHelpers::FTestContext Ctx;
	Ctx.TM->InitializeGame(2, 2);

	// Turn should start at 1.
	TestEqual(TEXT("Turn starts at 1"), Ctx.TM->GetTurnNumber(), 1);

	// 4 active wizards.
	TestEqual(TEXT("4 active wizards"), Ctx.TM->GetActiveWizardCount(), 4);

	// Current wizard should be 0 (first human).
	TestEqual(TEXT("Current wizard is 0"), Ctx.TM->GetCurrentWizard(), 0);

	// First wizard should be human.
	TestTrue(TEXT("First wizard is human"), Ctx.TM->IsHumanTurn());

	// Game should not be over.
	TestFalse(TEXT("Game is not over"), Ctx.TM->IsGameOver());

	return true;
}

// ---------------------------------------------------------------------------
// 2. EndPlayerTurn — advances to next wizard
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTurnManagerEndTurnTest,
	"CoM.TurnManager.EndPlayerTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMTurnManagerEndTurnTest::RunTest(const FString& Parameters)
{
	CoMTurnManagerTestHelpers::FTestContext Ctx;
	Ctx.TM->InitializeGame(2, 2); // wizards 0,1 = human; 2,3 = AI

	// Wizard 0 is current.
	TestEqual(TEXT("Start: wizard 0"), Ctx.TM->GetCurrentWizard(), 0);

	// End wizard 0's turn — should advance to wizard 1.
	Ctx.TM->EndPlayerTurn();
	TestEqual(TEXT("After first EndTurn: wizard 1"), Ctx.TM->GetCurrentWizard(), 1);

	// End wizard 1's turn — should advance to wizard 2.
	Ctx.TM->EndPlayerTurn();
	TestEqual(TEXT("After second EndTurn: wizard 2"), Ctx.TM->GetCurrentWizard(), 2);

	// Wizard 2 is AI.
	TestFalse(TEXT("Wizard 2 is AI"), Ctx.TM->IsHumanTurn());

	return true;
}

// ---------------------------------------------------------------------------
// 3. ProcessFullTurn — all subsystems called, turn increments
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTurnManagerProcessTest,
	"CoM.TurnManager.ProcessFullTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMTurnManagerProcessTest::RunTest(const FString& Parameters)
{
	CoMTurnManagerTestHelpers::FTestContext Ctx;
	Ctx.TM->InitializeGame(1, 1); // 2 wizards total

	TestEqual(TEXT("Turn starts at 1"), Ctx.TM->GetTurnNumber(), 1);

	// Manually call ProcessFullTurn (normally called when all wizards
	// have ended their input phase).
	Ctx.TM->ProcessFullTurn();

	// Turn should have incremented.
	TestEqual(TEXT("Turn advanced to 2"), Ctx.TM->GetTurnNumber(), 2);

	// Game should still be active (2 wizards remain).
	TestFalse(TEXT("Game not over"), Ctx.TM->IsGameOver());

	// Run another turn.
	Ctx.TM->ProcessFullTurn();
	TestEqual(TEXT("Turn advanced to 3"), Ctx.TM->GetTurnNumber(), 3);

	return true;
}

// ---------------------------------------------------------------------------
// 4. HumanVsAI — human turns require input, AI ordering is correct
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTurnManagerHumanVsAITest,
	"CoM.TurnManager.HumanVsAI",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMTurnManagerHumanVsAITest::RunTest(const FString& Parameters)
{
	CoMTurnManagerTestHelpers::FTestContext Ctx;
	Ctx.TM->InitializeGame(1, 3); // wizard 0 = human, 1-3 = AI

	// First wizard is human.
	TestTrue(TEXT("Wizard 0 is human"), Ctx.TM->IsHumanTurn());
	TestEqual(TEXT("Current wizard is 0"), Ctx.TM->GetCurrentWizard(), 0);

	// After human ends turn, next should be wizard 1 (AI).
	Ctx.TM->EndPlayerTurn();
	TestEqual(TEXT("Next wizard is 1"), Ctx.TM->GetCurrentWizard(), 1);
	TestFalse(TEXT("Wizard 1 is AI"), Ctx.TM->IsHumanTurn());

	// AI wizards 1, 2, 3 end their turns — triggers ProcessFullTurn.
	Ctx.TM->EndPlayerTurn(); // wizard 1 done
	Ctx.TM->EndPlayerTurn(); // wizard 2 done
	// wizard 3 ends — this is the last wizard, so ProcessFullTurn fires.
	const int32 TurnBefore = Ctx.TM->GetTurnNumber();
	Ctx.TM->EndPlayerTurn(); // wizard 3 done => ProcessFullTurn => turn increments.
	TestTrue(TEXT("Turn incremented after all wizards acted"),
		Ctx.TM->GetTurnNumber() > TurnBefore);

	// Back to wizard 0 (human) for the next round.
	TestEqual(TEXT("Back to wizard 0"), Ctx.TM->GetCurrentWizard(), 0);
	TestTrue(TEXT("Wizard 0 is still human"), Ctx.TM->IsHumanTurn());

	return true;
}

// ---------------------------------------------------------------------------
// 5. WizardElimination — eliminated wizard is skipped
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTurnManagerEliminationTest,
	"CoM.TurnManager.WizardElimination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMTurnManagerEliminationTest::RunTest(const FString& Parameters)
{
	CoMTurnManagerTestHelpers::FTestContext Ctx;
	Ctx.TM->InitializeGame(2, 2); // wizards 0,1 = human; 2,3 = AI

	TestEqual(TEXT("4 active wizards"), Ctx.TM->GetActiveWizardCount(), 4);

	// Eliminate wizard 1.
	Ctx.TM->EliminateWizard(1);
	TestEqual(TEXT("3 active wizards after elimination"), Ctx.TM->GetActiveWizardCount(), 3);

	// Wizard 0 ends turn — wizard 1 is eliminated, so next should be wizard 2.
	Ctx.TM->EndPlayerTurn();
	TestEqual(TEXT("Skipped eliminated wizard 1, now wizard 2"),
		Ctx.TM->GetCurrentWizard(), 2);

	// Eliminate wizard 2 and 3.
	Ctx.TM->EliminateWizard(2);
	Ctx.TM->EliminateWizard(3);
	TestEqual(TEXT("1 active wizard"), Ctx.TM->GetActiveWizardCount(), 1);

	// Ending the last remaining wizard's turn should still work
	// and detect that only 1 wizard remains (game over check happens
	// inside ProcessFullTurn).

	return true;
}

// ---------------------------------------------------------------------------
// 6. FogOfWar — basic reveal and query
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMFogOfWarBasicTest,
	"CoM.FogOfWar.RevealAndQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMFogOfWarBasicTest::RunTest(const FString& Parameters)
{
	CoMTurnManagerTestHelpers::FTestContext Ctx;

	const FIntPoint Tile(10, 10);
	const ECoMPlane Plane = ECoMPlane::Aurelith;

	// Initially not visible or explored.
	TestFalse(TEXT("Tile not visible initially"),
		Ctx.FoW->IsTileVisible(0, Plane, Tile));
	TestFalse(TEXT("Tile not explored initially"),
		Ctx.FoW->IsTileExplored(0, Plane, Tile));

	// Reveal the tile for wizard 0.
	Ctx.FoW->RevealTile(0, Plane, Tile);
	TestTrue(TEXT("Tile visible after reveal"),
		Ctx.FoW->IsTileVisible(0, Plane, Tile));
	TestTrue(TEXT("Tile explored after reveal"),
		Ctx.FoW->IsTileExplored(0, Plane, Tile));

	// Wizard 1 should NOT see it.
	TestFalse(TEXT("Wizard 1 cannot see wizard 0's tile"),
		Ctx.FoW->IsTileVisible(1, Plane, Tile));

	// Clear vision — visible goes away, explored stays.
	Ctx.FoW->ClearCurrentVision();
	TestFalse(TEXT("Tile not visible after clear"),
		Ctx.FoW->IsTileVisible(0, Plane, Tile));
	TestTrue(TEXT("Tile still explored after clear"),
		Ctx.FoW->IsTileExplored(0, Plane, Tile));

	return true;
}

// ---------------------------------------------------------------------------
// 7. FogOfWar — RevealArea covers Manhattan diamond
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMFogOfWarAreaTest,
	"CoM.FogOfWar.RevealArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMFogOfWarAreaTest::RunTest(const FString& Parameters)
{
	CoMTurnManagerTestHelpers::FTestContext Ctx;

	const FIntPoint Center(20, 20);
	const ECoMPlane Plane = ECoMPlane::Aurelith;
	const int32 Radius = 2;

	Ctx.FoW->RevealArea(0, Plane, Center, Radius);

	// Center should be visible.
	TestTrue(TEXT("Center visible"), Ctx.FoW->IsTileVisible(0, Plane, Center));

	// Tiles at Manhattan distance exactly 2 should be visible.
	TestTrue(TEXT("(22,20) visible"),
		Ctx.FoW->IsTileVisible(0, Plane, FIntPoint(22, 20)));
	TestTrue(TEXT("(18,20) visible"),
		Ctx.FoW->IsTileVisible(0, Plane, FIntPoint(18, 20)));
	TestTrue(TEXT("(20,22) visible"),
		Ctx.FoW->IsTileVisible(0, Plane, FIntPoint(20, 22)));
	TestTrue(TEXT("(21,21) visible"),
		Ctx.FoW->IsTileVisible(0, Plane, FIntPoint(21, 21)));

	// Tile at Manhattan distance 3 should NOT be visible.
	TestFalse(TEXT("(23,20) not visible"),
		Ctx.FoW->IsTileVisible(0, Plane, FIntPoint(23, 20)));
	TestFalse(TEXT("(22,21) not visible"),
		Ctx.FoW->IsTileVisible(0, Plane, FIntPoint(22, 21)));

	// Count revealed tiles: Manhattan diamond of radius 2 = 1 + 4 + 8 = 13 tiles.
	int32 VisibleCount = 0;
	for (int32 DX = -3; DX <= 3; ++DX)
	{
		for (int32 DY = -3; DY <= 3; ++DY)
		{
			if (Ctx.FoW->IsTileVisible(0, Plane, FIntPoint(Center.X + DX, Center.Y + DY)))
			{
				VisibleCount++;
			}
		}
	}
	TestEqual(TEXT("Manhattan diamond radius 2 = 13 tiles"), VisibleCount, 13);

	return true;
}
