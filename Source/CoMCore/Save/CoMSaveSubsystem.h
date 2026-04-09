// Copyright Mythforge Studios. All Rights Reserved.
// CoMSaveSubsystem.h — Save/Load system for Shattered Arcana
// Serializes complete game state to disk via UE5 USaveGame framework.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/Turn/CoMDeterministicRandom.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/Espionage/CoMEspionageSubsystem.h"
#include "CoMCore/Combat/CoMNavalSubsystem.h"
#include "CoMCore/Quests/CoMQuestSubsystem.h"
#include "CoMSaveSubsystem.generated.h"

// Forward declarations
class UCoMTurnSubsystem;
class UCoMWorldMapSubsystem;
class UCoMUnitSubsystem;
class UCoMHeroSubsystem;
class UCoMDragonSubsystem;
class UCoMQuestSubsystem;
class ACoMGameState;

// ─────────────────────────────────────────────────────────────────────────────
// SAVE SLOT INFO — lightweight metadata for the load screen
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct COMCORE_API FCoMSaveSlotInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString WizardName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TurnNumber = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime Timestamp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsAutosave = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SaveVersion = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// SAVE DATA — complete serializable game state
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT()
struct COMCORE_API FCoMSaveData
{
    GENERATED_BODY()

    // ── Header ───────────────────────────────────────────────────────────────
    UPROPERTY() FString   SaveName;
    UPROPERTY() FDateTime SaveTimestamp;
    UPROPERTY() int32     SaveVersion     = 1;
    UPROPERTY() int32     TurnNumber      = 0;
    UPROPERTY() FString   WizardName;

    // ── Snapshot (turn identity, PRNG, hash) ─────────────────────────────────
    UPROPERTY() FCoMGameStateSnapshot Snapshot;

    // ── World Time (from ACoMGameState) ──────────────────────────────────────
    UPROPERTY() int32     WorldYear         = 1400;
    UPROPERTY() int32     WorldSeasonIndex  = 0;
    UPROPERTY() int32     SeasonTurn        = 1;
    UPROPERTY() TArray<ECoMPlane> DiscoveredPlanes;

    // ── World Map — flattened tile layers ────────────────────────────────────
    UPROPERTY() TArray<FCoMMapLayerData> AllMapLayers;

    // ── Cities ───────────────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMCityData> AllCities;
    UPROPERTY() int32 NextCityID = 1;

    // ── Units & Armies ───────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMUnitInstance> AllUnits;
    UPROPERTY() TArray<FCoMArmyGroup>    AllArmies;
    UPROPERTY() int32 NextUnitID = 1;
    UPROPERTY() int32 NextArmyID = 1;

    // ── Diplomacy ────────────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMDiplomaticRelation>    AllRelations;
    UPROPERTY() TArray<FCoMTreatyProposal>        PendingProposals;
    UPROPERTY() TArray<FCoMDiplomaticPersonality> AllPersonalities;
    // Personality keys (wizard IDs) parallel AllPersonalities
    UPROPERTY() TArray<int32> PersonalityWizardIDs;

    // ── Magic ────────────────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMWizardMagicState> AllMagicStates;
    UPROPERTY() TArray<FCoMSpellCast>        ActiveCastings;
    UPROPERTY() TArray<int32>                ActiveCastingWizardIDs;
    UPROPERTY() TArray<FCoMActiveRitual>     AllRituals;
    UPROPERTY() TArray<FCoMActiveRune>       AllRunes;

    // ── Heroes ───────────────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMHeroPersonality>   AllHeroPersonalities;
    UPROPERTY() TArray<int32>                 HeroPersonalityUnitIDs;
    UPROPERTY() TArray<FCoMHeroRelationship>  AllHeroRelationships;
    // Hero tier/class/owner maps serialized as parallel arrays
    UPROPERTY() TArray<int32>          HeroTierUnitIDs;
    UPROPERTY() TArray<uint8>          HeroTierValues;
    UPROPERTY() TArray<int32>          HeroClassUnitIDs;
    UPROPERTY() TArray<uint8>          HeroClassValues;
    UPROPERTY() TArray<int32>          HeroOwnerUnitIDs;
    UPROPERTY() TArray<int32>          HeroOwnerWizardIDs;

    // ── Dragons ──────────────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMDragonInstance> AllDragons;
    UPROPERTY() TArray<FCoMDragonDomain>  AllDragonDomains;
    UPROPERTY() TArray<FCoMDragonEgg>     AllDragonEggs;

    // ── Naval ────────────────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMFleet>    AllFleets;
    UPROPERTY() TArray<FCoMBlockade> AllBlockades;
    UPROPERTY() TArray<FCoMSeaMonster> AllSeaMonsters;

    // ── Espionage ────────────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMSpyAgent>      AllAgents;
    UPROPERTY() TArray<FCoMCounterIntel>  AllCounterIntel;
    UPROPERTY() TArray<int32>             CounterIntelWizardIDs;
    UPROPERTY() TArray<FCoMMissionResult> MissionHistory;

    // ── Quests ───────────────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMQuest>         AllQuests;
    UPROPERTY() FCoMQuestGenConfig        QuestGenConfig;

    // ── Sieges ────────────────────────────────────────────────────────────────
    UPROPERTY() TArray<FCoMSiegeState>    AllSieges;

    // ── Turn subsystem state ─────────────────────────────────────────────────
    UPROPERTY() TArray<int32> WizardTurnOrder;
    UPROPERTY() int32 TurnOrderPosition = 0;
    UPROPERTY() FCoMDeterministicRandom RNGState;
};

// ─────────────────────────────────────────────────────────────────────────────
// USaveGame wrapper — UE5 serialization entry point
// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class COMCORE_API UCoMSaveGameObject : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FCoMSaveData Data;
};

// ─────────────────────────────────────────────────────────────────────────────
// SAVE SUBSYSTEM
// ─────────────────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameSaved, const FString&, SlotName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLoaded, const FString&, SlotName);

UCLASS()
class COMCORE_API UCoMSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ── Save ─────────────────────────────────────────────────────────────────

    /** Save the entire game state to a named slot. Returns true on success. */
    UFUNCTION(BlueprintCallable, Category = "CoM|Save")
    bool SaveGame(const FString& SlotName);

    // ── Load ─────────────────────────────────────────────────────────────────

    /** Load a game from a named slot. Returns true on success. */
    UFUNCTION(BlueprintCallable, Category = "CoM|Save")
    bool LoadGame(const FString& SlotName);

    // ── Slot Management ──────────────────────────────────────────────────────

    /** Returns metadata for all save slots found on disk. */
    UFUNCTION(BlueprintCallable, Category = "CoM|Save")
    TArray<FCoMSaveSlotInfo> GetSaveSlots() const;

    /** Delete a save file and its metadata. Returns true on success. */
    UFUNCTION(BlueprintCallable, Category = "CoM|Save")
    bool DeleteSave(const FString& SlotName);

    /** Returns true if a save with the given slot name exists on disk. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Save")
    bool DoesSaveExist(const FString& SlotName) const;

    // ── Autosave ─────────────────────────────────────────────────────────────

    /** Trigger an autosave. Rotates between Autosave_0 .. Autosave_2. */
    UFUNCTION(BlueprintCallable, Category = "CoM|Save")
    void Autosave();

    // ── QuickSave / QuickLoad ────────────────────────────────────────────────

    /** Quick-save to the "QuickSave" slot. */
    UFUNCTION(BlueprintCallable, Category = "CoM|Save")
    void QuickSave();

    /** Quick-load from the "QuickSave" slot. */
    UFUNCTION(BlueprintCallable, Category = "CoM|Save")
    void QuickLoad();

    // ── Delegates ────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintAssignable, Category = "CoM|Save")
    FOnGameSaved OnGameSaved;

    UPROPERTY(BlueprintAssignable, Category = "CoM|Save")
    FOnGameLoaded OnGameLoaded;

    // ── Constants ────────────────────────────────────────────────────────────

    static constexpr int32 CURRENT_SAVE_VERSION = 1;
    static constexpr int32 MAX_AUTOSAVE_SLOTS = 3;

private:
    // ── Collection helpers (SaveGame) ────────────────────────────────────────

    void CollectHeader(FCoMSaveData& OutData);
    void CollectWorldTime(FCoMSaveData& OutData);
    void CollectWorldMap(FCoMSaveData& OutData);
    void CollectCities(FCoMSaveData& OutData);
    void CollectUnits(FCoMSaveData& OutData);
    void CollectDiplomacy(FCoMSaveData& OutData);
    void CollectMagic(FCoMSaveData& OutData);
    void CollectHeroes(FCoMSaveData& OutData);
    void CollectDragons(FCoMSaveData& OutData);
    void CollectNaval(FCoMSaveData& OutData);
    void CollectEspionage(FCoMSaveData& OutData);
    void CollectQuests(FCoMSaveData& OutData);
    void CollectTurnState(FCoMSaveData& OutData);

    // ── Restore helpers (LoadGame) ───────────────────────────────────────────

    void RestoreWorldTime(const FCoMSaveData& InData);
    void RestoreWorldMap(const FCoMSaveData& InData);
    void RestoreCities(const FCoMSaveData& InData);
    void RestoreUnits(const FCoMSaveData& InData);
    void RestoreDiplomacy(const FCoMSaveData& InData);
    void RestoreMagic(const FCoMSaveData& InData);
    void RestoreHeroes(const FCoMSaveData& InData);
    void RestoreDragons(const FCoMSaveData& InData);
    void RestoreNaval(const FCoMSaveData& InData);
    void RestoreEspionage(const FCoMSaveData& InData);
    void RestoreQuests(const FCoMSaveData& InData);
    void RestoreTurnState(const FCoMSaveData& InData);

    // ── Metadata I/O ─────────────────────────────────────────────────────────

    /** Write a lightweight JSON .meta file alongside the .sav for the load screen. */
    void WriteSlotMetadata(const FString& SlotName, const FCoMSaveSlotInfo& Info);

    /** Read a .meta file. Returns false if not found. */
    bool ReadSlotMetadata(const FString& SlotName, FCoMSaveSlotInfo& OutInfo) const;

    /** Returns the directory where save files are stored. */
    FString GetSaveDirectory() const;

    /** Returns the full path for a save file. */
    FString GetSaveFilePath(const FString& SlotName) const;

    /** Returns the full path for a metadata file. */
    FString GetMetaFilePath(const FString& SlotName) const;

    // ── Autosave rotation ────────────────────────────────────────────────────

    int32 AutosaveIndex = 0;
};
