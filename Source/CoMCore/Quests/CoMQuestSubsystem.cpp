// Copyright Mythforge Studios. All Rights Reserved.
// CoMQuestSubsystem.cpp -- Procedural quest generation, tracking, completion, rewards
// Phase 8 -- Shattered Arcana

#include "CoMQuestSubsystem.h"
#include "CoMCore/CoreTypes/CoMConstants.h"

// =====================================================================
// Thematic word tables for procedural quest text
// =====================================================================

namespace CoMQuestText
{
    static const FString Prefixes[] = {
        TEXT("The Lost"),     TEXT("Ancient"),      TEXT("Shadow's"),
        TEXT("Forgotten"),    TEXT("Cursed"),        TEXT("Sacred"),
        TEXT("The Shattered"),TEXT("Twilight"),      TEXT("Crimson"),
        TEXT("The Sunken"),   TEXT("Ember"),         TEXT("The Sealed"),
        TEXT("Storm-Touched"),TEXT("The Hidden"),    TEXT("Haunted"),
        TEXT("The Burning"),  TEXT("Frost-Bound"),   TEXT("The Silent"),
    };
    static constexpr int32 NumPrefixes = UE_ARRAY_COUNT(Prefixes);

    static const FString Nouns[] = {
        TEXT("Vault"),   TEXT("Tower"),   TEXT("Blade"),   TEXT("Crown"),
        TEXT("Tome"),    TEXT("Chalice"), TEXT("Throne"),  TEXT("Sanctum"),
        TEXT("Gate"),    TEXT("Obelisk"), TEXT("Shrine"),  TEXT("Citadel"),
        TEXT("Spire"),   TEXT("Crypt"),   TEXT("Altar"),   TEXT("Scepter"),
        TEXT("Sigil"),   TEXT("Seal"),    TEXT("Codex"),   TEXT("Relic"),
    };
    static constexpr int32 NumNouns = UE_ARRAY_COUNT(Nouns);

    static const FString Adjectives[] = {
        TEXT("Forgotten"),  TEXT("Ancient"),    TEXT("Burning"),
        TEXT("Frozen"),     TEXT("Sunken"),     TEXT("Shadowed"),
        TEXT("Twisted"),    TEXT("Golden"),     TEXT("Obsidian"),
        TEXT("Spectral"),   TEXT("Verdant"),    TEXT("Ashen"),
    };
    static constexpr int32 NumAdjectives = UE_ARRAY_COUNT(Adjectives);

    static const FString Creatures[] = {
        TEXT("Wyrm"),        TEXT("Behemoth"),   TEXT("Lich"),
        TEXT("Hydra"),       TEXT("Basilisk"),   TEXT("Chimera"),
        TEXT("Demon Lord"),  TEXT("Shadow Fiend"),TEXT("Elder Drake"),
        TEXT("Bone Colossus"),TEXT("Plague Wraith"),TEXT("Void Stalker"),
    };
    static constexpr int32 NumCreatures = UE_ARRAY_COUNT(Creatures);

    static const FString PlaneNames[] = {
        TEXT("Aurelith"),   TEXT("Noctharion"),  TEXT("Verdantis"),
        TEXT("Infernyx"),   TEXT("Aethermist"),  TEXT("Abyssal"),
        TEXT("Ethereal"),   TEXT("Feywild"),
    };
    static constexpr int32 NumPlaneNames = UE_ARRAY_COUNT(PlaneNames);

    static const FString LorePhrases[] = {
        TEXT("Legends speak of a power buried beneath the earth, waiting to be claimed by one bold enough to seek it."),
        TEXT("The whispers of the Arcane winds carry a name long forgotten -- a name tied to destiny itself."),
        TEXT("An ancient pact was broken, and now the balance must be restored before darkness consumes all."),
        TEXT("The stars have aligned, and the path to glory lies open for those who dare to walk it."),
        TEXT("From the depths of the Underdark, a tremor echoes -- something stirs, something hungry."),
        TEXT("A rival wizard's hubris has torn a rift in the planar fabric. The consequences must be contained."),
        TEXT("The dragon lords grow restless. Their hoards swell with stolen relics, and their patience wanes."),
        TEXT("A prophecy carved into the walls of a ruined temple speaks of this very moment."),
    };
    static constexpr int32 NumLorePhrases = UE_ARRAY_COUNT(LorePhrases);
}

// =====================================================================
// Quest generation constants
// =====================================================================

namespace CoMQuestConstants
{
    /** Minimum available quests a wizard should always have. */
    static constexpr int32 MIN_AVAILABLE_PER_WIZARD = 2;

    /** New quests attempted per generation interval. */
    static constexpr int32 QUESTS_PER_GENERATION = 2;

    /** Quest chain length range. */
    static constexpr int32 CHAIN_LENGTH_MIN = 2;
    static constexpr int32 CHAIN_LENGTH_MAX = 4;

    /** Difficulty scaling -- turn divisor for difficulty floor. */
    static constexpr int32 DIFFICULTY_TURN_DIVISOR = 20;

    /** Gold reward per difficulty point (tier 1). */
    static constexpr int32 GOLD_PER_DIFFICULTY = 50;

    /** Mana reward per difficulty point (tier 1). */
    static constexpr int32 MANA_PER_DIFFICULTY = 25;

    /** Fame reward per difficulty point. */
    static constexpr int32 FAME_PER_DIFFICULTY = 5;
}

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RngStream.Initialize(0x51756573);
    NextQuestId = 1;
    NextChainId = 1;
    LastGenerationTurn = 0;
}

void UCoMQuestSubsystem::Deinitialize()
{
    AllQuests.Empty();
    Super::Deinitialize();
}

// =====================================================================
// Quest Lifecycle
// =====================================================================

bool UCoMQuestSubsystem::AcceptQuest(int32 QuestId, int32 WizardId)
{
    FCoMQuest* Quest = AllQuests.Find(QuestId);
    if (!Quest)
    {
        return false;
    }

    if (Quest->Status != ECoMQuestStatus::Available)
    {
        return false;
    }

    // Check if offered to a different wizard.
    if (Quest->OfferedToWizardId != -1 && Quest->OfferedToWizardId != WizardId)
    {
        return false;
    }

    // Check max active quests.
    if (GetActiveQuestCount(WizardId) >= GenConfig.MaxActiveQuestsPerWizard)
    {
        return false;
    }

    Quest->Status = ECoMQuestStatus::Active;
    Quest->OfferedToWizardId = WizardId;
    Quest->AcceptedTurn = LastGenerationTurn; // Best approximation; caller may pass current turn via GenerateQuest.

    OnQuestAccepted.Broadcast(QuestId, WizardId);
    return true;
}

void UCoMQuestSubsystem::AbandonQuest(int32 QuestId)
{
    FCoMQuest* Quest = AllQuests.Find(QuestId);
    if (!Quest)
    {
        return;
    }

    if (Quest->Status != ECoMQuestStatus::Active)
    {
        return;
    }

    const int32 WizardId = Quest->OfferedToWizardId;
    Quest->Status = ECoMQuestStatus::Failed;
    OnQuestFailed.Broadcast(QuestId, WizardId);
}

void UCoMQuestSubsystem::ReportObjectiveProgress(int32 QuestId, int32 ObjectiveIndex, int32 ProgressDelta)
{
    FCoMQuest* Quest = AllQuests.Find(QuestId);
    if (!Quest)
    {
        return;
    }

    if (Quest->Status != ECoMQuestStatus::Active)
    {
        return;
    }

    if (!Quest->Objectives.IsValidIndex(ObjectiveIndex))
    {
        return;
    }

    FCoMQuestObjective& Obj = Quest->Objectives[ObjectiveIndex];
    if (Obj.bCompleted)
    {
        return;
    }

    Obj.CurrentCount = FMath::Min(Obj.CurrentCount + ProgressDelta, Obj.RequiredCount);

    OnQuestObjectiveProgress.Broadcast(QuestId, ObjectiveIndex, Obj.CurrentCount);

    if (Obj.CurrentCount >= Obj.RequiredCount)
    {
        Obj.bCompleted = true;

        // Unhide the next objective in sequence if it was hidden.
        const int32 NextIdx = ObjectiveIndex + 1;
        if (Quest->Objectives.IsValidIndex(NextIdx) && Quest->Objectives[NextIdx].bHidden)
        {
            Quest->Objectives[NextIdx].bHidden = false;
        }
    }

    CheckQuestCompletion(QuestId);
}

void UCoMQuestSubsystem::ForceCompleteQuest(int32 QuestId)
{
    FCoMQuest* Quest = AllQuests.Find(QuestId);
    if (!Quest)
    {
        return;
    }

    // Mark all objectives complete.
    for (FCoMQuestObjective& Obj : Quest->Objectives)
    {
        Obj.CurrentCount = Obj.RequiredCount;
        Obj.bCompleted = true;
    }

    Quest->Status = ECoMQuestStatus::Completed;

    const int32 WizardId = Quest->OfferedToWizardId;
    if (WizardId >= 0)
    {
        GrantRewards(WizardId, *Quest);
    }

    OnQuestCompleted.Broadcast(QuestId, WizardId);

    // Trigger next quest in chain.
    if (Quest->NextQuestInChainId > 0)
    {
        FCoMQuest* NextQuest = AllQuests.Find(Quest->NextQuestInChainId);
        if (NextQuest && (NextQuest->Status == ECoMQuestStatus::Available
                       || NextQuest->Status == ECoMQuestStatus::Expired))
        {
            // Unlock the next quest in the chain and offer to the same wizard.
            NextQuest->Status = ECoMQuestStatus::Available;
            NextQuest->OfferedToWizardId = WizardId;
            OnQuestAvailable.Broadcast(NextQuest->QuestId, WizardId);
        }
    }
}

// =====================================================================
// Quest Queries
// =====================================================================

TArray<FCoMQuest> UCoMQuestSubsystem::GetAvailableQuests(int32 WizardId) const
{
    TArray<FCoMQuest> Result;
    for (const auto& Pair : AllQuests)
    {
        const FCoMQuest& Q = Pair.Value;
        if (Q.Status == ECoMQuestStatus::Available &&
            (Q.OfferedToWizardId == -1 || Q.OfferedToWizardId == WizardId))
        {
            Result.Add(Q);
        }
    }
    return Result;
}

TArray<FCoMQuest> UCoMQuestSubsystem::GetActiveQuests(int32 WizardId) const
{
    TArray<FCoMQuest> Result;
    for (const auto& Pair : AllQuests)
    {
        const FCoMQuest& Q = Pair.Value;
        if (Q.Status == ECoMQuestStatus::Active && Q.OfferedToWizardId == WizardId)
        {
            Result.Add(Q);
        }
    }
    return Result;
}

TArray<FCoMQuest> UCoMQuestSubsystem::GetCompletedQuests(int32 WizardId) const
{
    TArray<FCoMQuest> Result;
    for (const auto& Pair : AllQuests)
    {
        const FCoMQuest& Q = Pair.Value;
        if (Q.Status == ECoMQuestStatus::Completed && Q.OfferedToWizardId == WizardId)
        {
            Result.Add(Q);
        }
    }
    return Result;
}

const FCoMQuest* UCoMQuestSubsystem::GetQuest(int32 QuestId) const
{
    return AllQuests.Find(QuestId);
}

int32 UCoMQuestSubsystem::GetActiveQuestCount(int32 WizardId) const
{
    int32 Count = 0;
    for (const auto& Pair : AllQuests)
    {
        if (Pair.Value.Status == ECoMQuestStatus::Active && Pair.Value.OfferedToWizardId == WizardId)
        {
            ++Count;
        }
    }
    return Count;
}

bool UCoMQuestSubsystem::IsTileQuestTarget(int32 WizardId, ECoMPlane Plane, FIntPoint Tile) const
{
    for (const auto& Pair : AllQuests)
    {
        const FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active || Q.OfferedToWizardId != WizardId)
        {
            continue;
        }

        for (const FCoMQuestObjective& Obj : Q.Objectives)
        {
            if (!Obj.bCompleted && Obj.TargetPlane == Plane && Obj.TargetTile == Tile)
            {
                return true;
            }
        }
    }
    return false;
}

// =====================================================================
// Quest Generation
// =====================================================================

int32 UCoMQuestSubsystem::GenerateQuest(int32 WizardId, int32 CurrentTurn)
{
    const ECoMQuestType Type = SelectQuestType(WizardId, CurrentTurn);
    return GenerateQuestOfType(Type, WizardId, CurrentTurn);
}

int32 UCoMQuestSubsystem::GenerateQuestOfType(ECoMQuestType Type, int32 WizardId, int32 CurrentTurn)
{
    const int32 Difficulty = CalculateDifficulty(WizardId, CurrentTurn);

    FCoMQuest Quest;
    Quest.QuestId = NextQuestId++;
    Quest.QuestType = Type;
    Quest.Status = ECoMQuestStatus::Available;
    Quest.OfferedToWizardId = WizardId;
    Quest.GeneratedTurn = CurrentTurn;
    Quest.Difficulty = Difficulty;
    Quest.ExpiryTurn = CurrentTurn + GenConfig.DefaultExpiryTurns;
    Quest.TurnLimit = -1; // Default: no time limit after acceptance.

    // Certain quest types get time limits.
    switch (Type)
    {
    case ECoMQuestType::DefendCity:
        Quest.TurnLimit = 5 + Difficulty; // Hold for N turns.
        break;
    case ECoMQuestType::EscortHero:
        Quest.TurnLimit = 10 + Difficulty * 2;
        break;
    case ECoMQuestType::RitualChallenge:
        Quest.TurnLimit = 8 + Difficulty;
        break;
    default:
        break;
    }

    Quest.Objectives = GenerateObjectives(Type, WizardId, Difficulty, CurrentTurn);
    Quest.Rewards = GenerateRewards(Type, Difficulty);
    GenerateQuestText(Quest);
    Quest.LoreText = GenerateLoreText(Type, Difficulty);

    AllQuests.Add(Quest.QuestId, Quest);
    OnQuestAvailable.Broadcast(Quest.QuestId, WizardId);

    return Quest.QuestId;
}

int32 UCoMQuestSubsystem::GenerateQuestChain(int32 WizardId, int32 CurrentTurn, int32 ChainLength)
{
    ChainLength = FMath::Clamp(ChainLength, CoMQuestConstants::CHAIN_LENGTH_MIN, CoMQuestConstants::CHAIN_LENGTH_MAX);

    const int32 ChainId = NextChainId++;

    // Pick a base quest type for the chain theme.
    const ECoMQuestType BaseType = SelectQuestType(WizardId, CurrentTurn);

    // Progression of types within the chain.
    static const ECoMQuestType ChainProgression[] = {
        ECoMQuestType::ExploreSite,
        ECoMQuestType::DefeatMonster,
        ECoMQuestType::RecoverArtifact,
        ECoMQuestType::ConquerCity,
    };
    static constexpr int32 NumProgression = UE_ARRAY_COUNT(ChainProgression);

    TArray<int32> ChainQuestIds;
    int32 FirstQuestId = -1;

    for (int32 Step = 0; Step < ChainLength; ++Step)
    {
        // Difficulty escalates through the chain.
        const int32 StepDifficulty = FMath::Clamp(
            CalculateDifficulty(WizardId, CurrentTurn) + Step * 2,
            GenConfig.MinDifficulty,
            GenConfig.MaxDifficulty);

        // Select type: first step uses base type, subsequent steps progress.
        ECoMQuestType StepType = BaseType;
        if (Step > 0 && Step < NumProgression)
        {
            StepType = ChainProgression[Step];
        }
        else if (Step >= NumProgression)
        {
            StepType = ChainProgression[NumProgression - 1];
        }

        FCoMQuest Quest;
        Quest.QuestId = NextQuestId++;
        Quest.QuestType = StepType;
        Quest.OfferedToWizardId = WizardId;
        Quest.GeneratedTurn = CurrentTurn;
        Quest.Difficulty = StepDifficulty;
        Quest.ExpiryTurn = CurrentTurn + GenConfig.DefaultExpiryTurns + Step * 10;
        Quest.ChainId = ChainId;

        // First quest is Available, rest are locked until prior completes.
        Quest.Status = (Step == 0) ? ECoMQuestStatus::Available : ECoMQuestStatus::Expired;

        Quest.Objectives = GenerateObjectives(StepType, WizardId, StepDifficulty, CurrentTurn);
        Quest.Rewards = GenerateRewards(StepType, StepDifficulty);
        GenerateQuestText(Quest);
        Quest.LoreText = GenerateLoreText(StepType, StepDifficulty);

        // Append chain step to title.
        Quest.Title = FString::Printf(TEXT("%s (Part %d)"), *Quest.Title, Step + 1);

        AllQuests.Add(Quest.QuestId, Quest);
        ChainQuestIds.Add(Quest.QuestId);

        if (Step == 0)
        {
            FirstQuestId = Quest.QuestId;
        }
    }

    // Link the chain: each quest points to the next.
    for (int32 i = 0; i < ChainQuestIds.Num() - 1; ++i)
    {
        FCoMQuest* Current = AllQuests.Find(ChainQuestIds[i]);
        if (Current)
        {
            Current->NextQuestInChainId = ChainQuestIds[i + 1];
        }
    }

    // Only broadcast the first quest as available.
    if (FirstQuestId > 0)
    {
        OnQuestAvailable.Broadcast(FirstQuestId, WizardId);
    }

    return FirstQuestId;
}

int32 UCoMQuestSubsystem::GenerateHeroQuestRumor(int32 HeroId, int32 WizardId, int32 CurrentTurn)
{
    if (!GenConfig.bHeroQuestRumors)
    {
        return -1;
    }

    // Hero rumors bias toward explore and recover artifact quests.
    const int32 Roll = RngStream.RandRange(0, 99);
    ECoMQuestType Type;
    if (Roll < 40)
    {
        Type = ECoMQuestType::ExploreSite;
    }
    else if (Roll < 70)
    {
        Type = ECoMQuestType::RecoverArtifact;
    }
    else if (Roll < 85)
    {
        Type = ECoMQuestType::SlayDragon;
    }
    else
    {
        Type = ECoMQuestType::ClearDungeon;
    }

    const int32 QuestId = GenerateQuestOfType(Type, WizardId, CurrentTurn);

    // Tag the quest giver.
    FCoMQuest* Quest = AllQuests.Find(QuestId);
    if (Quest)
    {
        Quest->QuestGiverHeroId = HeroId;
        // Prepend "Rumor: " to the title.
        Quest->Title = FString::Printf(TEXT("Rumor: %s"), *Quest->Title);
    }

    return QuestId;
}

// =====================================================================
// Event Hooks
// =====================================================================

void UCoMQuestSubsystem::OnTileVisited(int32 WizardId, ECoMPlane Plane, FIntPoint Tile)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active || Q.OfferedToWizardId != WizardId)
        {
            continue;
        }

        // Check explore/clear dungeon/planar expedition objectives.
        if (Q.QuestType != ECoMQuestType::ExploreSite &&
            Q.QuestType != ECoMQuestType::ClearDungeon &&
            Q.QuestType != ECoMQuestType::PlanarExpedition &&
            Q.QuestType != ECoMQuestType::EscortHero)
        {
            continue;
        }

        for (int32 i = 0; i < Q.Objectives.Num(); ++i)
        {
            FCoMQuestObjective& Obj = Q.Objectives[i];
            if (Obj.bCompleted || Obj.bHidden)
            {
                continue;
            }
            if (Obj.TargetPlane == Plane && Obj.TargetTile == Tile)
            {
                ReportObjectiveProgress(Q.QuestId, i, 1);
            }
        }
    }
}

void UCoMQuestSubsystem::OnCombatVictory(int32 WizardId, int32 DefeatedEntityId, FIntPoint Tile)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active || Q.OfferedToWizardId != WizardId)
        {
            continue;
        }

        if (Q.QuestType != ECoMQuestType::DefeatMonster &&
            Q.QuestType != ECoMQuestType::ClearDungeon)
        {
            continue;
        }

        for (int32 i = 0; i < Q.Objectives.Num(); ++i)
        {
            FCoMQuestObjective& Obj = Q.Objectives[i];
            if (Obj.bCompleted || Obj.bHidden)
            {
                continue;
            }

            // Match by entity ID or by tile location.
            if (Obj.TargetEntityId == DefeatedEntityId ||
                (Obj.TargetTile == Tile && Obj.TargetEntityId == -1))
            {
                ReportObjectiveProgress(Q.QuestId, i, 1);
            }
        }
    }
}

void UCoMQuestSubsystem::OnCityCapture(int32 WizardId, int32 CityId)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active || Q.OfferedToWizardId != WizardId)
        {
            continue;
        }

        if (Q.QuestType != ECoMQuestType::ConquerCity)
        {
            continue;
        }

        for (int32 i = 0; i < Q.Objectives.Num(); ++i)
        {
            FCoMQuestObjective& Obj = Q.Objectives[i];
            if (Obj.bCompleted || Obj.bHidden)
            {
                continue;
            }
            if (Obj.TargetEntityId == CityId)
            {
                ReportObjectiveProgress(Q.QuestId, i, 1);
            }
        }
    }
}

void UCoMQuestSubsystem::OnDragonSlain(int32 WizardId, int32 DragonId)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active || Q.OfferedToWizardId != WizardId)
        {
            continue;
        }

        if (Q.QuestType != ECoMQuestType::SlayDragon)
        {
            continue;
        }

        for (int32 i = 0; i < Q.Objectives.Num(); ++i)
        {
            FCoMQuestObjective& Obj = Q.Objectives[i];
            if (Obj.bCompleted || Obj.bHidden)
            {
                continue;
            }
            if (Obj.TargetEntityId == DragonId || Obj.TargetEntityId == -1)
            {
                ReportObjectiveProgress(Q.QuestId, i, 1);
            }
        }
    }
}

void UCoMQuestSubsystem::OnAllianceFormed(int32 WizardId, int32 AllyWizardId)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active || Q.OfferedToWizardId != WizardId)
        {
            continue;
        }

        if (Q.QuestType != ECoMQuestType::FormAlliance &&
            Q.QuestType != ECoMQuestType::DiplomaticMission)
        {
            continue;
        }

        for (int32 i = 0; i < Q.Objectives.Num(); ++i)
        {
            FCoMQuestObjective& Obj = Q.Objectives[i];
            if (Obj.bCompleted || Obj.bHidden)
            {
                continue;
            }
            if (Obj.TargetWizardId == AllyWizardId || Obj.TargetWizardId == -1)
            {
                ReportObjectiveProgress(Q.QuestId, i, 1);
            }
        }
    }
}

void UCoMQuestSubsystem::OnSpellStolen(int32 WizardId, FName SpellName)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active || Q.OfferedToWizardId != WizardId)
        {
            continue;
        }

        if (Q.QuestType != ECoMQuestType::StealSpell)
        {
            continue;
        }

        for (int32 i = 0; i < Q.Objectives.Num(); ++i)
        {
            FCoMQuestObjective& Obj = Q.Objectives[i];
            if (Obj.bCompleted || Obj.bHidden)
            {
                continue;
            }
            // Match by description containing the spell name, or if target is generic.
            if (Obj.TargetEntityId == -1)
            {
                ReportObjectiveProgress(Q.QuestId, i, 1);
            }
        }
    }
}

void UCoMQuestSubsystem::OnRitualComplete(int32 WizardId, FName RitualId)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active || Q.OfferedToWizardId != WizardId)
        {
            continue;
        }

        if (Q.QuestType != ECoMQuestType::RitualChallenge)
        {
            continue;
        }

        for (int32 i = 0; i < Q.Objectives.Num(); ++i)
        {
            FCoMQuestObjective& Obj = Q.Objectives[i];
            if (Obj.bCompleted || Obj.bHidden)
            {
                continue;
            }
            // Generic ritual objective completion.
            ReportObjectiveProgress(Q.QuestId, i, 1);
        }
    }
}

void UCoMQuestSubsystem::OnCityDefended(int32 WizardId, int32 CityId, int32 TurnsHeld)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active || Q.OfferedToWizardId != WizardId)
        {
            continue;
        }

        if (Q.QuestType != ECoMQuestType::DefendCity)
        {
            continue;
        }

        for (int32 i = 0; i < Q.Objectives.Num(); ++i)
        {
            FCoMQuestObjective& Obj = Q.Objectives[i];
            if (Obj.bCompleted || Obj.bHidden)
            {
                continue;
            }
            if (Obj.TargetEntityId == CityId || Obj.TargetEntityId == -1)
            {
                // Report the number of turns held as progress.
                const int32 Delta = TurnsHeld - Obj.CurrentCount;
                if (Delta > 0)
                {
                    ReportObjectiveProgress(Q.QuestId, i, Delta);
                }
            }
        }
    }
}

// =====================================================================
// Configuration
// =====================================================================

void UCoMQuestSubsystem::SetQuestGenConfig(const FCoMQuestGenConfig& Config)
{
    GenConfig = Config;
}

FCoMQuestGenConfig UCoMQuestSubsystem::GetQuestGenConfig() const
{
    return GenConfig;
}

// =====================================================================
// Turn Processing
// =====================================================================

void UCoMQuestSubsystem::ProcessTurn(int32 CurrentTurn, int32 ActiveWizardCount)
{
    // 1. Expire unclaimed quests.
    ExpireQuests(CurrentTurn);

    // 2. Fail quests past their time limit.
    FailTimedOutQuests(CurrentTurn);

    // 3. Generate new quests at the configured interval.
    if (CurrentTurn - LastGenerationTurn >= GenConfig.QuestGenerationInterval)
    {
        LastGenerationTurn = CurrentTurn;

        // Generate quests only for active wizards, not all 14 slots.
        const int32 WizardCount = FMath::Clamp(ActiveWizardCount, 0, CoM::MAX_WIZARDS);
        for (int32 WizardId = 0; WizardId < WizardCount; ++WizardId)
        {
            const TArray<FCoMQuest> Available = GetAvailableQuests(WizardId);

            // Ensure minimum available quests per wizard.
            if (Available.Num() < CoMQuestConstants::MIN_AVAILABLE_PER_WIZARD)
            {
                const int32 ToGenerate = CoMQuestConstants::MIN_AVAILABLE_PER_WIZARD - Available.Num();
                for (int32 i = 0; i < ToGenerate; ++i)
                {
                    // Chance of chain quest.
                    const int32 ChainRoll = RngStream.RandRange(0, 99);
                    if (ChainRoll < GenConfig.ChainQuestChance)
                    {
                        const int32 ChainLen = RngStream.RandRange(
                            CoMQuestConstants::CHAIN_LENGTH_MIN,
                            CoMQuestConstants::CHAIN_LENGTH_MAX);
                        GenerateQuestChain(WizardId, CurrentTurn, ChainLen);
                    }
                    else
                    {
                        GenerateQuest(WizardId, CurrentTurn);
                    }
                }
            }

            // Standard generation: 1-2 new quests if below the pool cap.
            int32 TotalAvailable = 0;
            for (const auto& Pair : AllQuests)
            {
                if (Pair.Value.Status == ECoMQuestStatus::Available)
                {
                    ++TotalAvailable;
                }
            }

            if (TotalAvailable < GenConfig.MaxAvailableQuests)
            {
                const int32 ToGen = FMath::Min(
                    CoMQuestConstants::QUESTS_PER_GENERATION,
                    GenConfig.MaxAvailableQuests - TotalAvailable);

                for (int32 i = 0; i < ToGen; ++i)
                {
                    GenerateQuest(WizardId, CurrentTurn);
                }
            }
        }
    }
}

// =====================================================================
// Private: Quest Type Selection
// =====================================================================

ECoMQuestType UCoMQuestSubsystem::SelectQuestType(int32 WizardId, int32 CurrentTurn) const
{
    // Weighted random selection biased by game state approximations.
    // Without direct access to the world map subsystem, we use turn-based
    // heuristics: early game favors exploration, mid game combat, late game
    // diplomacy and planar quests.

    struct FWeightedType
    {
        ECoMQuestType Type;
        int32 Weight;
    };

    TArray<FWeightedType> Weights;

    // Base weights.
    Weights.Add({ ECoMQuestType::ExploreSite,       20 });
    Weights.Add({ ECoMQuestType::DefeatMonster,      15 });
    Weights.Add({ ECoMQuestType::RecoverArtifact,    12 });
    Weights.Add({ ECoMQuestType::ConquerCity,         8 });
    Weights.Add({ ECoMQuestType::ClearDungeon,       10 });
    Weights.Add({ ECoMQuestType::SlayDragon,          5 });
    Weights.Add({ ECoMQuestType::DefendCity,          5 });
    Weights.Add({ ECoMQuestType::EscortHero,          5 });
    Weights.Add({ ECoMQuestType::DiplomaticMission,   5 });
    Weights.Add({ ECoMQuestType::StealSpell,          3 });
    Weights.Add({ ECoMQuestType::RitualChallenge,     3 });
    Weights.Add({ ECoMQuestType::PlanarExpedition,    3 });
    Weights.Add({ ECoMQuestType::FormAlliance,        3 });
    Weights.Add({ ECoMQuestType::TameDragon,          2 });
    Weights.Add({ ECoMQuestType::ProphecyQuest,       1 });

    // Adjust by game phase (turn-based heuristic).
    if (CurrentTurn < 20)
    {
        // Early game: boost exploration, reduce combat.
        for (auto& W : Weights)
        {
            if (W.Type == ECoMQuestType::ExploreSite || W.Type == ECoMQuestType::ClearDungeon)
            {
                W.Weight += 10;
            }
            if (W.Type == ECoMQuestType::ConquerCity || W.Type == ECoMQuestType::PlanarExpedition)
            {
                W.Weight = FMath::Max(W.Weight - 5, 0);
            }
        }
    }
    else if (CurrentTurn > 80)
    {
        // Late game: boost high-end quests.
        for (auto& W : Weights)
        {
            if (W.Type == ECoMQuestType::SlayDragon || W.Type == ECoMQuestType::PlanarExpedition ||
                W.Type == ECoMQuestType::ProphecyQuest || W.Type == ECoMQuestType::TameDragon)
            {
                W.Weight += 8;
            }
            if (W.Type == ECoMQuestType::ExploreSite)
            {
                W.Weight = FMath::Max(W.Weight - 10, 2);
            }
        }
    }

    // Compute total weight.
    int32 TotalWeight = 0;
    for (const auto& W : Weights)
    {
        TotalWeight += W.Weight;
    }

    if (TotalWeight <= 0)
    {
        return ECoMQuestType::ExploreSite;
    }

    // NOTE: RngStream is mutable but this method is const. We use a local copy
    // seeded from the subsystem's stream state to maintain determinism while
    // respecting the const interface.
    FRandomStream LocalRng(RngStream.GetCurrentSeed() + CurrentTurn + WizardId);
    int32 Roll = LocalRng.RandRange(0, TotalWeight - 1);

    for (const auto& W : Weights)
    {
        Roll -= W.Weight;
        if (Roll < 0)
        {
            return W.Type;
        }
    }

    return ECoMQuestType::ExploreSite;
}

// =====================================================================
// Private: Objective Generation
// =====================================================================

TArray<FCoMQuestObjective> UCoMQuestSubsystem::GenerateObjectives(
    ECoMQuestType Type, int32 WizardId, int32 Difficulty, int32 CurrentTurn) const
{
    TArray<FCoMQuestObjective> Objectives;

    // Number of objectives scales with difficulty: 1 at low, up to 3 at high.
    int32 ObjCount = 1;
    if (Difficulty >= 4)
    {
        ObjCount = 2;
    }
    if (Difficulty >= 7)
    {
        ObjCount = 3;
    }

    FRandomStream LocalRng(RngStream.GetCurrentSeed() + CurrentTurn * 31 + WizardId * 7);

    for (int32 i = 0; i < ObjCount; ++i)
    {
        FCoMQuestObjective Obj;
        Obj.bHidden = (i > 0 && Difficulty >= 5); // Higher difficulty hides later objectives.

        switch (Type)
        {
        case ECoMQuestType::ExploreSite:
        case ECoMQuestType::ClearDungeon:
        {
            // Location-based objective.
            const int32 TileX = LocalRng.RandRange(0, CoM::MAP_WIDTH - 1);
            const int32 TileY = LocalRng.RandRange(0, CoM::MAP_HEIGHT - 1);
            Obj.TargetTile = FIntPoint(TileX, TileY);
            Obj.TargetPlane = static_cast<ECoMPlane>(LocalRng.RandRange(0, CoM::NUM_PLANES - 1));
            Obj.RequiredCount = 1;
            Obj.Description = (Type == ECoMQuestType::ExploreSite)
                ? FString::Printf(TEXT("Explore the site at (%d, %d)"), TileX, TileY)
                : FString::Printf(TEXT("Clear the dungeon at (%d, %d)"), TileX, TileY);
            break;
        }

        case ECoMQuestType::DefeatMonster:
        {
            Obj.TargetEntityId = LocalRng.RandRange(100, 999);
            Obj.RequiredCount = 1 + i; // Later objectives may require more kills.
            Obj.Description = FString::Printf(TEXT("Defeat the creature (ID %d)"), Obj.TargetEntityId);
            break;
        }

        case ECoMQuestType::RecoverArtifact:
        {
            const int32 TileX = LocalRng.RandRange(0, CoM::MAP_WIDTH - 1);
            const int32 TileY = LocalRng.RandRange(0, CoM::MAP_HEIGHT - 1);
            Obj.TargetTile = FIntPoint(TileX, TileY);
            Obj.TargetPlane = static_cast<ECoMPlane>(LocalRng.RandRange(0, CoM::NUM_PLANES - 1));
            Obj.RequiredCount = 1;
            Obj.Description = FString::Printf(TEXT("Recover the artifact at (%d, %d)"), TileX, TileY);
            break;
        }

        case ECoMQuestType::EscortHero:
        {
            const int32 DestX = LocalRng.RandRange(0, CoM::MAP_WIDTH - 1);
            const int32 DestY = LocalRng.RandRange(0, CoM::MAP_HEIGHT - 1);
            Obj.TargetTile = FIntPoint(DestX, DestY);
            Obj.TargetPlane = static_cast<ECoMPlane>(LocalRng.RandRange(0, CoM::NUM_PLANES - 1));
            Obj.RequiredCount = 1;
            Obj.Description = FString::Printf(TEXT("Escort the hero to (%d, %d)"), DestX, DestY);
            break;
        }

        case ECoMQuestType::ConquerCity:
        {
            Obj.TargetEntityId = LocalRng.RandRange(1, 50); // Placeholder city ID.
            Obj.RequiredCount = 1;
            Obj.Description = FString::Printf(TEXT("Capture city %d"), Obj.TargetEntityId);
            break;
        }

        case ECoMQuestType::DiplomaticMission:
        case ECoMQuestType::FormAlliance:
        {
            Obj.TargetWizardId = LocalRng.RandRange(0, CoM::MAX_WIZARDS - 1);
            // Avoid self-targeting.
            if (Obj.TargetWizardId == WizardId)
            {
                Obj.TargetWizardId = (WizardId + 1) % CoM::MAX_WIZARDS;
            }
            Obj.RequiredCount = 1;
            Obj.Description = (Type == ECoMQuestType::FormAlliance)
                ? FString::Printf(TEXT("Form an alliance with wizard %d"), Obj.TargetWizardId)
                : FString::Printf(TEXT("Complete diplomatic mission with wizard %d"), Obj.TargetWizardId);
            break;
        }

        case ECoMQuestType::StealSpell:
        {
            Obj.TargetWizardId = LocalRng.RandRange(0, CoM::MAX_WIZARDS - 1);
            if (Obj.TargetWizardId == WizardId)
            {
                Obj.TargetWizardId = (WizardId + 1) % CoM::MAX_WIZARDS;
            }
            Obj.RequiredCount = 1;
            Obj.Description = FString::Printf(TEXT("Steal a spell from wizard %d"), Obj.TargetWizardId);
            break;
        }

        case ECoMQuestType::SlayDragon:
        case ECoMQuestType::TameDragon:
        {
            Obj.TargetEntityId = LocalRng.RandRange(1, 20); // Placeholder dragon ID.
            Obj.RequiredCount = 1;
            Obj.Description = (Type == ECoMQuestType::SlayDragon)
                ? FString::Printf(TEXT("Slay the dragon (ID %d)"), Obj.TargetEntityId)
                : FString::Printf(TEXT("Tame the dragon (ID %d)"), Obj.TargetEntityId);
            break;
        }

        case ECoMQuestType::PlanarExpedition:
        {
            Obj.TargetPlane = static_cast<ECoMPlane>(LocalRng.RandRange(0, CoM::NUM_PLANES - 1));
            const int32 TileX = LocalRng.RandRange(0, CoM::MAP_WIDTH - 1);
            const int32 TileY = LocalRng.RandRange(0, CoM::MAP_HEIGHT - 1);
            Obj.TargetTile = FIntPoint(TileX, TileY);
            Obj.RequiredCount = 1;
            Obj.Description = FString::Printf(TEXT("Lead an expedition to (%d, %d) on another plane"), TileX, TileY);
            break;
        }

        case ECoMQuestType::RitualChallenge:
        {
            Obj.RequiredCount = 1;
            Obj.Description = TEXT("Complete the ritual challenge");
            break;
        }

        case ECoMQuestType::DefendCity:
        {
            Obj.TargetEntityId = LocalRng.RandRange(1, 50); // Placeholder city ID.
            Obj.RequiredCount = 3 + Difficulty; // Hold for N turns.
            Obj.Description = FString::Printf(TEXT("Defend city %d for %d turns"), Obj.TargetEntityId, Obj.RequiredCount);
            break;
        }

        case ECoMQuestType::ProphecyQuest:
        {
            const int32 TileX = LocalRng.RandRange(0, CoM::MAP_WIDTH - 1);
            const int32 TileY = LocalRng.RandRange(0, CoM::MAP_HEIGHT - 1);
            Obj.TargetTile = FIntPoint(TileX, TileY);
            Obj.TargetPlane = static_cast<ECoMPlane>(LocalRng.RandRange(0, CoM::NUM_PLANES - 1));
            Obj.RequiredCount = 1;
            Obj.Description = FString::Printf(TEXT("Fulfill the prophecy at (%d, %d)"), TileX, TileY);
            break;
        }

        default:
        {
            Obj.RequiredCount = 1;
            Obj.Description = TEXT("Complete the objective");
            break;
        }
        }

        Objectives.Add(Obj);
    }

    return Objectives;
}

// =====================================================================
// Private: Reward Generation
// =====================================================================

TArray<FCoMQuestReward> UCoMQuestSubsystem::GenerateRewards(ECoMQuestType Type, int32 Difficulty) const
{
    TArray<FCoMQuestReward> Rewards;

    FRandomStream LocalRng(RngStream.GetCurrentSeed() + Difficulty * 17 + static_cast<int32>(Type) * 53);

    // Tier 1 (difficulty 1-3): gold and mana.
    // Tier 2 (difficulty 4-6): artifacts and heroes.
    // Tier 3 (difficulty 7-10): unique spells, dragon eggs, spell books.

    // Every quest gives gold.
    {
        FCoMQuestReward GoldReward;
        GoldReward.Category = ECoMRewardCategory::Resources;
        GoldReward.ResourceType = ECoMResource::GoldOre;
        GoldReward.Amount = CoMQuestConstants::GOLD_PER_DIFFICULTY * Difficulty;
        GoldReward.Description = FString::Printf(TEXT("%d Gold"), GoldReward.Amount);
        Rewards.Add(GoldReward);
    }

    // Difficulty 2+: mana.
    if (Difficulty >= 2)
    {
        FCoMQuestReward ManaReward;
        ManaReward.Category = ECoMRewardCategory::Resources;
        ManaReward.ResourceType = ECoMResource::Aetherium;
        ManaReward.Amount = CoMQuestConstants::MANA_PER_DIFFICULTY * Difficulty;
        ManaReward.Description = FString::Printf(TEXT("%d Mana"), ManaReward.Amount);
        Rewards.Add(ManaReward);
    }

    // Difficulty 3+: fame.
    if (Difficulty >= 3)
    {
        FCoMQuestReward FameReward;
        FameReward.Category = ECoMRewardCategory::Fame;
        FameReward.Amount = CoMQuestConstants::FAME_PER_DIFFICULTY * Difficulty;
        FameReward.Description = FString::Printf(TEXT("%d Fame"), FameReward.Amount);
        Rewards.Add(FameReward);
    }

    // Tier 2 rewards.
    if (Difficulty >= 4)
    {
        const int32 Roll = LocalRng.RandRange(0, 99);
        if (Roll < 50 || Type == ECoMQuestType::RecoverArtifact)
        {
            FCoMQuestReward ArtifactReward;
            ArtifactReward.Category = ECoMRewardCategory::Artifact;
            ArtifactReward.RewardId = LocalRng.RandRange(1000, 9999); // Placeholder artifact ID.
            ArtifactReward.Description = TEXT("A powerful artifact");
            Rewards.Add(ArtifactReward);
        }
        else
        {
            FCoMQuestReward HeroReward;
            HeroReward.Category = ECoMRewardCategory::Hero;
            HeroReward.RewardId = LocalRng.RandRange(1, 100); // Placeholder hero spec ID.
            HeroReward.Description = TEXT("A hero joins your cause");
            Rewards.Add(HeroReward);
        }
    }

    // Tier 3 rewards.
    if (Difficulty >= 7)
    {
        const int32 Roll = LocalRng.RandRange(0, 99);
        if (Roll < 25 || Type == ECoMQuestType::SlayDragon || Type == ECoMQuestType::TameDragon)
        {
            FCoMQuestReward DragonEggReward;
            DragonEggReward.Category = ECoMRewardCategory::DragonEgg;
            DragonEggReward.RewardId = LocalRng.RandRange(1, 8); // Dragon type.
            DragonEggReward.Description = TEXT("A dragon egg");
            Rewards.Add(DragonEggReward);
        }
        else if (Roll < 50)
        {
            FCoMQuestReward SpellReward;
            SpellReward.Category = ECoMRewardCategory::UniqueSpell;
            SpellReward.SpellName = FName(TEXT("QuestSpell_Unique"));
            SpellReward.Description = TEXT("A unique spell");
            Rewards.Add(SpellReward);
        }
        else if (Roll < 75)
        {
            FCoMQuestReward SpellBookReward;
            SpellBookReward.Category = ECoMRewardCategory::SpellBook;
            SpellBookReward.RewardId = LocalRng.RandRange(0, CoM::NUM_SPELL_REALMS - 1);
            SpellBookReward.Description = TEXT("A spell book");
            Rewards.Add(SpellBookReward);
        }
        else
        {
            FCoMQuestReward RetortReward;
            RetortReward.Category = ECoMRewardCategory::Retort;
            RetortReward.RewardId = LocalRng.RandRange(1, 20); // Placeholder retort ID.
            RetortReward.Description = TEXT("A unique wizard retort");
            Rewards.Add(RetortReward);
        }
    }

    return Rewards;
}

// =====================================================================
// Private: Quest Text Generation
// =====================================================================

void UCoMQuestSubsystem::GenerateQuestText(FCoMQuest& Quest) const
{
    FRandomStream LocalRng(RngStream.GetCurrentSeed() + Quest.QuestId * 13);

    const int32 PrefixIdx = LocalRng.RandRange(0, CoMQuestText::NumPrefixes - 1);
    const int32 NounIdx   = LocalRng.RandRange(0, CoMQuestText::NumNouns - 1);
    const int32 AdjIdx    = LocalRng.RandRange(0, CoMQuestText::NumAdjectives - 1);
    const int32 CreatureIdx = LocalRng.RandRange(0, CoMQuestText::NumCreatures - 1);

    switch (Quest.QuestType)
    {
    case ECoMQuestType::ExploreSite:
        Quest.Title = FString::Printf(TEXT("Explore the %s %s"),
            *CoMQuestText::Adjectives[AdjIdx], *CoMQuestText::Nouns[NounIdx]);
        Quest.Description = FString::Printf(
            TEXT("Venture into the %s %s and uncover its secrets."),
            *CoMQuestText::Adjectives[AdjIdx], *CoMQuestText::Nouns[NounIdx]);
        break;

    case ECoMQuestType::DefeatMonster:
        Quest.Title = FString::Printf(TEXT("Hunt the %s"),
            *CoMQuestText::Creatures[CreatureIdx]);
        Quest.Description = FString::Printf(
            TEXT("A terrible %s threatens the land. Hunt it down and destroy it."),
            *CoMQuestText::Creatures[CreatureIdx]);
        break;

    case ECoMQuestType::RecoverArtifact:
        Quest.Title = FString::Printf(TEXT("%s %s"),
            *CoMQuestText::Prefixes[PrefixIdx], *CoMQuestText::Nouns[NounIdx]);
        Quest.Description = FString::Printf(
            TEXT("Recover %s %s from the depths where it was lost."),
            *CoMQuestText::Prefixes[PrefixIdx], *CoMQuestText::Nouns[NounIdx]);
        break;

    case ECoMQuestType::EscortHero:
        Quest.Title = TEXT("The Guardian's Path");
        Quest.Description = TEXT("Escort a hero safely to their destination through hostile territory.");
        break;

    case ECoMQuestType::ConquerCity:
        Quest.Title = FString::Printf(TEXT("Siege of the %s %s"),
            *CoMQuestText::Adjectives[AdjIdx], *CoMQuestText::Nouns[NounIdx]);
        Quest.Description = FString::Printf(
            TEXT("Lay siege to and capture the fortified %s %s."),
            *CoMQuestText::Adjectives[AdjIdx], *CoMQuestText::Nouns[NounIdx]);
        break;

    case ECoMQuestType::DiplomaticMission:
        Quest.Title = TEXT("Shadow's Bargain");
        Quest.Description = TEXT("Negotiate a diplomatic accord with a rival wizard through cunning or force.");
        break;

    case ECoMQuestType::StealSpell:
        Quest.Title = FString::Printf(TEXT("%s %s"),
            *CoMQuestText::Prefixes[PrefixIdx], *CoMQuestText::Nouns[NounIdx]);
        Quest.Description = TEXT("Infiltrate a rival's sanctum and steal their arcane secrets.");
        break;

    case ECoMQuestType::SlayDragon:
        Quest.Title = FString::Printf(TEXT("Slay the %s Dragon"),
            *CoMQuestText::Adjectives[AdjIdx]);
        Quest.Description = FString::Printf(
            TEXT("A %s dragon has claimed a domain of its own. End its reign."),
            *CoMQuestText::Adjectives[AdjIdx]);
        break;

    case ECoMQuestType::PlanarExpedition:
    {
        const int32 PlaneIdx = LocalRng.RandRange(0, CoMQuestText::NumPlaneNames - 1);
        Quest.Title = FString::Printf(TEXT("Expedition to %s"),
            *CoMQuestText::PlaneNames[PlaneIdx]);
        Quest.Description = FString::Printf(
            TEXT("Lead an expedition into the plane of %s and survive its trials."),
            *CoMQuestText::PlaneNames[PlaneIdx]);
        break;
    }

    case ECoMQuestType::RitualChallenge:
        Quest.Title = FString::Printf(TEXT("The %s Ritual"),
            *CoMQuestText::Adjectives[AdjIdx]);
        Quest.Description = FString::Printf(
            TEXT("Complete the %s ritual before its power fades."),
            *CoMQuestText::Adjectives[AdjIdx]);
        break;

    case ECoMQuestType::DefendCity:
        Quest.Title = FString::Printf(TEXT("Defend the %s"),
            *CoMQuestText::Nouns[NounIdx]);
        Quest.Description = FString::Printf(
            TEXT("Hold the %s against relentless assault for %d turns."),
            *CoMQuestText::Nouns[NounIdx], Quest.TurnLimit > 0 ? Quest.TurnLimit : 5);
        break;

    case ECoMQuestType::ClearDungeon:
        Quest.Title = FString::Printf(TEXT("Depths of the %s %s"),
            *CoMQuestText::Adjectives[AdjIdx], *CoMQuestText::Nouns[NounIdx]);
        Quest.Description = FString::Printf(
            TEXT("Descend into the %s %s and clear all threats within."),
            *CoMQuestText::Adjectives[AdjIdx], *CoMQuestText::Nouns[NounIdx]);
        break;

    case ECoMQuestType::FormAlliance:
        Quest.Title = TEXT("The Pact of Unity");
        Quest.Description = TEXT("Form a lasting alliance with a rival wizard through shared purpose.");
        break;

    case ECoMQuestType::TameDragon:
        Quest.Title = FString::Printf(TEXT("Tame the %s Dragon"),
            *CoMQuestText::Adjectives[AdjIdx]);
        Quest.Description = FString::Printf(
            TEXT("Approach the %s dragon and bend it to your will through magic or might."),
            *CoMQuestText::Adjectives[AdjIdx]);
        break;

    case ECoMQuestType::ProphecyQuest:
        Quest.Title = FString::Printf(TEXT("The Prophecy of %s"),
            *CoMQuestText::Nouns[NounIdx]);
        Quest.Description = FString::Printf(
            TEXT("An ancient prophecy speaks of the %s. Fulfill its demands before it is too late."),
            *CoMQuestText::Nouns[NounIdx]);
        break;

    default:
        Quest.Title = TEXT("A Mysterious Quest");
        Quest.Description = TEXT("A mysterious task awaits.");
        break;
    }
}

FString UCoMQuestSubsystem::GenerateLoreText(ECoMQuestType Type, int32 Difficulty) const
{
    FRandomStream LocalRng(RngStream.GetCurrentSeed() + Difficulty * 41 + static_cast<int32>(Type) * 29);
    const int32 Idx = LocalRng.RandRange(0, CoMQuestText::NumLorePhrases - 1);
    return CoMQuestText::LorePhrases[Idx];
}

// =====================================================================
// Private: Quest Completion
// =====================================================================

void UCoMQuestSubsystem::CheckQuestCompletion(int32 QuestId)
{
    FCoMQuest* Quest = AllQuests.Find(QuestId);
    if (!Quest)
    {
        return;
    }

    if (Quest->Status != ECoMQuestStatus::Active)
    {
        return;
    }

    // Check if all objectives are completed.
    bool bAllComplete = true;
    for (const FCoMQuestObjective& Obj : Quest->Objectives)
    {
        if (!Obj.bCompleted)
        {
            bAllComplete = false;
            break;
        }
    }

    if (!bAllComplete)
    {
        return;
    }

    Quest->Status = ECoMQuestStatus::Completed;

    const int32 WizardId = Quest->OfferedToWizardId;
    if (WizardId >= 0)
    {
        GrantRewards(WizardId, *Quest);
    }

    OnQuestCompleted.Broadcast(QuestId, WizardId);

    // Chain: trigger next quest.
    if (Quest->NextQuestInChainId > 0)
    {
        FCoMQuest* NextQuest = AllQuests.Find(Quest->NextQuestInChainId);
        if (NextQuest && NextQuest->Status == ECoMQuestStatus::Available)
        {
            NextQuest->OfferedToWizardId = WizardId;
            OnQuestAvailable.Broadcast(NextQuest->QuestId, WizardId);
        }
    }
}

// =====================================================================
// Private: Reward Granting
// =====================================================================

void UCoMQuestSubsystem::GrantRewards(int32 WizardId, const FCoMQuest& Quest)
{
    for (const FCoMQuestReward& Reward : Quest.Rewards)
    {
        switch (Reward.Category)
        {
        case ECoMRewardCategory::Resources:
            // Integration point: add to resource subsystem.
            // For now, broadcast the reward event with the amount.
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting %d %s to wizard %d"),
                Quest.QuestId, Reward.Amount,
                Reward.ResourceType == ECoMResource::GoldOre ? TEXT("Gold") : TEXT("Mana"),
                WizardId);
            break;

        case ECoMRewardCategory::Artifact:
            // Integration point: create artifact instance.
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting artifact %d to wizard %d"),
                Quest.QuestId, Reward.RewardId, WizardId);
            break;

        case ECoMRewardCategory::Hero:
            // Integration point: spawn hero unit.
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting hero spec %d to wizard %d"),
                Quest.QuestId, Reward.RewardId, WizardId);
            break;

        case ECoMRewardCategory::UniqueSpell:
            // Integration point: add to wizard's known spells.
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting unique spell '%s' to wizard %d"),
                Quest.QuestId, *Reward.SpellName.ToString(), WizardId);
            break;

        case ECoMRewardCategory::CityAlliance:
            // Integration point: set city to allied.
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting city alliance %d to wizard %d"),
                Quest.QuestId, Reward.RewardId, WizardId);
            break;

        case ECoMRewardCategory::DragonEgg:
            // Integration point: create egg via dragon subsystem.
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting dragon egg type %d to wizard %d"),
                Quest.QuestId, Reward.RewardId, WizardId);
            break;

        case ECoMRewardCategory::Fame:
            // Integration point: add fame points.
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting %d fame to wizard %d"),
                Quest.QuestId, Reward.Amount, WizardId);
            break;

        case ECoMRewardCategory::SpellBook:
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting spell book realm %d to wizard %d"),
                Quest.QuestId, Reward.RewardId, WizardId);
            break;

        case ECoMRewardCategory::Retort:
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting retort %d to wizard %d"),
                Quest.QuestId, Reward.RewardId, WizardId);
            break;

        case ECoMRewardCategory::Territory:
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting territory to wizard %d"),
                Quest.QuestId, WizardId);
            break;

        case ECoMRewardCategory::UnitUnlock:
            UE_LOG(LogTemp, Log, TEXT("Quest %d: Granting unit unlock %d to wizard %d"),
                Quest.QuestId, Reward.RewardId, WizardId);
            break;

        default:
            break;
        }

        OnQuestRewardGranted.Broadcast(Quest.QuestId, Reward);
    }
}

// =====================================================================
// Private: Expiry and Failure
// =====================================================================

void UCoMQuestSubsystem::ExpireQuests(int32 CurrentTurn)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Available)
        {
            continue;
        }

        if (Q.ExpiryTurn >= 0 && CurrentTurn >= Q.ExpiryTurn)
        {
            Q.Status = ECoMQuestStatus::Expired;
        }
    }
}

void UCoMQuestSubsystem::FailTimedOutQuests(int32 CurrentTurn)
{
    for (auto& Pair : AllQuests)
    {
        FCoMQuest& Q = Pair.Value;
        if (Q.Status != ECoMQuestStatus::Active)
        {
            continue;
        }

        if (Q.TurnLimit > 0 && Q.AcceptedTurn >= 0)
        {
            const int32 TurnsElapsed = CurrentTurn - Q.AcceptedTurn;
            if (TurnsElapsed >= Q.TurnLimit)
            {
                Q.Status = ECoMQuestStatus::Failed;
                OnQuestFailed.Broadcast(Q.QuestId, Q.OfferedToWizardId);
            }
        }
    }
}

// =====================================================================
// Private: Difficulty Calculation
// =====================================================================

int32 UCoMQuestSubsystem::CalculateDifficulty(int32 WizardId, int32 CurrentTurn) const
{
    // Base difficulty scales with turn count.
    int32 BaseDiff = 1 + CurrentTurn / CoMQuestConstants::DIFFICULTY_TURN_DIVISOR;

    // Add some randomness.
    FRandomStream LocalRng(RngStream.GetCurrentSeed() + CurrentTurn * 3 + WizardId);
    BaseDiff += LocalRng.RandRange(-1, 1);

    return FMath::Clamp(BaseDiff, GenConfig.MinDifficulty, GenConfig.MaxDifficulty);
}

// =====================================================================
// Private: Target Finders (placeholder implementations)
// =====================================================================

FIntPoint UCoMQuestSubsystem::FindExplorationTarget(int32 WizardId, ECoMPlane& OutPlane) const
{
    // Placeholder: generate a random tile. Real implementation would query the
    // world map subsystem for unexplored sites near the wizard's territory.
    FRandomStream LocalRng(RngStream.GetCurrentSeed() + WizardId * 11);
    OutPlane = static_cast<ECoMPlane>(LocalRng.RandRange(0, CoM::NUM_PLANES - 1));
    return FIntPoint(
        LocalRng.RandRange(0, CoM::MAP_WIDTH - 1),
        LocalRng.RandRange(0, CoM::MAP_HEIGHT - 1));
}

int32 UCoMQuestSubsystem::FindDragonTarget(int32 WizardId) const
{
    // Placeholder: return a random dragon ID. Real implementation would query
    // the dragon subsystem for wild dragons.
    FRandomStream LocalRng(RngStream.GetCurrentSeed() + WizardId * 23);
    return LocalRng.RandRange(1, 20);
}

int32 UCoMQuestSubsystem::FindNeutralCityTarget(int32 WizardId) const
{
    // Placeholder: return a random city ID.
    FRandomStream LocalRng(RngStream.GetCurrentSeed() + WizardId * 37);
    return LocalRng.RandRange(1, 50);
}

int32 UCoMQuestSubsystem::FindRivalWizardTarget(int32 WizardId) const
{
    // Return any wizard that is not the requesting wizard.
    FRandomStream LocalRng(RngStream.GetCurrentSeed() + WizardId * 43);
    int32 Rival = LocalRng.RandRange(0, CoM::MAX_WIZARDS - 1);
    if (Rival == WizardId)
    {
        Rival = (WizardId + 1) % CoM::MAX_WIZARDS;
    }
    return Rival;
}

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load Export/Import
// ─────────────────────────────────────────────────────────────────────────────

void UCoMQuestSubsystem::ExportAll(TArray<FCoMQuest>& OutQuests, FCoMQuestGenConfig& OutConfig) const
{
    OutQuests.Empty();
    OutQuests.Reserve(AllQuests.Num());
    for (const auto& Pair : AllQuests)
    {
        OutQuests.Add(Pair.Value);
    }
    OutConfig = GenConfig;
}

void UCoMQuestSubsystem::ImportAll(const TArray<FCoMQuest>& InQuests, const FCoMQuestGenConfig& InConfig)
{
    AllQuests.Empty();
    NextQuestId = 1;
    NextChainId = 1;
    for (const FCoMQuest& Quest : InQuests)
    {
        AllQuests.Add(Quest.QuestId, Quest);
        if (Quest.QuestId >= NextQuestId)
        {
            NextQuestId = Quest.QuestId + 1;
        }
        if (Quest.ChainId >= NextChainId)
        {
            NextChainId = Quest.ChainId + 1;
        }
    }
    GenConfig = InConfig;

    UE_LOG(LogTemp, Log, TEXT("[QuestSubsystem] ImportAll: %d quests imported."), AllQuests.Num());
}
