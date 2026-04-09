#if WITH_AUTOMATION_TESTS
// Copyright Mythforge Studios. All Rights Reserved.
// CoMQuestSubsystemTests.cpp -- Automation tests for the quest subsystem
// Phase 8 -- Shattered Arcana

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CoMCore/Quests/CoMQuestSubsystem.h"

// ---------------------------------------------------------------------------
// Shared helper: create a quest subsystem with deterministic RNG
// ---------------------------------------------------------------------------


namespace CoMQuestTestHelpers
{
    struct FTestQuestWorld
    {
        UCoMQuestSubsystem* QS = nullptr;

        FTestQuestWorld()
        {
            QS = NewObject<UCoMQuestSubsystem>();

            // Initialize with a deterministic seed via config.
            // FSubsystemCollectionBase (protected)* DummyCollection = nullptr;
            // Direct initialization: the subsystem is a NewObject, so we
            // manually set up its state for testing.
            FCoMQuestGenConfig Config;
            Config.MaxActiveQuestsPerWizard = 5;
            Config.MaxAvailableQuests = 20;
            Config.QuestGenerationInterval = 5;
            Config.MinDifficulty = 1;
            Config.MaxDifficulty = 10;
            Config.bHeroQuestRumors = true;
            Config.ChainQuestChance = 20;
            Config.DefaultExpiryTurns = 15;
            QS->SetQuestGenConfig(Config);
        }
    };
}

// ---------------------------------------------------------------------------
// 1. CoM.Quest.GenerateQuest — generates a quest with valid fields
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCoMQuestGenerateTest,
    "CoM.Quest.GenerateQuest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMQuestGenerateTest::RunTest(const FString& Parameters)
{
    CoMQuestTestHelpers::FTestQuestWorld World;
    const int32 WizardId = 0;
    const int32 CurrentTurn = 10;

    const int32 QuestId = World.QS->GenerateQuest(WizardId, CurrentTurn);

    TestTrue(TEXT("GenerateQuest returns a valid quest ID"), QuestId > 0);

    const FCoMQuest* Quest = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest is retrievable by ID"), Quest);

    if (Quest)
    {
        TestEqual(TEXT("Quest status is Available"), Quest->Status, ECoMQuestStatus::Available);
        TestEqual(TEXT("Quest offered to correct wizard"), Quest->OfferedToWizardId, WizardId);
        TestEqual(TEXT("Quest generated on correct turn"), Quest->GeneratedTurn, CurrentTurn);
        TestTrue(TEXT("Quest has a title"), Quest->Title.Len() > 0);
        TestTrue(TEXT("Quest has a description"), Quest->Description.Len() > 0);
        TestTrue(TEXT("Quest has at least one objective"), Quest->Objectives.Num() >= 1);
        TestTrue(TEXT("Quest has at least one reward"), Quest->Rewards.Num() >= 1);
        TestTrue(TEXT("Quest difficulty >= 1"), Quest->Difficulty >= 1);
        TestTrue(TEXT("Quest difficulty <= 10"), Quest->Difficulty <= 10);
        TestTrue(TEXT("Quest has lore text"), Quest->LoreText.Len() > 0);
    }

    return true;
}

// ---------------------------------------------------------------------------
// 2. CoM.Quest.AcceptQuest — wizard can accept an available quest
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCoMQuestAcceptTest,
    "CoM.Quest.AcceptQuest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMQuestAcceptTest::RunTest(const FString& Parameters)
{
    CoMQuestTestHelpers::FTestQuestWorld World;
    const int32 WizardId = 0;
    const int32 CurrentTurn = 10;

    const int32 QuestId = World.QS->GenerateQuest(WizardId, CurrentTurn);
    TestTrue(TEXT("Quest generated"), QuestId > 0);

    // Accept the quest.
    const bool bAccepted = World.QS->AcceptQuest(QuestId, WizardId);
    TestTrue(TEXT("AcceptQuest returns true"), bAccepted);

    const FCoMQuest* Quest = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest exists after accept"), Quest);
    if (Quest)
    {
        TestEqual(TEXT("Quest status is Active after accept"), Quest->Status, ECoMQuestStatus::Active);
    }

    // Accepting the same quest again should fail.
    const bool bAcceptedAgain = World.QS->AcceptQuest(QuestId, WizardId);
    TestFalse(TEXT("Cannot accept an already-active quest"), bAcceptedAgain);

    // Accepting a quest offered to another wizard should fail.
    const int32 QuestId2 = World.QS->GenerateQuest(1, CurrentTurn); // Offered to wizard 1.
    const bool bWrongWizard = World.QS->AcceptQuest(QuestId2, 2); // Wizard 2 tries.
    TestFalse(TEXT("Cannot accept quest offered to another wizard"), bWrongWizard);

    // Active quest count should be 1 for wizard 0.
    TestEqual(TEXT("Active quest count is 1"), World.QS->GetActiveQuestCount(WizardId), 1);

    return true;
}

// ---------------------------------------------------------------------------
// 3. CoM.Quest.ReportProgress — objective progress tracking
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCoMQuestReportProgressTest,
    "CoM.Quest.ReportProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMQuestReportProgressTest::RunTest(const FString& Parameters)
{
    CoMQuestTestHelpers::FTestQuestWorld World;
    const int32 WizardId = 0;
    const int32 CurrentTurn = 10;

    const int32 QuestId = World.QS->GenerateQuestOfType(ECoMQuestType::DefeatMonster, WizardId, CurrentTurn);
    TestTrue(TEXT("Quest generated"), QuestId > 0);

    World.QS->AcceptQuest(QuestId, WizardId);

    const FCoMQuest* Quest = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest exists"), Quest);
    if (!Quest || Quest->Objectives.Num() == 0)
    {
        return true; // Cannot proceed without objectives.
    }

    // Report progress on the first objective.
    const int32 RequiredCount = Quest->Objectives[0].RequiredCount;
    World.QS->ReportObjectiveProgress(QuestId, 0, 1);

    // Re-fetch (pointer may be invalidated by TMap operations).
    const FCoMQuest* QuestAfter = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest still exists after progress"), QuestAfter);
    if (QuestAfter)
    {
        TestTrue(TEXT("Progress recorded"),
            QuestAfter->Objectives[0].CurrentCount >= 1);
    }

    // Report progress that cannot exceed required count.
    World.QS->ReportObjectiveProgress(QuestId, 0, RequiredCount + 10);
    const FCoMQuest* QuestCapped = World.QS->GetQuest(QuestId);
    if (QuestCapped && QuestCapped->Objectives.Num() > 0)
    {
        TestTrue(TEXT("Progress does not exceed required count"),
            QuestCapped->Objectives[0].CurrentCount <= QuestCapped->Objectives[0].RequiredCount);
    }

    // Invalid objective index should not crash.
    World.QS->ReportObjectiveProgress(QuestId, 999, 1);

    return true;
}

// ---------------------------------------------------------------------------
// 4. CoM.Quest.CompleteQuest — quest completion and rewards
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCoMQuestCompleteTest,
    "CoM.Quest.CompleteQuest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMQuestCompleteTest::RunTest(const FString& Parameters)
{
    CoMQuestTestHelpers::FTestQuestWorld World;
    const int32 WizardId = 0;
    const int32 CurrentTurn = 10;

    const int32 QuestId = World.QS->GenerateQuestOfType(ECoMQuestType::ExploreSite, WizardId, CurrentTurn);
    World.QS->AcceptQuest(QuestId, WizardId);

    // Complete all objectives manually.
    const FCoMQuest* Quest = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest exists"), Quest);
    if (Quest)
    {
        for (int32 i = 0; i < Quest->Objectives.Num(); ++i)
        {
            World.QS->ReportObjectiveProgress(QuestId, i, Quest->Objectives[i].RequiredCount);
        }
    }

    // Re-fetch and check completion.
    const FCoMQuest* CompletedQuest = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest still exists"), CompletedQuest);
    if (CompletedQuest)
    {
        TestEqual(TEXT("Quest status is Completed"), CompletedQuest->Status, ECoMQuestStatus::Completed);

        // All objectives should be marked complete.
        for (int32 i = 0; i < CompletedQuest->Objectives.Num(); ++i)
        {
            TestTrue(
                FString::Printf(TEXT("Objective %d is completed"), i),
                CompletedQuest->Objectives[i].bCompleted);
        }
    }

    // Completed quest should appear in completed list.
    const TArray<FCoMQuest> CompletedList = World.QS->GetCompletedQuests(WizardId);
    TestTrue(TEXT("Completed quests list is not empty"), CompletedList.Num() > 0);

    // Force-complete a fresh quest for coverage.
    const int32 QuestId2 = World.QS->GenerateQuestOfType(ECoMQuestType::DefeatMonster, WizardId, CurrentTurn);
    World.QS->AcceptQuest(QuestId2, WizardId);
    World.QS->ForceCompleteQuest(QuestId2);

    const FCoMQuest* Forced = World.QS->GetQuest(QuestId2);
    if (Forced)
    {
        TestEqual(TEXT("Force-completed quest is Completed"), Forced->Status, ECoMQuestStatus::Completed);
    }

    return true;
}

// ---------------------------------------------------------------------------
// 5. CoM.Quest.ExpireQuest — quests expire after their expiry turn
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCoMQuestExpireTest,
    "CoM.Quest.ExpireQuest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMQuestExpireTest::RunTest(const FString& Parameters)
{
    CoMQuestTestHelpers::FTestQuestWorld World;
    const int32 WizardId = 0;
    const int32 CurrentTurn = 10;

    // Set a short expiry for testing.
    FCoMQuestGenConfig Config = World.QS->GetQuestGenConfig();
    Config.DefaultExpiryTurns = 5;
    World.QS->SetQuestGenConfig(Config);

    const int32 QuestId = World.QS->GenerateQuest(WizardId, CurrentTurn);
    const FCoMQuest* Quest = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest generated"), Quest);

    if (Quest)
    {
        TestEqual(TEXT("Quest is Available"), Quest->Status, ECoMQuestStatus::Available);
        TestEqual(TEXT("Expiry turn is correct"), Quest->ExpiryTurn, CurrentTurn + 5);
    }

    // Process turn at exactly the expiry turn.
    World.QS->ProcessTurn(CurrentTurn + 5, CoM::MAX_WIZARDS);

    const FCoMQuest* ExpiredQuest = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest still exists after expiry"), ExpiredQuest);
    if (ExpiredQuest)
    {
        TestEqual(TEXT("Quest status is Expired"), ExpiredQuest->Status, ECoMQuestStatus::Expired);
    }

    // An accepted quest should not expire (it should fail if over time limit).
    const int32 QuestId2 = World.QS->GenerateQuest(WizardId, CurrentTurn + 6);
    World.QS->AcceptQuest(QuestId2, WizardId);
    World.QS->ProcessTurn(CurrentTurn + 30, CoM::MAX_WIZARDS);

    const FCoMQuest* ActiveQuest = World.QS->GetQuest(QuestId2);
    if (ActiveQuest)
    {
        // Active quest should not be expired (may be failed if time limit, but not expired).
        TestTrue(TEXT("Active quest is not expired"),
            ActiveQuest->Status != ECoMQuestStatus::Expired);
    }

    return true;
}

// ---------------------------------------------------------------------------
// 6. CoM.Quest.QuestChain — multi-step quest chain generation and linking
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCoMQuestChainTest,
    "CoM.Quest.QuestChain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMQuestChainTest::RunTest(const FString& Parameters)
{
    CoMQuestTestHelpers::FTestQuestWorld World;
    const int32 WizardId = 0;
    const int32 CurrentTurn = 10;
    const int32 ChainLength = 3;

    const int32 FirstQuestId = World.QS->GenerateQuestChain(WizardId, CurrentTurn, ChainLength);
    TestTrue(TEXT("Chain returns a valid first quest ID"), FirstQuestId > 0);

    const FCoMQuest* FirstQuest = World.QS->GetQuest(FirstQuestId);
    TestNotNull(TEXT("First quest in chain exists"), FirstQuest);

    if (!FirstQuest)
    {
        return true;
    }

    // Chain ID should be set.
    TestTrue(TEXT("First quest has a chain ID"), FirstQuest->ChainId > 0);

    // Title should contain "(Part 1)".
    TestTrue(TEXT("First quest title contains Part 1"),
        FirstQuest->Title.Contains(TEXT("Part 1")));

    // Next quest in chain should be set.
    TestTrue(TEXT("First quest links to next in chain"),
        FirstQuest->NextQuestInChainId > 0);

    // Walk the chain and verify structure.
    int32 StepCount = 0;
    int32 CurrentQuestId = FirstQuestId;
    int32 PrevDifficulty = 0;

    while (CurrentQuestId > 0 && StepCount < 10) // Safety limit.
    {
        const FCoMQuest* Q = World.QS->GetQuest(CurrentQuestId);
        TestNotNull(
            FString::Printf(TEXT("Chain quest %d exists"), CurrentQuestId), Q);

        if (!Q)
        {
            break;
        }

        // Chain ID should be consistent.
        TestEqual(TEXT("Chain ID is consistent"), Q->ChainId, FirstQuest->ChainId);

        // Difficulty should not decrease (may stay same or increase).
        TestTrue(
            FString::Printf(TEXT("Difficulty %d >= previous %d"), Q->Difficulty, PrevDifficulty),
            Q->Difficulty >= PrevDifficulty);

        PrevDifficulty = Q->Difficulty;
        CurrentQuestId = Q->NextQuestInChainId;
        StepCount++;
    }

    TestEqual(TEXT("Chain has correct number of steps"), StepCount, ChainLength);

    // Accept and complete first quest, verify next becomes available.
    World.QS->AcceptQuest(FirstQuestId, WizardId);
    World.QS->ForceCompleteQuest(FirstQuestId);

    const FCoMQuest* SecondQuest = World.QS->GetQuest(FirstQuest->NextQuestInChainId);
    if (SecondQuest)
    {
        TestEqual(TEXT("Second quest is offered to the same wizard"),
            SecondQuest->OfferedToWizardId, WizardId);
    }

    return true;
}

// ---------------------------------------------------------------------------
// 7. CoM.Quest.HeroRumor — hero quest rumor generation
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCoMQuestHeroRumorTest,
    "CoM.Quest.HeroRumor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMQuestHeroRumorTest::RunTest(const FString& Parameters)
{
    CoMQuestTestHelpers::FTestQuestWorld World;
    const int32 WizardId = 0;
    const int32 HeroId = 42;
    const int32 CurrentTurn = 10;

    const int32 QuestId = World.QS->GenerateHeroQuestRumor(HeroId, WizardId, CurrentTurn);
    TestTrue(TEXT("Hero rumor returns a valid quest ID"), QuestId > 0);

    const FCoMQuest* Quest = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Rumor quest exists"), Quest);

    if (Quest)
    {
        TestEqual(TEXT("Quest giver hero ID is set"), Quest->QuestGiverHeroId, HeroId);
        TestTrue(TEXT("Rumor quest title starts with 'Rumor:'"),
            Quest->Title.StartsWith(TEXT("Rumor:")));

        // Should be one of the rumor-biased types.
        const bool bValidType =
            Quest->QuestType == ECoMQuestType::ExploreSite ||
            Quest->QuestType == ECoMQuestType::RecoverArtifact ||
            Quest->QuestType == ECoMQuestType::SlayDragon ||
            Quest->QuestType == ECoMQuestType::ClearDungeon;
        TestTrue(TEXT("Rumor quest type is one of the expected types"), bValidType);
    }

    // Disable hero rumors and verify no quest is generated.
    FCoMQuestGenConfig Config = World.QS->GetQuestGenConfig();
    Config.bHeroQuestRumors = false;
    World.QS->SetQuestGenConfig(Config);

    const int32 DisabledId = World.QS->GenerateHeroQuestRumor(HeroId, WizardId, CurrentTurn);
    TestEqual(TEXT("No quest generated when hero rumors disabled"), DisabledId, -1);

    return true;
}

// ---------------------------------------------------------------------------
// 8. CoM.Quest.EventHooks — OnTileVisited triggers explore quest progress
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCoMQuestEventHooksTest,
    "CoM.Quest.EventHooks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMQuestEventHooksTest::RunTest(const FString& Parameters)
{
    CoMQuestTestHelpers::FTestQuestWorld World;
    const int32 WizardId = 0;
    const int32 CurrentTurn = 10;

    // Generate an explore quest so we know the target tile.
    const int32 QuestId = World.QS->GenerateQuestOfType(ECoMQuestType::ExploreSite, WizardId, CurrentTurn);
    TestTrue(TEXT("Explore quest generated"), QuestId > 0);

    World.QS->AcceptQuest(QuestId, WizardId);

    const FCoMQuest* Quest = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest exists"), Quest);
    if (!Quest || Quest->Objectives.Num() == 0)
    {
        return true;
    }

    // Get the target tile and plane from the first objective.
    const FIntPoint TargetTile = Quest->Objectives[0].TargetTile;
    const ECoMPlane TargetPlane = Quest->Objectives[0].TargetPlane;

    // Visiting a wrong tile should not advance progress.
    const FIntPoint WrongTile(TargetTile.X + 1, TargetTile.Y + 1);
    World.QS->OnTileVisited(WizardId, TargetPlane, WrongTile);

    const FCoMQuest* QuestAfterWrong = World.QS->GetQuest(QuestId);
    if (QuestAfterWrong && QuestAfterWrong->Objectives.Num() > 0)
    {
        TestEqual(TEXT("Wrong tile does not advance progress"),
            QuestAfterWrong->Objectives[0].CurrentCount, 0);
    }

    // Visiting the correct tile should advance progress and potentially complete.
    World.QS->OnTileVisited(WizardId, TargetPlane, TargetTile);

    const FCoMQuest* QuestAfterCorrect = World.QS->GetQuest(QuestId);
    TestNotNull(TEXT("Quest exists after correct tile visit"), QuestAfterCorrect);
    if (QuestAfterCorrect && QuestAfterCorrect->Objectives.Num() > 0)
    {
        TestTrue(TEXT("Correct tile advances progress"),
            QuestAfterCorrect->Objectives[0].CurrentCount >= 1);
    }

    // If the quest had only one objective requiring count 1, it should now be complete.
    if (QuestAfterCorrect && QuestAfterCorrect->Objectives.Num() == 1 &&
        QuestAfterCorrect->Objectives[0].RequiredCount == 1)
    {
        TestEqual(TEXT("Single-objective explore quest is completed"),
            QuestAfterCorrect->Status, ECoMQuestStatus::Completed);
    }

    // OnCombatVictory should not affect explore quests.
    const int32 QuestId2 = World.QS->GenerateQuestOfType(ECoMQuestType::ExploreSite, WizardId, CurrentTurn);
    World.QS->AcceptQuest(QuestId2, WizardId);
    World.QS->OnCombatVictory(WizardId, 999, FIntPoint(0, 0));

    const FCoMQuest* QuestNoCombat = World.QS->GetQuest(QuestId2);
    if (QuestNoCombat && QuestNoCombat->Objectives.Num() > 0)
    {
        TestEqual(TEXT("Combat victory does not affect explore quest"),
            QuestNoCombat->Objectives[0].CurrentCount, 0);
    }

    // Test OnCityCapture for a conquer quest.
    const int32 ConquerQuestId = World.QS->GenerateQuestOfType(ECoMQuestType::ConquerCity, WizardId, CurrentTurn);
    World.QS->AcceptQuest(ConquerQuestId, WizardId);

    const FCoMQuest* ConquerQuest = World.QS->GetQuest(ConquerQuestId);
    if (ConquerQuest && ConquerQuest->Objectives.Num() > 0)
    {
        const int32 TargetCityId = ConquerQuest->Objectives[0].TargetEntityId;
        World.QS->OnCityCapture(WizardId, TargetCityId);

        const FCoMQuest* ConquerAfter = World.QS->GetQuest(ConquerQuestId);
        if (ConquerAfter && ConquerAfter->Objectives.Num() > 0)
        {
            TestTrue(TEXT("City capture advances conquer quest"),
                ConquerAfter->Objectives[0].CurrentCount >= 1);
        }
    }

    // Test OnDragonSlain for a slay dragon quest.
    const int32 DragonQuestId = World.QS->GenerateQuestOfType(ECoMQuestType::SlayDragon, WizardId, CurrentTurn);
    World.QS->AcceptQuest(DragonQuestId, WizardId);

    const FCoMQuest* DragonQuest = World.QS->GetQuest(DragonQuestId);
    if (DragonQuest && DragonQuest->Objectives.Num() > 0)
    {
        const int32 TargetDragonId = DragonQuest->Objectives[0].TargetEntityId;
        World.QS->OnDragonSlain(WizardId, TargetDragonId);

        const FCoMQuest* DragonAfter = World.QS->GetQuest(DragonQuestId);
        if (DragonAfter && DragonAfter->Objectives.Num() > 0)
        {
            TestTrue(TEXT("Dragon slain advances slay dragon quest"),
                DragonAfter->Objectives[0].CurrentCount >= 1);
        }
    }

    return true;
}


#endif
