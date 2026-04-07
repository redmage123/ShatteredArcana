// Copyright Shattered Arcana. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/GameInstance.h"
#include "CoMCore/Units/CoMDragonSubsystem.h"

// Helper: get or create dragon subsystem from a transient game instance.
static UCoMDragonSubsystem* GetTestDragonSubsystem()
{
	UGameInstance* GI = NewObject<UGameInstance>();
	return GI->GetSubsystem<UCoMDragonSubsystem>();
}

// =====================================================================
// CoM.Dragon.SpawnDragon
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMDragonSpawnTest,
	"CoM.Dragon.SpawnDragon",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMDragonSpawnTest::RunTest(const FString& Parameters)
{
	UCoMDragonSubsystem* Sub = GetTestDragonSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMDragonSubsystem")); return false; }

	FRandomStream Rng(42);
	const int32 DragonID = Sub->SpawnDragon(/*TypeID=*/1, ECoMPlane::Arcanus, FIntPoint(10, 20), Rng);

	TestTrue(TEXT("DragonID should be positive"), DragonID > 0);

	const FCoMDragonInstance* Dragon = Sub->GetDragon(DragonID);
	TestNotNull(TEXT("Dragon instance should exist"), Dragon);

	if (Dragon)
	{
		TestEqual(TEXT("TypeID"), Dragon->TypeID, 1);
		TestEqual(TEXT("LairPosition"), Dragon->LairPosition, FIntPoint(10, 20));
		TestEqual(TEXT("Role should be Wild"), Dragon->Role, ECoMDragonRole::Wild);
		TestEqual(TEXT("Age should be 0"), Dragon->Age, 0);
	}

	// Second spawn gets a different ID.
	const int32 DragonID2 = Sub->SpawnDragon(2, ECoMPlane::Myrror, FIntPoint(5, 5), Rng);
	TestTrue(TEXT("Second DragonID should differ"), DragonID2 != DragonID);

	return true;
}

// =====================================================================
// CoM.Dragon.CreateDomain (regression: uses dragon's lair position)
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMDragonCreateDomainTest,
	"CoM.Dragon.CreateDomain",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMDragonCreateDomainTest::RunTest(const FString& Parameters)
{
	UCoMDragonSubsystem* Sub = GetTestDragonSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMDragonSubsystem")); return false; }

	FRandomStream Rng(99);
	const int32 DragonID = Sub->SpawnDragon(1, ECoMPlane::Arcanus, FIntPoint(15, 25), Rng);
	TestTrue(TEXT("Dragon spawned"), DragonID > 0);

	Sub->CreateDragonDomain(DragonID, /*InfluenceRadius=*/3);

	// Verify dragon is now a DomainRuler.
	const FCoMDragonInstance* Dragon = Sub->GetDragon(DragonID);
	TestNotNull(TEXT("Dragon exists"), Dragon);
	if (Dragon)
	{
		TestEqual(TEXT("Role promoted to DomainRuler"), Dragon->Role, ECoMDragonRole::DomainRuler);
		TestTrue(TEXT("DomainID assigned"), Dragon->DomainID > 0);

		// Regression: domain lair position must equal dragon's lair position.
		const FCoMDragonDomain* Domain = Sub->GetDomain(Dragon->DomainID);
		TestNotNull(TEXT("Domain exists"), Domain);
		if (Domain)
		{
			TestEqual(TEXT("Domain LairPosition matches dragon"), Domain->LairPosition, Dragon->LairPosition);
			TestEqual(TEXT("InfluenceRadius"), Domain->InfluenceRadius, 3);
			TestTrue(TEXT("ClaimedTiles non-empty"), Domain->ClaimedTiles.Num() > 0);
		}
	}

	// Creating a domain for a non-existent dragon should not crash.
	Sub->CreateDragonDomain(/*DragonID=*/9999, 5);

	return true;
}

// =====================================================================
// CoM.Dragon.LayEgg
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMDragonLayEggTest,
	"CoM.Dragon.LayEgg",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMDragonLayEggTest::RunTest(const FString& Parameters)
{
	UCoMDragonSubsystem* Sub = GetTestDragonSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMDragonSubsystem")); return false; }

	FRandomStream Rng(7);
	const int32 DragonID = Sub->SpawnDragon(3, ECoMPlane::Arcanus, FIntPoint(0, 0), Rng);

	const int32 EggID = Sub->LayDragonEgg(DragonID, /*PossessorWizard=*/0);
	TestTrue(TEXT("EggID should be positive"), EggID > 0);

	// Laying an egg for a non-existent parent returns -1.
	const int32 BadEgg = Sub->LayDragonEgg(/*ParentDragonID=*/9999, 0);
	TestEqual(TEXT("Invalid parent returns -1"), BadEgg, -1);

	return true;
}

// =====================================================================
// CoM.Dragon.HatchEgg
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMDragonHatchEggTest,
	"CoM.Dragon.HatchEgg",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMDragonHatchEggTest::RunTest(const FString& Parameters)
{
	UCoMDragonSubsystem* Sub = GetTestDragonSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMDragonSubsystem")); return false; }

	FRandomStream Rng(13);
	const int32 DragonID = Sub->SpawnDragon(2, ECoMPlane::Arcanus, FIntPoint(0, 0), Rng);
	const int32 EggID = Sub->LayDragonEgg(DragonID, 0);

	// Hatching too early should fail (< 10 turns incubation).
	const int32 TooEarly = Sub->HatchEgg(EggID);
	TestEqual(TEXT("Hatch too early returns -1"), TooEarly, -1);

	// Simulate 10 turns of incubation via ProcessDragonTurn.
	for (int32 i = 0; i < 10; ++i)
	{
		Sub->ProcessDragonTurn();
	}

	// Now hatching should succeed.
	const int32 HatchlingID = Sub->HatchEgg(EggID);
	TestTrue(TEXT("Hatchling ID should be positive"), HatchlingID > 0);

	const FCoMDragonInstance* Hatchling = Sub->GetDragon(HatchlingID);
	TestNotNull(TEXT("Hatchling instance exists"), Hatchling);

	// Hatching the same egg again should fail (egg removed).
	const int32 DoubleHatch = Sub->HatchEgg(EggID);
	TestEqual(TEXT("Double hatch returns -1"), DoubleHatch, -1);

	return true;
}

// =====================================================================
// CoM.Dragon.ProcessDragonTurn
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMDragonProcessTurnTest,
	"CoM.Dragon.ProcessDragonTurn",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMDragonProcessTurnTest::RunTest(const FString& Parameters)
{
	UCoMDragonSubsystem* Sub = GetTestDragonSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMDragonSubsystem")); return false; }

	FRandomStream Rng(77);
	const int32 DragonID = Sub->SpawnDragon(1, ECoMPlane::Arcanus, FIntPoint(5, 5), Rng);
	Sub->CreateDragonDomain(DragonID, /*InfluenceRadius=*/2);

	const FCoMDragonInstance* Dragon = Sub->GetDragon(DragonID);
	TestNotNull(TEXT("Dragon exists"), Dragon);

	const int32 InitialAge = Dragon ? Dragon->Age : -1;

	// Run one turn: dragon ages by 1.
	Sub->ProcessDragonTurn();
	Dragon = Sub->GetDragon(DragonID);
	if (Dragon)
	{
		TestEqual(TEXT("Age incremented by 1"), Dragon->Age, InitialAge + 1);
	}

	// Run turns up to 10 total to trigger domain expansion.
	for (int32 i = 1; i < 10; ++i)
	{
		Sub->ProcessDragonTurn();
	}

	Dragon = Sub->GetDragon(DragonID);
	if (Dragon)
	{
		TestEqual(TEXT("Age after 10 turns"), Dragon->Age, 10);

		const FCoMDragonDomain* Domain = Sub->GetDomain(Dragon->DomainID);
		TestNotNull(TEXT("Domain exists after turns"), Domain);
		if (Domain)
		{
			// Domain should have expanded once at age 10 (radius 2 -> 3).
			TestEqual(TEXT("Domain radius expanded"), Domain->InfluenceRadius, 3);
		}
	}

	return true;
}
