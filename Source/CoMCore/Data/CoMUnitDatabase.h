// Copyright Shattered Arcana. All Rights Reserved.
// CoMUnitDatabase.h — Static unit spec lookup table (no editor-authored assets needed)
// Inspired by Master of Magic / Caster of Magic racial unit rosters

#pragma once

#include "CoreMinimal.h"

/**
 * Static unit specification — mirrors UCoMUnitSpecDataAsset fields.
 * 15 races x 3 tiers + special units = ~50 entries.
 */
struct COMCORE_API FCoMUnitSpecInfo
{
	FName SpecID;
	FText DisplayName;
	FString RaceTag; // e.g., "Race.HighMen", "Race.HighElves"

	// Combat stats
	int32 HitPoints = 4;
	int32 MeleeAttack = 3;
	int32 RangedAttack = 0;
	int32 Defense = 2;
	int32 Resistance = 3;
	int32 Movement = 2;
	int32 Figures = 6; // Multi-figure units (like MoM swordsmen = 6 figures)
	int32 RangedShots = 0; // 0 = melee only

	// Cost
	int32 ProductionCost = 20;
	int32 UpkeepGold = 1;
	int32 UpkeepFood = 1;
	int32 UpkeepMana = 0;

	// Flags
	bool bFlying = false;
	bool bSwimming = false;
	bool bNonCorporeal = false;
	bool bHero = false;
	bool bSettler = false;
	bool bEngineer = false;

	// Unit tier / category tag
	FString CategoryTag; // "Infantry", "Cavalry", "Ranged", "Settler", "Hero"

	// Required building tag to recruit
	FString RequiredBuildingTag; // "Barracks", "Stable", "FightersGuild", etc.
};

/**
 * Static database of unit specs for all races.
 */
class COMCORE_API CoMUnitDatabase
{
public:
	/** Get unit spec info by ID. Returns a default if not found. */
	static const FCoMUnitSpecInfo& GetUnitSpec(FName SpecID);

	/** Get all unit spec IDs. */
	static TArray<FName> GetAllUnitSpecIDs();

	/** Get unit specs available for a given race tag. */
	static TArray<FName> GetUnitsForRace(const FString& RaceTag);

	/** Get unit specs that require a specific building. */
	static TArray<FName> GetUnitsRequiringBuilding(const FString& BuildingTag);

	/** Check if a spec ID exists. */
	static bool Contains(FName SpecID);

private:
	static void EnsureInitialized();
	static TMap<FName, FCoMUnitSpecInfo> UnitTable;
	static FCoMUnitSpecInfo DefaultUnit;
	static bool bInitialized;

	static void RegisterUnit(FCoMUnitSpecInfo&& Info);
	static void InitializeDatabase();
};
