// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMVictorySubsystem.h -- Victory condition evaluation, elimination tracking, and endgame state.
// Phase 8 -- Shattered Arcana

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMVictorySubsystem.generated.h"

class UCoMCitySubsystem;
class UCoMDiplomacySubsystem;
class UCoMMagicSubsystem;
class UCoMTurnSubsystem;
class UCoMAudioSubsystem;
class ACoMGameState;
class ACoMPlayerState;

// ---------------------------------------------------------------------------
// Delegates
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVictoryWizardEliminated, int32, WizardId, int32, ConquerorId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVictoryDeclared, int32, WizardId, ECoMVictoryType, VictoryType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefeat, int32, WizardId);

// ---------------------------------------------------------------------------
// Score breakdown (returned by CalculateScore for display)
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct COMCORE_API FCoMVictoryScoreEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 WizardId = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TotalScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CityScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 UnitScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SpellScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TurnScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 HeroScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 DragonScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 VictoryBonus = 0;
};

// ---------------------------------------------------------------------------
// Victory Subsystem
// ---------------------------------------------------------------------------

/**
 * UCoMVictorySubsystem
 *
 * GameInstance subsystem that evaluates all victory / defeat conditions each
 * turn, tracks eliminated wizards, declares victories, triggers audio and UI,
 * and provides final scoring for the endgame screen.
 *
 * Called by UCoMTurnSubsystem at the end of each turn cycle.
 *
 * Victory priority (first match wins per turn):
 *   1. Spell of Mastery (Magical) -- instant on successful cast
 *   2. Domination -- control all capital cities
 *   3. Conquest -- last wizard standing
 *   4. Diplomatic -- alliance with all survivors (min 4 alive)
 *   5. Pact -- contracts with 3 Archdevils
 *   6. Dissolution -- 3 planes destabilized + Sael-Thrix slain
 */
UCLASS()
class COMCORE_API UCoMVictorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// -- USubsystem interface -------------------------------------------------

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -- Core check (called each turn) ----------------------------------------

	/**
	 * Main entry point. Evaluates elimination, then checks all victory
	 * conditions for every surviving wizard. Call after all subsystem
	 * processing in the turn cycle.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	void CheckVictoryConditions(int32 CurrentTurn);

	// -- Individual condition checks ------------------------------------------

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	bool CheckDomination(int32 WizardId) const;

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	bool CheckSpellOfMastery(int32 WizardId) const;

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	bool CheckDiplomaticVictory(int32 WizardId) const;

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	bool CheckPactVictory(int32 WizardId) const;

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	bool CheckDissolutionVictory(int32 WizardId) const;

	// -- Elimination ----------------------------------------------------------

	/** Mark a wizard as eliminated and handle cleanup. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	void EliminateWizard(int32 WizardId, int32 ConquerorId = -1);

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	bool IsEliminated(int32 WizardId) const;

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	TArray<int32> GetEliminatedWizards() const { return EliminatedWizards; }

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	int32 GetSurvivingWizardCount() const;

	// -- Victory declaration --------------------------------------------------

	/** Declare a victory: sets game-over state, plays audio, fires delegates. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	void DeclareVictory(int32 WizardId, ECoMVictoryType Type);

	// -- Queries --------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	bool IsGameOver() const { return bGameOver; }

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	int32 GetWinningWizard() const { return WinningWizardId; }

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	ECoMVictoryType GetWinningCondition() const { return WinningCondition; }

	// -- Scoring --------------------------------------------------------------

	/** Calculate a wizard's score. Includes a 1000-point bonus for the winner. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	int32 CalculateScore(int32 WizardId) const;

	/** Detailed score breakdown for the victory screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	FCoMVictoryScoreEntry CalculateDetailedScore(int32 WizardId) const;

	/** Get sorted rankings (highest score first) for all wizards. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	TArray<FCoMVictoryScoreEntry> GetAllScores() const;

	// -- Dissolution flags (set by quest/combat subsystems) -------------------

	/** Mark a plane as destabilized (>50% corruption). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	void SetPlaneDestabilized(ECoMPlane Plane, bool bDestabilized);

	/** Mark Sael-Thrix as slain. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	void SetSaelThrixSlain(bool bSlain) { bSaelThrixSlain = bSlain; }

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	bool IsSaelThrixSlain() const { return bSaelThrixSlain; }

	UFUNCTION(BlueprintPure, Category = "CoM|Victory")
	int32 GetDestabilizedPlaneCount() const { return DestabilizedPlanes.Num(); }

	// -- Spell of Mastery notification ----------------------------------------

	/** Call when a wizard begins researching a Spell of Mastery. Notifies all. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	void NotifySpellOfMasteryResearchStarted(int32 WizardId);

	/** Call when a wizard successfully casts a Spell of Mastery. Instant win. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	void NotifySpellOfMasteryCast(int32 WizardId);

	// -- Delegates ------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Victory")
	FOnVictoryWizardEliminated OnWizardEliminated;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Victory")
	FOnVictoryDeclared OnVictoryDeclared;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Victory")
	FOnDefeat OnDefeat;

	// -- Save/Load Export/Import ----------------------------------------------

	/** Export eliminated wizards for save serialization. */
	TArray<int32> ExportEliminatedWizards() const { return EliminatedWizards; }

	/** Import eliminated wizards from save data. */
	void ImportEliminatedWizards(const TArray<int32>& Data);

	/** Full state export for save system. */
	void ExportAll(TArray<int32>& OutEliminated, bool& OutGameOver,
	               int32& OutWinningWizardId, ECoMVictoryType& OutWinningCondition,
	               TArray<ECoMPlane>& OutDestabilizedPlanes, bool& OutSaelThrixSlain,
	               bool& OutSpellOfMasteryCast, int32& OutMasteryCasterWizardId) const;

	/** Full state import from save data. */
	void ImportAll(const TArray<int32>& InEliminated, bool InGameOver,
	               int32 InWinningWizardId, ECoMVictoryType InWinningCondition,
	               const TArray<ECoMPlane>& InDestabilizedPlanes, bool InSaelThrixSlain,
	               bool InSpellOfMasteryCast, int32 InMasteryCasterWizardId);

private:
	// -- Internal helpers -----------------------------------------------------

	/** Check if any wizard has 0 cities and should be eliminated. */
	void CheckForEliminations(int32 CurrentTurn);

	/** Get all surviving (non-eliminated) wizard indices. */
	TArray<int32> GetSurvivingWizardIndices() const;

	/** Determine if a wizard is the human player (for defeat notifications). */
	bool IsHumanPlayer(int32 WizardId) const;

	/** Get the total wizard count from game state. */
	int32 GetTotalWizardCount() const;

	/** Collect the original capital city IDs for all wizards. */
	TMap<int32, int32> GetOriginalCapitals() const;

	// -- State ----------------------------------------------------------------

	/** Wizards that have been eliminated from the game. */
	UPROPERTY()
	TArray<int32> EliminatedWizards;

	/** Map of wizard->victory type for any wizard that has won. */
	UPROPERTY()
	TMap<int32, ECoMVictoryType> WizardVictories;

	/** True once any victory has been declared. */
	bool bGameOver = false;

	/** The wizard who won. */
	int32 WinningWizardId = -1;

	/** How the game was won. */
	ECoMVictoryType WinningCondition = ECoMVictoryType::MAX;

	// -- Dissolution tracking -------------------------------------------------

	/** Planes that have been destabilized (>50% corruption). */
	UPROPERTY()
	TSet<ECoMPlane> DestabilizedPlanes;

	/** Whether Sael-Thrix has been slain. */
	bool bSaelThrixSlain = false;

	// -- Spell of Mastery tracking --------------------------------------------

	/** True if a wizard has successfully cast a Spell of Mastery. */
	bool bSpellOfMasteryCast = false;

	/** Which wizard cast the Spell of Mastery. */
	int32 MasteryCasterWizardId = -1;

	// -- Constants ------------------------------------------------------------

	/** Minimum surviving wizards for Diplomatic victory. */
	static constexpr int32 MinWizardsForDiplomaticVictory = 4;

	/** Number of Archdevil pacts required for Pact victory. */
	static constexpr int32 RequiredArchdevilPacts = 3;

	/** Number of destabilized planes required for Dissolution victory. */
	static constexpr int32 RequiredDestabilizedPlanes = 3;

	// -- Score constants ------------------------------------------------------

	static constexpr int32 ScorePerCity = 100;
	static constexpr int32 ScorePerUnit = 10;
	static constexpr int32 ScorePerSpell = 20;
	static constexpr int32 ScorePerTurn = 1;
	static constexpr int32 ScorePerHero = 50;
	static constexpr int32 ScorePerDragon = 200;
	static constexpr int32 ScoreVictoryBonus = 1000;
};
