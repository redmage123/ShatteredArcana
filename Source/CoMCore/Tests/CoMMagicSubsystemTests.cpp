// Copyright Mythforge Studios. All Rights Reserved.
// CoMMagicSubsystemTests.cpp — Unit and regression tests
// Phase 7 tests — Shattered Arcana

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"

#if WITH_AUTOMATION_TESTS

namespace CoMMagicTests
{

UCoMMagicSubsystem* CreateTestSubsystem()
{
    UCoMMagicSubsystem* Sub = NewObject<UCoMMagicSubsystem>();
    FSubsystemCollectionBase DummyCollection;
    Sub->Initialize(DummyCollection);
    return Sub;
}

// ─────────────────────────────────────────────────────────────────────────────
// MANA STATE
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicInitialState,
    "CoM.Magic.InitialState",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicInitialState::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    TestEqual("Default mana", State.CurrentMana, 0);
    TestEqual("Default max mana", State.MaxMana, 100);
    TestEqual("Default mana per turn", State.ManaPerTurn, 10);
    TestEqual("No known spells", State.KnownSpells.Num(), 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// MANA NODES
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicClaimNode,
    "CoM.Magic.ClaimManaNode",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicClaimNode::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FIntPoint Node(10, 10);
    bool Claimed = Sub->ClaimManaNode(0, Node);
    TestTrue("Node claimed", Claimed);
    // Can't claim same node twice
    bool DupClaim = Sub->ClaimManaNode(0, Node);
    TestFalse("Duplicate claim rejected", DupClaim);
    // Other wizard can't claim it
    bool OtherClaim = Sub->ClaimManaNode(1, Node);
    TestFalse("Other wizard claim rejected", OtherClaim);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicReleaseNode,
    "CoM.Magic.ReleaseManaNode",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicReleaseNode::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FIntPoint Node(10, 10);
    Sub->ClaimManaNode(0, Node);
    Sub->ReleaseManaNode(0, Node);
    // Now another wizard can claim it
    bool Claimed = Sub->ClaimManaNode(1, Node);
    TestTrue("Node available after release", Claimed);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// SPELL CASTING
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicCanCastSpell,
    "CoM.Magic.CanCastSpell",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicCanCastSpell::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    State.CurrentMana = 50;
    FName SpellId = FName(TEXT("Fireball"));
    // Can't cast unknown spell
    FCoMSpellCast Cast;
    Cast.SpellId = SpellId;
    Cast.CasterWizardId = 0;
    Cast.ManaCost = 20;
    TestFalse("Can't cast unknown spell", Sub->CanCastSpell(0, SpellId, Cast));
    // Learn spell
    State.KnownSpells.Add(SpellId);
    TestTrue("Can cast known spell with enough mana", Sub->CanCastSpell(0, SpellId, Cast));
    // Not enough mana
    Cast.ManaCost = 100;
    TestFalse("Can't cast without enough mana", Sub->CanCastSpell(0, SpellId, Cast));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicCastSpellInstant,
    "CoM.Magic.CastSpellInstant",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicCastSpellInstant::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    State.CurrentMana = 50;
    FName SpellId = FName(TEXT("Fireball"));
    State.KnownSpells.Add(SpellId);
    FCoMSpellCast Cast;
    Cast.SpellId = SpellId;
    Cast.CasterWizardId = 0;
    Cast.ManaCost = 20;
    Cast.CastingTime = 1; // Instant
    bool Success = Sub->CastSpell(Cast);
    TestTrue("Spell cast", Success);
    TestEqual("Mana deducted", State.CurrentMana, 30);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicCancelCasting,
    "CoM.Magic.CancelCasting",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicCancelCasting::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    State.CurrentMana = 100;
    FName SpellId = FName(TEXT("Earthquake"));
    State.KnownSpells.Add(SpellId);
    FCoMSpellCast Cast;
    Cast.SpellId = SpellId;
    Cast.CasterWizardId = 0;
    Cast.ManaCost = 40;
    Cast.CastingTime = 3;
    Cast.Scope = ECoMSpellScope::Overworld;
    Sub->CastSpell(Cast);
    TestEqual("Mana after cast", State.CurrentMana, 60);
    Sub->CancelCasting(0);
    TestEqual("50% refund", State.CurrentMana, 80); // 60 + 40/2
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// RESEARCH
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicResearch,
    "CoM.Magic.StartResearch",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicResearch::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FName SpellId = FName(TEXT("Lightning"));
    bool Started = Sub->StartResearch(0, SpellId);
    TestTrue("Research started", Started);
    TestEqual("Progress at 0", Sub->GetResearchProgress(0), 0);
    // Can't research already known spell
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    State.KnownSpells.Add(SpellId);
    bool Dup = Sub->StartResearch(0, SpellId);
    TestFalse("Can't research known spell", Dup);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// RITUALS
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicRitual,
    "CoM.Magic.BeginRitual",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicRitual::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FName RitualId = FName(TEXT("SummonDragon"));
    bool Started = Sub->BeginRitual(0, RitualId, 20);
    TestTrue("Ritual started", Started);
    TArray<FCoMActiveRitual> Rituals = Sub->GetActiveRituals(0);
    TestEqual("One active ritual", Rituals.Num(), 1);
    Sub->CancelRitual(0, RitualId);
    Rituals = Sub->GetActiveRituals(0);
    TestEqual("No rituals after cancel", Rituals.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicInterruptRitual,
    "CoM.Magic.InterruptRitual",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicInterruptRitual::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FName RitualId = FName(TEXT("OpenPortal"));
    Sub->BeginRitual(0, RitualId, 10);
    bool Interrupted = Sub->InterruptRitual(0, RitualId);
    TestTrue("Ritual interrupted", Interrupted);
    TestEqual("No active rituals", Sub->GetActiveRituals(0).Num(), 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// RUNES — including regression tests
// ─────────────────────────────────────────────────────────────────────────────

// REGRESSION: InscribeRune should not allow mana to go negative
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicInscribeRuneNoUnderflow,
    "CoM.Magic.InscribeRuneNoUnderflow",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicInscribeRuneNoUnderflow::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    State.CurrentMana = 10; // Less than inscription cost
    FName RuneId = FName(TEXT("FireRune"));
    bool Inscribed = Sub->InscribeRune(0, RuneId, ECoMRuneTarget::Weapon, 1);
    TestFalse("Cannot inscribe with insufficient mana", Inscribed);
    TestTrue("Mana did not go negative", State.CurrentMana >= 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicInscribeRuneSuccess,
    "CoM.Magic.InscribeRune",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicInscribeRuneSuccess::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    State.CurrentMana = 50;
    FName RuneId = FName(TEXT("IceRune"));
    bool Inscribed = Sub->InscribeRune(0, RuneId, ECoMRuneTarget::Armor, 1);
    TestTrue("Rune inscribed", Inscribed);
    TestEqual("Mana deducted", State.CurrentMana, 30);
    TArray<FCoMActiveRune> Runes = Sub->GetRunesOnTarget(ECoMRuneTarget::Armor, 1);
    TestEqual("One rune on target", Runes.Num(), 1);
    return true;
}

// REGRESSION: RemoveRune should actually remove the rune
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicRemoveRune,
    "CoM.Magic.RemoveRune",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicRemoveRune::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    State.CurrentMana = 50;
    FName RuneId = FName(TEXT("LightningRune"));
    Sub->InscribeRune(0, RuneId, ECoMRuneTarget::Weapon, 5);
    TArray<FCoMActiveRune> Runes = Sub->GetRunesOnTarget(ECoMRuneTarget::Weapon, 5);
    TestEqual("Rune exists", Runes.Num(), 1);
    // Get the instance ID from the rune
    int32 InstanceId = Runes[0].InstanceId;
    TestTrue("Instance ID is valid", InstanceId > 0);
    Sub->RemoveRune(InstanceId);
    Runes = Sub->GetRunesOnTarget(ECoMRuneTarget::Weapon, 5);
    TestEqual("Rune removed", Runes.Num(), 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ENCHANTMENTS
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicEnchantment,
    "CoM.Magic.MaintainEnchantment",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicEnchantment::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    FName SpellId = FName(TEXT("ProtectionAura"));
    State.KnownSpells.Add(SpellId);
    bool Maintained = Sub->MaintainEnchantment(0, SpellId);
    TestTrue("Enchantment maintained", Maintained);
    TestEqual("One active enchantment", State.ActiveEnchantments.Num(), 1);
    // Can't maintain duplicate
    bool Dup = Sub->MaintainEnchantment(0, SpellId);
    TestFalse("Duplicate rejected", Dup);
    Sub->DismissEnchantment(0, SpellId);
    TestEqual("Enchantment dismissed", State.ActiveEnchantments.Num(), 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// TURN PROCESSING — including regression
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicProcessTurn,
    "CoM.Magic.ProcessTurn",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicProcessTurn::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    State.CurrentMana = 0;
    State.MaxMana = 200;
    Sub->ClaimManaNode(0, FIntPoint(5, 5));
    Sub->ProcessTurn(1);
    TestTrue("Mana increased from income", State.CurrentMana > 0);
    return true;
}

// REGRESSION: ResearchSpend should never go negative
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicProcessTurnNegativeResearch,
    "CoM.Magic.ProcessTurnNegativeResearch",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicProcessTurnNegativeResearch::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMWizardMagicState& State = Sub->GetWizardMagic(0);
    State.CurrentMana = 50;
    State.MaxMana = 200;
    State.ResearchAllocation = 10;
    // Create heavy enchantment maintenance that exceeds income
    for (int32 i = 0; i < 20; ++i)
    {
        FName SpellId = FName(*FString::Printf(TEXT("Spell_%d"), i));
        State.KnownSpells.Add(SpellId);
        State.ActiveEnchantments.Add(SpellId);
    }
    FName ResearchSpell = FName(TEXT("Research"));
    Sub->StartResearch(0, ResearchSpell);
    int32 ProgressBefore = State.ResearchProgress;
    Sub->ProcessTurn(1);
    // Research progress should NOT decrease
    TestTrue("Research progress did not go negative",
        State.ResearchProgress >= ProgressBefore);
    // Mana should not be more than it was + income
    TestTrue("Mana is non-negative", State.CurrentMana >= 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// COUNTER-MAGIC
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagicCounterSpell,
    "CoM.Magic.CounterSpell",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMagicCounterSpell::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    // Wizard 1 is casting
    FCoMWizardMagicState& State1 = Sub->GetWizardMagic(1);
    State1.CurrentMana = 100;
    State1.CastingSkill = 20;
    FName SpellId = FName(TEXT("Doom"));
    State1.KnownSpells.Add(SpellId);
    FCoMSpellCast Cast;
    Cast.SpellId = SpellId;
    Cast.CasterWizardId = 1;
    Cast.ManaCost = 30;
    Cast.CastingTime = 3;
    Cast.Scope = ECoMSpellScope::Overworld;
    Sub->CastSpell(Cast);
    // Wizard 0 tries to counter with massive mana
    FCoMWizardMagicState& State0 = Sub->GetWizardMagic(0);
    State0.CurrentMana = 200;
    State0.CastingSkill = 80;
    bool Countered = Sub->CounterSpell(0, 1, 100);
    // With 200 mana + 80 skill vs 30 mana spell + 20 skill, counter should succeed
    // (though there's randomness involved)
    TestTrue("Mana was spent on counter attempt", State0.CurrentMana < 200);
    return true;
}

} // namespace CoMMagicTests

#endif // WITH_AUTOMATION_TESTS
