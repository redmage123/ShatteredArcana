// Copyright Mythforge Studios. All Rights Reserved.
// CoMCommandQueue.h — Unified player command interface + per-wizard queue
//
// Architecture mandate (§6.2 Unified Input Abstraction):
//   Human players, AI wizards, and remote network players all issue commands
//   through ICoMPlayerCommand. No subsystem knows or cares which controller
//   type produced a command.
//
// Dependency rule: this header lives in CoMCore. CoMNetwork, CoMAI, and CoMUI
// all implement ICoMPlayerCommand — they depend on CoMCore, never the reverse.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCommandQueue.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// COMMAND TYPE ENUM
// ─────────────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class ECoMCommandType : uint8
{
    // Movement
    MoveArmy,
    MoveHero,
    EnterPortal,

    // City / production
    SetProductionQueue,
    BuyBuilding,
    RazeCity,

    // Magic
    CastSpell,
    ResearchSpell,
    CastRitual,
    AbandonEnchantment,

    // Diplomacy
    DeclareWar,
    OfferPeace,
    OfferTrade,
    ProposeAlliance,
    BreakAlliance,
    BreakPact,         // Infernal contract breach

    // Units
    DisbandUnit,
    MergeArmies,
    SplitArmy,

    // Turn control
    EndTurn,
    Surrender,

    MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// COMMAND PAYLOAD — Plain Old Data, fully serializable
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Compact POD struct carrying all data for one wizard command.
 * Must be fully representable by FArchive (no raw pointers, no TMap).
 * Network layer serializes this to wire format via UCoMCommandSerializer.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMCommandPayload
{
    GENERATED_BODY()

    // Command classification
    UPROPERTY(SaveGame) ECoMCommandType CommandType = ECoMCommandType::EndTurn;

    // Issuing wizard (0–47). Must match queue owner — validated server-side.
    UPROPERTY(SaveGame) int32 WizardIndex = -1;

    // Turn number this command was issued on. Validated against server turn.
    UPROPERTY(SaveGame) int32 TurnNumber = 0;

    // ── Generic parameter slots (semantics depend on CommandType) ──────────
    // Prefer named fields over a generic blob so switch/case code is readable.

    // Primary target — tile index, unit ID, city ID, etc.
    UPROPERTY(SaveGame) int32 TargetID_A = -1;
    // Secondary target (e.g., destination tile for MoveArmy)
    UPROPERTY(SaveGame) int32 TargetID_B = -1;
    // Plane index for cross-plane commands
    UPROPERTY(SaveGame) ECoMPlane TargetPlane = ECoMPlane::Aurelith;
    // Spell/research/building asset name reference (stable FName, not ptr)
    UPROPERTY(SaveGame) FName AssetRef = NAME_None;
    // Quantity (e.g. gold offered in trade, units to split)
    UPROPERTY(SaveGame) int32 Quantity = 0;

    // Checksum for anti-cheat — computed by issuer, verified by UCoMAntiCheatValidator.
    UPROPERTY(SaveGame) int32 PayloadChecksum = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// PLAYER COMMAND INTERFACE
// ─────────────────────────────────────────────────────────────────────────────

UINTERFACE(MinimalAPI, BlueprintType)
class UCoMPlayerCommand : public UInterface
{
    GENERATED_BODY()
};

/**
 * Pure virtual interface implemented by:
 *   - ACoMHumanPlayerController  (CoMCore)
 *   - ACoMAIPlayerController     (CoMAI)
 *   - UCoMNetworkProxy           (CoMNetwork)
 *
 * UCoMTurnSubsystem only touches this interface — it never knows which
 * concrete type it is talking to.
 */
class COMCORE_API ICoMPlayerCommand
{
    GENERATED_BODY()
public:
    virtual ECoMCommandType    GetType()        const = 0;
    virtual int32              GetWizardIndex() const = 0;
    virtual FCoMCommandPayload GetPayload()     const = 0;

    /**
     * Pre-queue validation against a game snapshot.
     * Must be pure / deterministic — no I/O, no random, no FDateTime::Now().
     * Returns false if the command is illegal in the given state.
     */
    virtual bool Validate(const struct FCoMGameStateSnapshot& State) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// COMMAND QUEUE — Per-wizard, one per slot, serializable
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Ordered queue of commands for a single wizard slot.
 *
 * Lifecycle:
 *   1. Human/AI/Network fills the queue during the COMMAND PHASE.
 *   2. UCoMTurnSubsystem::ResolveCommands() drains it.
 *   3. Commands are archived in FCoMGameStateSnapshot::PendingCommands for replay.
 *
 * Stored as a UPROPERTY TArray — serializes cleanly via FArchive.
 * Never use TMap or TSet here; insertion order must be deterministic.
 */
UCLASS(BlueprintType, Within=CoMTurnSubsystem)
class COMCORE_API UCoMCommandQueue : public UObject
{
    GENERATED_BODY()
public:
    // ── Queue operations ──────────────────────────────────────────────────────

    /** Append a validated command to this wizard's queue. */
    UFUNCTION(BlueprintCallable)
    void Enqueue(const FCoMCommandPayload& Command);

    /** Remove and return the front command. Call only during resolution phase. */
    bool Dequeue(FCoMCommandPayload& OutCommand);

    /** Peek at the front command without removing it. */
    bool Peek(FCoMCommandPayload& OutCommand) const;

    UFUNCTION(BlueprintPure) bool IsEmpty()   const { return Commands.IsEmpty(); }
    UFUNCTION(BlueprintPure) int32 Num()      const { return Commands.Num(); }
    UFUNCTION(BlueprintCallable) void Clear()       { Commands.Empty(); }

    // ── Identity ──────────────────────────────────────────────────────────────

    /** Wizard slot this queue belongs to (0–47). Set once at creation. */
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 WizardIndex = -1;

    /** True once the wizard has submitted EndTurn this phase. */
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bReady = false;

    // ── Serializable payload array ────────────────────────────────────────────
    // Using TArray (not TMap/TSet) — iteration order is deterministic.

    UPROPERTY(SaveGame) TArray<FCoMCommandPayload> Commands;
};
