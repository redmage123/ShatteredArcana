// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMReplayRecorder.cpp — JSON-Lines replay log writer.
//
// Format spec: engineering/network/determinism-ci.md §4.1
//   {"turn":N,"wizard":W,"cmd":"MoveArmy","payload":"<hex>"}
// One line per command, in submission order.
// "payload" = hex-encoded UCoMCommandSerializer::Serialize() output (wire format v0x01).
//
// Flush policy: flush to disk at every turn boundary (OnTurnEnded) + on StopRecording.
// This limits data loss to one turn's commands if the process is killed mid-game.
//
// Owner: Network Architect

#include "CoMReplayRecorder.h"
#include "CoMCommandSerializer.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"
#include "CoMCore/Turn/CoMCommandQueue.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers (file-scope)
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
    /** Convert ECoMCommandType to a stable ASCII string for the JSON log.
     *  Strings must never be re-ordered — they are committed into fixture files. */
    static const TCHAR* CommandTypeToString(ECoMCommandType Type)
    {
        switch (Type)
        {
        // Movement
        case ECoMCommandType::MoveArmy:           return TEXT("MoveArmy");
        case ECoMCommandType::MoveHero:           return TEXT("MoveHero");
        case ECoMCommandType::EnterPortal:        return TEXT("EnterPortal");
        // City / production
        case ECoMCommandType::SetProductionQueue: return TEXT("SetProductionQueue");
        case ECoMCommandType::BuyBuilding:        return TEXT("BuyBuilding");
        case ECoMCommandType::RazeCity:           return TEXT("RazeCity");
        // Magic
        case ECoMCommandType::CastSpell:          return TEXT("CastSpell");
        case ECoMCommandType::ResearchSpell:      return TEXT("ResearchSpell");
        case ECoMCommandType::CastRitual:         return TEXT("CastRitual");
        case ECoMCommandType::AbandonEnchantment: return TEXT("AbandonEnchantment");
        // Diplomacy
        case ECoMCommandType::DeclareWar:         return TEXT("DeclareWar");
        case ECoMCommandType::OfferPeace:         return TEXT("OfferPeace");
        case ECoMCommandType::OfferTrade:         return TEXT("OfferTrade");
        case ECoMCommandType::ProposeAlliance:    return TEXT("ProposeAlliance");
        case ECoMCommandType::BreakAlliance:      return TEXT("BreakAlliance");
        case ECoMCommandType::BreakPact:          return TEXT("BreakPact");
        // Units
        case ECoMCommandType::DisbandUnit:        return TEXT("DisbandUnit");
        case ECoMCommandType::MergeArmies:        return TEXT("MergeArmies");
        case ECoMCommandType::SplitArmy:          return TEXT("SplitArmy");
        // Turn control
        case ECoMCommandType::EndTurn:            return TEXT("EndTurn");
        case ECoMCommandType::Surrender:          return TEXT("Surrender");
        // Fallback — should never reach here if the enum is kept in sync.
        default:                                  return TEXT("Unknown");
        }
    }

    /** Append a UTF-8-encoded FString line (plus '\n') to a byte buffer. */
    static void AppendLine(TArray<uint8>& Buffer, const FString& Line)
    {
        const FTCHARToUTF8 Conv(*Line);
        const int32 Len = Conv.Length();
        Buffer.Append(reinterpret_cast<const uint8*>(Conv.Get()), Len);
        Buffer.Add('\n');
    }

    /** Build the file path for the replay log.
     *  Pattern: <ProjectSaved>/Replays/replay_0x<SEED>_t<START_TURN>.jsonl */
    static FString MakeReplayPath(uint64 Seed, int32 StartTurn)
    {
        const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Replays"));
        return FPaths::Combine(Dir,
            FString::Printf(TEXT("replay_0x%016llX_t%d.jsonl"), Seed, StartTurn));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// UWorldSubsystem lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void UCoMReplayRecorder::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Bind to UCoMTurnSubsystem::OnTurnEnded for periodic flush at turn boundaries.
    // TurnSubsystem is a GameInstanceSubsystem — access via the owning GameInstance.
    if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        if (UCoMTurnSubsystem* TS = GI->GetSubsystem<UCoMTurnSubsystem>())
        {
            TS->OnTurnEnded.AddDynamic(this, &UCoMReplayRecorder::OnTurnEnded_Handler);
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("UCoMReplayRecorder::Initialize — UCoMTurnSubsystem not found; "
                     "turn-boundary flush will not occur."));
        }
    }
}

void UCoMReplayRecorder::Deinitialize()
{
    // Safety: flush + stop if the world is torn down while we are recording.
    if (bRecording)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCoMReplayRecorder::Deinitialize — recording still active; forcing flush."));
        FlushToDisk();
        bRecording = false;
        Buffer.Empty();
    }
    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void UCoMReplayRecorder::StartRecording(int32 InitialTurn, int64 Seed)
{
    if (bRecording)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCoMReplayRecorder::StartRecording — already recording; ignoring."));
        return;
    }

    BaseTurn      = InitialTurn;
    RecordedSeed  = Seed;
    bRecording    = true;
    Buffer.Empty();

    // Write header line first so the file is identifiable even if the game crashes
    // before any commands are recorded.
    const FString Header = FString::Printf(
        TEXT("{\"type\":\"header\",\"seed\":\"0x%016llX\",\"start_turn\":%d}"),
        RecordedSeed, BaseTurn);
    AppendLine(Buffer, Header);

    UE_LOG(LogTemp, Log,
        TEXT("UCoMReplayRecorder: Recording started. Seed=0x%016llX StartTurn=%d"),
        RecordedSeed, BaseTurn);
}

void UCoMReplayRecorder::RecordCommand(int32 TurnNumber, int32 WizardIndex,
                                       const FCoMCommandPayload& Payload)
{
    if (!bRecording)
    {
        return;
    }

    // Serialize payload to v0x01 wire format.
    const TArray<uint8> WireBytes = UCoMCommandSerializer::Serialize(Payload);

    // An empty result indicates serialization failure (mismatched payload).
    if (WireBytes.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCoMReplayRecorder::RecordCommand — Serialize returned empty for "
                 "Wizard %d Turn %d CommandType %d. Entry skipped."),
            WizardIndex, TurnNumber, static_cast<int32>(Payload.CommandType));
        return;
    }

    // Encode wire bytes as lower-case hex string.
    // Pre-reserve to avoid reallocations (each byte → 2 hex chars).
    FString HexPayload;
    HexPayload.Reserve(WireBytes.Num() * 2);
    static constexpr char kHex[] = "0123456789abcdef";
    for (const uint8 Byte : WireBytes)
    {
        HexPayload.AppendChar(static_cast<TCHAR>(kHex[Byte >> 4]));
        HexPayload.AppendChar(static_cast<TCHAR>(kHex[Byte & 0xF]));
    }

    // Build JSON-Lines entry per determinism-ci.md §4.1.
    const FString Line = FString::Printf(
        TEXT("{\"turn\":%d,\"wizard\":%d,\"cmd\":\"%s\",\"payload\":\"%s\"}"),
        TurnNumber, WizardIndex,
        CommandTypeToString(Payload.CommandType),
        *HexPayload);

    AppendLine(Buffer, Line);
}

void UCoMReplayRecorder::FlushToDisk()
{
    if (Buffer.IsEmpty())
    {
        return;  // Nothing to write.
    }

    const FString Path = MakeReplayPath(RecordedSeed, BaseTurn);

    // Ensure Replays directory exists (FFileHelper will not create it).
    const FString Dir = FPaths::GetPath(Path);
    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    if (!PF.DirectoryExists(*Dir))
    {
        PF.CreateDirectoryTree(*Dir);
    }

    // Append mode: use raw platform API so we don't lose prior turns on each flush.
    // (FFileHelper::SaveArrayToFile overwrites — we call it only once per session
    //  via StopRecording; mid-game flushes append.)
    IFileHandle* Handle = PF.OpenWrite(*Path, /*bAppend=*/true, /*bAllowRead=*/false);
    if (Handle)
    {
        Handle->Write(Buffer.GetData(), Buffer.Num());
        delete Handle;

        UE_LOG(LogTemp, Log,
            TEXT("UCoMReplayRecorder::FlushToDisk — %d bytes appended → %s"),
            Buffer.Num(), *Path);

        // Clear the in-memory buffer — already on disk.
        Buffer.Empty();
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("UCoMReplayRecorder::FlushToDisk — could not open file for writing: %s"), *Path);
        // Do NOT clear Buffer on failure; retry on next flush opportunity.
    }
}

void UCoMReplayRecorder::StopRecording()
{
    if (!bRecording)
    {
        return;
    }
    FlushToDisk();
    bRecording = false;
    Buffer.Empty();
    UE_LOG(LogTemp, Log,
        TEXT("UCoMReplayRecorder: Recording stopped. File: %s"),
        *MakeReplayPath(RecordedSeed, BaseTurn));
}

// ─────────────────────────────────────────────────────────────────────────────
// Private callbacks
// ─────────────────────────────────────────────────────────────────────────────

void UCoMReplayRecorder::OnTurnEnded_Handler(int32 TurnNumber)
{
    // Flush at every turn boundary. This limits maximum data loss to one turn's
    // worth of commands if the process is killed between flushes.
    if (bRecording)
    {
        FlushToDisk();
    }
}
