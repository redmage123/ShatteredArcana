#if WITH_AUTOMATION_TESTS
// Copyright Mythforge Studios. All Rights Reserved.
// CoMIntegrationTests.cpp — Cross-subsystem integration and E2E tests
// Phase 6+7 — Shattered Arcana

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"
#include "CoMCore/Espionage/CoMEspionageSubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"

namespace CoMIntegrationTests
{

// Helper: create all Phase 7 subsystems
struct FPhase7World
{
    UCoMDiplomacySubsystem* Diplomacy;
    UCoMEspionageSubsystem* Espionage;
    UCoMMagicSubsystem* Magic;

    FPhase7World()
    {
        // Create subsystems directly — no game instance wiring needed for unit tests.
        Diplomacy = NewObject<UCoMDiplomacySubsystem>();
        Espionage = NewObject<UCoMEspionageSubsystem>();
        Magic = NewObject<UCoMMagicSubsystem>();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// INTEGRATION: War declaration → mutual defense cascade
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntWarCascade,
    "CoM.Integration.WarDeclarationMutualDefenseCascade",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FIntWarCascade::RunTest(const FString& Parameters)
{
    FPhase7World World;
    // Setup: 4 wizards, 1-2 allied, 2-3 allied
    for (int32 i = 0; i < 4; ++i)
        for (int32 j = i + 1; j < 4; ++j)
            World.Diplomacy->MakeFirstContact(i, j, 1);

    FCoMDiplomaticRelation& Rel12 = World.Diplomacy->GetRelation(1, 2);
    Rel12.CurrentTreaty = ECoMTreatyType::DefensivePact;
    FCoMDiplomaticRelation& Rel23 = World.Diplomacy->GetRelation(2, 3);
    Rel23.CurrentTreaty = ECoMTreatyType::DefensivePact;

    // Wizard 0 attacks wizard 1
    World.Diplomacy->DeclareWar(0, 1);

    TestTrue("0 at war with 1", World.Diplomacy->AreAtWar(0, 1));
    TestTrue("0 at war with 2 (ally of 1)", World.Diplomacy->AreAtWar(0, 2));
    // Note: 2-3 pact does NOT trigger because 2 is defender not attacker
    // (mutual defense only triggers when the ally is attacked, not when the ally joins an existing war)

    TArray<int32> Enemies0 = World.Diplomacy->GetEnemies(0);
    TestTrue("Wizard 0 has at least 2 enemies", Enemies0.Num() >= 2);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// INTEGRATION: Mana economy → research → spell learned
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntManaResearch,
    "CoM.Integration.ManaNodeClaimResearchCompletion",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FIntManaResearch::RunTest(const FString& Parameters)
{
    FPhase7World World;
    FCoMWizardMagicState& State = World.Magic->GetWizardMagic(0);
    State.MaxMana = 1000;
    State.CurrentMana = 0;

    // Claim 3 mana nodes
    World.Magic->ClaimManaNode(0, FIntPoint(10, 10));
    World.Magic->ClaimManaNode(0, FIntPoint(20, 20));
    World.Magic->ClaimManaNode(0, FIntPoint(30, 30));

    // Start researching a spell
    FName SpellId = FName(TEXT("MagicMissile"));
    World.Magic->StartResearch(0, SpellId);
    State.ResearchAllocation = 50; // Heavy investment

    // Process turns until research completes
    bool Learned = false;
    for (int32 Turn = 1; Turn <= 50; ++Turn)
    {
        World.Magic->ProcessTurn(Turn);
        if (State.KnownSpells.Contains(SpellId))
        {
            Learned = true;
            AddInfo(FString::Printf(TEXT("Spell learned on turn %d"), Turn));
            break;
        }
    }

    TestTrue("Spell was eventually learned", Learned);
    TestTrue("Mana accumulated from nodes", State.CurrentMana > 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// INTEGRATION: Espionage mission lifecycle
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntSpyMissionLifecycle,
    "CoM.Integration.SpyMissionFullLifecycle",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FIntSpyMissionLifecycle::RunTest(const FString& Parameters)
{
    FPhase7World World;
    // Recruit a spy
    int32 AgentId = World.Espionage->RecruitAgent(0, TEXT("Shadow"), 70, 1);
    TestTrue("Agent recruited", AgentId > 0);

    // Send on spy mission
    bool Sent = World.Espionage->AssignMission(AgentId, ECoMAgentMission::Spy, 1, FIntPoint::ZeroValue);
    TestTrue("Mission assigned", Sent);

    // Process turns until mission resolves
    for (int32 Turn = 1; Turn <= 10; ++Turn)
    {
        World.Espionage->ProcessTurn(Turn);
        FCoMSpyAgent* Agent = World.Espionage->GetAgent(AgentId);
        if (!Agent || Agent->CurrentMission == ECoMAgentMission::MAX)
        {
            break; // Mission completed (or agent killed)
        }
    }

    // Check mission history
    TArray<FCoMMissionResult> History = World.Espionage->GetMissionHistory(0);
    TestTrue("At least one mission in history", History.Num() > 0);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// INTEGRATION: Diplomacy + gifts + trade treaty
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntTradeRelations,
    "CoM.Integration.TradeTreatyReputationGrowth",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FIntTradeRelations::RunTest(const FString& Parameters)
{
    FPhase7World World;
    World.Diplomacy->MakeFirstContact(0, 1, 1);

    // Start with negative reputation
    World.Diplomacy->ModifyReputation(0, 1, -50, TEXT("Initial tension"));
    TestTrue("Negative reputation", World.Diplomacy->GetReputation(0, 1) < 0);

    // Send gifts to improve relations
    TMap<ECoMResource, int32> Gift;
    Gift.Add(ECoMResource::GoldOre, 20);
    World.Diplomacy->SendGift(0, 1, Gift, 100);

    int32 RepAfterGift = World.Diplomacy->GetReputation(0, 1);
    TestTrue("Reputation improved from gift", RepAfterGift > -50);

    // Establish trade treaty
    FCoMDiplomaticRelation& Rel = World.Diplomacy->GetRelation(0, 1);
    Rel.CurrentTreaty = ECoMTreatyType::TradeAgreement;

    // Process turns — trade should passively improve reputation
    int32 RepBeforeTrade = World.Diplomacy->GetReputation(0, 1);
    for (int32 Turn = 1; Turn <= 10; ++Turn)
    {
        World.Diplomacy->ProcessTurn(Turn);
    }
    int32 RepAfterTrade = World.Diplomacy->GetReputation(0, 1);

    // Trade adds +2/turn but decay subtracts ~1/turn, so net gain ~+1/turn
    // Over 10 turns, should be ~+10 net
    TestTrue("Trade improved reputation over time", RepAfterTrade > RepBeforeTrade);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// E2E: Full turn simulation — all subsystems process turn
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FE2EFullTurnSimulation,
    "CoM.E2E.FullTurnSimulation",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FE2EFullTurnSimulation::RunTest(const FString& Parameters)
{
    FPhase7World World;

    // Setup: 3 wizards with various states
    for (int32 i = 0; i < 3; ++i)
    {
        // Magic state
        FCoMWizardMagicState& MState = World.Magic->GetWizardMagic(i);
        MState.MaxMana = 200;
        MState.CurrentMana = 100;
        MState.CastingSkill = 20 + i * 10;
        World.Magic->ClaimManaNode(i, FIntPoint(i * 20, 10));

        // Diplomacy
        for (int32 j = i + 1; j < 3; ++j)
        {
            World.Diplomacy->MakeFirstContact(i, j, 1);
        }

        // AI personality
        FCoMDiplomaticPersonality Personality;
        Personality.Aggressiveness = 30 + i * 20;
        Personality.TradeAffinity = 60 - i * 10;
        Personality.Trustworthiness = 70;
        World.Diplomacy->SetPersonality(i, Personality);

        // Espionage
        World.Espionage->RecruitAgent(i, TEXT(""), 40 + i * 10, 1);
    }

    // Wizard 0 and 1 at war
    World.Diplomacy->DeclareWar(0, 1);

    // Wizard 1 sends spy against wizard 0
    TArray<FCoMSpyAgent> Agents1 = World.Espionage->GetAgents(1);
    if (Agents1.Num() > 0)
    {
        World.Espionage->AssignMission(Agents1[0].AgentId, ECoMAgentMission::Spy, 0, FIntPoint::ZeroValue);
    }

    // Wizard 2 starts research
    World.Magic->StartResearch(2, FName(TEXT("Heal")));
    FCoMWizardMagicState& MState2 = World.Magic->GetWizardMagic(2);
    MState2.ResearchAllocation = 5;

    // Simulate 20 turns — should not crash
    for (int32 Turn = 1; Turn <= 20; ++Turn)
    {
        World.Diplomacy->ProcessTurn(Turn);
        World.Espionage->ProcessTurn(Turn);
        World.Magic->ProcessTurn(Turn);
    }

    // Verify state is consistent after 20 turns
    TestTrue("Wizard 0 still at war with 1", World.Diplomacy->AreAtWar(0, 1));
    TestTrue("Wizard 2 mana non-negative", World.Magic->GetCurrentMana(2) >= 0);
    TestTrue("War weariness accumulated", World.Diplomacy->GetWarWeariness(0, 21) > 0);

    // All subsystems survived 20 turns without crash
    TestTrue("E2E simulation completed", true);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// E2E: Multi-wizard diplomacy scenario
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FE2EMultiWizardDiplomacy,
    "CoM.E2E.MultiWizardDiplomacyScenario",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FE2EMultiWizardDiplomacy::RunTest(const FString& Parameters)
{
    FPhase7World World;

    // 5 wizards: varying personalities
    for (int32 i = 0; i < 5; ++i)
    {
        for (int32 j = i + 1; j < 5; ++j)
            World.Diplomacy->MakeFirstContact(i, j, 1);

        FCoMDiplomaticPersonality P;
        P.Aggressiveness = (i == 0) ? 90 : 30;  // Wizard 0 is aggressive
        P.TradeAffinity = (i == 0) ? 10 : 70;
        P.Trustworthiness = 50 + i * 5;
        P.Forgiveness = 40 + i * 8;
        World.Diplomacy->SetPersonality(i, P);
    }

    // Aggressive wizard declares war
    World.Diplomacy->DeclareWar(0, 1);

    // Others form defensive alliances
    FCoMDiplomaticRelation& Rel23 = World.Diplomacy->GetRelation(2, 3);
    Rel23.CurrentTreaty = ECoMTreatyType::DefensivePact;

    // Simulate 30 turns of AI diplomacy
    for (int32 Turn = 1; Turn <= 30; ++Turn)
    {
        World.Diplomacy->ProcessTurn(Turn);
    }

    // After 30 turns of war, weariness should be very high
    int32 Weariness = World.Diplomacy->GetWarWeariness(0, 31);
    TestTrue("Significant war weariness after 30 turns", Weariness > 50);

    // Reputation should have decayed toward 0 for non-combatants
    int32 Rep34 = World.Diplomacy->GetReputation(3, 4);
    // Should be near 0 since no interactions beyond first contact
    TestTrue("Non-combatant reputation near zero", FMath::Abs(Rep34) < 50);

    return true;
}

} // namespace CoMIntegrationTests

#endif
