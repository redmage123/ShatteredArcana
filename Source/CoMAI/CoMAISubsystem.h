// Copyright Mythforge Studios. All Rights Reserved.
// CoMAISubsystem.h -- Main entry point for the AI module. Owns planner, executor, and difficulty.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMAITypes.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMAISubsystem.generated.h"

class UCoMAIStrategicPlanner;
class UCoMAITacticalExecutor;
class UCoMAIDifficultyModifier;

/**
 * UCoMAISubsystem
 *
 * GameInstance subsystem that orchestrates all AI wizard turns. Owns the strategic
 * planner and tactical executor as child UObjects, and exposes the difficulty modifier.
 *
 * Self-registers with UCoMTurnSubsystem::OnGamePhaseChanged so that AI turns are
 * processed automatically when the game enters ECoMGamePhase::AITurn -- no circular
 * module dependency required.
 *
 * Usage:
 *   1. Game start: call SetDifficulty() based on player choice.
 *   2. Each turn: the delegate fires ProcessAITurns() during AITurn phase.
 *   3. ProcessAITurns iterates AI wizards, evaluates strategy, then executes it.
 */
UCLASS()
class COMAI_API UCoMAISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- Main entry point ------------------------------------------------

	/**
	 * Process AI turns for all AI-controlled wizards.
	 * Can be called directly, but is also auto-invoked via OnGamePhaseChanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoMAI")
	void ProcessAITurns();

	// ---- Configuration ---------------------------------------------------

	/** Set the AI difficulty level for the current game. */
	UFUNCTION(BlueprintCallable, Category = "CoMAI")
	void SetDifficulty(ECoMAIDifficulty NewDifficulty);

	/** Get the current AI difficulty level. */
	UFUNCTION(BlueprintPure, Category = "CoMAI")
	ECoMAIDifficulty GetDifficulty() const { return CurrentDifficulty; }

	// ---- Queries ---------------------------------------------------------

	/**
	 * Returns true if the wizard at WizardId is AI-controlled.
	 * A wizard is AI-controlled if it has a valid slot in WizardStates
	 * but no owning APlayerController.
	 */
	UFUNCTION(BlueprintPure, Category = "CoMAI")
	bool IsAIWizard(int32 WizardId) const;

	/** Get the most recent strategy computed for a wizard (for debug display). */
	UFUNCTION(BlueprintPure, Category = "CoMAI")
	FCoMAIStrategy GetLastStrategy(int32 WizardId) const;

	/** Get the difficulty modifier object (for other subsystems that need to apply bonuses). */
	UFUNCTION(BlueprintPure, Category = "CoMAI")
	UCoMAIDifficultyModifier* GetDifficultyModifier() const { return DifficultyModifier; }

private:
	/** Delegate callback: fires when the game phase changes. */
	UFUNCTION()
	void OnGamePhaseChanged(ECoMGamePhase OldPhase, ECoMGamePhase NewPhase);

	UPROPERTY()
	TObjectPtr<UCoMAIStrategicPlanner> Planner;

	UPROPERTY()
	TObjectPtr<UCoMAITacticalExecutor> Executor;

	UPROPERTY()
	TObjectPtr<UCoMAIDifficultyModifier> DifficultyModifier;

	UPROPERTY()
	ECoMAIDifficulty CurrentDifficulty = ECoMAIDifficulty::Normal;

	/** Cached strategies per wizard for debug access. */
	UPROPERTY()
	TMap<int32, FCoMAIStrategy> LastStrategies;

	/** Whether we have bound to the turn subsystem delegate. */
	bool bDelegateBound = false;
};
