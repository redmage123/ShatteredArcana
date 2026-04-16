// Copyright Shattered Arcana. All Rights Reserved.
// CoMBuildingDatabase.h — Static building lookup table (no editor-authored assets needed)
// Inspired by Master of Magic / Caster of Magic building progression

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CoMCore/CoreTypes/CoMEnums.h"

/**
 * Static building info — all 46 universal buildings + 45 racial buildings = 91 total.
 * This struct mirrors UCoMBuildingDataAsset fields but lives in code
 * so the game has real data without needing editor-authored data assets.
 */
struct COMCORE_API FCoMBuildingInfo
{
	FName BuildingID;
	FText DisplayName;
	int32 ProductionCost = 50;
	int32 UpkeepGold = 0;

	// Economy bonuses
	int32 FoodBonus = 0;
	int32 GoldBonus = 0;
	int32 ProductionBonus = 0;
	int32 ManaBonus = 0;
	int32 ResearchBonus = 0;
	int32 UnrestReduction = 0;
	int32 HappinessBonus = 0;
	int32 PopCapBonus = 0;

	// Defense
	int32 WallHP = 0;

	// Prerequisites (FName IDs of required buildings)
	TArray<FName> Prerequisites;

	// Percentage multipliers (e.g., Bank +50% gold = 150)
	int32 GoldMultiplierPercent = 100; // 100 = no bonus

	// Special flags
	bool bCoastalOnly = false;
	bool bEnablesHeroRecruitment = false;
	bool bEnablesSpyRecruitment = false;
	bool bCapitalMarker = false;
	bool bEnablesSummoning = false;
	bool bEnablesCrossPlane = false;
	bool bEnablesTransmute = false;

	// Unit enablement tags (what this building enables for recruitment)
	FString EnablesUnitTag; // e.g., "Infantry", "Cavalry", "Naval", "Elite", "Dragon"

	/** XP bonus granted to newly recruited units in this city */
	int32 RecruitXPBonus = 0;

	/** Attack bonus granted to all new units recruited */
	int32 RecruitAttackBonus = 0;

	/** Defense bonus granted to all units (global city effect) */
	int32 CityDefenseBonus = 0;

	/** Summoning cost reduction */
	int32 SummonCostReduction = 0;

	// ── Race-specific building fields ────────────────────────────────────

	/** If set, only cities whose PrimaryRaceTag matches can build this. */
	FGameplayTag RaceRequirement;

	/** Visual architecture style — determines which art variant to render. */
	ECoMArchitectureStyle Architecture = ECoMArchitectureStyle::Human;
};

/**
 * Static database of all 91 buildings. No UObject overhead.
 * Call GetBuildingInfo() to look up by FName.
 */
class COMCORE_API CoMBuildingDatabase
{
public:
	/** Get building info by ID. Returns a default (cost 50) if not found. */
	static const FCoMBuildingInfo& GetBuildingInfo(FName BuildingID);

	/** Get all building IDs in the database. */
	static TArray<FName> GetAllBuildingIDs();

	/** Check if a building ID exists in the database. */
	static bool Contains(FName BuildingID);

	/** Get buildings that have no prerequisites (tier 0). */
	static TArray<FName> GetStarterBuildings();

	/** Map a race tag to its architectural style. */
	static ECoMArchitectureStyle GetArchitectureForRace(FGameplayTag RaceTag);

	/** Get all racial building IDs for a specific race tag. */
	static TArray<FName> GetBuildingsForRace(FGameplayTag RaceTag);

private:
	static void EnsureInitialized();
	static TMap<FName, FCoMBuildingInfo> BuildingTable;
	static FCoMBuildingInfo DefaultBuilding;
	static bool bInitialized;

	static void RegisterBuilding(FCoMBuildingInfo&& Info);
	static void InitializeDatabase();
};
