#if WITH_AUTOMATION_TESTS
// Copyright Shattered Arcana. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/GameInstance.h"
#include "CoMCore/Units/CoMHeroSubsystem.h"

// Helper: get or create hero subsystem from a transient game instance.

static UCoMHeroSubsystem* GetTestHeroSubsystem()
{
	return NewObject<UCoMHeroSubsystem>();
}

// =====================================================================
// CoM.Hero.InitializePersonality
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMHeroInitPersonalityTest,
	"CoM.Hero.InitializePersonality",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMHeroInitPersonalityTest::RunTest(const FString& Parameters)
{
	UCoMHeroSubsystem* Sub = GetTestHeroSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMHeroSubsystem")); return false; }

	FRandomStream Rng(42);
	Sub->InitializeHeroPersonality(/*HeroUnitID=*/100, Rng);

	const FCoMHeroPersonality P = Sub->GetPersonality(100);

	// Primary and secondary traits must differ.
	TestTrue(TEXT("Primary != Secondary trait"),
		P.PrimaryTrait != P.SecondaryTrait);

	// Loyalty should start at 100.
	TestTrue(TEXT("Initial loyalty is 100"), P.Loyalty == 100);

	// Ambition in [30, 90].
	TestTrue(TEXT("Ambition >= 30"), P.Ambition >= 30);
	TestTrue(TEXT("Ambition <= 90"), P.Ambition <= 90);

	// Getting personality for unknown hero returns a default.
	const FCoMHeroPersonality Default = Sub->GetPersonality(9999);
	TestTrue(TEXT("Default loyalty"), Default.Loyalty == 0);

	return true;
}

// =====================================================================
// CoM.Hero.GetLoyalty
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMHeroGetLoyaltyTest,
	"CoM.Hero.GetLoyalty",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMHeroGetLoyaltyTest::RunTest(const FString& Parameters)
{
	UCoMHeroSubsystem* Sub = GetTestHeroSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMHeroSubsystem")); return false; }

	// Uninitialized hero returns default loyalty (100).
	const FFixed64 Default = Sub->GetLoyalty(999);
	TestTrue(TEXT("Default loyalty is 100"), Default == FFixed64(100));

	// After initialization loyalty should be 100.
	FRandomStream Rng(1);
	Sub->InitializeHeroPersonality(200, Rng);
	TestTrue(TEXT("Initialized loyalty is 100"), Sub->GetLoyalty(200) == FFixed64(100));

	return true;
}

// =====================================================================
// CoM.Hero.ModifyLoyalty
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMHeroModifyLoyaltyTest,
	"CoM.Hero.ModifyLoyalty",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMHeroModifyLoyaltyTest::RunTest(const FString& Parameters)
{
	UCoMHeroSubsystem* Sub = GetTestHeroSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMHeroSubsystem")); return false; }

	FRandomStream Rng(5);
	Sub->InitializeHeroPersonality(300, Rng);

	// Add loyalty — already at 100, so +50 should clamp to 100.
	Sub->ModifyLoyalty(300, FFixed64(50));
	TestTrue(TEXT("Loyalty clamped at 100 after +50"), Sub->GetLoyalty(300) == FFixed64(100));

	// Still clamped at 100 after another +100.
	Sub->ModifyLoyalty(300, FFixed64(100));
	TestTrue(TEXT("Loyalty still clamped at 100"), Sub->GetLoyalty(300) == FFixed64(100));

	// Subtract loyalty.
	Sub->ModifyLoyalty(300, FFixed64(-250));
	TestTrue(TEXT("Loyalty clamped at 0"), Sub->GetLoyalty(300) == FFixed64(0));

	// Modifying unknown hero is a no-op (no crash).
	Sub->ModifyLoyalty(9999, FFixed64(10));

	return true;
}

// =====================================================================
// CoM.Hero.CheckDesertion
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMHeroCheckDesertionTest,
	"CoM.Hero.CheckDesertion",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMHeroCheckDesertionTest::RunTest(const FString& Parameters)
{
	UCoMHeroSubsystem* Sub = GetTestHeroSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMHeroSubsystem")); return false; }

	FRandomStream Rng(10);
	Sub->InitializeHeroPersonality(400, Rng);

	// At loyalty 100, desertion should never trigger.
	const bool HighLoyalty = Sub->CheckDesertion(400);
	TestFalse(TEXT("No desertion at loyalty 100"), HighLoyalty);

	// Drop loyalty to critical levels and run multiple checks.
	Sub->ModifyLoyalty(400, FFixed64(-95)); // loyalty = 5
	// We can't guarantee the roll outcome, but the function should not crash.
	// Run it several times to exercise the path.
	for (int32 i = 0; i < 20; ++i)
	{
		Sub->CheckDesertion(400);
	}

	// Unknown hero returns false.
	TestFalse(TEXT("Unknown hero does not desert"), Sub->CheckDesertion(9999));

	return true;
}

// =====================================================================
// CoM.Hero.TierConfig
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMHeroTierConfigTest,
	"CoM.Hero.TierConfig",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMHeroTierConfigTest::RunTest(const FString& Parameters)
{
	// GetTierConfig is static; no subsystem instance needed but we test it.
	const FCoMHeroTierConfig Adventurer = UCoMHeroSubsystem::GetTierConfig(ECoMHeroTier::Adventurer);
	TestEqual(TEXT("Adventurer tier"), Adventurer.Tier, ECoMHeroTier::Adventurer);
	TestEqual(TEXT("Adventurer FameRequired"), Adventurer.FameRequired, 0);
	TestTrue(TEXT("Adventurer hireable from tavern"), Adventurer.bHirableFromTavern);
	TestFalse(TEXT("Adventurer no quest"), Adventurer.bQuestRequired);
	TestEqual(TEXT("Adventurer MaxPerWizard"), Adventurer.MaxPerWizard, 4);

	const FCoMHeroTierConfig Demigod = UCoMHeroSubsystem::GetTierConfig(ECoMHeroTier::Demigod);
	TestEqual(TEXT("Demigod tier"), Demigod.Tier, ECoMHeroTier::Demigod);
	TestTrue(TEXT("Demigod quest required"), Demigod.bQuestRequired);
	TestFalse(TEXT("Demigod not from tavern"), Demigod.bHirableFromTavern);
	TestEqual(TEXT("Demigod MaxPerWizard"), Demigod.MaxPerWizard, 1);
	TestEqual(TEXT("Demigod FameRequired"), Demigod.FameRequired, 40);

	const FCoMHeroTierConfig Champion = UCoMHeroSubsystem::GetTierConfig(ECoMHeroTier::Champion);
	TestEqual(TEXT("Champion FameRequired"), Champion.FameRequired, 15);
	TestEqual(TEXT("Champion AbilitySlots"), Champion.AbilitySlots, 3);

	return true;
}

// =====================================================================
// CoM.Hero.CanRecruitTier
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMHeroCanRecruitTierTest,
	"CoM.Hero.CanRecruitTier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMHeroCanRecruitTierTest::RunTest(const FString& Parameters)
{
	UCoMHeroSubsystem* Sub = GetTestHeroSubsystem();
	if (!Sub) { AddError(TEXT("Failed to get UCoMHeroSubsystem")); return false; }

	// Wizard 0 with fame 0 can recruit Adventurer.
	TestTrue(TEXT("Can recruit Adventurer at fame 0"),
		Sub->CanRecruitTier(0, ECoMHeroTier::Adventurer, 0));

	// Wizard 0 with fame 0 cannot recruit Hero (requires fame 5).
	TestFalse(TEXT("Cannot recruit Hero at fame 0"),
		Sub->CanRecruitTier(0, ECoMHeroTier::Hero, 0));

	// Wizard 0 with fame 5 can recruit Hero.
	TestTrue(TEXT("Can recruit Hero at fame 5"),
		Sub->CanRecruitTier(0, ECoMHeroTier::Hero, 5));

	// Wizard 0 with fame 50 can recruit Demigod (fame >= 40).
	TestTrue(TEXT("Can recruit Demigod at fame 50"),
		Sub->CanRecruitTier(0, ECoMHeroTier::Demigod, 50));

	// Cannot recruit Demigod at fame 30.
	TestFalse(TEXT("Cannot recruit Demigod at fame 30"),
		Sub->CanRecruitTier(0, ECoMHeroTier::Demigod, 30));

	return true;
}

// =====================================================================
// CoM.Hero.AvailableClasses
// =====================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMHeroAvailableClassesTest,
	"CoM.Hero.AvailableClasses",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMHeroAvailableClassesTest::RunTest(const FString& Parameters)
{
	// GetAvailableClasses is static.
	const TArray<ECoMHeroClass> AdventurerClasses =
		UCoMHeroSubsystem::GetAvailableClasses(ECoMHeroTier::Adventurer, ECoMWizardClass::MAX);

	TestTrue(TEXT("Adventurer has available classes"), AdventurerClasses.Num() > 0);

	// Fighter should be available at Adventurer tier.
	TestTrue(TEXT("Fighter available at Adventurer"),
		AdventurerClasses.Contains(ECoMHeroClass::Fighter));

	// Ascendant should NOT be available at Adventurer tier (Demigod only).
	TestFalse(TEXT("Ascendant not at Adventurer"),
		AdventurerClasses.Contains(ECoMHeroClass::Ascendant));

	// Demigod tier should include Ascendant.
	const TArray<ECoMHeroClass> DemigodClasses =
		UCoMHeroSubsystem::GetAvailableClasses(ECoMHeroTier::Demigod, ECoMWizardClass::MAX);
	TestTrue(TEXT("Ascendant available at Demigod"),
		DemigodClasses.Contains(ECoMHeroClass::Ascendant));

	// DragonKnight should be available at Champion but not Adventurer.
	TestFalse(TEXT("DragonKnight not at Adventurer"),
		AdventurerClasses.Contains(ECoMHeroClass::DragonKnight));

	const TArray<ECoMHeroClass> ChampionClasses =
		UCoMHeroSubsystem::GetAvailableClasses(ECoMHeroTier::Champion, ECoMWizardClass::MAX);
	TestTrue(TEXT("DragonKnight at Champion"),
		ChampionClasses.Contains(ECoMHeroClass::DragonKnight));

	return true;
}

#endif
