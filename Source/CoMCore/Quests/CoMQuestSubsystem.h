// Copyright Mythforge Studios. All Rights Reserved.
// CoMQuestSubsystem.h — Procedural quest generation, tracking, completion, rewards
// Phase 8 — Shattered Arcana

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMQuestSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// QUEST TYPE
// ─────────────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class ECoMQuestType : uint8
{
    ExploreSite       UMETA(DisplayName = "Explore Site"),         // Visit and clear an explorable site
    DefeatMonster     UMETA(DisplayName = "Defeat Monster"),       // Slay a specific creature/army
    RecoverArtifact   UMETA(DisplayName = "Recover Artifact"),     // Find an artifact at a location
    EscortHero        UMETA(DisplayName = "Escort Hero"),          // Get a hero safely to a destination
    ConquerCity       UMETA(DisplayName = "Conquer City"),         // Capture a specific neutral/enemy city
    DiplomaticMission UMETA(DisplayName = "Diplomatic Mission"),   // Achieve a diplomatic goal
    StealSpell        UMETA(DisplayName = "Steal Spell"),          // Espionage: steal a spell from a rival
    SlayDragon        UMETA(DisplayName = "Slay Dragon"),          // Kill a wild dragon in its domain
    PlanarExpedition  UMETA(DisplayName = "Planar Expedition"),    // Send army to another plane and survive
    RitualChallenge   UMETA(DisplayName = "Ritual Challenge"),     // Complete a ritual under constraints
    DefendCity        UMETA(DisplayName = "Defend City"),          // Hold a city for N turns against attack
    ClearDungeon      UMETA(DisplayName = "Clear Dungeon"),        // Defeat all encounters in a dungeon
    FormAlliance      UMETA(DisplayName = "Form Alliance"),        // Ally with a specific wizard
    TameDragon        UMETA(DisplayName = "Tame Dragon"),          // Claim a wild dragon as companion
    ProphecyQuest     UMETA(DisplayName = "Prophecy Quest"),       // Multi-step chain quest with branching
    MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// QUEST STATUS
// ─────────────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class ECoMQuestStatus : uint8
{
    Available   UMETA(DisplayName = "Available"),    // Offered but not accepted
    Active      UMETA(DisplayName = "Active"),       // Accepted, in progress
    Completed   UMETA(DisplayName = "Completed"),    // Objectives met, rewards given
    Failed      UMETA(DisplayName = "Failed"),       // Time expired or objective lost
    Expired     UMETA(DisplayName = "Expired"),      // Never accepted, removed
    MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// QUEST REWARD
// ─────────────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class ECoMRewardCategory : uint8
{
    Artifact        UMETA(DisplayName = "Artifact"),        // Unique magic item
    Hero            UMETA(DisplayName = "Hero"),            // Recruitable hero joins
    UniqueSpell     UMETA(DisplayName = "Unique Spell"),    // Spell not available through research
    Resources       UMETA(DisplayName = "Resources"),       // Gold, mana, materials
    CityAlliance    UMETA(DisplayName = "City Alliance"),   // Neutral city becomes allied
    DragonEgg       UMETA(DisplayName = "Dragon Egg"),      // Hatchable dragon egg
    Territory       UMETA(DisplayName = "Territory"),       // Control of tiles/region
    Fame            UMETA(DisplayName = "Fame"),            // Fame points for hero recruitment
    SpellBook       UMETA(DisplayName = "Spell Book"),      // Additional spell book in a realm
    Retort          UMETA(DisplayName = "Retort"),          // Unique wizard ability
    UnitUnlock      UMETA(DisplayName = "Unit Unlock"),     // Unlock a special unit type
    MAX UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FCoMQuestReward
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMRewardCategory Category = ECoMRewardCategory::Resources;

    /** Reward-specific ID: artifact ID, hero spec ID, spell FName as int hash, etc. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RewardId = 0;

    /** For resources: amount of gold/mana/etc. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Amount = 0;

    /** Resource type (for resource rewards). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMResource ResourceType = ECoMResource::GoldOre;

    /** Spell name (for unique spell rewards). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SpellName;

    /** Display description of the reward. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;
};

// ─────────────────────────────────────────────────────────────────────────────
// QUEST OBJECTIVE
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMQuestObjective
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    /** Target tile (for location-based objectives). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint TargetTile = FIntPoint(-1, -1);

    /** Target plane. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMPlane TargetPlane = ECoMPlane::Aurelith;

    /** Target entity ID (city, unit, dragon, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetEntityId = -1;

    /** Target wizard (for diplomatic/espionage quests). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetWizardId = -1;

    /** Required count (e.g., kill 3 monsters, hold for 5 turns). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RequiredCount = 1;

    /** Current progress. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentCount = 0;

    /** Whether this objective is completed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCompleted = false;

    /** Optional: if true, this objective is hidden until a prior objective completes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHidden = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// QUEST INSTANCE
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMQuest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 QuestId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMQuestType QuestType = ECoMQuestType::ExploreSite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMQuestStatus Status = ECoMQuestStatus::Available;

    /** Wizard this quest is offered to (-1 = available to all). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 OfferedToWizardId = -1;

    /** Hero who discovered/offers this quest (-1 = system-generated). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 QuestGiverHeroId = -1;

    /** Ordered list of objectives (may include hidden/branching steps). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FCoMQuestObjective> Objectives;

    /** Rewards granted on completion. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FCoMQuestReward> Rewards;

    /** Turn this quest was generated. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GeneratedTurn = 0;

    /** Turn this quest was accepted. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AcceptedTurn = -1;

    /** Turn this quest expires if not accepted (-1 = no expiry). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ExpiryTurn = -1;

    /** Turn limit after acceptance (-1 = no time limit). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TurnLimit = -1;

    /** Difficulty rating 1-10 (affects reward quality). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Difficulty = 1;

    /** If part of a chain, the ID of the next quest triggered on completion. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NextQuestInChainId = -1;

    /** Chain ID — all quests in the same chain share this. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ChainId = -1;

    /** Whether this is a repeatable quest. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRepeatable = false;

    /** Lore/flavor text shown on quest details screen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LoreText;
};

// ─────────────────────────────────────────────────────────────────────────────
// QUEST GENERATION CONFIG
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMQuestGenConfig
{
    GENERATED_BODY()

    /** Maximum active quests per wizard. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxActiveQuestsPerWizard = 5;

    /** Maximum available (unaccepted) quests in the pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxAvailableQuests = 20;

    /** Turns between new quest generation attempts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 QuestGenerationInterval = 5;

    /** Minimum difficulty for quests (scales with game turn). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinDifficulty = 1;

    /** Maximum difficulty. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxDifficulty = 10;

    /** Whether heroes in taverns can generate quests. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHeroQuestRumors = true;

    /** Probability (0-100) of a quest chain instead of standalone. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ChainQuestChance = 20;

    /** Number of turns before unclaimed quests expire. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 DefaultExpiryTurns = 15;
};

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATES
// ─────────────────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestAvailable, int32, QuestId, int32, WizardId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestAccepted, int32, QuestId, int32, WizardId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestCompleted, int32, QuestId, int32, WizardId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestFailed, int32, QuestId, int32, WizardId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnQuestObjectiveProgress, int32, QuestId, int32, ObjectiveIndex, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestRewardGranted, int32, QuestId, FCoMQuestReward, Reward);

// ─────────────────────────────────────────────────────────────────────────────
// QUEST SUBSYSTEM
// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class COMCORE_API UCoMQuestSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ── Quest Lifecycle ──────────────────────────────────────────────────────

    /** Accept an available quest. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    bool AcceptQuest(int32 QuestId, int32 WizardId);

    /** Abandon an active quest. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void AbandonQuest(int32 QuestId);

    /** Report progress on a quest objective (e.g., monster killed, tile visited). */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void ReportObjectiveProgress(int32 QuestId, int32 ObjectiveIndex, int32 ProgressDelta = 1);

    /** Force-complete a quest (debug/cheat). */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void ForceCompleteQuest(int32 QuestId);

    // ── Quest Queries ────────────────────────────────────────────────────────

    /** Get all available quests for a wizard. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    TArray<FCoMQuest> GetAvailableQuests(int32 WizardId) const;

    /** Get all active quests for a wizard. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    TArray<FCoMQuest> GetActiveQuests(int32 WizardId) const;

    /** Get completed quests for a wizard. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    TArray<FCoMQuest> GetCompletedQuests(int32 WizardId) const;

    /** Get a specific quest by ID. */
    const FCoMQuest* GetQuest(int32 QuestId) const;

    /** Get active quest count for a wizard. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    int32 GetActiveQuestCount(int32 WizardId) const;

    /** Check if a specific tile is a quest objective target. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    bool IsTileQuestTarget(int32 WizardId, ECoMPlane Plane, FIntPoint Tile) const;

    // ── Quest Generation ─────────────────────────────────────────────────────

    /** Manually trigger quest generation for a wizard. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    int32 GenerateQuest(int32 WizardId, int32 CurrentTurn);

    /** Generate a quest of a specific type (for testing/scripting). */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    int32 GenerateQuestOfType(ECoMQuestType Type, int32 WizardId, int32 CurrentTurn);

    /** Generate a quest chain (multi-step quest with branching). */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    int32 GenerateQuestChain(int32 WizardId, int32 CurrentTurn, int32 ChainLength = 3);

    /** Generate a hero quest rumor (from a hero in a tavern). */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    int32 GenerateHeroQuestRumor(int32 HeroId, int32 WizardId, int32 CurrentTurn);

    // ── Event Hooks (called by other subsystems) ─────────────────────────────

    /** Called when a unit visits a tile — may trigger explore quests. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void OnTileVisited(int32 WizardId, ECoMPlane Plane, FIntPoint Tile);

    /** Called when a combat is won — may trigger defeat/slay quests. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void OnCombatVictory(int32 WizardId, int32 DefeatedEntityId, FIntPoint Tile);

    /** Called when a city is captured — may trigger conquer quests. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void OnCityCapture(int32 WizardId, int32 CityId);

    /** Called when a dragon is slain — may trigger slay dragon quests. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void OnDragonSlain(int32 WizardId, int32 DragonId);

    /** Called when an alliance is formed — may trigger diplomatic quests. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void OnAllianceFormed(int32 WizardId, int32 AllyWizardId);

    /** Called when a spell is stolen — may trigger steal quests. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void OnSpellStolen(int32 WizardId, FName SpellName);

    /** Called when a ritual completes — may trigger ritual quests. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void OnRitualComplete(int32 WizardId, FName RitualId);

    /** Called when a city is defended for N turns — may trigger defend quests. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void OnCityDefended(int32 WizardId, int32 CityId, int32 TurnsHeld);

    // ── Configuration ────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Quests")
    void SetQuestGenConfig(const FCoMQuestGenConfig& Config);

    UFUNCTION(BlueprintCallable, Category = "Quests")
    FCoMQuestGenConfig GetQuestGenConfig() const;

    // ── Turn Processing ──────────────────────────────────────────────────────

    /** Process quests for the turn: generate new, expire old, check time limits. */
    UFUNCTION(BlueprintCallable, Category = "Quests")
    void ProcessTurn(int32 CurrentTurn, int32 ActiveWizardCount);

    // ── Delegates ────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintAssignable, Category = "Quests")
    FOnQuestAvailable OnQuestAvailable;

    UPROPERTY(BlueprintAssignable, Category = "Quests")
    FOnQuestAccepted OnQuestAccepted;

    UPROPERTY(BlueprintAssignable, Category = "Quests")
    FOnQuestCompleted OnQuestCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Quests")
    FOnQuestFailed OnQuestFailed;

    UPROPERTY(BlueprintAssignable, Category = "Quests")
    FOnQuestObjectiveProgress OnQuestObjectiveProgress;

    UPROPERTY(BlueprintAssignable, Category = "Quests")
    FOnQuestRewardGranted OnQuestRewardGranted;

private:
    /** Select a quest type appropriate for the current game state. */
    ECoMQuestType SelectQuestType(int32 WizardId, int32 CurrentTurn) const;

    /** Generate objectives for a quest type. */
    TArray<FCoMQuestObjective> GenerateObjectives(ECoMQuestType Type, int32 WizardId,
                                                    int32 Difficulty, int32 CurrentTurn) const;

    /** Generate rewards appropriate to difficulty and quest type. */
    TArray<FCoMQuestReward> GenerateRewards(ECoMQuestType Type, int32 Difficulty) const;

    /** Generate quest title and description. */
    void GenerateQuestText(FCoMQuest& Quest) const;

    /** Generate lore/flavor text for a quest. */
    FString GenerateLoreText(ECoMQuestType Type, int32 Difficulty) const;

    /** Check if all objectives of a quest are complete and finalize it. */
    void CheckQuestCompletion(int32 QuestId);

    /** Grant rewards for a completed quest. */
    void GrantRewards(int32 WizardId, const FCoMQuest& Quest);

    /** Expire old available quests. */
    void ExpireQuests(int32 CurrentTurn);

    /** Fail quests that exceeded their time limit. */
    void FailTimedOutQuests(int32 CurrentTurn);

    /** Calculate difficulty based on game turn and wizard power. */
    int32 CalculateDifficulty(int32 WizardId, int32 CurrentTurn) const;

    /** Find a valid unexplored site for an explore quest. */
    FIntPoint FindExplorationTarget(int32 WizardId, ECoMPlane& OutPlane) const;

    /** Find a valid dragon for a slay quest. */
    int32 FindDragonTarget(int32 WizardId) const;

    /** Find a valid neutral city for a conquer/alliance quest. */
    int32 FindNeutralCityTarget(int32 WizardId) const;

    /** Find a rival wizard for a diplomatic/espionage quest. */
    int32 FindRivalWizardTarget(int32 WizardId) const;

    UPROPERTY()
    TMap<int32, FCoMQuest> AllQuests;

    UPROPERTY()
    FCoMQuestGenConfig GenConfig;

    UPROPERTY()
    FRandomStream RngStream;

    int32 NextQuestId = 1;
    int32 NextChainId = 1;
    int32 LastGenerationTurn = 0;
};
