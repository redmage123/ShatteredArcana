// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMReplayRecorder.h — Writes input logs for determinism CI + post-game replay
// Wire format: JSON-Lines per determinism-ci.md §4.1
// Owner: Network Architect

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CoMReplayRecorder.generated.h"

struct FCoMCommandPayload;

UCLASS()
class COMNETWORK_API UCoMReplayRecorder : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // UWorldSubsystem lifecycle — binds/unbinds turn delegates.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Begin a new recording session (called at game start or on -RecordReplay flag).
    UFUNCTION(BlueprintCallable) void StartRecording(int32 InitialTurn, int64 Seed);

    // Append one command to the log. Called by UCoMCommandQueue after every Enqueue.
    void RecordCommand(int32 TurnNumber, int32 WizardIndex, const FCoMCommandPayload& Payload);

    // Flush in-memory buffer to disk (idempotent; safe to call mid-game).
    void FlushToDisk();

    // Stop recording and write the final flush.
    UFUNCTION(BlueprintCallable) void StopRecording();

    // True while a recording session is active.
    UFUNCTION(BlueprintPure) bool IsRecording() const { return bRecording; }

private:
    // Bound to UCoMTurnSubsystem::OnTurnEnded — flushes at every turn boundary.
    UFUNCTION() void OnTurnEnded_Handler(int32 TurnNumber);

    bool      bRecording    = false;
    int32     BaseTurn      = 0;
    uint64    RecordedSeed  = 0;
    TArray<uint8> Buffer;   // Accumulates UTF-8 JSON-Lines until FlushToDisk().
};
