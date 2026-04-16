#if WITH_AUTOMATION_TESTS
// Copyright Mythforge Studios. All Rights Reserved.
// CoMUnitSubsystemTests.cpp — Unit and regression tests
// Phase 7 tests — Shattered Arcana

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/CoreTypes/CoMStructs.h"

namespace CoMUnitTests
{

UCoMUnitSubsystem* CreateTestSubsystem()
{
	UCoMUnitSubsystem* Sub = NewObject<UCoMUnitSubsystem>();
	// Subsystem auto-initializes with world context
	return Sub;
}

// ─────────────────────────────────────────────────────────────────────────────
// ARMY CREATION
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitCreateArmy,
	"CoM.Unit.CreateArmy",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FUnitCreateArmy::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 ArmyID = Sub->CreateArmy(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5));
	TestTrue("Army ID is positive", ArmyID > 0);
	const FCoMArmyGroup* Army = Sub->GetArmy(ArmyID);
	TestNotNull("Army exists", Army);
	TestEqual("Owner wizard", Army->OwnerWizardIndex, 0);
	TestEqual("Position X", Army->Position.X, 5);
	TestEqual("Position Y", Army->Position.Y, 5);
	TestEqual("Empty army has 0 units", Army->UnitIDs.Num(), 0);
	// Second army gets different ID
	int32 ArmyID2 = Sub->CreateArmy(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(10, 10));
	TestTrue("Second army has different ID", ArmyID2 != ArmyID);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UNIT MANAGEMENT
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitAddToArmy,
	"CoM.Unit.AddUnitToArmy",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FUnitAddToArmy::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 ArmyID = Sub->CreateArmy(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5));
	// Spawn units and add to army
	int32 Unit1 = Sub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5), 0);
	int32 Unit2 = Sub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5), 0);
	bool Added1 = Sub->AddUnitToArmy(Unit1, ArmyID);
	bool Added2 = Sub->AddUnitToArmy(Unit2, ArmyID);
	TestTrue("First unit added", Added1);
	TestTrue("Second unit added", Added2);
	const FCoMArmyGroup* Army = Sub->GetArmy(ArmyID);
	TestEqual("Army has 2 units", Army->UnitIDs.Num(), 2);
	// Fill to MAX_ARMY_SIZE (9) and try to add one more
	for (int32 i = 2; i < 9; ++i)
	{
		int32 UnitID = Sub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5), 0);
		Sub->AddUnitToArmy(UnitID, ArmyID);
	}
	Army = Sub->GetArmy(ArmyID);
	TestEqual("Army at max size", Army->UnitIDs.Num(), 9);
	int32 ExtraUnit = Sub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5), 0);
	bool Overflow = Sub->AddUnitToArmy(ExtraUnit, ArmyID);
	TestFalse("Cannot exceed MAX_ARMY_SIZE", Overflow);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// MERGE ARMIES
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitMergeArmies,
	"CoM.Unit.MergeArmies",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FUnitMergeArmies::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 ArmyA = Sub->CreateArmy(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5));
	int32 ArmyB = Sub->CreateArmy(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5));
	// Add 3 units to A and 2 to B
	for (int32 i = 0; i < 3; ++i)
	{
		int32 UnitID = Sub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5), 0);
		Sub->AddUnitToArmy(UnitID, ArmyA);
	}
	for (int32 i = 0; i < 2; ++i)
	{
		int32 UnitID = Sub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5), 0);
		Sub->AddUnitToArmy(UnitID, ArmyB);
	}
	bool Merged = Sub->MergeArmies(ArmyB, ArmyA);
	TestTrue("Merge succeeded", Merged);
	const FCoMArmyGroup* Dest = Sub->GetArmy(ArmyA);
	TestEqual("Destination has 5 units", Dest->UnitIDs.Num(), 5);
	// Source should be destroyed
	TestNull("Source army destroyed", Sub->GetArmy(ArmyB));
	// Merge that would exceed MAX_ARMY_SIZE should fail
	int32 ArmyC = Sub->CreateArmy(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5));
	for (int32 i = 0; i < 5; ++i)
	{
		int32 UnitID = Sub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5), 0);
		Sub->AddUnitToArmy(UnitID, ArmyC);
	}
	bool OverMerge = Sub->MergeArmies(ArmyC, ArmyA);
	TestFalse("Merge rejected when exceeding MAX_ARMY_SIZE", OverMerge);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// SPLIT ARMY
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitSplitArmy,
	"CoM.Unit.SplitArmy",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FUnitSplitArmy::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	int32 ArmyID = Sub->CreateArmy(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5));
	TArray<int32> SpawnedUnits;
	for (int32 i = 0; i < 4; ++i)
	{
		int32 UnitID = Sub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(5, 5), 0);
		Sub->AddUnitToArmy(UnitID, ArmyID);
		SpawnedUnits.Add(UnitID);
	}
	// Split 2 units into a new army
	TArray<int32> ToSplit;
	ToSplit.Add(SpawnedUnits[0]);
	ToSplit.Add(SpawnedUnits[1]);
	int32 NewArmyID = Sub->SplitArmy(ArmyID, ToSplit);
	TestTrue("New army created", NewArmyID != INDEX_NONE);
	const FCoMArmyGroup* Original = Sub->GetArmy(ArmyID);
	const FCoMArmyGroup* NewArmy = Sub->GetArmy(NewArmyID);
	TestEqual("Original has 2 units", Original->UnitIDs.Num(), 2);
	TestEqual("New army has 2 units", NewArmy->UnitIDs.Num(), 2);
	TestEqual("New army at same position", NewArmy->Position, Original->Position);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// MAP WRAP
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitWrapX,
	"CoM.Unit.WrapX",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FUnitWrapX::RunTest(const FString& Parameters)
{
	auto* Sub = CreateTestSubsystem();
	// Create army at right edge and move past it — position should wrap
	int32 ArmyID = Sub->CreateArmy(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(159, 50));
	int32 UnitID = Sub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(159, 50), 0);
	Sub->AddUnitToArmy(UnitID, ArmyID);
	// Move to a destination that wraps around (x = 161 should become 1)
	Sub->MoveArmy(ArmyID, FIntPoint(161, 50));
	Sub->ProcessMovementTurn();
	const FCoMArmyGroup* Army = Sub->GetArmy(ArmyID);
	// After movement processing, position should be wrapped within [0, 160)
	TestTrue("X position is within map bounds", Army->Position.X >= 0 && Army->Position.X < 160);
	return true;
}

} // namespace CoMUnitTests

#endif
