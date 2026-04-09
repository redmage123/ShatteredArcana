// Copyright Mythforge Studios. All Rights Reserved.
// CoMStructs.h — Core data structures for Shattered Arcana
// Sourced from game-design/master-plan.md
// Platform-agnostic: no OS/platform/rendering headers

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMCore/CoreTypes/CoMFixedPoint.h"
#include "CoMCore/Turn/CoMCommandQueue.h"
#include "CoMCore/Turn/CoMDeterministicRandom.h"
#include "CoMStructs.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// FORWARD DECLARATIONS
// ─────────────────────────────────────────────────────────────────────────────

// (used in struct fields that reference DataAssets — see Data/ subdir)
class UCoMUnitSpecDataAsset;

// ─────────────────────────────────────────────────────────────────────────────
// CORE MAP / TILE
// ─────────────────────────────────────────────────────────────────────────────

/**
 * All data for a single map tile.
 * Used in both the 160×100 surface/Underdark grids and the underwater zone sub-grids.
 * Kept as a plain USTRUCT (no UObject overhead) — stored in flat TArrays.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMTileData
{
	GENERATED_BODY()

	/** Grid position within the layer (0–159, 0–99 for main grids). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint Position = FIntPoint(0, 0);

	/** Terrain type — determines movement cost, food yield, terrain features. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMTerrain Terrain = ECoMTerrain::Grassland;

	/** Which resource node (if any) is on this tile. None = ECoMResource::None. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMResource Resource = ECoMResource::None;

	/** Wizard index of the city that owns this tile (-1 = unclaimed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 OwnerWizardIndex = CoM::WIZARD_INDEX_NONE;

	/** ID of the city at this tile (-1 = none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CityID = -1;

	/** ID of the road/highway on this tile (0 = none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RoadLevel = 0;

	/** Per-wizard fog-revealed bitmask (bit N = wizard N has ever seen this tile). */
	UPROPERTY(EditAnywhere) uint32 FogRevealed = 0;

	/** Per-wizard current-vision bitmask (bit N = wizard N sees this tile now). */
	UPROPERTY(EditAnywhere) uint32 CurrentVision = 0;

	/** Active corruption type on this tile (None = uncorrupted). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMCorruptionType Corruption = ECoMCorruptionType::None;

	/** Ley line presence: bitmask of ley line IDs passing through (compact encoding). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> LeyLineIDs;

	/** Portal ID at this tile (-1 = none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PortalID = -1;

	/** Mine ID at this tile (-1 = none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MineID = -1;

	/** Explorable site ID at this tile (-1 = none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SiteID = -1;

	/** Movement cost modifier (1.0 = normal). Enchantments and corruption can change this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 MoveCostModifier = FFixed64(1);

	/** True if this tile is impassable to all ground units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bImpassable = false;

	/** True if this tile has been modified by magic (corruption, growth, etc). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bMagicallyAltered = false;
};

/**
 * A full 160×100 map layer (surface or Underdark for a given plane).
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMMapLayerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane    Plane = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer Layer = ECoMMapLayer::Surface;

	/** 16,000 tiles in row-major order: Index = Y * MAP_WIDTH + X */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMTileData> Tiles;

	/** Convenience: returns tile index for a given position. */
	FORCEINLINE int32 TileIndex(int32 X, int32 Y) const
	{
		// Horizontal wrap (east/west)
		if (CoM::MAP_WRAP_X)
		{
			X = ((X % CoM::MAP_WIDTH) + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
		}
		return FMath::Clamp(Y, 0, CoM::MAP_HEIGHT - 1) * CoM::MAP_WIDTH
		     + FMath::Clamp(X, 0, CoM::MAP_WIDTH - 1);
	}

	FORCEINLINE FCoMTileData* GetTile(int32 X, int32 Y)
	{
		const int32 Idx = TileIndex(X, Y);
		return Tiles.IsValidIndex(Idx) ? &Tiles[Idx] : nullptr;
	}

	FORCEINLINE const FCoMTileData* GetTile(int32 X, int32 Y) const
	{
		const int32 Idx = TileIndex(X, Y);
		return Tiles.IsValidIndex(Idx) ? &Tiles[Idx] : nullptr;
	}
};

/**
 * All map data for one of the eight planes:
 * a surface layer, an Underdark layer, and a set of underwater zones.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMPlaneData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane    PlaneID       = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSpellRealm AlignedRealm = ECoMSpellRealm::Life;

	/** Surface 160×100 map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FCoMMapLayerData Surface;

	/** Underdark 160×100 map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FCoMMapLayerData Underdark;

	/** Mana multiplier for all spellcasting on this plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 ManaMultiplier = FFixed64(1);
};

/**
 * A single underwater zone (network of ~20–40 tiles beneath ocean tiles).
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMUnderwaterZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32      ZoneID       = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane  Plane        = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint  SurfaceCenter = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32      Radius       = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32      Depth        = 1;  // Affects pressure/light
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMTileData> Tiles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>  ConnectedZoneIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FIntPoint> SurfaceAccessPoints;
};

// ─────────────────────────────────────────────────────────────────────────────
// STAT MODIFIER
// ─────────────────────────────────────────────────────────────────────────────

/**
 * A single stat modifier — flat bonus or percentage multiplier on a named stat.
 * Used by enchantments, skills, items, artifacts, world events, terrain, etc.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMStatModifier
{
	GENERATED_BODY()

	/** Which stat is being modified (matches FName keys in UCoMStatBlock). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName StatName;

	/** Flat additive value (e.g., +3 attack). Applied before multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 FlatBonus;

	/** Percentage multiplier (1.0 = no change, 1.25 = +25%, 0.75 = -25%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 Multiplier = FFixed64(1);

	/** Source tag — identifies where this modifier came from for stacking rules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag SourceTag;

	/** True if this modifier is permanent; false if it expires with its parent effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPermanent = false;

	/** Remaining duration in turns (-1 = indefinite). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 DurationTurns = -1;
};

// ─────────────────────────────────────────────────────────────────────────────
// SKILL ENTRY
// ─────────────────────────────────────────────────────────────────────────────

/**
 * A unit ability or special skill reference.
 * Used in unit data assets, rune effects, hero abilities, dragon innate skills, etc.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMSkillEntry
{
	GENERATED_BODY()

	/** The skill / ability identifier (matches UCoMSkillDataAsset PrimaryAssetId). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SkillID;

	/** Skill level or potency (meaning depends on the skill). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Level = 1;

	/** Optional numeric parameter (e.g., range, radius, mana cost override). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 Parameter;

	/** Gameplay tag for category / filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag CategoryTag;

	/** True if this skill is innate (always active) vs. activated (requires action). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bInnate = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// ENCHANTMENT INSTANCE
// ─────────────────────────────────────────────────────────────────────────────

/**
 * A live enchantment applied to any enchantable object (unit, building, tile, etc.).
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMEnchantmentInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32        EnchantmentID  = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName        SpellID;          // Source spell
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32        CasterWizardIndex = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32        TurnsRemaining    = -1; // -1 = permanent
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64     UpkeepManaCost;            // Per turn
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMStatModifier> Modifiers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMSkillEntry>   GrantedSkills;
};

// ─────────────────────────────────────────────────────────────────────────────
// WEATHER
// ─────────────────────────────────────────────────────────────────────────────

/** A single weather zone moving across the map. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMWeatherZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint         Center = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             Radius         = 15;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMWeatherType   Type           = ECoMWeatherType::Clear;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64          Intensity      = FFixed64::Half(); // 0.0–1.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             TurnsRemaining = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64          MovementDirX     = FFixed64(1);   // Deterministic replacement for FVector2D
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64          MovementDirY     = FFixed64(0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64          MovementSpeed  = FFixed64(1);      // Tiles/turn
};

/** Aggregated weather gameplay effects for resolving modifiers. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMWeatherEffects
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MovementPenalty   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RangedAttackPen   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MeleeAttackPen    = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SightReduction    = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 ChaosMagicMult    = FFixed64(1);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 NatureMagicMult   = FFixed64(1);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 DeathMagicMult    = FFixed64(1);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 SorceryMagicMult  = FFixed64(1);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 FireMagicMult     = FFixed64(1);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 LifeMagicMult     = FFixed64(1);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool  bFlyingGrounded   = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AttritionDamage   = 0; // Per turn to exposed units
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 AmbushChanceBonus;
};

/** Per-plane current weather state. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMWeatherState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMWeatherZone> ActiveZones;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                   SeasonTurnCounter = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSeason              CurrentSeason = ECoMSeason::Season1;
};

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL
// ─────────────────────────────────────────────────────────────────────────────

/** A portal — inter-tile connection (surface-to-surface, surface-to-Underdark, or inter-plane). */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMPortal
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            PortalID           = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPortalType   Type               = ECoMPortalType::NaturalGateway;

	// Source
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane        SourcePlane        = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer     SourceLayer        = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint        SourcePosition = FIntPoint(0, 0);

	// Destination
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane        DestPlane          = ECoMPlane::Noctharion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer     DestLayer          = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint        DestPosition = FIntPoint(0, 0);

	// Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool             bBidirectional     = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool             bAlwaysActive      = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMWeatherType  RequiredWeather    = ECoMWeatherType::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            ActivationManaCost = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool             bDiscovered        = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool             bPlayerBuilt       = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            OwnerWizardIndex   = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            Capacity           = 0; // 0 = unlimited
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool             bShifting          = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            ShiftIntervalTurns = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMEnchantmentInstance> Enchantments;
};

// ─────────────────────────────────────────────────────────────────────────────
// LEY LINES
// ─────────────────────────────────────────────────────────────────────────────

/** A single magical energy current flowing across a plane. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMLeyLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           LeyLineID         = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane       Plane             = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer    Layer             = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSpellRealm  Alignment         = ECoMSpellRealm::Arcane;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FIntPoint> Path;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64         PowerLevel        = FFixed64(1); // 1–10
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           SourceNodeID      = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           DestNodeID        = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool            bSevered          = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           ControllingWizard = CoM::WIZARD_INDEX_NONE;
};

/** A ley line intersection — strategic node where multiple lines converge. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMLeyIntersection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32        IntersectionID    = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint    Position = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane    Plane             = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer Layer             = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> ConvergingLeyLineIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64      CombinedPower;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32        ControllingWizard = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool         bHasNexus         = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// UNITS
// ─────────────────────────────────────────────────────────────────────────────

/**
 * All base combat stats for a unit spec.
 * Actual instance values are resolved via FCoMStatResolver from this block + modifiers.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMStatBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MeleeAttack    = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RangedAttack   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Defense        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Resistance     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HitPoints      = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Movement       = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Figures        = 1;  // Units in the stack
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Upkeep         = 0;  // Gold/turn
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ProductionCost = 1;
};

/** A live unit instance (one "piece" on the map or in a battle). */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMUnitInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             UnitID            = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName             SpecID;           // UCoMUnitSpecDataAsset
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             OwnerWizardIndex  = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag      RaceTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             CurrentHP         = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             MaxHP             = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             Experience        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             Level             = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMSkillEntry>         Skills;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMEnchantmentInstance> Enchantments;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool              bIsHero           = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool              bFlying           = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool              bIsSettler        = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMovementType  MovementType      = ECoMMovementType::Walking;

	// COM-056a: plane-restricted units (Three Planes expansion).
	// If true, this unit cannot enter any plane other than NativePlane (UCoMWorldMapSubsystem enforces this).
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool              bHomePlaneOnly    = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane         NativePlane       = ECoMPlane::MAX; // MAX = unrestricted
};

/**
 * A stack of up to MAX_ARMY_SIZE units moving on the overworld map.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMArmyGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             ArmyGroupID       = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             OwnerWizardIndex  = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>     UnitIDs;            // Up to MAX_ARMY_SIZE
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint         Position = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane         Plane             = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer      Layer             = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             MovementRemaining = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FIntPoint> PlannedRoute;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool              bInCombat         = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMEnchantmentInstance> GroupEnchantments;
};

// ─────────────────────────────────────────────────────────────────────────────
// HEROES
// ─────────────────────────────────────────────────────────────────────────────

/** A hero's personality profile — drives loyalty, AI behaviour, and relationship formation. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMHeroPersonality
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPersonalityTrait PrimaryTrait   = ECoMPersonalityTrait::Brave;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPersonalityTrait SecondaryTrait = ECoMPersonalityTrait::Loyal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSpellRealm       PreferredRealm = ECoMSpellRealm::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag         PreferredRace;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag         DislikedRace;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                Loyalty        = 75; // 0–100
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                Ambition       = 50; // 0–100
};

/** Relationship record between two heroes. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMHeroRelationship
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  HeroA_UnitID            = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  HeroB_UnitID            = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMHeroRelationType   Type                    = ECoMHeroRelationType::Neutral;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  RelationStrength        = 0; // 0–100
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  TurnsSinceLastInteraction = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// DRAGONS
// ─────────────────────────────────────────────────────────────────────────────

/** A live dragon on the world map. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMDragonInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 DragonID              = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName                 DragonTypeID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText                 PersonalName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMDragonRole        Role                  = ECoMDragonRole::Wild;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMDragonAge         Age                   = ECoMDragonAge::Young;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 TurnsAlive            = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 UnitID                = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 DomainID              = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>         DomainArmyIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 HoardGold             = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 HoardMana             = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>         HoardItemIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 TerritorialAggression = 50; // 0–100
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 BondedHeroUnitID      = -1; // Companion bond
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint             LairPosition          = FIntPoint(0, 0);
};

/** A dragon lord's claimed territory. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMDragonDomain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 DomainID              = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 RulerDragonID         = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane             Plane                 = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer          Layer                 = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint             LairPosition = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 InfluenceRadius       = 8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FIntPoint>     ClaimedTiles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>         ArmyGroupIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 Gold                  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 Mana                  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>         ArtifactItemIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName>         HoardedSpellScrolls;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName>         HoardedRuneKnowledge;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<int32, int32>    WizardRelations;       // WizardIndex -> score
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<int32, ECoMDragonTreatyType> WizardTreaties;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64               DomainGrowthRate      = FFixed64(0.1f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 ArmyRecruitmentRate   = 5; // Turns per new unit
};

/** A dragon egg that can be found, stolen, or laid by a Dragon Lord. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMDragonEgg
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              EggID                      = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName              DragonTypeID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              ParentDragonID             = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              PossessorWizardIndex       = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              IncubationTurnsRemaining   = 15;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSpellRealm     HatchingRealmRequired      = ECoMSpellRealm::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              HatchingManaCost           = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool               bImprints                  = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// APEX RULERS (Dragon Lord equivalent — Three Planes expansion, COM-056b)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * A condition that must be satisfied before a wizard can open Pact negotiations
 * with a specific Archdevil. Each of the three active Archdevils has a unique
 * requirement set (see game-design/spells/three-planes-unit-rosters.md §Infernal Apex).
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMPactRequirement
{
	GENERATED_BODY()

	/** Wizard class required for this Archdevil to even accept an audience (ECoMWizardClass::None = any). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMWizardClass RequiredWizardClass   = ECoMWizardClass::None;

	/** Minimum Infernal cities the wizard must control before talks open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           MinControlledCities   = 0;

	/** Minimum Spirit or Binding books the wizard must hold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           MinSchoolBooks        = 0;

	/** Specific spell the wizard must have researched before negotiations begin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName           RequiredSpellID;

	/** Resource tribute type demanded as part of the deal (ECoMResource::None = no tribute). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMResource    TributeResource       = ECoMResource::None;

	/** Tribute quantity per turn once the contract is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           TributeQuantity       = 0;
};

/**
 * A live Apex Ruler instance on the world map.
 * Extends the Dragon Lord design (FCoMDragonInstance) to cover Demon Princes,
 * Archdevils, and Ethereal Wardens.
 *
 * COM-056b notes:
 *  - DemonPrince: high BetrayalProbability (0.35–0.45), never negotiates unless
 *    wizard controls ≥3 Abyssal cities. ChaosStorm domain events can shift lair.
 *  - Archdevil: cannot break contracts (bContractsUnbreakable); each holds a
 *    PactVictoryKeyIndex. Pact Victory requires all 3 active Archdevils contracted.
 *  - EtherealWarden: bCanNegotiate=false on Sael-Thrix. Others negotiate only via
 *    Spirit magic spells. bDissolutionKey flags Yrel-Vashen + Voreth.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMApexRulerInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                     ApexRulerID          = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName                     SpecID;              // UCoMApexRulerDataAsset
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText                     PersonalName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMApexRulerClass        RulerClass           = ECoMApexRulerClass::Dragon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane                 HomePlane            = ECoMPlane::Abyssal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                     UnitID               = -1;   // Live unit on map
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                     DomainID             = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>             DomainArmyIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                     TerritorialAggression = 50;  // 0–100
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMSkillEntry>    UniqueAbilities;     // 4 per apex

	// Diplomacy flags
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                     bCanNegotiate          = true;   // false = Sael-Thrix
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                     bContractsUnbreakable  = false;  // true = Asmodaran
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64                 BetrayalProbability;             // 0–1; 0.45 for Vaethmor
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FCoMPactRequirement      NegotiationRequirement;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<int32, int32>       WizardRelations;       // WizardIndex -> score

	// Victory condition hooks (COM-056c)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                     bPactVictoryKey        = false;  // Must be contracted for Pact Victory
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                     bDissolutionKey        = false;  // Domain must be destabilized for Dissolution Victory
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                     bDissolutionAnchor     = false;  // Must be slain to complete Dissolution Victory (Sael-Thrix)

	// Runtime state
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>            ContractedWizardIDs;   // Active Archdevil contracts
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                     bDomainDestabilized    = false;  // Dissolution prerequisite met
};

// ─────────────────────────────────────────────────────────────────────────────
// VICTORY STATE (COM-056c)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Per-wizard victory progress snapshot.
 * Stored in UCoMTurnSubsystem and evaluated each turn via EvaluateVictoryConditions().
 * All fields are UPROPERTY so they serialize cleanly into save games.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMVictoryState
{
	GENERATED_BODY()

	/** Which wizard this tracks (index into game state wizard array). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                WizardIndex          = CoM::WIZARD_INDEX_NONE;

	/** First victory type this wizard has achieved this game (None = still playing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMVictoryType      AchievedVictory      = ECoMVictoryType::None;

	/** Turn on which the victory was confirmed (0 = not yet achieved). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                VictoryTurn          = 0;

	// --- Pact Victory progress (COM-056c) ---
	/** IDs of Archdevil ApexRulers this wizard currently holds contracts with. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>        ActiveArchdevilContracts;

	/** True once all 3 active Archdevils are simultaneously contracted — triggers win evaluation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                 bPactVictoryAchieved = false;

	// --- Dissolution Victory progress (COM-056c) ---
	/** How many of the 3 required Dissolution Rifts are currently active (0–3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                ActiveDissolutionRifts = 0;

	/** Which Apex domain destabilization steps are complete (should reach 2 before Sael-Thrix slain). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>        DestabilizedApexDomainIDs;

	/** True once Sael-Thrix has been slain with all 3 rifts active — triggers win evaluation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                 bDissolutionVictoryAchieved = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// CITIES & MINES
// ─────────────────────────────────────────────────────────────────────────────

/** A mine node on the map — must be constructed by engineers. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMMineData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            MineID          = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint        Position = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane        Plane           = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer     Layer           = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMResource     ResourceType    = ECoMResource::Iron;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            OwnerCityID     = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            MineLevel       = 1; // 1–3
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            WorkersAssigned = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool             bExhausted      = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            TurnsUntilBuilt = 0; // 0 = operational
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMEnchantmentInstance> Enchantments;
};

/** Underdark-specific city modifier profile. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMUnderdarkCityModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 MiningBonus             = FFixed64(1.5f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 FarmingPenalty;                 // No traditional farms
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 FungalFoodPerTile       = FFixed64(1.5f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool  bRequiresLightSource      = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 NoLightProductionPenalty = FFixed64::Half(); // ×0.5 prod
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 NoLightUnrestBonus      = FFixed64(2);        // +2 unrest
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 TunnelConstructionSpeed = FFixed64(1);        // Turns/tile
};

// ─────────────────────────────────────────────────────────────────────────────
// NAVAL
// ─────────────────────────────────────────────────────────────────────────────

/** A wizard's naval fleet. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMFleet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               FleetID            = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               OwnerWizardIndex   = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>       ShipUnitIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               AdmiralHeroID      = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint           Position = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane           Plane              = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer        Layer              = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FIntPoint>   Route;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMFleetFormation  Formation          = ECoMFleetFormation::Line;
};

// ─────────────────────────────────────────────────────────────────────────────
// SIEGE
// ─────────────────────────────────────────────────────────────────────────────

/** State of an ongoing city siege. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMSiegeState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               SiegeID             = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               AttackerArmyGroupID = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               DefenderCityID      = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSiegePhase      Phase               = ECoMSiegePhase::Approach;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               TurnsUnderSiege     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               CityFoodReserves    = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               CityMorale          = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               WallHP              = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               WallMaxHP           = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FIntPoint>   BreachPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               GatehouseHP         = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                bSupplyLinesCut     = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>       SiegeEquipmentIDs;
};

// ─────────────────────────────────────────────────────────────────────────────
// TRADE
// ─────────────────────────────────────────────────────────────────────────────

/** A trade route between two cities. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMTradeRoute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  RouteID          = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  SourceCityID     = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  DestCityID       = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMTradeGood          Good             = ECoMTradeGood::Grain;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  Quantity         = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  GoldPerTurn      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  RouteLength      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                   bInterplanar     = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64                RiskFactor       = FFixed64(0.05f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  TurnsActive      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMTradeRouteStatus   Status           = ECoMTradeRouteStatus::Active;
};

/** A bilateral trade agreement between two wizards. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMTradeAgreement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                   WizardA      = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                   WizardB      = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMTradeRoute>  Routes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                   TariffRateA  = 0; // Percentage
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                   TariffRateB  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMTradeAgreementType  Type         = ECoMTradeAgreementType::Open;
};

// ─────────────────────────────────────────────────────────────────────────────
// DIPLOMACY
// ─────────────────────────────────────────────────────────────────────────────

/** Full diplomatic relation record between two wizards. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMDiplomacyRelation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                          WizardA           = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                          WizardB           = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                          RelationScore     = 0; // -200 to +200
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<ECoMTreatyType>         ActiveTreaties;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                          TrustLevel        = 50; // 0–100
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64                        BorderFriction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                          TradeValue        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                           bHasEmbassy       = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                          FavorDebt         = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMAlignment                  AlignmentA        = ECoMAlignment::Neutral;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMAlignment                  AlignmentB        = ECoMAlignment::Neutral;
};

// ─────────────────────────────────────────────────────────────────────────────
// MAGIC — PLANE-NATIVE SPELL DATA
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Describes how a spell behaves relative to its native plane.
 * Plane-native schools (Radiance, Shadow, Primal, Magma, Crystal, Chaos, Binding, Spirit)
 * cost less on their home plane and significantly more cross-plane.
 * Universal schools (Life, Death, Nature, Arcane) ignore this struct.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMPlaneSpellData
{
	GENERATED_BODY()

	/** Which spell school this belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSpellRealm School = ECoMSpellRealm::Arcane;

	/** The plane on which this spell is native (irrelevant for Universal schools). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane NativePlane = ECoMPlane::Aurelith;

	/** If true, the spell fails entirely when cast off its home plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHomePlaneOnly = false;

	/** Mana cost multiplier when cast on the native plane (e.g. 0.5 = 50% cost). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 NativeCostMultiplier = FFixed64::Half();

	/** Mana cost multiplier when cast from any other plane (e.g. 3.0 = 300% cost). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 CrossPlaneCostMultiplier = FFixed64(3);

	/** Research cost multiplier when the wizard has no books in the native school (e.g. 2.0 = 2× cost). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 NonNativeResearchMultiplier = FFixed64(2);
};

// ─────────────────────────────────────────────────────────────────────────────
// MAGIC — RITUAL
// ─────────────────────────────────────────────────────────────────────────────

/** Requirements that must be met before a ritual can begin. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMRitualRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           MinCasters           = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           TurnsToComplete      = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32           ManaCost             = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSpellRealm  RequiredRealm        = ECoMSpellRealm::Arcane;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag    RequiredResource;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane       RequiredPlane        = ECoMPlane::MAX; // MAX = any plane
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool            bRequiresManaNode    = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool            bRequiresFullMoon    = false; // Turn counter mod
};

/** A currently-channeling ritual. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMActiveRitual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            RitualID            = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName            RitualSpellID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            WizardIndex         = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint        Location = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane        Plane               = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>    ParticipantUnitIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            TurnsRemaining      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            ManaInvested        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            ManaCostTotal       = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            ManaChannelPerTurn  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64          Stability           = FFixed64(100); // 0–100
};

// ─────────────────────────────────────────────────────────────────────────────
// MAGIC — RUNE
// ─────────────────────────────────────────────────────────────────────────────

/** A rune inscription applied to a target object. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMRuneInscription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            InscriptionID       = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName            RuneID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMRuneTarget   TargetType          = ECoMRuneTarget::Unit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            TargetID            = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            InscribedByWizard   = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            InscribedByUnitID   = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            Durability          = -1; // -1 = permanent
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            TurnsToComplete     = 0;  // 0 = active
};

// ─────────────────────────────────────────────────────────────────────────────
// WORLD EVENTS
// ─────────────────────────────────────────────────────────────────────────────

/** A world event currently in progress. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMWorldEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                     EventID         = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMWorldEventType        Type            = ECoMWorldEventType::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                     TurnTriggered   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                     Duration        = 5; // Turns
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<ECoMPlane>         AffectedPlanes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<ECoMMapLayer>      AffectedLayers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint                 EpicenterPosition = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                     EpicenterRadius = 0; // 0 = global
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64                   Intensity       = FFixed64(1);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMStatModifier>  GlobalModifiers;
};

// ─────────────────────────────────────────────────────────────────────────────
// CORRUPTION
// ─────────────────────────────────────────────────────────────────────────────

/** An active corruption zone spreading across a plane. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMCorruptionZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              ZoneID            = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMCorruptionType Type              = ECoMCorruptionType::InfernyxCorruption;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane          Plane             = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer       Layer             = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint          Center = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              Radius            = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64            Intensity         = FFixed64::Half(); // 0–1
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64            SpreadRate        = FFixed64(0.1f); // Tiles/turn
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              SourceWizardIndex = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool               bNatural          = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// EXPLORATION & ARTIFACTS
// ─────────────────────────────────────────────────────────────────────────────

/** One step in a legendary artifact's quest chain. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMArtifactQuestStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText                    Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMArtifactQuestType    Type            = ECoMArtifactQuestType::VisitLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool                     bCompleted      = false;
	// Target details depend on Type — stored as a flexible map
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, int32>       TargetParams;
};

/** A unique legendary artifact with quest chain. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMLegendaryArtifact
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                         ArtifactID           = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName                         ArtifactName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText                         LoreDescription;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                         ItemInstanceID       = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMArtifactLocation          LocationType         = ECoMArtifactLocation::InDungeon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>                 FragmentSiteIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMArtifactQuestStep> QuestChain;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                         CurrentQuestStep     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                         PossessorWizardIndex = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMStatModifier>      BasePower;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMStatModifier>      UnlockedPower;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMSkillEntry>        BaseSkills;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMSkillEntry>        UnlockedSkills;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName                         UnlockedSpellGrant;
};

// ─────────────────────────────────────────────────────────────────────────────
// MIGRATION & DISEASE
// ─────────────────────────────────────────────────────────────────────────────

/** A migration wave of population moving between cities. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMMigrationEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               SourceCityID       = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               DestCityID         = -1; // -1 = lost to wilderness
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               PopulationMoving   = 0;  // In 1,000s
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag        RaceTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMigrationReason Reason             = ECoMMigrationReason::War;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               TurnsInTransit     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32               TurnsRemaining     = 3;
};

/** An active disease outbreak in a city. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMDiseaseOutbreak
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32             OutbreakID   = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMDiseaseType  Type         = ECoMDiseaseType::CommonPlague;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            CityID       = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            Severity     = 1;  // 1–10
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32            TurnsActive  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64          SpreadChance = FFixed64(0.1f); // Per trade route per turn
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool             bQuarantined = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// WARBANDS
// ─────────────────────────────────────────────────────────────────────────────

/** Named warband leader — hero-equivalent NPC. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMWarbandLeader
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText                  Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  UnitID        = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMSkillEntry> UniqueSkills;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                  Level         = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName                  UniqueItemID; // Drops on death
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText                  BackstorySnippet;
};

/** A wandering warband — named, organized, with unique behavior. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMWarband
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 WarbandID       = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText                 WarbandName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMWarbandType       Type            = ECoMWarbandType::Bandits;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FCoMWarbandLeader     Leader;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>         UnitIDs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint             Position = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane             Plane           = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMMapLayer          Layer           = ECoMMapLayer::Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMWarbandBehavior   Behavior        = ECoMWarbandBehavior::Patrol;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 ThreatRating    = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FIntPoint>     PatrolRoute;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 TurnsSinceRaid  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 OrganizationID  = -1; // -1 = independent
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64               GrowthRate      = FFixed64(0.05f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                 MaxSize         = 9;
};

// ─────────────────────────────────────────────────────────────────────────────
// ORGANIZATIONS
// ─────────────────────────────────────────────────────────────────────────────

/** An active organization agent in the field. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMOrgAgent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText              Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              SkillLevel      = 1; // 1–10
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMAgentMission   CurrentMission  = ECoMAgentMission::Spy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint          Location = FIntPoint(0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane          Plane           = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              TargetWizardIndex = CoM::WIZARD_INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32              TurnsOnMission  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool               bDiscovered     = false;
};

/** A full independent organization faction. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMOrganization
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                    OrgID            = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText                    Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMOrgType              Type             = ECoMOrgType::MerchantGuild;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText                    Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMPlane                HomePlane        = ECoMPlane::Aurelith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<ECoMPlane>        ActivePlanes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMOrgAgent>     Agents;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                    Treasury         = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                    Influence        = 50; // 0–100
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<int32, int32>       WizardStanding;   // WizardIndex -> score
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>            MemberWizards;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32                    MembershipCost   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32>            AffiliatedWarbandIDs;
};

// ─────────────────────────────────────────────────────────────────────────────
// MULTIPLAYER — GAME STATE SNAPSHOT
// Per §6.3 Serializable Game State (architecture.md)
// All fields are UPROPERTY SaveGame — serialized via FArchive.
// No raw pointers. No TMap/TSet. StateHash used for determinism CI + anti-cheat.
// ─────────────────────────────────────────────────────────────────────────────

// Forward declarations for snapshot sub-structs (defined in their own headers).
struct FCoMWizardState;
struct FCoMWorldMapState;
struct FCoMCityState;
struct FCoMArmyState;
struct FCoMEnchantmentState;
struct FCoMWorldEventState;
struct FCoMDiplomacyMatrix;

/**
 * Complete world state at any turn boundary.
 * Saved to disk, used as network sync packet for reconnects/spectators,
 * and hashed for determinism CI gate.
 *
 * NOTE: FCoMCommandQueue is stored by value here so replays are self-contained.
 * This struct is the authoritative source — never reconstruct state from anything else.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMGameStateSnapshot
{
	GENERATED_BODY()

	// ── Turn identity ─────────────────────────────────────────────────────────
	UPROPERTY(SaveGame) int32    TurnNumber       = 0;
	UPROPERTY(SaveGame) int64   DeterministicSeed = 0;

	// ── World state (all 8 planes × 2 layers = 16 map layers) ────────────────
	// FCoMWorldMapState defined in World/CoMWorldMapSubsystem.h
	// TArray of per-plane tile arrays — never TMap, never TSet.

	// ── Per-wizard state (max 14 players; sub-structs TBD in CoMPlayerState.h) ─
	UPROPERTY(SaveGame) int32 ActiveWizardCount = 0;

	// ── Pending commands (per wizard slot, for replay) ────────────────────────
	// Stored as flat payload arrays (not UObject refs) for FArchive compat.
	// Index matches wizard slot 0–13 (MAX_WIZARDS=14).
	// NOT UPROPERTY — nested TArrays are not allowed in UPROPERTY (UHT restriction).
	// Serialized manually via FArchive in UCoMSaveSubsystem. See BV02-FIX-001.
	TArray<TArray<FCoMCommandPayload>> PendingCommandsByWizard;

	// ── PRNG state — must be snapshot'd for determinism ───────────────────────
	UPROPERTY(SaveGame) FCoMDeterministicRandom RNGState;

	// ── Integrity ─────────────────────────────────────────────────────────────
	// Hash of all fields above. Computed by UCoMSaveSubsystem after serialization.
	// Compared across two simulation runs in CI to verify determinism.
	// Also sent to all clients after turn resolution for anti-cheat.
	UPROPERTY(SaveGame) int64 StateHash = 0;
};
