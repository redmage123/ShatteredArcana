// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMTurnManager.generated.h"

// Forward declarations — subsystems are fetched at call time, never cached.
class UCoMSeasonSubsystem;
class UCoMHeroSubsystem;

// ═══════════════════════════════════════════════════════════════════════════
// Turn Phases — the fixed sequence in which subsystems are processed
// each global turn.  Matches Master of Magic / Caster of Magic phase order
// with extensions for the additional Shattered Arcana systems.
// ═══════════════════════════════════════════════════════════════════════════

UENUM(BlueprintType)
enum class ECoMTurnPhase : uint8
{
	Movement    UMETA(DisplayName = "Movement"),
	Combat      UMETA(DisplayName = "Combat"),
	Siege       UMETA(DisplayName = "Siege"),
	Naval       UMETA(DisplayName = "Naval"),
	Production  UMETA(DisplayName = "Production"),
	Magic       UMETA(DisplayName = "Magic"),
	Heroes      UMETA(DisplayName = "Heroes"),
	Dragons     UMETA(DisplayName = "Dragons"),
	Diplomacy   UMETA(DisplayName = "Diplomacy"),
	Espionage   UMETA(DisplayName = "Espionage"),
	Quests      UMETA(DisplayName = "Quests"),
	FogOfWar    UMETA(DisplayName = "Fog of War"),
	Victory     UMETA(DisplayName = "Victory"),
	MAX         UMETA(Hidden)
};

// ═══════════════════════════════════════════════════════════════════════════
// Delegates
// ═══════════════════════════════════════════════════════════════════════════

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnBegin, int32, TurnNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnEnd, int32, TurnNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseBegin, ECoMTurnPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWizardEliminated, int32, WizardId);

/**
 * Master turn orchestrator.
 *
 * Owns the turn counter, wizard ordering, phase sequencing, and the
 * ProcessFullTurn() routine that calls every subsystem in the correct
 * order.  Human wizards are placed first in the turn order; AI wizards
 * follow.
 *
 * Subsystems are resolved at call time via GetGameInstance()->GetSubsystem<>()
 * (GameInstance subsystems) or GetWorld()->GetSubsystem<>() (World subsystems).
 * No pointers are cached across frames.
 */
UCLASS()
class COMCORE_API UCoMTurnManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -----------------------------------------------------------------
	// Game setup
	// -----------------------------------------------------------------

	/**
	 * Set up wizards, build the turn order (humans first, then AI),
	 * and call world generation.  Resets the turn counter to 1.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Turn")
	void InitializeGame(int32 NumHumans, int32 NumAI);

	// -----------------------------------------------------------------
	// Turn flow
	// -----------------------------------------------------------------

	/**
	 * Mark the current wizard's input phase as done.
	 * Advances CurrentWizardIndex; when all wizards have acted the
	 * simultaneous resolution phase (ProcessFullTurn) is triggered.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Turn")
	void EndPlayerTurn();

	/**
	 * Master orchestration — runs every subsystem in the canonical
	 * phase order, then increments CurrentTurnNumber.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Turn")
	void ProcessFullTurn();

	// -----------------------------------------------------------------
	// Queries
	// -----------------------------------------------------------------

	/** True when the current wizard slot belongs to a human player. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Turn")
	bool IsHumanTurn() const;

	/** Index of the wizard whose input is currently expected. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Turn")
	int32 GetCurrentWizard() const;

	/** The global turn counter (starts at 1). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Turn")
	int32 GetTurnNumber() const;

	/** True when the game has ended (only one wizard left, or victory achieved). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Turn")
	bool IsGameOver() const;

	/** Number of non-eliminated wizards still in the game. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Turn")
	int32 GetActiveWizardCount() const;

	/** Eliminate a wizard (marks them as eliminated, fires delegate). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Turn")
	void EliminateWizard(int32 WizardId);

	// -----------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Turn")
	FOnTurnBegin OnTurnBegin;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Turn")
	FOnTurnEnd OnTurnEnd;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Turn")
	FOnPhaseBegin OnPhaseBegin;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Turn")
	FOnWizardEliminated OnWizardEliminated;

private:
	// -----------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------

	/** Execute a single phase: broadcast delegate, call subsystem, log. */
	void RunPhase(ECoMTurnPhase Phase);

	/** Check whether any wizard qualifies for elimination this turn. */
	void CheckForEliminations();

	/** Placeholder: check for a victory condition. */
	bool CheckVictory() const;

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	/** Global turn counter, 1-based. */
	UPROPERTY()
	int32 CurrentTurnNumber = 0;

	/** Index into TurnOrder for the wizard whose input phase it is. */
	UPROPERTY()
	int32 CurrentWizardIndex = 0;

	/** Ordered list of wizard indices. Humans appear first. */
	UPROPERTY()
	TArray<int32> TurnOrder;

	/** True if the wizard at the given index is human-controlled. */
	UPROPERTY()
	TMap<int32, bool> WizardIsHuman;

	/** True if the wizard has been eliminated from the game. */
	UPROPERTY()
	TMap<int32, bool> WizardEliminated;

	/** The phase currently being executed inside ProcessFullTurn(). */
	UPROPERTY()
	ECoMTurnPhase CurrentPhase = ECoMTurnPhase::Movement;

	/** Set to true when the game ends. */
	UPROPERTY()
	bool bGameOver = false;
};
