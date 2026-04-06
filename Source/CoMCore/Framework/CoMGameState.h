// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameState.h — Global turn state, wizard roster, world time, and game phase.
// COM-027
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CoreTypes/CoMEnums.h"
#include "CoreTypes/CoMStructs.h"
#include "CoreTypes/CoMConstants.h"
#include "CoMGameState.generated.h"

class ACoMPlayerState;

/**
 * ACoMGameState
 *
 * Replicated authority for all global turn-state: current phase, world time,
 * discovered planes, the full wizard roster, the diplomacy matrix, active
 * global enchantments, and in-progress rituals.
 *
 * The engine's PlayerArray (AGameStateBase) is preserved.  WizardStates is a
 * parallel, index-addressed view for O(1) per-wizard lookups without a linear
 * search through PlayerArray.
 */
UCLASS()
class COMCORE_API ACoMGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACoMGameState();

	// ─── Turn & Phase ─────────────────────────────────────────────────────────

	/** Monotonically increasing turn counter (starts at 1). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn")
	int32 CurrentTurn = 1;

	/** The active phase within the current turn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn")
	ECoMGamePhase CurrentPhase = ECoMGamePhase::WorldProcessing;

	/**
	 * Index (0–MAX_WIZARDS-1) of the wizard currently acting.
	 * CoM::WIZARD_INDEX_NONE during WorldProcessing and EndOfTurn phases.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn")
	int32 ActiveWizardIndex = CoM::WIZARD_INDEX_NONE;

	// ─── World Time ───────────────────────────────────────────────────────────

	/** In-game calendar year (starts at 1400, honouring the MoM convention). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time")
	int32 WorldYear = 1400;

	/** Season index within the current year: 0=Spring, 1=Summer, 2=Autumn, 3=Winter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time")
	int32 WorldSeasonIndex = 0;

	/** Turn within the current season (1-based; resets at each season boundary). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time")
	int32 SeasonTurn = 1;

	// ─── Plane Visibility ─────────────────────────────────────────────────────

	/**
	 * Planes entered or scried by at least one wizard this game.
	 * Aurelith is pre-populated in the constructor; others unlock on first entry.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
	TSet<ECoMPlane> DiscoveredPlanes;

	// ─── Wizard Roster ────────────────────────────────────────────────────────

	/**
	 * Index-addressed wizard state pointers (slot == WizardIndex).
	 * Pre-allocated to CoM::MAX_WIZARDS entries; null slots are unoccupied.
	 * Kept in sync via AddPlayerState / RemovePlayerState overrides.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizards")
	TArray<TObjectPtr<ACoMPlayerState>> WizardStates;

	// ─── Diplomacy ────────────────────────────────────────────────────────────

	/** Bilateral diplomatic relations; one entry per normalised (A < B) wizard pair. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Diplomacy")
	TArray<FCoMDiplomacyRelation> DiplomacyMatrix;

	// ─── Magic ────────────────────────────────────────────────────────────────

	/**
	 * World-wide overworld enchantments currently active.
	 * Entries are CoMSpellDataAsset row names.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Magic")
	TArray<FName> ActiveGlobalEnchantments;

	/** All multi-caster rituals currently in progress. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Magic")
	TArray<FCoMActiveRitual> ActiveRituals;

	// ─── Accessors ────────────────────────────────────────────────────────────

	/** Returns the wizard state for the given index, or nullptr if invalid/empty. */
	UFUNCTION(BlueprintPure, Category = "Wizards")
	ACoMPlayerState* GetWizardByIndex(int32 WizardIndex) const;

	UFUNCTION(BlueprintPure, Category = "Turn")
	ECoMGamePhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Turn")
	int32 GetCurrentTurn() const { return CurrentTurn; }

	/**
	 * Looks up the diplomacy relation between two wizards.
	 * Normalises (A, B) so A < B before searching DiplomacyMatrix.
	 * Returns true and fills OutRelation on success.
	 */
	UFUNCTION(BlueprintPure, Category = "Diplomacy")
	bool GetDiplomacyRelation(int32 WizardA, int32 WizardB,
	                          FCoMDiplomacyRelation& OutRelation) const;

	// ─── Overrides ────────────────────────────────────────────────────────────

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void BeginPlay() override;

private:
	/** UCoMTurnSubsystem is the sole caller of TickWorldTime(). */
	friend class UCoMTurnSubsystem;

	/** Advance SeasonTurn / WorldSeasonIndex / WorldYear at the top of each turn. */
	void TickWorldTime();
};
