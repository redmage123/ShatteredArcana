// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMTurnSubsystem.h — Game-phase FSM driving the full Caster of Magic turn cycle.
// COM-028

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoreTypes/CoMEnums.h"
#include "CoreTypes/CoMConstants.h"
#include "CoMDeterministicRandom.h"
#include "CoMTurnSubsystem.generated.h"

class ACoMGameState;
class ACoMPlayerState;

// ─────────────────────────────────────────────────────────────────────────────
// Per-wizard sub-phase (used inside PlayerTurn / AITurn)
// ─────────────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class ECoMWizardPhase : uint8
{
	Planning     UMETA(DisplayName = "Planning"),     // Wizard reviews situation, issues orders
	Movement     UMETA(DisplayName = "Movement"),     // Armies move, exploration
	Combat       UMETA(DisplayName = "Combat"),       // Queued battles in this wizard's territory
	Magic        UMETA(DisplayName = "Magic"),        // Spellcasting: research, global spells, rituals
	Income       UMETA(DisplayName = "Income"),       // Gold / mana / production ticked
	EndTurn      UMETA(DisplayName = "End Turn"),     // Wizard signals done; clean-up

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// Delegates
// ─────────────────────────────────────────────────────────────────────────────

/** Fired when the global game phase changes (WorldProcessing, PlayerTurn, etc.). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGamePhaseChanged,
	ECoMGamePhase, OldPhase,
	ECoMGamePhase, NewPhase);

/** Fired when the active wizard index changes. NewIndex == WIZARD_INDEX_NONE during global phases. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveWizardChanged,
	int32, OldWizardIndex,
	int32, NewWizardIndex);

/** Fired when the per-wizard sub-phase advances. Only meaningful during PlayerTurn / AITurn. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWizardPhaseChanged,
	ECoMWizardPhase, OldPhase,
	ECoMWizardPhase, NewPhase);

/** Fired at the very start of a new turn (after world-processing completes). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnStarted, int32, TurnNumber);

/** Fired at the very end of a turn (after all wizards have acted). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnEnded, int32, TurnNumber);

/** Fired when the player hits End Turn with idle armies/cities. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIdleWarning, int32, IdleArmyCount, int32, IdleCityCount);

// ─────────────────────────────────────────────────────────────────────────────

/**
 * UCoMTurnSubsystem
 *
 * GameInstance subsystem — single authority for the Caster of Magic turn cycle.
 *
 * Full turn sequence:
 *   1. WorldProcessing  — weather, migration, disease, ritual ticks, Ley Line fluctuation
 *   2. For each active wizard (in ascending WizardIndex order):
 *        a. [PlayerTurn | AITurn] with sub-phases:
 *           Planning → Movement → Combat → Magic → Income → EndTurn
 *   3. CombatResolution — any unresolved army-vs-army battles
 *   4. EndOfTurn        — diplomacy checks, victory conditions, autosave trigger
 *   5. Advance turn counter → return to step 1
 *
 * Game logic calls AdvanceWizardPhase() (or SkipToNextWizard()) to drive progression.
 * The subsystem writes CurrentPhase and ActiveWizardIndex back to ACoMGameState
 * and fires delegates so UI, AI, and audio subsystems can react.
 *
 * Thread safety: NOT thread-safe. All methods must be called from the game thread.
 */
UCLASS()
class COMCORE_API UCoMTurnSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── USubsystem ────────────────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── Turn Lifecycle ────────────────────────────────────────────────────

	/**
	 * Starts the very first turn (Turn 1, WorldProcessing phase).
	 * Should be called from ACoMGameMode::BeginPlay after world gen is complete.
	 */
	UFUNCTION(BlueprintCallable, Category = "TurnSubsystem")
	void StartGame();

	/**
	 * Advances the current wizard's sub-phase by one step.
	 * When EndTurn is reached, moves to the next wizard (or CombatResolution).
	 * No-op if called outside a wizard turn.
	 */
	UFUNCTION(BlueprintCallable, Category = "TurnSubsystem")
	void AdvanceWizardPhase();

	/**
	 * Immediately ends the current wizard's turn and advances to the next wizard.
	 * Skips any remaining sub-phases. Useful for AI quick-end and surrender.
	 */
	UFUNCTION(BlueprintCallable, Category = "TurnSubsystem")
	void SkipToNextWizard();

	/**
	 * Signals that the current global phase is complete (WorldProcessing, CombatResolution,
	 * EndOfTurn). Only valid when no wizard is active. Advances to the next outer phase.
	 */
	UFUNCTION(BlueprintCallable, Category = "TurnSubsystem")
	void AdvanceGlobalPhase();

	/**
	 * Runs a complete turn cycle: world processing, all subsystem ticks
	 * (movement, combat, production, magic, heroes, dragons, diplomacy,
	 * espionage, quests, fog-of-war, victory), then advances the turn counter.
	 *
	 * This is the single-call entry point that replaces UCoMTurnManager::ProcessFullTurn().
	 */
	UFUNCTION(BlueprintCallable, Category = "TurnSubsystem")
	void ProcessFullTurn();

	// ── Queries ───────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "TurnSubsystem")
	ECoMGamePhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "TurnSubsystem")
	int32 GetActiveWizardIndex() const { return ActiveWizardIndex; }

	UFUNCTION(BlueprintPure, Category = "TurnSubsystem")
	ECoMWizardPhase GetCurrentWizardPhase() const { return CurrentWizardPhase; }

	UFUNCTION(BlueprintPure, Category = "TurnSubsystem")
	int32 GetCurrentTurn() const { return CurrentTurn; }

	/** Returns true if a human player (not AI) is currently in the active wizard slot. */
	UFUNCTION(BlueprintPure, Category = "TurnSubsystem")
	bool IsPlayerTurn() const;

	/** Returns true if the game has started (StartGame has been called). */
	UFUNCTION(BlueprintPure, Category = "TurnSubsystem")
	bool HasGameStarted() const { return bGameStarted; }

	/** Returns the number of non-eliminated wizards still in the game. */
	UFUNCTION(BlueprintPure, Category = "TurnSubsystem")
	int32 GetActiveWizardCount() const;

	// ── Idle Checks ──────────────────────────────────────────────────────

	/** Get armies belonging to WizardId that still have movement remaining. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TurnSubsystem")
	TArray<int32> GetIdleArmies(int32 WizardId) const;

	/** Get cities belonging to WizardId that have empty production queues. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TurnSubsystem")
	TArray<int32> GetIdleCities(int32 WizardId) const;

	/**
	 * Check for idle armies/cities before ending the turn.
	 * If any found, broadcasts OnIdleWarning and returns true (turn not ended).
	 * If none found, returns false (caller should proceed with EndTurn).
	 */
	UFUNCTION(BlueprintCallable, Category = "TurnSubsystem")
	bool CheckIdleBeforeEndTurn(int32 WizardId);

	/** Game speed multiplier (affects AI turn processing delay). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TurnSubsystem")
	float GameSpeed = 1.0f;

	/** Set game speed: 0.5x, 1x, 2x, 4x. */
	UFUNCTION(BlueprintCallable, Category = "TurnSubsystem")
	void SetGameSpeed(float Speed);

	// ── Delegates ─────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "TurnSubsystem")
	FOnGamePhaseChanged OnGamePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "TurnSubsystem")
	FOnActiveWizardChanged OnActiveWizardChanged;

	UPROPERTY(BlueprintAssignable, Category = "TurnSubsystem")
	FOnWizardPhaseChanged OnWizardPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "TurnSubsystem")
	FOnTurnStarted OnTurnStarted;

	UPROPERTY(BlueprintAssignable, Category = "TurnSubsystem")
	FOnTurnEnded OnTurnEnded;

	/** Fired when player tries to end turn with idle armies/cities. */
	UPROPERTY(BlueprintAssignable, Category = "TurnSubsystem")
	FOnIdleWarning OnIdleWarning;

	// ── Save/Load Export/Import ───────────────────────────────────────────

	/** Export turn-relevant state for save serialization. */
	void ExportSaveState(TArray<int32>& OutWizardTurnOrder, int32& OutTurnOrderPosition,
	                     FCoMDeterministicRandom& OutRNGState) const;

	/** Import turn-relevant state from save data. */
	void ImportSaveState(const TArray<int32>& InWizardTurnOrder, int32 InTurnOrderPosition,
	                     const FCoMDeterministicRandom& InRNGState);

private:
	// ── Internal State ────────────────────────────────────────────────────

	/** Current global turn number. */
	int32 CurrentTurn = 0;

	/** Current outer game phase. */
	ECoMGamePhase CurrentPhase = ECoMGamePhase::WorldProcessing;

	/** Index of the wizard whose turn it is, or CoM::WIZARD_INDEX_NONE. */
	int32 ActiveWizardIndex = CoM::WIZARD_INDEX_NONE;

	/** Sub-phase within the current wizard's turn. */
	ECoMWizardPhase CurrentWizardPhase = ECoMWizardPhase::Planning;

	/** Ordered list of wizard indices that will act this turn, built at WorldProcessing start. */
	TArray<int32> WizardTurnOrder;

	/** Position in WizardTurnOrder for the current wizard. */
	int32 TurnOrderPosition = 0;

	bool bGameStarted = false;

	// ── Phase Processors ─────────────────────────────────────────────────

	/** Called once per turn during WorldProcessing phase (weather, rituals, Ley Lines, etc.). */
	void ProcessWorldPhase();

	/** Tick all gameplay subsystems in canonical phase order (movement, combat, production, magic, etc.). */
	void ProcessAllSubsystemTurns();

	/** Begins a specific wizard's turn (sets active index, resets sub-phase to Planning). */
	void BeginWizardTurn(int32 WizardIndex);

	/** Ends the current wizard's turn; advances to next wizard or CombatResolution. */
	void EndWizardTurn();

	/** Global combat resolution after all wizards have moved. */
	void ProcessCombatResolution();

	/** End-of-turn cleanup: diplomacy ticks, victory check, autosave hint. */
	void ProcessEndOfTurn();

	/** Increment turn counter and start the next WorldProcessing phase. */
	void BeginNextTurn();

	// ── Helpers ───────────────────────────────────────────────────────────

	/** Builds the ordered wizard turn list from ACoMGameState::WizardStates. */
	void BuildWizardTurnOrder();

	/** Writes CurrentPhase and ActiveWizardIndex back to ACoMGameState (if available). */
	void SyncToGameState();

	/** Sets CurrentPhase and fires OnGamePhaseChanged. */
	void SetPhase(ECoMGamePhase NewPhase);

	/** Sets ActiveWizardIndex and fires OnActiveWizardChanged. */
	void SetActiveWizard(int32 NewIndex);

	/** Sets CurrentWizardPhase and fires OnWizardPhaseChanged. */
	void SetWizardPhase(ECoMWizardPhase NewPhase);

	/** Resolves which outer phase (PlayerTurn or AITurn) to use for a wizard. */
	ECoMGamePhase GetWizardPhaseType(int32 WizardIndex) const;

	/** Cached weak pointer to the game state. Refreshed each turn. */
	TWeakObjectPtr<ACoMGameState> CachedGameState;
};
