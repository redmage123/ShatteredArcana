#if WITH_AUTOMATION_TESTS
// Copyright Mythforge Studios. All Rights Reserved.
// CoMEspionageSubsystemTests.cpp — Unit and regression tests
// Phase 7 tests — Shattered Arcana

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CoMCore/Espionage/CoMEspionageSubsystem.h"

namespace CoMEspionageTests
{

UCoMEspionageSubsystem* CreateTestSubsystem()
{
    UCoMEspionageSubsystem* Sub = NewObject<UCoMEspionageSubsystem>();
    // Auto-initialized via subsystem framework
    return Sub;
}

// ─────────────────────────────────────────────────────────────────────────────
// AGENT MANAGEMENT
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspRecruitAgent,
    "CoM.Espionage.RecruitAgent",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspRecruitAgent::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Id = Sub->RecruitAgent(0, TEXT("Shadow Walker"), 50, 1);
    TestTrue("Agent ID is positive", Id > 0);
    FCoMSpyAgent* Agent = Sub->GetAgent(Id);
    TestNotNull("Agent exists", Agent);
    TestEqual("Owner wizard", Agent->OwnerWizardId, 0);
    TestEqual("Skill level", Agent->SkillLevel, 50);
    TestEqual("Name set", Agent->AgentName, TEXT("Shadow Walker"));
    TestEqual("No mission", Agent->CurrentMission, ECoMAgentMission::MAX);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspGetAgents,
    "CoM.Espionage.GetAgents",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspGetAgents::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->RecruitAgent(0, TEXT("A"), 30, 1);
    Sub->RecruitAgent(0, TEXT("B"), 40, 1);
    Sub->RecruitAgent(1, TEXT("C"), 50, 1);
    TArray<FCoMSpyAgent> Agents0 = Sub->GetAgents(0);
    TArray<FCoMSpyAgent> Agents1 = Sub->GetAgents(1);
    TestEqual("Wizard 0 has 2 agents", Agents0.Num(), 2);
    TestEqual("Wizard 1 has 1 agent", Agents1.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspDismissAgent,
    "CoM.Espionage.DismissAgent",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspDismissAgent::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Id = Sub->RecruitAgent(0, TEXT(""), 30, 1);
    Sub->DismissAgent(Id);
    TestNull("Agent no longer exists", Sub->GetAgent(Id));
    return true;
}

// REGRESSION: Dismissing a counter-spy should decrement CounterSpyAgents
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspDismissCounterSpy,
    "CoM.Espionage.DismissCounterSpy",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspDismissCounterSpy::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Id = Sub->RecruitAgent(0, TEXT(""), 50, 1);
    Sub->AssignCounterSpy(0, Id);
    FCoMCounterIntel CI = Sub->GetCounterIntel(0);
    TestEqual("Counter spy count = 1", CI.CounterSpyAgents, 1);
    Sub->DismissAgent(Id);
    CI = Sub->GetCounterIntel(0);
    TestEqual("Counter spy count decremented to 0", CI.CounterSpyAgents, 0);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// MISSIONS
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspAssignMission,
    "CoM.Espionage.AssignMission",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspAssignMission::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Id = Sub->RecruitAgent(0, TEXT(""), 50, 1);
    bool Started = Sub->AssignMission(Id, ECoMAgentMission::Spy, 1, FIntPoint(0, 0));
    TestTrue("Mission started", Started);
    FCoMSpyAgent* Agent = Sub->GetAgent(Id);
    TestEqual("Mission type set", Agent->CurrentMission, ECoMAgentMission::Spy);
    TestEqual("Target wizard set", Agent->TargetWizardId, 1);
    TestTrue("Mission turns > 0", Agent->MissionTurnsLeft > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspAssignMissionBusy,
    "CoM.Espionage.AssignMissionBusy",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspAssignMissionBusy::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Id = Sub->RecruitAgent(0, TEXT(""), 50, 1);
    Sub->AssignMission(Id, ECoMAgentMission::Spy, 1, FIntPoint(0, 0));
    bool Second = Sub->AssignMission(Id, ECoMAgentMission::Sabotage, 2, FIntPoint(0, 0));
    TestFalse("Cannot assign while busy", Second);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspCancelMission,
    "CoM.Espionage.CancelMission",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspCancelMission::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Id = Sub->RecruitAgent(0, TEXT(""), 50, 1);
    Sub->AssignMission(Id, ECoMAgentMission::Spy, 1, FIntPoint(0, 0));
    Sub->CancelMission(Id);
    FCoMSpyAgent* Agent = Sub->GetAgent(Id);
    TestEqual("Mission cleared", Agent->CurrentMission, ECoMAgentMission::MAX);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspMissionSuccessChance,
    "CoM.Espionage.MissionSuccessChance",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspMissionSuccessChance::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Rookie = Sub->RecruitAgent(0, TEXT(""), 20, 1);
    int32 Veteran = Sub->RecruitAgent(0, TEXT(""), 80, 1);
    int32 RookieChance = Sub->GetMissionSuccessChance(Rookie, ECoMAgentMission::Spy, 1);
    int32 VeteranChance = Sub->GetMissionSuccessChance(Veteran, ECoMAgentMission::Spy, 1);
    TestTrue("Veteran has higher chance", VeteranChance > RookieChance);
    TestTrue("Chance >= 5", RookieChance >= 5);
    TestTrue("Chance <= 95", VeteranChance <= 95);
    // Harder missions have lower base chance
    int32 SpyChance = Sub->GetMissionSuccessChance(Veteran, ECoMAgentMission::Spy, 1);
    int32 AssassinChance = Sub->GetMissionSuccessChance(Veteran, ECoMAgentMission::Assassinate, 1);
    TestTrue("Assassination harder than spy", AssassinChance < SpyChance);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// TURN PROCESSING
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspProcessTurn,
    "CoM.Espionage.ProcessTurn",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspProcessTurn::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Id = Sub->RecruitAgent(0, TEXT(""), 50, 1);
    Sub->AssignMission(Id, ECoMAgentMission::Spy, 1, FIntPoint(0, 0)); // Duration 2 turns
    Sub->ProcessTurn(1);
    FCoMSpyAgent* Agent = Sub->GetAgent(Id);
    if (Agent) TestTrue("Turns left decreased", Agent->MissionTurnsLeft < 2);
    return true;
}

// REGRESSION: ResolveMission should not crash when agent is killed
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspNoUseAfterFree,
    "CoM.Espionage.ResolveMissionNoUseAfterFree",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspNoUseAfterFree::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    // Create many agents and send them on dangerous missions
    for (int32 i = 0; i < 20; ++i)
    {
        int32 Id = Sub->RecruitAgent(0, TEXT(""), 10, 1); // Low skill = high fail rate
        Sub->AssignMission(Id, ECoMAgentMission::Assassinate, 1, FIntPoint(0, 0));
    }
    // Process enough turns for missions to complete — should not crash
    for (int32 Turn = 1; Turn <= 10; ++Turn)
    {
        Sub->ProcessTurn(Turn);
    }
    // If we get here, no crash occurred
    TestTrue("No use-after-free crash", true);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// COUNTERINTELLIGENCE
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspCounterIntel,
    "CoM.Espionage.CounterIntel",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspCounterIntel::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    Sub->SetSecurityBudget(0, 100);
    FCoMCounterIntel CI = Sub->GetCounterIntel(0);
    TestTrue("Security level increased", CI.SecurityLevel > 20);
    // Counter-intel reduces enemy success chance
    int32 EnemyAgent = Sub->RecruitAgent(1, TEXT(""), 50, 1);
    int32 ChanceBefore = Sub->GetMissionSuccessChance(EnemyAgent, ECoMAgentMission::Spy, 0);
    Sub->SetSecurityBudget(0, 500);
    int32 ChanceAfter = Sub->GetMissionSuccessChance(EnemyAgent, ECoMAgentMission::Spy, 0);
    TestTrue("Higher security budget reduces enemy chance", ChanceAfter < ChanceBefore);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// CAPTURED AGENTS
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspRansomAgent,
    "CoM.Espionage.RansomCapturedAgent",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspRansomAgent::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Id = Sub->RecruitAgent(1, TEXT(""), 50, 1);
    FCoMSpyAgent* Agent = Sub->GetAgent(Id);
    Agent->bCaptured = true;
    int32 SkillBefore = Agent->SkillLevel;
    Sub->RansomCapturedAgent(0, Id);
    Agent = Sub->GetAgent(Id);
    TestNotNull("Agent still exists after ransom", Agent);
    TestFalse("No longer captured", Agent->bCaptured);
    TestTrue("Skill reduced from captivity", Agent->SkillLevel < SkillBefore);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEspLevelUp,
    "CoM.Espionage.LevelUp",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FEspLevelUp::RunTest(const FString& Parameters)
{
    auto* Sub = CreateTestSubsystem();
    int32 Id = Sub->RecruitAgent(0, TEXT(""), 20, 1);
    FCoMSpyAgent* Agent = Sub->GetAgent(Id);
    Agent->Experience = 100; // 2 levels worth
    // Trigger level check via process turn (which calls CheckLevelUp)
    Sub->ProcessTurn(1);
    Agent = Sub->GetAgent(Id);
    if (Agent)
    {
        TestTrue("Skill increased from XP", Agent->SkillLevel > 20);
    }
    return true;
}

} // namespace CoMEspionageTests

#endif
