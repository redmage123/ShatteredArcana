// Copyright Shattered Arcana. All Rights Reserved.
// CoMSpellDatabase.h — Static spell lookup table (tier-based system for 600 spells)
// Inspired by Master of Magic / Caster of Magic spell tiers

#pragma once

#include "CoreMinimal.h"
#include "CoMCore/CoreTypes/CoMEnums.h"

/**
 * Spell target type for the database (simplified from ECoMSpellTargetType).
 */
enum class ECoMSpellTarget : uint8
{
	NoTarget,
	UnitTarget,
	CityTarget,
	TileTarget,
	ArmyTarget
};

/**
 * Spell effect category — determines targeting and resolution behavior.
 */
enum class ECoMSpellEffect : uint8
{
	Damage,
	UnitBuff,
	UnitDebuff,
	CityBuff,
	CityDebuff,
	Terrain,
	GlobalEnchantment,
	Summon,
	Healing,
	Divination,
	Dispel,
	Diplomatic      // Affects wizard relations/reputation
};

/**
 * Static spell info — mirrors UCoMSpellDataAsset fields.
 * 10 realms x 4 tiers x 15 spells = 600 spells, resolved by tier pattern.
 */
struct COMCORE_API FCoMSpellInfo
{
	FName SpellID;
	FText DisplayName;

	ECoMSpellRealm Realm = ECoMSpellRealm::Arcane;
	ECoMSpellRarity Rarity = ECoMSpellRarity::Common;
	ECoMSpellScope Scope = ECoMSpellScope::Combat;
	ECoMSpellTarget TargetType = ECoMSpellTarget::UnitTarget;
	ECoMSpellEffect EffectType = ECoMSpellEffect::Damage;

	int32 ResearchCost = 50;
	int32 CastingCost = 5;
	int32 UpkeepMana = 0; // -1 = not ongoing, 0+ = mana/turn
	int32 Range = 4;

	bool bSummon = false;
	bool bInstant = true;
	bool bOngoing = false;
	bool bAOE = false;
	int32 AOERadius = 0;

	int32 DamageBase = 0;
	int32 HealAmount = 0;
};

/**
 * Static spell database. Spells are looked up by FName ID.
 * The ID pattern encodes realm + tier: e.g., "Life_T1_Healing_Word"
 *
 * For the 600-spell system, GetSpellInfo() parses the tier from the ID
 * and returns appropriate costs when an exact match isn't in the table.
 */
class COMCORE_API CoMSpellDatabase
{
public:
	/** Get spell info by ID. Falls back to tier-based defaults if not explicitly registered. */
	static FCoMSpellInfo GetSpellInfo(FName SpellID);

	/** Get all explicitly registered spell IDs. */
	static TArray<FName> GetAllSpellIDs();

	/** Get spells for a given realm. */
	static TArray<FName> GetSpellsForRealm(ECoMSpellRealm Realm);

	/** Get spells of a given rarity tier. */
	static TArray<FName> GetSpellsByRarity(ECoMSpellRarity Rarity);

	/** Get spells for a specific realm AND rarity tier. */
	static TArray<FName> GetSpellsForRealmAndRarity(ECoMSpellRealm Realm, ECoMSpellRarity Rarity);

	/**
	 * Get all spells available to a wizard based on their book allocation.
	 * 1 book = Common, 2-3 books = +Uncommon, 4-5 = +Rare, 6+ = +Very Rare.
	 * Returns two arrays: learnable (researchable) and guaranteed starting spells.
	 * In MoM, you get 1 guaranteed spell per 2 books in Common tier.
	 */
	static void GetSpellsForBookCount(ECoMSpellRealm Realm, int32 BookCount,
		TArray<FName>& OutLearnableSpells, TArray<FName>& OutStartingSpells);

	/** Get the research cost for a spell by its rarity tier. */
	static int32 GetTierResearchCost(ECoMSpellRarity Rarity);

	/** Get the casting cost for a spell by its rarity tier. */
	static int32 GetTierCastingCost(ECoMSpellRarity Rarity);

	/** Get the range for a spell by its rarity tier. */
	static int32 GetTierRange(ECoMSpellRarity Rarity);

	/** Check if a spell ID exists in the explicit table. */
	static bool Contains(FName SpellID);

private:
	static void EnsureInitialized();
	static TMap<FName, FCoMSpellInfo> SpellTable;
	static bool bInitialized;

	static void RegisterSpell(FCoMSpellInfo&& Info);
	static void InitializeDatabase();

	/** Parse realm from a spell ID string (e.g., "Life_T1_..." -> Life). */
	static ECoMSpellRealm ParseRealm(const FString& SpellIDStr);

	/** Parse rarity tier from a spell ID string (e.g., "..._T2_..." -> Uncommon). */
	static ECoMSpellRarity ParseRarity(const FString& SpellIDStr);

	/** Parse effect type from a spell ID string. */
	static ECoMSpellEffect ParseEffect(const FString& SpellIDStr);
};
