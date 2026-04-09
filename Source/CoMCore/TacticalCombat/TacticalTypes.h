// Copyright Mythforge Studios. All Rights Reserved.
// TacticalTypes.h -- Grid-based tactical combat types for Shattered Arcana
#pragma once

#include "CoreMinimal.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "TacticalTypes.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Grid Constants
// ─────────────────────────────────────────────────────────────────────────────

/** Tactical grid width (columns). MoM used 14; we use 12 for tighter engagements. */
constexpr int32 TACTICAL_GRID_WIDTH  = 12;

/** Tactical grid height (rows). */
constexpr int32 TACTICAL_GRID_HEIGHT = 8;

/** Maximum turns before a battle is declared a draw. */
constexpr int32 TACTICAL_MAX_TURNS   = 50;

/** Default ranged attack range (tiles). */
constexpr int32 TACTICAL_DEFAULT_RANGE = 6;

// ─────────────────────────────────────────────────────────────────────────────
// Enums
// ─────────────────────────────────────────────────────────────────────────────

/** Actions a unit can take during its turn on the tactical grid. */
UENUM(BlueprintType)
enum class ECoMTacticalAction : uint8
{
	Move         UMETA(DisplayName = "Move"),
	MeleeAttack  UMETA(DisplayName = "Melee Attack"),
	RangedAttack UMETA(DisplayName = "Ranged Attack"),
	CastSpell    UMETA(DisplayName = "Cast Spell"),
	Defend       UMETA(DisplayName = "Defend"),       // +50% defense this turn
	Wait         UMETA(DisplayName = "Wait"),          // act later in initiative order
	Flee         UMETA(DisplayName = "Flee"),           // attempt to leave combat
	MAX UMETA(Hidden)
};

/** Outcome of a tactical battle. */
UENUM(BlueprintType)
enum class ECoMCombatResult : uint8
{
	AttackerVictory  UMETA(DisplayName = "Attacker Victory"),
	DefenderVictory  UMETA(DisplayName = "Defender Victory"),
	AttackerFlee     UMETA(DisplayName = "Attacker Fled"),
	DefenderFlee     UMETA(DisplayName = "Defender Fled"),
	Draw             UMETA(DisplayName = "Draw"),
	Ongoing          UMETA(DisplayName = "Ongoing"),   // battle not yet resolved
	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// Tile
// ─────────────────────────────────────────────────────────────────────────────

/**
 * A single tile on the 12x8 tactical grid.
 * Walls appear during siege combat on the defender side.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMTacticalTile
{
	GENERATED_BODY()

	/** Terrain type on this tile (copied/generated from overworld). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoMTerrain Terrain = ECoMTerrain::Grassland;

	/** Index into the Units array, or -1 if unoccupied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OccupyingUnitIndex = -1;

	/** True if this tile is a wall segment (siege battles). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsWall = false;

	/** Wall hit points. 0 = no wall or wall destroyed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WallHP = 0;

	/** Elevation: 0 = flat, 1 = hill (+2 def), 2 = high ground (+3 def). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Elevation = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Tactical Unit
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Runtime snapshot of a unit participating in tactical combat.
 * Created from FCoMUnitInstance at battle start; casualties are
 * written back to the overworld unit roster at battle end.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMTacticalUnit
{
	GENERATED_BODY()

	/** ID into the overworld FCoMUnitInstance table (for write-back). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 UnitInstanceId = -1;

	/** Which wizard owns this unit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 OwnerWizardId = -1;

	/** True if on the attacking side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsAttacker = true;

	/** Current position on the grid (column, row). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint GridPosition = FIntPoint(-1, -1);

	// ── Combat Stats ─────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CurrentHP        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxHP            = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MeleeAttack      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RangedAttack     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Defense          = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MovementPoints   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MovementRemaining = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Initiative       = 0;

	// ── Turn State ───────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasActed    = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsDefending = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsDead      = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasFled     = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsHero      = false;

	/** Movement type affects pathfinding (flying ignores terrain, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMovementType MovementType = ECoMMovementType::Walking;

	// ── Helpers ──────────────────────────────────────────────────────────────

	bool IsAlive() const { return !bIsDead && !bHasFled; }
};
