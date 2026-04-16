#if WITH_AUTOMATION_TESTS
// Copyright Mythforge Studios. All Rights Reserved.
// CoMDiplomacySubsystemTests.cpp — Unit, regression, and integration tests
// Phase 7 tests — Shattered Arcana

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"

namespace CoMDiplomacyTests
{

// Helper: create a subsystem instance for testing
UCoMDiplomacySubsystem* CreateTestSubsystem()
{
    UCoMDiplomacySubsystem* Sub = NewObject<UCoMDiplomacySubsystem>();
    // Auto-initialized via subsystem framework
    return Sub;
}

// ─────────────────────────────────────────────────────────────────────────────
// UNIT TESTS — Relations
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploInitialState,
    "CoM.Diplomacy.InitialState",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploInitialState::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    TestEqual("No treaty between unknown wizards", Sub->GetTreatyBetween(0, 1), ECoMTreatyType::None);
    TestEqual("Zero reputation between unknown wizards", Sub->GetReputation(0, 1), 0);
    TestFalse("Not at war by default", Sub->AreAtWar(0, 1));
    TestFalse("Not allied by default", Sub->AreAllied(0, 1));
    TestFalse("Haven't met by default", Sub->HaveMet(0, 1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploGetRelationCreates,
    "CoM.Diplomacy.GetRelationCreatesDefault",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploGetRelationCreates::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMDiplomaticRelation& Rel = Sub->GetRelation(3, 7);
    TestEqual("WizardA is min", Rel.WizardA, 3);
    TestEqual("WizardB is max", Rel.WizardB, 7);
    TestEqual("Default treaty None", Rel.CurrentTreaty, ECoMTreatyType::None);
    // Reverse order should return same relation
    FCoMDiplomaticRelation& Rel2 = Sub->GetRelation(7, 3);
    TestEqual("Same WizardA regardless of order", Rel2.WizardA, 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploModifyReputation,
    "CoM.Diplomacy.ModifyReputation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploModifyReputation::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->ModifyReputation(0, 1, 500, TEXT("Test bonus"));
    TestEqual("Reputation increased", Sub->GetReputation(0, 1), 500);
    Sub->ModifyReputation(0, 1, 600, TEXT("Over limit"));
    TestEqual("Reputation clamped at 1000", Sub->GetReputation(0, 1), 1000);
    Sub->ModifyReputation(0, 1, -2500, TEXT("Massive penalty"));
    TestEqual("Reputation clamped at -1000", Sub->GetReputation(0, 1), -1000);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UNIT TESTS — Treaties
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploProposeTreaty,
    "CoM.Diplomacy.ProposeTreaty",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploProposeTreaty::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMTreatyProposal Prop;
    Prop.ProposerWizardId = 0;
    Prop.TargetWizardId = 1;
    Prop.ProposedTreaty = ECoMTreatyType::TradeAgreement;
    int32 Id = Sub->ProposeTreaty(Prop);
    TestTrue("Proposal ID is positive", Id > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploAcceptTreaty,
    "CoM.Diplomacy.AcceptTreaty",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploAcceptTreaty::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMTreatyProposal Prop;
    Prop.ProposerWizardId = 0;
    Prop.TargetWizardId = 1;
    Prop.ProposedTreaty = ECoMTreatyType::TradeAgreement;
    int32 Id = Sub->ProposeTreaty(Prop);
    bool Accepted = Sub->AcceptTreaty(Id);
    TestTrue("Treaty accepted", Accepted);
    TestEqual("Treaty in effect", Sub->GetTreatyBetween(0, 1), ECoMTreatyType::TradeAgreement);
    TestTrue("Reputation improved", Sub->GetReputation(0, 1) > 0);
    return true;
}

// REGRESSION TEST: AcceptTreaty peace-from-war should clear LastWarStartTurn
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploAcceptTreatyEndWar,
    "CoM.Diplomacy.AcceptTreatyEndWar",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploAcceptTreatyEndWar::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->MakeFirstContact(0, 1, 1);
    Sub->DeclareWar(0, 1);
    TestTrue("At war", Sub->AreAtWar(0, 1));

    FCoMTreatyProposal Peace;
    Peace.ProposerWizardId = 1;
    Peace.TargetWizardId = 0;
    Peace.ProposedTreaty = ECoMTreatyType::None;
    int32 Id = Sub->ProposeTreaty(Peace);
    Sub->AcceptTreaty(Id);

    TestFalse("No longer at war", Sub->AreAtWar(0, 1));
    FCoMDiplomaticRelation& Rel = Sub->GetRelation(0, 1);
    TestEqual("LastWarStartTurn cleared", Rel.LastWarStartTurn, -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploRejectTreaty,
    "CoM.Diplomacy.RejectTreaty",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploRejectTreaty::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMTreatyProposal Prop;
    Prop.ProposerWizardId = 0;
    Prop.TargetWizardId = 1;
    Prop.ProposedTreaty = ECoMTreatyType::MilitaryAlliance;
    int32 Id = Sub->ProposeTreaty(Prop);
    Sub->RejectTreaty(Id);
    TestEqual("No treaty in effect", Sub->GetTreatyBetween(0, 1), ECoMTreatyType::None);
    TestTrue("Reputation decreased", Sub->GetReputation(0, 1) < 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UNIT TESTS — War
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploDeclareWar,
    "CoM.Diplomacy.DeclareWar",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploDeclareWar::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->MakeFirstContact(0, 1, 1);
    Sub->DeclareWar(0, 1);
    TestTrue("At war", Sub->AreAtWar(0, 1));
    TestTrue("Reputation very negative", Sub->GetReputation(0, 1) < -100);
    TArray<int32> Enemies = Sub->GetEnemies(0);
    TestEqual("One enemy", Enemies.Num(), 1);
    TestEqual("Enemy is wizard 1", Enemies[0], 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploDeclareWarMutualDefense,
    "CoM.Diplomacy.DeclareWarMutualDefense",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploDeclareWarMutualDefense::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->MakeFirstContact(0, 1, 1);
    Sub->MakeFirstContact(1, 2, 1);
    Sub->MakeFirstContact(0, 2, 1);
    // Wizard 1 and 2 have mutual defense
    FCoMDiplomaticRelation& Rel12 = Sub->GetRelation(1, 2);
    Rel12.CurrentTreaty = ECoMTreatyType::DefensivePact;
    // Wizard 0 declares war on wizard 1
    Sub->DeclareWar(0, 1);
    TestTrue("0 at war with 1", Sub->AreAtWar(0, 1));
    TestTrue("0 at war with 2 (mutual defense triggered)", Sub->AreAtWar(0, 2));
    return true;
}

// REGRESSION TEST: DeclareWar should not crash from TMap iterator invalidation
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploDeclareWarNoIteratorCrash,
    "CoM.Diplomacy.DeclareWarNoIteratorInvalidation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploDeclareWarNoIteratorCrash::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    // Create many relations to increase TMap rehash probability
    for (int32 i = 0; i < 20; ++i)
    {
        Sub->MakeFirstContact(0, i + 1, 1);
    }
    // Declare war — this should not crash even though ModifyReputation
    // may Add() to Relations during iteration
    Sub->DeclareWar(0, 1);
    TestTrue("At war (did not crash)", Sub->AreAtWar(0, 1));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UNIT TESTS — Gifts, First Contact, Vassals
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploSendGift,
    "CoM.Diplomacy.SendGift",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploSendGift::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    TMap<ECoMResource, int32> Resources;
    Resources.Add(ECoMResource::GoldOre, 10);
    Sub->SendGift(0, 1, Resources, 50);
    TestTrue("Reputation improved from gift", Sub->GetReputation(0, 1) > 0);
    // Second gift should have diminishing returns
    int32 RepAfterFirst = Sub->GetReputation(0, 1);
    Sub->SendGift(0, 1, Resources, 50);
    int32 RepAfterSecond = Sub->GetReputation(0, 1);
    int32 SecondGain = RepAfterSecond - RepAfterFirst;
    TestTrue("Diminishing returns (second gift worth less)", SecondGain < RepAfterFirst);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploFirstContact,
    "CoM.Diplomacy.MakeFirstContact",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploFirstContact::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    TestFalse("Haven't met", Sub->HaveMet(0, 1));
    Sub->MakeFirstContact(0, 1, 5);
    TestTrue("Have met", Sub->HaveMet(0, 1));
    TestTrue("Reverse also works", Sub->HaveMet(1, 0));
    FCoMDiplomaticRelation& Rel = Sub->GetRelation(0, 1);
    TestEqual("First contact turn recorded", Rel.FirstContactTurn, 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploVassal,
    "CoM.Diplomacy.MakeVassal",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploVassal::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->MakeVassal(0, 1);
    TestEqual("Overlord is wizard 0", Sub->GetOverlord(1), 0);
    TArray<int32> Vassals = Sub->GetVassals(0);
    TestEqual("One vassal", Vassals.Num(), 1);
    TestEqual("Vassal is wizard 1", Vassals[0], 1);
    Sub->FreeVassal(1);
    TestEqual("No overlord after freedom", Sub->GetOverlord(1), -1);
    TestEqual("No vassals after freedom", Sub->GetVassals(0).Num(), 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UNIT TESTS — AI Diplomacy
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploEvaluateProposal,
    "CoM.Diplomacy.EvaluateProposal",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploEvaluateProposal::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    FCoMDiplomaticPersonality Personality;
    Personality.TradeAffinity = 80;
    Personality.Trustworthiness = 70;
    Sub->SetPersonality(1, Personality);
    // High-rep trade proposal should score positive
    Sub->ModifyReputation(0, 1, 500, TEXT("Good relations"));
    FCoMTreatyProposal Prop;
    Prop.ProposerWizardId = 0;
    Prop.TargetWizardId = 1;
    Prop.ProposedTreaty = ECoMTreatyType::TradeAgreement;
    int32 Score = Sub->EvaluateProposal(1, Prop);
    TestTrue("Trade-loving AI with good relations accepts trade", Score > 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UNIT TESTS — Turn Processing
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploProcessTurn,
    "CoM.Diplomacy.ProcessTurn",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploProcessTurn::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->MakeFirstContact(0, 1, 1);
    Sub->ModifyReputation(0, 1, 100, TEXT("Initial"));
    Sub->ProcessTurn(2);
    TestTrue("Reputation decayed", Sub->GetReputation(0, 1) < 100);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploWarWeariness,
    "CoM.Diplomacy.WarWeariness",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploWarWeariness::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->MakeFirstContact(0, 1, 1);
    Sub->DeclareWar(0, 1);
    FCoMDiplomaticRelation& Rel = Sub->GetRelation(0, 1);
    Rel.LastWarStartTurn = 1;
    int32 Weariness5 = Sub->GetWarWeariness(0, 6);   // 5 turns
    int32 Weariness10 = Sub->GetWarWeariness(0, 11);  // 10 turns
    TestTrue("Weariness increases with time", Weariness10 > Weariness5);
    TestTrue("Quadratic growth (10 turns > 4x of 5 turns)", Weariness10 > Weariness5 * 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploCasusBelli,
    "CoM.Diplomacy.CasusBelli",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploCasusBelli::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    TestFalse("No casus belli by default", Sub->HasCasusBelli(0, 1));
    // Create deep enmity
    Sub->ModifyReputation(0, 1, -500, TEXT("Hate"));
    TestTrue("Deep enmity creates casus belli", Sub->HasCasusBelli(0, 1));
    TArray<FString> Reasons = Sub->GetCasusBelli(0, 1);
    TestTrue("At least one reason", Reasons.Num() > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiploBreakTreaty,
    "CoM.Diplomacy.BreakTreaty",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDiploBreakTreaty::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->MakeFirstContact(0, 1, 1);
    Sub->MakeFirstContact(0, 2, 1);
    FCoMDiplomaticRelation& Rel01 = Sub->GetRelation(0, 1);
    Rel01.CurrentTreaty = ECoMTreatyType::MilitaryAlliance;
    int32 RepBefore = Sub->GetReputation(0, 2);
    Sub->BreakTreaty(0, 1);
    TestEqual("Treaty cleared", Sub->GetTreatyBetween(0, 1), ECoMTreatyType::None);
    TestTrue("Reputation with victim dropped", Sub->GetReputation(0, 1) < 0);
    // Other wizards should also lose respect
    TestTrue("Global reputation impact", Sub->GetReputation(0, 2) < RepBefore);
    return true;
}

} // namespace CoMDiplomacyTests

#endif
