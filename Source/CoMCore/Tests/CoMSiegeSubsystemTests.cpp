#if WITH_AUTOMATION_TESTS
// Copyright Mythforge Studios. All Rights Reserved.
// CoMSiegeSubsystemTests.cpp — Unit and regression tests
// Phase 7 tests — Shattered Arcana

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CoMCore/Combat/CoMSiegeSubsystem.h"

namespace CoMSiegeTests
{

UCoMSiegeSubsystem* CreateTestSubsystem()
{
	UCoMSiegeSubsystem* Sub = NewObject<UCoMSiegeSubsystem>();
	// Subsystem auto-initializes with world context
	return Sub;
}

// ─────────────────────────────────────────────────────────────────────────────
// SIEGE LIFECYCLE
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiegeBeginSiege,
	"CoM.Siege.BeginSiege",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FSiegeBeginSiege::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 SiegeID = Sub->BeginSiege(/*AttackerArmyID=*/1, /*DefenderCityID=*/10);
	TestTrue("Siege ID is positive", SiegeID > 0);
	const FCoMSiegeState* Siege = Sub->GetSiege(SiegeID);
	TestNotNull("Siege state exists", Siege);
	TestEqual("Attacker army ID", Siege->AttackerArmyGroupID, 1);
	TestEqual("Defender city ID", Siege->DefenderCityID, 10);
	TestTrue("City is under siege", Sub->IsCityUnderSiege(10));
	TestTrue("Food reserves initialized", Siege->CityFoodReserves > 0);
	TestTrue("Morale initialized", Siege->CityMorale > 0);
	TestTrue("Wall HP initialized", Siege->WallHP > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiegeBeginSiegeDuplicate,
	"CoM.Siege.BeginSiegeDuplicate",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FSiegeBeginSiegeDuplicate::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 First = Sub->BeginSiege(1, 10);
	TestTrue("First siege created", First > 0);
	int32 Second = Sub->BeginSiege(2, 10);
	TestEqual("Duplicate siege on same city rejected", Second, -1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiegeEndSiege,
	"CoM.Siege.EndSiege",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FSiegeEndSiege::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 SiegeID = Sub->BeginSiege(1, 10);
	TestTrue("City under siege", Sub->IsCityUnderSiege(10));
	Sub->EndSiege(SiegeID, /*bAttackerVictory=*/false);
	TestFalse("City no longer under siege", Sub->IsCityUnderSiege(10));
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// TURN PROCESSING
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiegeProcessTurn,
	"CoM.Siege.ProcessSiegeTurn",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FSiegeProcessTurn::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 SiegeID = Sub->BeginSiege(1, 10);
	const FCoMSiegeState* Siege = Sub->GetSiege(SiegeID);
	int32 InitialFood = Siege->CityFoodReserves;
	// Process a turn — food should decrease
	Sub->ProcessSiegeTurn();
	Siege = Sub->GetSiege(SiegeID);
	if (Siege)
	{
		TestTrue("Food reserves decreased", Siege->CityFoodReserves < InitialFood);
		TestEqual("Turns elapsed incremented", Siege->TurnsUnderSiege, 1);
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// SUPPLY LINES
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiegeCutSupplyLines,
	"CoM.Siege.CutSupplyLines",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FSiegeCutSupplyLines::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 SiegeID = Sub->BeginSiege(1, 10);
	const FCoMSiegeState* Siege = Sub->GetSiege(SiegeID);
	TestFalse("Supply lines intact initially", Siege->bSupplyLinesCut);
	Sub->CutSupplyLines(SiegeID);
	Siege = Sub->GetSiege(SiegeID);
	TestTrue("Supply lines are cut", Siege->bSupplyLinesCut);
	// With cut supply lines, starvation should be faster
	int32 FoodBefore = Siege->CityFoodReserves;
	Sub->ProcessSiegeTurn();
	Siege = Sub->GetSiege(SiegeID);
	if (Siege)
	{
		int32 FoodLost = FoodBefore - Siege->CityFoodReserves;
		TestTrue("Food loss is accelerated", FoodLost >= 3);
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// AUTO-SURRENDER
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiegeAutoSurrender,
	"CoM.Siege.AutoSurrender",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FSiegeAutoSurrender::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 SiegeID = Sub->BeginSiege(1, 10);
	Sub->CutSupplyLines(SiegeID);
	// Process many turns to exhaust food and morale
	for (int32 Turn = 0; Turn < 100; ++Turn)
	{
		Sub->ProcessSiegeTurn();
		if (!Sub->IsCityUnderSiege(10))
		{
			break;
		}
	}
	// City should have surrendered or siege ended due to starvation
	TestFalse("Siege ended via auto-surrender or starvation", Sub->IsCityUnderSiege(10));
	return true;
}

} // namespace CoMSiegeTests

#endif
