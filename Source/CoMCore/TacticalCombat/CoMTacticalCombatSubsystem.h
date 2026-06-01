// Copyright Mythforge Studios. All Rights Reserved.
// CoMTacticalCombatSubsystem.h -- Grid-based tactical battle state machine
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/Framework/CoMGameInstance.h"
#include "CoMCore/TacticalCombat/TacticalTypes.h"
#include "CoMCore/Turn/CoMDeterministicRandom.h"
#include "CoMTacticalCombatSubsystem.generated.h"

struct FCoMCombatResult;

// ═══════════════════════════════════════════════════════════════════════════
// Delegates
// ═══════════════════════════════════════════════════════════════════════════

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalBattleStarted, int32, TurnCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalUnitTurnStarted, int32, UnitIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTacticalUnitMoved, int32, UnitIndex, FIntPoint, NewPosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTacticalUnitDamaged, int32, UnitIndex, int32, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalUnitKilled, int32, UnitIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalBattleEnded, ECoMCombatResult, Result);

// ═══════════════════════════════════════════════════════════════════════════
// A* pathfinding node (internal)
// ═══════════════════════════════════════════════════════════════════════════

struct FTacticalPathNode
{
	FIntPoint Position;
	int32 GCost  = 0;  // Cost from start
	int32 HCost  = 0;  // Heuristic (Manhattan distance to target)
	int32 FCost() const { return GCost + HCost; }
	FIntPoint Parent = FIntPoint(-1, -1);
};

// ═══════════════════════════════════════════════════════════════════════════
// UCoMTacticalCombatSubsystem
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Manages the full lifecycle of a MoM-style tactical grid battle.
 *
 * Lifecycle:
 *   1. ACoMCombatGameMode calls InitializeBattle() with the CombatContext
 *   2. Subsystem generates the grid, places units, rolls initiative
 *   3. Turns proceed unit-by-unit in initiative order
 *   4. Player/AI issues actions (Move, Attack, Defend, etc.)
 *   5. When one side is eliminated, FinalizeBattle() writes casualties back
 *
 * This subsystem lives on the GameInstance so it persists across any
 * sub-level streaming that might occur within the combat map.
 */
UCLASS()
class COMCORE_API UCoMTacticalCombatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ─── Setup ───────────────────────────────────────────────────────────────

	/** Initialize a new tactical battle from the CombatContext. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	void InitializeBattle(const FCoMCombatContext& Context);

	/**
	 * Run a tactical battle headlessly to completion and return the result.
	 * Both sides act via DecideAIAction/ExecuteAITurn. Used by the playtest
	 * harness and AI-vs-AI overworld auto-resolve so every battle has real
	 * tactical fidelity instead of stat-comparison roulette.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	struct FCoMCombatResult RunHeadlessBattle(const FCoMCombatContext& Context, int32 MaxRounds = 30);

	/** Generate the tactical grid based on overworld terrain. */
	void GenerateGrid(ECoMTerrain OverworldTerrain, bool bIsSiege);

	/** Place attacker and defender units onto the grid. */
	void PlaceUnits(const TArray<int32>& AttackerUnitIDs, int32 AttackerWizardId,
	                const TArray<int32>& DefenderUnitIDs, int32 DefenderWizardId);

	/** Roll initiative for all units and sort the turn order. */
	void RollInitiative();

	// ─── Turn Flow ───────────────────────────────────────────────────────────

	/** Begin the current unit's turn (reset movement, clear defend). */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	void BeginUnitTurn(int32 UnitIndex);

	/** End the current unit's turn and advance to the next. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	void EndUnitTurn();

	/** Advance to the next unit in initiative order. */
	void NextUnit();

	// ─── Actions ─────────────────────────────────────────────────────────────

	/** Move a unit along the shortest path to TargetTile. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	bool MoveUnit(int32 UnitIndex, FIntPoint TargetTile);

	/** Execute a melee attack from one unit against an adjacent enemy. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	bool MeleeAttack(int32 AttackerIndex, int32 DefenderIndex);

	/** Execute a ranged attack against an enemy within range. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	bool RangedAttack(int32 AttackerIndex, int32 DefenderIndex);

	/** Cast a combat spell at a target tile. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	bool CastCombatSpell(int32 CasterIndex, FName SpellId, FIntPoint Target);

	/** Unit assumes defensive stance (+50% defense until next turn). */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	bool Defend(int32 UnitIndex);

	/** Unit waits, pushed to end of current initiative round. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	bool Wait(int32 UnitIndex);

	/** Attempt to flee combat. 50% base + speed advantage. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	bool AttemptFlee(int32 UnitIndex);

	// ─── Queries ─────────────────────────────────────────────────────────────

	/** Get all tiles a unit can reach this turn (A* reachability). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	TArray<FIntPoint> GetValidMoves(int32 UnitIndex) const;

	/** Get all enemy unit indices adjacent to the given unit. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	TArray<int32> GetValidMeleeTargets(int32 UnitIndex) const;

	/** Get all enemy unit indices within ranged attack range. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	TArray<int32> GetValidRangedTargets(int32 UnitIndex) const;

	/** True if units A and B are on adjacent tiles (8-directional). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	bool IsUnitAdjacent(int32 A, int32 B) const;

	/** Manhattan distance between two grid positions. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	static int32 GetManhattanDistance(FIntPoint A, FIntPoint B);

	/** Get the tactical unit occupying a grid position, or nullptr. */
	const FCoMTacticalUnit* GetUnitAt(FIntPoint Pos) const;

	/** Check if a tile is passable for a given movement type. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	bool IsTilePassable(FIntPoint Pos, ECoMMovementType MoveType) const;

	/** True if the battle is currently active. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	bool IsBattleActive() const { return bBattleActive; }

	/** Get the current combat result (Ongoing if battle is active). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	ECoMCombatResult GetResult() const { return Result; }

	/** Get current turn count. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	int32 GetTurnCount() const { return TurnCount; }

	/** Get the index of the unit whose turn it is. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|TacticalCombat")
	int32 GetCurrentUnitIndex() const { return CurrentUnitIndex; }

	/** Access the full Units array (read-only). */
	const TArray<FCoMTacticalUnit>& GetUnits() const { return Units; }

	/** Access the grid. */
	const TArray<TArray<FCoMTacticalTile>>& GetGrid() const { return Grid; }

	// ─── AI ──────────────────────────────────────────────────────────────────

	/**
	 * Decide what action an AI-controlled unit should take.
	 * Simple priority: adjacent enemy -> melee; ranged target -> ranged;
	 * can reach enemy -> move toward; otherwise -> defend.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	ECoMTacticalAction DecideAIAction(int32 UnitIndex);

	/** Execute the AI decision for a unit (calls DecideAIAction + performs it). */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	void ExecuteAITurn(int32 UnitIndex);

	/** Threat-aware AI target selection. Prefers killable, high-damage, low-HP units. */
	int32 PickBestTarget(const TArray<int32>& Candidates) const;

	// ─── Resolution ──────────────────────────────────────────────────────────

	/** Check if the battle has ended (one side wiped/fled, or turn limit). */
	void CheckBattleEnd();

	/** Apply casualties back to overworld armies and distribute XP. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TacticalCombat")
	void FinalizeBattle();

	// ─── Delegates ───────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "CoM|TacticalCombat")
	FOnTacticalBattleStarted OnBattleStarted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|TacticalCombat")
	FOnTacticalUnitTurnStarted OnUnitTurnStarted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|TacticalCombat")
	FOnTacticalUnitMoved OnUnitMoved;

	UPROPERTY(BlueprintAssignable, Category = "CoM|TacticalCombat")
	FOnTacticalUnitDamaged OnUnitDamaged;

	UPROPERTY(BlueprintAssignable, Category = "CoM|TacticalCombat")
	FOnTacticalUnitKilled OnUnitKilled;

	UPROPERTY(BlueprintAssignable, Category = "CoM|TacticalCombat")
	FOnTacticalBattleEnded OnBattleEnded;

private:
	// ─── Internal Combat Resolution ──────────────────────────────────────────

	/** Resolve a melee hit (hit chance, damage, terrain modifiers). */
	void ResolveMeleeHit(FCoMTacticalUnit& Attacker, FCoMTacticalUnit& Defender);

	/** Resolve a ranged hit (range penalty, defense halved). */
	void ResolveRangedHit(FCoMTacticalUnit& Attacker, FCoMTacticalUnit& Defender);

	/** Apply raw damage to a unit, check for death. */
	void ApplyDamage(FCoMTacticalUnit& Target, int32 Damage);

	/** Kill a unit: mark dead, free its tile. */
	void KillUnit(FCoMTacticalUnit& Unit);

	// ─── Pathfinding ─────────────────────────────────────────────────────────

	/** A* pathfinding on the tactical grid. Returns path (start excluded, target included). */
	TArray<FIntPoint> FindPath(FIntPoint Start, FIntPoint End, ECoMMovementType MoveType) const;

	/** Movement cost for entering a tile. Returns -1 if impassable. */
	int32 GetTileCost(FIntPoint Pos, ECoMMovementType MoveType) const;

	/** Get the terrain defense bonus for a tile's elevation. */
	int32 GetElevationDefenseBonus(FIntPoint Pos) const;

	/** Find the nearest enemy index to a given unit. Returns -1 if none. */
	int32 FindNearestEnemy(int32 UnitIndex) const;

	/** True if the given grid coordinate is within bounds. */
	static bool IsInBounds(FIntPoint Pos);

	// ─── State ───────────────────────────────────────────────────────────────

	/** 2D grid: Grid[Row][Column]. */
	TArray<TArray<FCoMTacticalTile>> Grid;

	/** All units participating in this battle. */
	UPROPERTY()
	TArray<FCoMTacticalUnit> Units;

	/** Unit indices sorted by initiative (descending — highest acts first). */
	TArray<int32> InitiativeOrder;

	/** Position within InitiativeOrder — whose turn it is. */
	int32 CurrentInitiativePos = 0;

	/** Convenience: the actual unit index of the current unit. */
	int32 CurrentUnitIndex = -1;

	/** Number of full rounds completed. */
	int32 TurnCount = 0;

	/** True while a battle is in progress. */
	bool bBattleActive = false;

	/** Battle outcome (Ongoing while active). */
	ECoMCombatResult Result = ECoMCombatResult::Ongoing;

	/** Stored combat context for finalization. */
	FCoMCombatContext StoredContext;

	/** Deterministic RNG seeded from the overworld tile position. */
	FCoMDeterministicRandom Rng;
};
