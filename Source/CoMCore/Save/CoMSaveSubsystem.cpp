// Copyright Mythforge Studios. All Rights Reserved.
// CoMSaveSubsystem.cpp — Save/Load system implementation

#include "Save/CoMSaveSubsystem.h"

#include "Framework/CoMGameInstance.h"
#include "Framework/CoMGameState.h"
#include "Wizards/CoMPlayerState.h"
#include "Turn/CoMTurnSubsystem.h"
#include "World/CoMWorldMapSubsystem.h"
#include "Economy/CoMCitySubsystem.h"
#include "Units/CoMUnitSubsystem.h"
#include "Units/CoMHeroSubsystem.h"
#include "Units/CoMDragonSubsystem.h"
#include "Diplomacy/CoMDiplomacySubsystem.h"
#include "Magic/CoMMagicSubsystem.h"
#include "Espionage/CoMEspionageSubsystem.h"
#include "Combat/CoMNavalSubsystem.h"
#include "Quests/CoMQuestSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMSave, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// USubsystem interface
// ─────────────────────────────────────────────────────────────────────────────

void UCoMSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    AutosaveIndex = 0;

    // Ensure the save directory exists
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*GetSaveDirectory());

    UE_LOG(LogCoMSave, Log, TEXT("UCoMSaveSubsystem initialized. Save dir: %s"), *GetSaveDirectory());
}

void UCoMSaveSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// SaveGame
// ─────────────────────────────────────────────────────────────────────────────

bool UCoMSaveSubsystem::SaveGame(const FString& SlotName)
{
    UE_LOG(LogCoMSave, Log, TEXT("SaveGame: Saving to slot '%s'..."), *SlotName);

    UCoMSaveGameObject* SaveObject = NewObject<UCoMSaveGameObject>();
    FCoMSaveData& Data = SaveObject->Data;

    // Collect all subsystem state
    CollectHeader(Data);
    Data.SaveName = SlotName;
    CollectWorldTime(Data);
    CollectWorldMap(Data);
    CollectCities(Data);
    CollectUnits(Data);
    CollectDiplomacy(Data);
    CollectMagic(Data);
    CollectHeroes(Data);
    CollectDragons(Data);
    CollectNaval(Data);
    CollectEspionage(Data);
    CollectQuests(Data);
    CollectTurnState(Data);

    // Use UE5 SaveGameToSlot — handles serialization of all UPROPERTY fields
    const bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);

    if (bSuccess)
    {
        // Write lightweight metadata for the load screen
        FCoMSaveSlotInfo Meta;
        Meta.SlotName    = SlotName;
        Meta.WizardName  = Data.WizardName;
        Meta.TurnNumber  = Data.TurnNumber;
        Meta.Timestamp   = Data.SaveTimestamp;
        Meta.bIsAutosave = SlotName.StartsWith(TEXT("Autosave_"));
        Meta.SaveVersion = CURRENT_SAVE_VERSION;
        WriteSlotMetadata(SlotName, Meta);

        OnGameSaved.Broadcast(SlotName);
        UE_LOG(LogCoMSave, Log, TEXT("SaveGame: Slot '%s' saved successfully (Turn %d)."),
               *SlotName, Data.TurnNumber);
    }
    else
    {
        UE_LOG(LogCoMSave, Error, TEXT("SaveGame: Failed to save slot '%s'."), *SlotName);
    }

    return bSuccess;
}

// ─────────────────────────────────────────────────────────────────────────────
// LoadGame
// ─────────────────────────────────────────────────────────────────────────────

bool UCoMSaveSubsystem::LoadGame(const FString& SlotName)
{
    UE_LOG(LogCoMSave, Log, TEXT("LoadGame: Loading slot '%s'..."), *SlotName);

    if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        UE_LOG(LogCoMSave, Warning, TEXT("LoadGame: Slot '%s' does not exist."), *SlotName);
        return false;
    }

    USaveGame* LoadedSave = UGameplayStatics::LoadGameFromSlot(SlotName, 0);
    UCoMSaveGameObject* SaveObject = Cast<UCoMSaveGameObject>(LoadedSave);
    if (!SaveObject)
    {
        UE_LOG(LogCoMSave, Error, TEXT("LoadGame: Failed to deserialize slot '%s'."), *SlotName);
        return false;
    }

    const FCoMSaveData& Data = SaveObject->Data;

    // Version check
    if (Data.SaveVersion > CURRENT_SAVE_VERSION)
    {
        UE_LOG(LogCoMSave, Error,
               TEXT("LoadGame: Save version %d is newer than current %d. Cannot load."),
               Data.SaveVersion, CURRENT_SAVE_VERSION);
        return false;
    }

    UE_LOG(LogCoMSave, Log, TEXT("LoadGame: Restoring state from Turn %d..."), Data.TurnNumber);

    // Restore all subsystem state
    RestoreWorldTime(Data);
    RestoreWorldMap(Data);
    RestoreCities(Data);
    RestoreUnits(Data);
    RestoreDiplomacy(Data);
    RestoreMagic(Data);
    RestoreHeroes(Data);
    RestoreDragons(Data);
    RestoreNaval(Data);
    RestoreEspionage(Data);
    RestoreQuests(Data);
    RestoreTurnState(Data);

    // Mark GameInstance as loaded game
    if (UCoMGameInstance* GI = Cast<UCoMGameInstance>(GetGameInstance()))
    {
        GI->LoadedSaveSlotName = SlotName;
    }

    OnGameLoaded.Broadcast(SlotName);
    UE_LOG(LogCoMSave, Log, TEXT("LoadGame: Slot '%s' restored successfully (Turn %d)."),
           *SlotName, Data.TurnNumber);

    // Transition to the overworld level
    UGameplayStatics::OpenLevel(GetGameInstance()->GetWorld(), FName(TEXT("L_Overworld")));

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Slot Management
// ─────────────────────────────────────────────────────────────────────────────

TArray<FCoMSaveSlotInfo> UCoMSaveSubsystem::GetSaveSlots() const
{
    TArray<FCoMSaveSlotInfo> Result;

    // Scan the save directory for .meta files
    const FString SaveDir = GetSaveDirectory();
    TArray<FString> MetaFiles;
    IFileManager::Get().FindFiles(MetaFiles, *(SaveDir / TEXT("*.meta")), true, false);

    for (const FString& MetaFile : MetaFiles)
    {
        FCoMSaveSlotInfo Info;
        const FString SlotName = FPaths::GetBaseFilename(MetaFile);
        if (ReadSlotMetadata(SlotName, Info))
        {
            Result.Add(Info);
        }
    }

    // Sort by timestamp (newest first)
    Result.Sort([](const FCoMSaveSlotInfo& A, const FCoMSaveSlotInfo& B)
    {
        return A.Timestamp > B.Timestamp;
    });

    return Result;
}

bool UCoMSaveSubsystem::DeleteSave(const FString& SlotName)
{
    const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, 0);

    // Also delete the metadata file
    const FString MetaPath = GetMetaFilePath(SlotName);
    IFileManager::Get().Delete(*MetaPath);

    if (bDeleted)
    {
        UE_LOG(LogCoMSave, Log, TEXT("DeleteSave: Slot '%s' deleted."), *SlotName);
    }
    else
    {
        UE_LOG(LogCoMSave, Warning, TEXT("DeleteSave: Failed to delete slot '%s'."), *SlotName);
    }

    return bDeleted;
}

bool UCoMSaveSubsystem::DoesSaveExist(const FString& SlotName) const
{
    return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Autosave / Quick Save / Quick Load
// ─────────────────────────────────────────────────────────────────────────────

void UCoMSaveSubsystem::Autosave()
{
    const FString SlotName = FString::Printf(TEXT("Autosave_%d"), AutosaveIndex);
    AutosaveIndex = (AutosaveIndex + 1) % MAX_AUTOSAVE_SLOTS;

    UE_LOG(LogCoMSave, Log, TEXT("Autosave: Saving to slot '%s'..."), *SlotName);
    SaveGame(SlotName);
}

void UCoMSaveSubsystem::QuickSave()
{
    SaveGame(TEXT("QuickSave"));
}

void UCoMSaveSubsystem::QuickLoad()
{
    LoadGame(TEXT("QuickSave"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Collection helpers — gather state from each subsystem
// ─────────────────────────────────────────────────────────────────────────────

void UCoMSaveSubsystem::CollectHeader(FCoMSaveData& OutData)
{
    OutData.SaveTimestamp = FDateTime::Now();
    OutData.SaveVersion  = CURRENT_SAVE_VERSION;

    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    if (UCoMTurnSubsystem* Turn = GI->GetSubsystem<UCoMTurnSubsystem>())
    {
        OutData.TurnNumber = Turn->GetCurrentTurn();
    }

    // Try to get wizard name from GameInstance new-game settings or first player state
    if (UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GI))
    {
        OutData.WizardName = CoMGI->NewGameSettings.WizardName.ToString();
    }

    // If wizard name is empty, try getting it from the game state
    if (OutData.WizardName.IsEmpty())
    {
        UWorld* World = GI->GetWorld();
        if (World)
        {
            if (ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState()))
            {
                if (ACoMPlayerState* PS = GS->GetWizardByIndex(0))
                {
                    OutData.WizardName = PS->GetPlayerName();
                }
            }
        }
    }
}

void UCoMSaveSubsystem::CollectWorldTime(FCoMSaveData& OutData)
{
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World) { return; }

    ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState());
    if (!GS) { return; }

    OutData.WorldYear        = GS->WorldYear;
    OutData.WorldSeasonIndex = GS->WorldSeasonIndex;
    OutData.SeasonTurn       = GS->SeasonTurn;
    OutData.DiscoveredPlanes = GS->DiscoveredPlanes.Array();
}

void UCoMSaveSubsystem::CollectWorldMap(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMWorldMapSubsystem* WorldMap = GI->GetSubsystem<UCoMWorldMapSubsystem>();
    if (!WorldMap || !WorldMap->IsInitialized()) { return; }

    OutData.AllMapLayers = WorldMap->ExportAllLayers();
}

void UCoMSaveSubsystem::CollectCities(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMCitySubsystem* Cities = GI->GetSubsystem<UCoMCitySubsystem>();
    if (!Cities) { return; }

    Cities->ExportAll(OutData.AllCities, OutData.NextCityID);
}

void UCoMSaveSubsystem::CollectUnits(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>();
    if (!Units) { return; }

    Units->ExportAll(OutData.AllUnits, OutData.AllArmies,
                     OutData.NextUnitID, OutData.NextArmyID);
}

void UCoMSaveSubsystem::CollectDiplomacy(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMDiplomacySubsystem* Diplomacy = GI->GetSubsystem<UCoMDiplomacySubsystem>();
    if (!Diplomacy) { return; }

    Diplomacy->ExportAll(OutData.AllRelations, OutData.PendingProposals,
                         OutData.AllPersonalities, OutData.PersonalityWizardIDs);
}

void UCoMSaveSubsystem::CollectMagic(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>();
    if (!Magic) { return; }

    Magic->ExportAll(OutData.AllMagicStates, OutData.ActiveCastings,
                     OutData.ActiveCastingWizardIDs, OutData.AllRituals, OutData.AllRunes);
}

void UCoMSaveSubsystem::CollectHeroes(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMHeroSubsystem* Heroes = GI->GetSubsystem<UCoMHeroSubsystem>();
    if (!Heroes) { return; }

    Heroes->ExportAll(OutData.AllHeroPersonalities, OutData.HeroPersonalityUnitIDs,
                      OutData.AllHeroRelationships,
                      OutData.HeroTierUnitIDs, OutData.HeroTierValues,
                      OutData.HeroClassUnitIDs, OutData.HeroClassValues,
                      OutData.HeroOwnerUnitIDs, OutData.HeroOwnerWizardIDs);
}

void UCoMSaveSubsystem::CollectDragons(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMDragonSubsystem* Dragons = GI->GetSubsystem<UCoMDragonSubsystem>();
    if (!Dragons) { return; }

    Dragons->ExportAll(OutData.AllDragons, OutData.AllDragonDomains, OutData.AllDragonEggs);
}

void UCoMSaveSubsystem::CollectNaval(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMNavalSubsystem* Naval = GI->GetSubsystem<UCoMNavalSubsystem>();
    if (!Naval) { return; }

    Naval->ExportAll(OutData.AllFleets, OutData.AllBlockades, OutData.AllSeaMonsters);
}

void UCoMSaveSubsystem::CollectEspionage(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMEspionageSubsystem* Espionage = GI->GetSubsystem<UCoMEspionageSubsystem>();
    if (!Espionage) { return; }

    Espionage->ExportAll(OutData.AllAgents, OutData.AllCounterIntel,
                         OutData.CounterIntelWizardIDs, OutData.MissionHistory);
}

void UCoMSaveSubsystem::CollectQuests(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMQuestSubsystem* Quests = GI->GetSubsystem<UCoMQuestSubsystem>();
    if (!Quests) { return; }

    Quests->ExportAll(OutData.AllQuests, OutData.QuestGenConfig);
}

void UCoMSaveSubsystem::CollectTurnState(FCoMSaveData& OutData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMTurnSubsystem* Turn = GI->GetSubsystem<UCoMTurnSubsystem>();
    if (!Turn) { return; }

    Turn->ExportSaveState(OutData.WizardTurnOrder, OutData.TurnOrderPosition, OutData.RNGState);

    // Also collect the snapshot
    UWorld* World = GI->GetWorld();
    if (World)
    {
        if (ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState()))
        {
            OutData.Snapshot.TurnNumber       = GS->CurrentTurn;
            OutData.Snapshot.ActiveWizardCount = 0;
            for (const auto& WS : GS->WizardStates)
            {
                if (WS) { OutData.Snapshot.ActiveWizardCount++; }
            }
        }
    }
    OutData.Snapshot.RNGState = OutData.RNGState;
}

// ─────────────────────────────────────────────────────────────────────────────
// Restore helpers — push state back into each subsystem
// ─────────────────────────────────────────────────────────────────────────────

void UCoMSaveSubsystem::RestoreWorldTime(const FCoMSaveData& InData)
{
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World) { return; }

    ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState());
    if (!GS) { return; }

    GS->CurrentTurn      = InData.TurnNumber;
    GS->WorldYear        = InData.WorldYear;
    GS->WorldSeasonIndex = InData.WorldSeasonIndex;
    GS->SeasonTurn       = InData.SeasonTurn;
    GS->DiscoveredPlanes.Empty();
    for (ECoMPlane Plane : InData.DiscoveredPlanes)
    {
        GS->DiscoveredPlanes.Add(Plane);
    }
}

void UCoMSaveSubsystem::RestoreWorldMap(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMWorldMapSubsystem* WorldMap = GI->GetSubsystem<UCoMWorldMapSubsystem>();
    if (!WorldMap) { return; }

    WorldMap->ImportAllLayers(InData.AllMapLayers);
}

void UCoMSaveSubsystem::RestoreCities(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMCitySubsystem* Cities = GI->GetSubsystem<UCoMCitySubsystem>();
    if (!Cities) { return; }

    Cities->ImportAll(InData.AllCities, InData.NextCityID);
}

void UCoMSaveSubsystem::RestoreUnits(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>();
    if (!Units) { return; }

    Units->ImportAll(InData.AllUnits, InData.AllArmies,
                     InData.NextUnitID, InData.NextArmyID);
}

void UCoMSaveSubsystem::RestoreDiplomacy(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMDiplomacySubsystem* Diplomacy = GI->GetSubsystem<UCoMDiplomacySubsystem>();
    if (!Diplomacy) { return; }

    Diplomacy->ImportAll(InData.AllRelations, InData.PendingProposals,
                         InData.AllPersonalities, InData.PersonalityWizardIDs);
}

void UCoMSaveSubsystem::RestoreMagic(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>();
    if (!Magic) { return; }

    Magic->ImportAll(InData.AllMagicStates, InData.ActiveCastings,
                     InData.ActiveCastingWizardIDs, InData.AllRituals, InData.AllRunes);
}

void UCoMSaveSubsystem::RestoreHeroes(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMHeroSubsystem* Heroes = GI->GetSubsystem<UCoMHeroSubsystem>();
    if (!Heroes) { return; }

    Heroes->ImportAll(InData.AllHeroPersonalities, InData.HeroPersonalityUnitIDs,
                      InData.AllHeroRelationships,
                      InData.HeroTierUnitIDs, InData.HeroTierValues,
                      InData.HeroClassUnitIDs, InData.HeroClassValues,
                      InData.HeroOwnerUnitIDs, InData.HeroOwnerWizardIDs);
}

void UCoMSaveSubsystem::RestoreDragons(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMDragonSubsystem* Dragons = GI->GetSubsystem<UCoMDragonSubsystem>();
    if (!Dragons) { return; }

    Dragons->ImportAll(InData.AllDragons, InData.AllDragonDomains, InData.AllDragonEggs);
}

void UCoMSaveSubsystem::RestoreNaval(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMNavalSubsystem* Naval = GI->GetSubsystem<UCoMNavalSubsystem>();
    if (!Naval) { return; }

    Naval->ImportAll(InData.AllFleets, InData.AllBlockades, InData.AllSeaMonsters);
}

void UCoMSaveSubsystem::RestoreEspionage(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMEspionageSubsystem* Espionage = GI->GetSubsystem<UCoMEspionageSubsystem>();
    if (!Espionage) { return; }

    Espionage->ImportAll(InData.AllAgents, InData.AllCounterIntel,
                         InData.CounterIntelWizardIDs, InData.MissionHistory);
}

void UCoMSaveSubsystem::RestoreQuests(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMQuestSubsystem* Quests = GI->GetSubsystem<UCoMQuestSubsystem>();
    if (!Quests) { return; }

    Quests->ImportAll(InData.AllQuests, InData.QuestGenConfig);
}

void UCoMSaveSubsystem::RestoreTurnState(const FCoMSaveData& InData)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMTurnSubsystem* Turn = GI->GetSubsystem<UCoMTurnSubsystem>();
    if (!Turn) { return; }

    Turn->ImportSaveState(InData.WizardTurnOrder, InData.TurnOrderPosition, InData.RNGState);
}

// ─────────────────────────────────────────────────────────────────────────────
// Metadata I/O — lightweight JSON for the load screen
// ─────────────────────────────────────────────────────────────────────────────

void UCoMSaveSubsystem::WriteSlotMetadata(const FString& SlotName, const FCoMSaveSlotInfo& Info)
{
    TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
    Json->SetStringField(TEXT("SlotName"),    Info.SlotName);
    Json->SetStringField(TEXT("WizardName"),  Info.WizardName);
    Json->SetNumberField(TEXT("TurnNumber"),  Info.TurnNumber);
    Json->SetStringField(TEXT("Timestamp"),   Info.Timestamp.ToIso8601());
    Json->SetBoolField(TEXT("bIsAutosave"),   Info.bIsAutosave);
    Json->SetNumberField(TEXT("SaveVersion"), Info.SaveVersion);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Json, Writer);

    FFileHelper::SaveStringToFile(JsonString, *GetMetaFilePath(SlotName));
}

bool UCoMSaveSubsystem::ReadSlotMetadata(const FString& SlotName, FCoMSaveSlotInfo& OutInfo) const
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *GetMetaFilePath(SlotName)))
    {
        return false;
    }

    TSharedPtr<FJsonObject> Json;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
    {
        return false;
    }

    OutInfo.SlotName    = Json->GetStringField(TEXT("SlotName"));
    OutInfo.WizardName  = Json->GetStringField(TEXT("WizardName"));
    OutInfo.TurnNumber  = static_cast<int32>(Json->GetNumberField(TEXT("TurnNumber")));
    OutInfo.bIsAutosave = Json->GetBoolField(TEXT("bIsAutosave"));
    OutInfo.SaveVersion = static_cast<int32>(Json->GetNumberField(TEXT("SaveVersion")));

    FDateTime::ParseIso8601(*Json->GetStringField(TEXT("Timestamp")), OutInfo.Timestamp);

    return true;
}

FString UCoMSaveSubsystem::GetSaveDirectory() const
{
    return FPaths::ProjectSavedDir() / TEXT("SaveGames");
}

FString UCoMSaveSubsystem::GetSaveFilePath(const FString& SlotName) const
{
    return GetSaveDirectory() / (SlotName + TEXT(".sav"));
}

FString UCoMSaveSubsystem::GetMetaFilePath(const FString& SlotName) const
{
    return GetSaveDirectory() / (SlotName + TEXT(".meta"));
}
