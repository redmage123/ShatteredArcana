// Copyright Shattered Arcana. All Rights Reserved.
// CoMBuildingDatabase.cpp — Static building data for all 46 universal + 45 racial = 91 buildings
// Values inspired by Master of Magic / Caster of Magic

#include "CoMBuildingDatabase.h"
#include "CoMCore/CoreTypes/CoMGameplayTags.h"

TMap<FName, FCoMBuildingInfo> CoMBuildingDatabase::BuildingTable;
FCoMBuildingInfo CoMBuildingDatabase::DefaultBuilding;
bool CoMBuildingDatabase::bInitialized = false;

void CoMBuildingDatabase::RegisterBuilding(FCoMBuildingInfo&& Info)
{
	BuildingTable.Add(Info.BuildingID, MoveTemp(Info));
}

void CoMBuildingDatabase::EnsureInitialized()
{
	if (!bInitialized)
	{
		DefaultBuilding.BuildingID = FName(TEXT("Unknown"));
		DefaultBuilding.DisplayName = FText::FromString(TEXT("Unknown Building"));
		DefaultBuilding.ProductionCost = 50;
		InitializeDatabase();
		bInitialized = true;
	}
}

const FCoMBuildingInfo& CoMBuildingDatabase::GetBuildingInfo(FName BuildingID)
{
	EnsureInitialized();
	if (const FCoMBuildingInfo* Found = BuildingTable.Find(BuildingID))
	{
		return *Found;
	}
	return DefaultBuilding;
}

TArray<FName> CoMBuildingDatabase::GetAllBuildingIDs()
{
	EnsureInitialized();
	TArray<FName> Result;
	BuildingTable.GetKeys(Result);
	return Result;
}

bool CoMBuildingDatabase::Contains(FName BuildingID)
{
	EnsureInitialized();
	return BuildingTable.Contains(BuildingID);
}

TArray<FName> CoMBuildingDatabase::GetStarterBuildings()
{
	EnsureInitialized();
	TArray<FName> Result;
	for (const auto& Pair : BuildingTable)
	{
		if (Pair.Value.Prerequisites.Num() == 0)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

// =====================================================================
// Building Definitions — 46 universal + 45 racial = 91 buildings
// =====================================================================

void CoMBuildingDatabase::InitializeDatabase()
{
	// ─── Tier 0: No Prerequisites ────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Granary"));
		B.DisplayName = FText::FromString(TEXT("Granary"));
		B.ProductionCost = 40;
		B.UpkeepGold = 1;
		B.FoodBonus = 2;
		B.PopCapBonus = 1;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Marketplace"));
		B.DisplayName = FText::FromString(TEXT("Marketplace"));
		B.ProductionCost = 80;
		B.UpkeepGold = 1;
		B.GoldBonus = 3;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Smithy"));
		B.DisplayName = FText::FromString(TEXT("Smithy"));
		B.ProductionCost = 60;
		B.UpkeepGold = 1;
		B.ProductionBonus = 1;
		B.EnablesUnitTag = TEXT("Iron");
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Library"));
		B.DisplayName = FText::FromString(TEXT("Library"));
		B.ProductionCost = 100;
		B.UpkeepGold = 1;
		B.ResearchBonus = 3;
		B.EnablesUnitTag = TEXT("Magician");
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Temple"));
		B.DisplayName = FText::FromString(TEXT("Temple"));
		B.ProductionCost = 80;
		B.UpkeepGold = 1;
		B.ManaBonus = 2;
		B.UnrestReduction = 1;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Shrine"));
		B.DisplayName = FText::FromString(TEXT("Shrine"));
		B.ProductionCost = 40;
		B.UpkeepGold = 0;
		B.ManaBonus = 1;
		B.HappinessBonus = 1;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Barracks"));
		B.DisplayName = FText::FromString(TEXT("Barracks"));
		B.ProductionCost = 50;
		B.UpkeepGold = 1;
		B.EnablesUnitTag = TEXT("Infantry");
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Tavern"));
		B.DisplayName = FText::FromString(TEXT("Tavern"));
		B.ProductionCost = 60;
		B.UpkeepGold = 1;
		B.GoldBonus = 1;
		B.bEnablesHeroRecruitment = true;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Docks"));
		B.DisplayName = FText::FromString(TEXT("Docks"));
		B.ProductionCost = 100;
		B.UpkeepGold = 1;
		B.GoldBonus = 2;
		B.bCoastalOnly = true;
		B.EnablesUnitTag = TEXT("Naval");
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("WallsWood"));
		B.DisplayName = FText::FromString(TEXT("Walls (Wood)"));
		B.ProductionCost = 50;
		B.UpkeepGold = 0;
		B.WallHP = 5;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Memorial"));
		B.DisplayName = FText::FromString(TEXT("Memorial"));
		B.ProductionCost = 100;
		B.UpkeepGold = 0;
		B.HappinessBonus = 2;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Tier 1: Single Prerequisite ────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Stable"));
		B.DisplayName = FText::FromString(TEXT("Stable"));
		B.ProductionCost = 80;
		B.UpkeepGold = 1;
		B.EnablesUnitTag = TEXT("Cavalry");
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Aqueduct"));
		B.DisplayName = FText::FromString(TEXT("Aqueduct"));
		B.ProductionCost = 150;
		B.UpkeepGold = 1;
		B.FoodBonus = 3;
		B.PopCapBonus = 2;
		B.Prerequisites.Add(FName(TEXT("Granary")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Bank"));
		B.DisplayName = FText::FromString(TEXT("Bank"));
		B.ProductionCost = 200;
		B.UpkeepGold = 2;
		B.GoldBonus = 5;
		B.GoldMultiplierPercent = 150; // +50% gold income
		B.Prerequisites.Add(FName(TEXT("Marketplace")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("MageTower"));
		B.DisplayName = FText::FromString(TEXT("Mage Tower"));
		B.ProductionCost = 250;
		B.UpkeepGold = 2;
		B.ResearchBonus = 5;
		B.ManaBonus = 3;
		B.Prerequisites.Add(FName(TEXT("Library")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Cathedral"));
		B.DisplayName = FText::FromString(TEXT("Cathedral"));
		B.ProductionCost = 400;
		B.UpkeepGold = 3;
		B.ManaBonus = 5;
		B.UnrestReduction = 3;
		B.HappinessBonus = 2;
		B.Prerequisites.Add(FName(TEXT("Temple")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Colosseum"));
		B.DisplayName = FText::FromString(TEXT("Colosseum"));
		B.ProductionCost = 250;
		B.UpkeepGold = 2;
		B.UnrestReduction = 2;
		B.GoldBonus = 1;
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Shipyard"));
		B.DisplayName = FText::FromString(TEXT("Shipyard"));
		B.ProductionCost = 200;
		B.UpkeepGold = 2;
		B.bCoastalOnly = true;
		B.EnablesUnitTag = TEXT("Warship");
		B.Prerequisites.Add(FName(TEXT("Docks")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Lighthouse"));
		B.DisplayName = FText::FromString(TEXT("Lighthouse"));
		B.ProductionCost = 150;
		B.UpkeepGold = 1;
		B.GoldBonus = 3; // Trade income from maritime navigation
		B.bCoastalOnly = true;
		B.Prerequisites.Add(FName(TEXT("Docks")));
		// Also grants +2 vision range over water (implemented in FogOfWarSubsystem)
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("WallsStone"));
		B.DisplayName = FText::FromString(TEXT("Walls (Stone)"));
		B.ProductionCost = 150;
		B.UpkeepGold = 1;
		B.WallHP = 15;
		B.Prerequisites.Add(FName(TEXT("WallsWood")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Observatory"));
		B.DisplayName = FText::FromString(TEXT("Observatory"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.ResearchBonus = 3;
		B.Prerequisites.Add(FName(TEXT("Library")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("ThievesGuild"));
		B.DisplayName = FText::FromString(TEXT("Thieves' Guild"));
		B.ProductionCost = 150;
		B.UpkeepGold = 2;
		B.bEnablesSpyRecruitment = true;
		B.Prerequisites.Add(FName(TEXT("Tavern")));
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Tier 2: Two Prerequisites ──────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Armory"));
		B.DisplayName = FText::FromString(TEXT("Armory"));
		B.ProductionCost = 120;
		B.UpkeepGold = 1;
		B.RecruitAttackBonus = 1;
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Oracle"));
		B.DisplayName = FText::FromString(TEXT("Oracle"));
		B.ProductionCost = 400;
		B.UpkeepGold = 3;
		B.ResearchBonus = 5;
		B.Prerequisites.Add(FName(TEXT("Temple")));
		B.Prerequisites.Add(FName(TEXT("Library")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("WallsIron"));
		B.DisplayName = FText::FromString(TEXT("Walls (Iron)"));
		B.ProductionCost = 300;
		B.UpkeepGold = 2;
		B.WallHP = 30;
		B.Prerequisites.Add(FName(TEXT("WallsStone")));
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("EnchanterWorkshop"));
		B.DisplayName = FText::FromString(TEXT("Enchanter's Workshop"));
		B.ProductionCost = 300;
		B.UpkeepGold = 2;
		B.ProductionBonus = 3;
		B.Prerequisites.Add(FName(TEXT("MageTower")));
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("SummoningCircle"));
		B.DisplayName = FText::FromString(TEXT("Summoning Circle"));
		B.ProductionCost = 350;
		B.UpkeepGold = 2;
		B.bEnablesSummoning = true;
		B.SummonCostReduction = 2;
		B.Prerequisites.Add(FName(TEXT("MageTower")));
		B.Prerequisites.Add(FName(TEXT("Temple")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("AlchemistLab"));
		B.DisplayName = FText::FromString(TEXT("Alchemist Lab"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.ProductionBonus = 2;
		B.bEnablesTransmute = true;
		B.Prerequisites.Add(FName(TEXT("Library")));
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Tier 3: Deep Prerequisites ─────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("FightersGuild"));
		B.DisplayName = FText::FromString(TEXT("Fighter's Guild"));
		B.ProductionCost = 200;
		B.UpkeepGold = 2;
		B.EnablesUnitTag = TEXT("Elite");
		B.RecruitXPBonus = 2;
		B.Prerequisites.Add(FName(TEXT("Armory")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("WarCollege"));
		B.DisplayName = FText::FromString(TEXT("War College"));
		B.ProductionCost = 300;
		B.UpkeepGold = 3;
		B.CityDefenseBonus = 1;
		B.Prerequisites.Add(FName(TEXT("FightersGuild")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("WizardGuild"));
		B.DisplayName = FText::FromString(TEXT("Wizard Guild"));
		B.ProductionCost = 500;
		B.UpkeepGold = 3;
		B.ResearchBonus = 10;
		B.Prerequisites.Add(FName(TEXT("MageTower")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("DragonRoost"));
		B.DisplayName = FText::FromString(TEXT("Dragon Roost"));
		B.ProductionCost = 400;
		B.UpkeepGold = 3;
		B.EnablesUnitTag = TEXT("Dragon");
		B.Prerequisites.Add(FName(TEXT("FightersGuild")));
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Tier 4: Endgame / Capital Buildings ────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Palace"));
		B.DisplayName = FText::FromString(TEXT("Palace"));
		B.ProductionCost = 500;
		B.UpkeepGold = 4;
		B.GoldBonus = 10;
		B.ManaBonus = 5;
		B.HappinessBonus = 3;
		B.bCapitalMarker = true;
		B.Prerequisites.Add(FName(TEXT("Cathedral")));
		B.Prerequisites.Add(FName(TEXT("Bank")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("PlanarBeacon"));
		B.DisplayName = FText::FromString(TEXT("Planar Beacon"));
		B.ProductionCost = 500;
		B.UpkeepGold = 4;
		B.bEnablesCrossPlane = true;
		B.Prerequisites.Add(FName(TEXT("WizardGuild")));
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Farms / Agriculture ────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Farm"));
		B.DisplayName = FText::FromString(TEXT("Farm"));
		B.ProductionCost = 30;
		B.UpkeepGold = 0;
		B.FoodBonus = 3;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Plantation"));
		B.DisplayName = FText::FromString(TEXT("Plantation"));
		B.ProductionCost = 120;
		B.UpkeepGold = 1;
		B.FoodBonus = 2;
		B.GoldBonus = 2;
		B.Prerequisites.Add(FName(TEXT("Granary")));
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Resource Gathering ─────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Sawmill"));
		B.DisplayName = FText::FromString(TEXT("Sawmill"));
		B.ProductionCost = 60;
		B.UpkeepGold = 0;
		B.ProductionBonus = 2;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Mine"));
		B.DisplayName = FText::FromString(TEXT("Mine"));
		B.ProductionCost = 80;
		B.UpkeepGold = 1;
		B.GoldBonus = 2;
		B.ProductionBonus = 1;
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Quarry"));
		B.DisplayName = FText::FromString(TEXT("Quarry"));
		B.ProductionCost = 100;
		B.UpkeepGold = 1;
		B.ProductionBonus = 3;
		B.Prerequisites.Add(FName(TEXT("Mine")));
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Military Training ──────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("ArcheryRange"));
		B.DisplayName = FText::FromString(TEXT("Archery Range"));
		B.ProductionCost = 80;
		B.UpkeepGold = 1;
		B.EnablesUnitTag = TEXT("Ranged");
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("SiegeWorkshop"));
		B.DisplayName = FText::FromString(TEXT("Siege Workshop"));
		B.ProductionCost = 200;
		B.UpkeepGold = 2;
		B.EnablesUnitTag = TEXT("Siege");
		B.Prerequisites.Add(FName(TEXT("Armory")));
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Social / Cultural ──────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Amphitheater"));
		B.DisplayName = FText::FromString(TEXT("Amphitheater"));
		B.ProductionCost = 150;
		B.UpkeepGold = 1;
		B.HappinessBonus = 3;
		B.UnrestReduction = 1;
		B.Prerequisites.Add(FName(TEXT("Memorial")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("University"));
		B.DisplayName = FText::FromString(TEXT("University"));
		B.ProductionCost = 300;
		B.UpkeepGold = 2;
		B.ResearchBonus = 8;
		B.Prerequisites.Add(FName(TEXT("Observatory")));
		B.Prerequisites.Add(FName(TEXT("Library")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Hospital"));
		B.DisplayName = FText::FromString(TEXT("Hospital"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.FoodBonus = 1;
		B.PopCapBonus = 2; // Population growth bonus
		B.Prerequisites.Add(FName(TEXT("Aqueduct")));
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Magical ────────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("ManaVault"));
		B.DisplayName = FText::FromString(TEXT("Mana Vault"));
		B.ProductionCost = 250;
		B.UpkeepGold = 2;
		B.ManaBonus = 5;
		// Also stores excess mana (implemented in ManaSubsystem)
		B.Prerequisites.Add(FName(TEXT("MageTower")));
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("RuneForge"));
		B.DisplayName = FText::FromString(TEXT("Rune Forge"));
		B.ProductionCost = 350;
		B.UpkeepGold = 2;
		B.ProductionBonus = 2;
		B.EnablesUnitTag = TEXT("Rune"); // Enables Rune inscription
		B.Prerequisites.Add(FName(TEXT("EnchanterWorkshop")));
		RegisterBuilding(MoveTemp(B));
	}

	// =====================================================================
	// Racial Buildings — 45 buildings (3 per 15 major races)
	// =====================================================================

	// ─── High Men ────────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("KnightsChapterHouse"));
		B.DisplayName = FText::FromString(TEXT("Knight's Chapter House"));
		B.ProductionCost = 200;
		B.UpkeepGold = 2;
		B.HappinessBonus = 1;
		B.RecruitAttackBonus = 2;
		B.EnablesUnitTag = TEXT("Paladin");
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		B.Prerequisites.Add(FName(TEXT("Stable")));
		B.RaceRequirement = CoMTags::Race::HighMen;
		B.Architecture = ECoMArchitectureStyle::Human;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("RoyalCourt"));
		B.DisplayName = FText::FromString(TEXT("Royal Court"));
		B.ProductionCost = 400;
		B.UpkeepGold = 3;
		B.GoldBonus = 5;
		B.EnablesUnitTag = TEXT("RoyalGuard");
		B.Prerequisites.Add(FName(TEXT("Palace")));
		B.RaceRequirement = CoMTags::Race::HighMen;
		B.Architecture = ECoMArchitectureStyle::Human;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("CathedralOfLight"));
		B.DisplayName = FText::FromString(TEXT("Cathedral of Light"));
		B.ProductionCost = 350;
		B.UpkeepGold = 3;
		B.ManaBonus = 3;
		B.HappinessBonus = 3;
		B.Prerequisites.Add(FName(TEXT("Cathedral")));
		B.RaceRequirement = CoMTags::Race::HighMen;
		B.Architecture = ECoMArchitectureStyle::Human;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── High Elves ──────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("StarlightSpire"));
		B.DisplayName = FText::FromString(TEXT("Starlight Spire"));
		B.ProductionCost = 300;
		B.UpkeepGold = 2;
		B.ResearchBonus = 5;
		B.ManaBonus = 3;
		B.RecruitAttackBonus = 2; // +2 ranged for archers
		B.Prerequisites.Add(FName(TEXT("MageTower")));
		B.RaceRequirement = CoMTags::Race::HighElves;
		B.Architecture = ECoMArchitectureStyle::Elven;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Moonwell"));
		B.DisplayName = FText::FromString(TEXT("Moonwell"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.ManaBonus = 4;
		B.Prerequisites.Add(FName(TEXT("Shrine")));
		B.RaceRequirement = CoMTags::Race::HighElves;
		B.Architecture = ECoMArchitectureStyle::Elven;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("AncientGrove"));
		B.DisplayName = FText::FromString(TEXT("Ancient Grove"));
		B.ProductionCost = 250;
		B.UpkeepGold = 1;
		B.FoodBonus = 2; // Living forest provides sustenance
		B.ManaBonus = 1; // Nature magic from ancient trees
		B.EnablesUnitTag = TEXT("Nature"); // Grants Nature units
		B.RaceRequirement = CoMTags::Race::HighElves;
		B.Architecture = ECoMArchitectureStyle::Elven;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Dwarves ─────────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("AdamantineForge"));
		B.DisplayName = FText::FromString(TEXT("Adamantine Forge"));
		B.ProductionCost = 400;
		B.UpkeepGold = 3;
		B.ProductionBonus = 5;
		B.RecruitAttackBonus = 2;
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		B.Prerequisites.Add(FName(TEXT("Armory")));
		B.RaceRequirement = CoMTags::Race::Dwarves;
		B.Architecture = ECoMArchitectureStyle::Dwarven;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("DeepMineComplex"));
		B.DisplayName = FText::FromString(TEXT("Deep Mine Complex"));
		B.ProductionCost = 300;
		B.UpkeepGold = 2;
		B.ProductionBonus = 8;
		B.GoldBonus = 3;
		B.RaceRequirement = CoMTags::Race::Dwarves;
		B.Architecture = ECoMArchitectureStyle::Dwarven;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("RunesmithHall"));
		B.DisplayName = FText::FromString(TEXT("Runesmith Hall"));
		B.ProductionCost = 350;
		B.UpkeepGold = 2;
		B.ManaBonus = 3;
		B.Prerequisites.Add(FName(TEXT("EnchanterWorkshop")));
		B.RaceRequirement = CoMTags::Race::Dwarves;
		B.Architecture = ECoMArchitectureStyle::Dwarven;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Draconians ──────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("DragonHatchery"));
		B.DisplayName = FText::FromString(TEXT("Dragon Hatchery"));
		B.ProductionCost = 500;
		B.UpkeepGold = 4;
		B.EnablesUnitTag = TEXT("DragonEgg");
		B.Prerequisites.Add(FName(TEXT("DragonRoost")));
		B.RaceRequirement = CoMTags::Race::Draconians;
		B.Architecture = ECoMArchitectureStyle::Draconic;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("VolcanicForge"));
		B.DisplayName = FText::FromString(TEXT("Volcanic Forge"));
		B.ProductionCost = 350;
		B.UpkeepGold = 2;
		B.ProductionBonus = 3; // Superior to Smithy
		B.EnablesUnitTag = TEXT("Mithril"); // Enables Mithril weapons
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		B.RaceRequirement = CoMTags::Race::Draconians;
		B.Architecture = ECoMArchitectureStyle::Draconic;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("SkyRoost"));
		B.DisplayName = FText::FromString(TEXT("Sky Roost"));
		B.ProductionCost = 250;
		B.UpkeepGold = 2;
		B.EnablesUnitTag = TEXT("Flying"); // Enables Flying units
		// +1 movement for air units (implemented in MovementSubsystem)
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		B.RaceRequirement = CoMTags::Race::Draconians;
		B.Architecture = ECoMArchitectureStyle::Draconic;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Dark Elves ──────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("ShadowAcademy"));
		B.DisplayName = FText::FromString(TEXT("Shadow Academy"));
		B.ProductionCost = 300;
		B.UpkeepGold = 2;
		B.bEnablesSpyRecruitment = true;
		B.Prerequisites.Add(FName(TEXT("Library")));
		B.Prerequisites.Add(FName(TEXT("ThievesGuild")));
		B.RaceRequirement = CoMTags::Race::DarkElves;
		B.Architecture = ECoMArchitectureStyle::Dark;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("SacrificialAltar"));
		B.DisplayName = FText::FromString(TEXT("Sacrificial Altar"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.ManaBonus = 10; // Represents population-to-mana conversion potential
		B.Prerequisites.Add(FName(TEXT("Temple")));
		B.RaceRequirement = CoMTags::Race::DarkElves;
		B.Architecture = ECoMArchitectureStyle::Dark;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("WebCitadel"));
		B.DisplayName = FText::FromString(TEXT("Web Citadel"));
		B.ProductionCost = 350;
		B.UpkeepGold = 2;
		B.CityDefenseBonus = 2; // Web-reinforced fortification
		B.WallHP = 20;
		B.Prerequisites.Add(FName(TEXT("WallsStone")));
		B.RaceRequirement = CoMTags::Race::DarkElves;
		B.Architecture = ECoMArchitectureStyle::Dark;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Demons (Demonkin) ───────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("HellfirePit"));
		B.DisplayName = FText::FromString(TEXT("Hellfire Pit"));
		B.ProductionCost = 400;
		B.UpkeepGold = 3;
		B.ManaBonus = 5;
		B.SummonCostReduction = 3;
		B.Prerequisites.Add(FName(TEXT("SummoningCircle")));
		B.RaceRequirement = CoMTags::Race::Demonkin;
		B.Architecture = ECoMArchitectureStyle::Demonic;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("SoulForge"));
		B.DisplayName = FText::FromString(TEXT("Soul Forge"));
		B.ProductionCost = 300;
		B.UpkeepGold = 2;
		B.ProductionBonus = 3;
		B.EnablesUnitTag = TEXT("Hellforged"); // Enables Hellforged weapons (damage bonus)
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		B.RaceRequirement = CoMTags::Race::Demonkin;
		B.Architecture = ECoMArchitectureStyle::Demonic;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("TortureChamber"));
		B.DisplayName = FText::FromString(TEXT("Torture Chamber"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.UnrestReduction = 2; // Fear keeps populace in line
		B.EnablesUnitTag = TEXT("Torturer"); // Enables Torturer unit
		B.bEnablesSpyRecruitment = true;
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		B.RaceRequirement = CoMTags::Race::Demonkin;
		B.Architecture = ECoMArchitectureStyle::Demonic;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Orcs ────────────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("WaaaghTotem"));
		B.DisplayName = FText::FromString(TEXT("Waaagh! Totem"));
		B.ProductionCost = 150;
		B.UpkeepGold = 1;
		B.RecruitAttackBonus = 2; // +2 attack bonus for all recruited units
		B.HappinessBonus = 1; // Morale boost from battle fervor
		B.CityDefenseBonus = -1; // -1 defense (bloodlust trade-off)
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		B.RaceRequirement = CoMTags::Race::Orcs;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("WarPit"));
		B.DisplayName = FText::FromString(TEXT("War Pit"));
		B.ProductionCost = 250;
		B.UpkeepGold = 2;
		B.RecruitXPBonus = 1;
		B.Prerequisites.Add(FName(TEXT("Colosseum")));
		B.RaceRequirement = CoMTags::Race::Orcs;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("LootHoard"));
		B.DisplayName = FText::FromString(TEXT("Loot Hoard"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.GoldBonus = 3;
		B.Prerequisites.Add(FName(TEXT("Marketplace")));
		B.RaceRequirement = CoMTags::Race::Orcs;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Merfolk ─────────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("TidalPool"));
		B.DisplayName = FText::FromString(TEXT("Tidal Pool"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.FoodBonus = 2; // Ocean harvest
		B.EnablesUnitTag = TEXT("AquaticElite"); // Enables Aquatic Elite units
		B.bCoastalOnly = true;
		B.Prerequisites.Add(FName(TEXT("Docks")));
		B.RaceRequirement = CoMTags::Race::Merfolk;
		B.Architecture = ECoMArchitectureStyle::Aquatic;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("CoralPalace"));
		B.DisplayName = FText::FromString(TEXT("Coral Palace"));
		B.ProductionCost = 400;
		B.UpkeepGold = 3;
		B.GoldBonus = 5;
		B.Prerequisites.Add(FName(TEXT("Palace")));
		B.RaceRequirement = CoMTags::Race::Merfolk;
		B.Architecture = ECoMArchitectureStyle::Aquatic;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("PearlDiversLodge"));
		B.DisplayName = FText::FromString(TEXT("Pearl Diver's Lodge"));
		B.ProductionCost = 150;
		B.UpkeepGold = 1;
		B.GoldBonus = 2;
		B.ManaBonus = 1;
		B.bCoastalOnly = true;
		B.RaceRequirement = CoMTags::Race::Merfolk;
		B.Architecture = ECoMArchitectureStyle::Aquatic;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Undead (Shades — closest to classic Undead race) ────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("Necropolis"));
		B.DisplayName = FText::FromString(TEXT("Necropolis"));
		B.ProductionCost = 400;
		B.UpkeepGold = 3;
		B.ManaBonus = 3; // Death magic emanation
		B.EnablesUnitTag = TEXT("Skeleton,Wraith"); // Enables Skeleton AND Wraith units
		B.Prerequisites.Add(FName(TEXT("Temple")));
		B.RaceRequirement = CoMTags::Race::Shades;
		B.Architecture = ECoMArchitectureStyle::Dark;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("BoneForge"));
		B.DisplayName = FText::FromString(TEXT("Bone Forge"));
		B.ProductionCost = 250;
		B.UpkeepGold = 0; // No maintenance for undead theme
		B.ProductionBonus = 2;
		B.EnablesUnitTag = TEXT("Cursed"); // Enables Cursed weapons (life drain)
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		B.RaceRequirement = CoMTags::Race::Shades;
		B.Architecture = ECoMArchitectureStyle::Dark;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("PlagueCauldron"));
		B.DisplayName = FText::FromString(TEXT("Plague Cauldron"));
		B.ProductionCost = 300;
		B.UpkeepGold = 2;
		B.UnrestReduction = 1; // Fear of plague keeps order
		B.EnablesUnitTag = TEXT("Plague"); // Enables Plague spell at city
		// Also causes -1 unrest in nearby enemy cities (implemented in UnrestSubsystem)
		B.Prerequisites.Add(FName(TEXT("AlchemistLab")));
		B.RaceRequirement = CoMTags::Race::Shades;
		B.Architecture = ECoMArchitectureStyle::Dark;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Halflings ───────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("LuckyCloverFarm"));
		B.DisplayName = FText::FromString(TEXT("Lucky Clover Farm"));
		B.ProductionCost = 100;
		B.UpkeepGold = 0;
		B.FoodBonus = 4; // Extraordinary farming
		B.HappinessBonus = 1; // Good harvests bring joy
		B.Prerequisites.Add(FName(TEXT("Granary")));
		B.RaceRequirement = CoMTags::Race::Halflings;
		B.Architecture = ECoMArchitectureStyle::Human;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("BurglarsGuild"));
		B.DisplayName = FText::FromString(TEXT("Burglar's Guild"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.GoldBonus = 3; // Ill-gotten gains
		B.EnablesUnitTag = TEXT("Rogue"); // Enables Rogue units
		B.bEnablesSpyRecruitment = true;
		// +20% spy efficiency (implemented in EspionageSubsystem)
		B.Prerequisites.Add(FName(TEXT("ThievesGuild")));
		B.RaceRequirement = CoMTags::Race::Halflings;
		B.Architecture = ECoMArchitectureStyle::Human;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("FeastHall"));
		B.DisplayName = FText::FromString(TEXT("Feast Hall"));
		B.ProductionCost = 150;
		B.UpkeepGold = 1;
		B.HappinessBonus = 3;
		B.FoodBonus = 2; // Communal feasting
		B.Prerequisites.Add(FName(TEXT("Tavern")));
		B.RaceRequirement = CoMTags::Race::Halflings;
		B.Architecture = ECoMArchitectureStyle::Human;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Trolls ──────────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("RegenerationPool"));
		B.DisplayName = FText::FromString(TEXT("Regeneration Pool"));
		B.ProductionCost = 300;
		B.UpkeepGold = 2;
		B.ManaBonus = 1; // Primal regenerative magic
		B.EnablesUnitTag = TEXT("Regeneration"); // All units gain Regeneration ability
		B.Prerequisites.Add(FName(TEXT("Shrine")));
		B.RaceRequirement = CoMTags::Race::Trolls;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("TrollBridge"));
		B.DisplayName = FText::FromString(TEXT("Troll Bridge"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.GoldBonus = 3;
		// Also generates toll income from passing trade routes (implemented in TradeSubsystem)
		B.RaceRequirement = CoMTags::Race::Trolls;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("CarnagePit"));
		B.DisplayName = FText::FromString(TEXT("Carnage Pit"));
		B.ProductionCost = 250;
		B.UpkeepGold = 2;
		B.RecruitAttackBonus = 2; // +2 attack for recruited units
		B.EnablesUnitTag = TEXT("Berserker"); // Enables Berserker units
		B.Prerequisites.Add(FName(TEXT("FightersGuild")));
		B.RaceRequirement = CoMTags::Race::Trolls;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Gnolls ──────────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("HyenaKennels"));
		B.DisplayName = FText::FromString(TEXT("Hyena Kennels"));
		B.ProductionCost = 150;
		B.UpkeepGold = 1;
		B.EnablesUnitTag = TEXT("PackCavalry");
		B.Prerequisites.Add(FName(TEXT("Stable")));
		B.RaceRequirement = CoMTags::Race::Gnolls;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("RaidersCamp"));
		B.DisplayName = FText::FromString(TEXT("Raider's Camp"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.GoldBonus = 2;
		B.EnablesUnitTag = TEXT("Scout"); // Enables Scout units
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		B.RaceRequirement = CoMTags::Race::Gnolls;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("BoneShrine"));
		B.DisplayName = FText::FromString(TEXT("Bone Shrine"));
		B.ProductionCost = 100;
		B.UpkeepGold = 0;
		B.ManaBonus = 2;
		B.ResearchBonus = 1; // Ancestral bone divination
		B.Prerequisites.Add(FName(TEXT("Shrine")));
		B.RaceRequirement = CoMTags::Race::Gnolls;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Lizardmen ───────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("SwampHatchery"));
		B.DisplayName = FText::FromString(TEXT("Swamp Hatchery"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.FoodBonus = 3;
		B.EnablesUnitTag = TEXT("Raptor"); // Enables Raptor cavalry
		B.Prerequisites.Add(FName(TEXT("Granary")));
		B.RaceRequirement = CoMTags::Race::Lizardmen;
		B.Architecture = ECoMArchitectureStyle::Aquatic;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("VenomLab"));
		B.DisplayName = FText::FromString(TEXT("Venom Lab"));
		B.ProductionCost = 250;
		B.UpkeepGold = 2;
		B.ProductionBonus = 1; // Venom processing
		B.EnablesUnitTag = TEXT("Poison"); // Enables Poison weapons for all units
		B.Prerequisites.Add(FName(TEXT("AlchemistLab")));
		B.RaceRequirement = CoMTags::Race::Lizardmen;
		B.Architecture = ECoMArchitectureStyle::Aquatic;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("ScaleForge"));
		B.DisplayName = FText::FromString(TEXT("Scale Forge"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.ProductionBonus = 2; // Scale armor production
		B.CityDefenseBonus = 1; // +1 defense for recruited units
		B.Prerequisites.Add(FName(TEXT("Smithy")));
		B.RaceRequirement = CoMTags::Race::Lizardmen;
		B.Architecture = ECoMArchitectureStyle::Aquatic;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Nomads ──────────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("CaravanBazaar"));
		B.DisplayName = FText::FromString(TEXT("Caravan Bazaar"));
		B.ProductionCost = 250;
		B.UpkeepGold = 2;
		B.GoldBonus = 5;
		B.GoldMultiplierPercent = 200; // Trade route income doubled
		B.Prerequisites.Add(FName(TEXT("Marketplace")));
		B.RaceRequirement = CoMTags::Race::Nomads;
		B.Architecture = ECoMArchitectureStyle::Human;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("HorseLordsHall"));
		B.DisplayName = FText::FromString(TEXT("Horse Lord's Hall"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.EnablesUnitTag = TEXT("EliteCavalry");
		// +2 movement for cavalry units (implemented in MovementSubsystem)
		B.Prerequisites.Add(FName(TEXT("Stable")));
		B.RaceRequirement = CoMTags::Race::Nomads;
		B.Architecture = ECoMArchitectureStyle::Human;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("DesertOasis"));
		B.DisplayName = FText::FromString(TEXT("Desert Oasis"));
		B.ProductionCost = 150;
		B.UpkeepGold = 0;
		B.FoodBonus = 3;
		B.GoldBonus = 1; // Oasis trade stop
		B.HappinessBonus = 2;
		B.RaceRequirement = CoMTags::Race::Nomads;
		B.Architecture = ECoMArchitectureStyle::Human;
		RegisterBuilding(MoveTemp(B));
	}

	// ─── Beastmen ────────────────────────────────────────────────────────

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("PrimalShrine"));
		B.DisplayName = FText::FromString(TEXT("Primal Shrine"));
		B.ProductionCost = 200;
		B.UpkeepGold = 1;
		B.ManaBonus = 3;
		B.EnablesUnitTag = TEXT("TotemGuard"); // Enables Totem Guard units
		B.Prerequisites.Add(FName(TEXT("Shrine")));
		B.Prerequisites.Add(FName(TEXT("Temple")));
		B.RaceRequirement = CoMTags::Race::Beastmen;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("BeastPens"));
		B.DisplayName = FText::FromString(TEXT("Beast Pens"));
		B.ProductionCost = 150;
		B.UpkeepGold = 1;
		B.FoodBonus = 1;
		B.EnablesUnitTag = TEXT("BeastCavalry");
		B.Prerequisites.Add(FName(TEXT("Barracks")));
		B.RaceRequirement = CoMTags::Race::Beastmen;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}

	{
		FCoMBuildingInfo B;
		B.BuildingID = FName(TEXT("TotemCircle"));
		B.DisplayName = FText::FromString(TEXT("Totem Circle"));
		B.ProductionCost = 250;
		B.UpkeepGold = 2;
		B.SummonCostReduction = 4; // -10% summon cost (Nature summoning focus)
		B.bEnablesSummoning = true; // Enables Nature summoning
		B.EnablesUnitTag = TEXT("NatureSummon"); // Nature-realm summons
		B.Prerequisites.Add(FName(TEXT("SummoningCircle")));
		B.RaceRequirement = CoMTags::Race::Beastmen;
		B.Architecture = ECoMArchitectureStyle::Orcish;
		RegisterBuilding(MoveTemp(B));
	}
}

// =====================================================================
// GetArchitectureForRace — map each race tag to an architecture style
// =====================================================================

ECoMArchitectureStyle CoMBuildingDatabase::GetArchitectureForRace(FGameplayTag RaceTag)
{
	// ── Human style ──────────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::HighMen)     return ECoMArchitectureStyle::Human;
	if (RaceTag == CoMTags::Race::Nomads)      return ECoMArchitectureStyle::Human;
	if (RaceTag == CoMTags::Race::Halflings)   return ECoMArchitectureStyle::Human;
	if (RaceTag == CoMTags::Race::Barbarians)  return ECoMArchitectureStyle::Human;

	// ── Elven style ──────────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::HighElves)   return ECoMArchitectureStyle::Elven;
	if (RaceTag == CoMTags::Race::Treants)     return ECoMArchitectureStyle::Elven;

	// ── Dwarven style ────────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::Dwarves)       return ECoMArchitectureStyle::Dwarven;
	if (RaceTag == CoMTags::Race::DeepDwarves)   return ECoMArchitectureStyle::Dwarven;
	if (RaceTag == CoMTags::Race::MagmaDwarves)  return ECoMArchitectureStyle::Dwarven;
	if (RaceTag == CoMTags::Race::HellmirrorDwarves) return ECoMArchitectureStyle::Dwarven;
	if (RaceTag == CoMTags::Race::Goblins)       return ECoMArchitectureStyle::Dwarven;

	// ── Orcish style ─────────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::Orcs)        return ECoMArchitectureStyle::Orcish;
	if (RaceTag == CoMTags::Race::Gnolls)      return ECoMArchitectureStyle::Orcish;
	if (RaceTag == CoMTags::Race::Beastmen)    return ECoMArchitectureStyle::Orcish;
	if (RaceTag == CoMTags::Race::Trolls)      return ECoMArchitectureStyle::Orcish;

	// ── Dark style ───────────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::DarkElves)   return ECoMArchitectureStyle::Dark;
	if (RaceTag == CoMTags::Race::Drow)        return ECoMArchitectureStyle::Dark;
	if (RaceTag == CoMTags::Race::Shades)      return ECoMArchitectureStyle::Dark;
	if (RaceTag == CoMTags::Race::Nightbinders) return ECoMArchitectureStyle::Dark;
	if (RaceTag == CoMTags::Race::ShadowPact)  return ECoMArchitectureStyle::Dark;

	// ── Draconic style ───────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::Draconians)  return ECoMArchitectureStyle::Draconic;
	if (RaceTag == CoMTags::Race::Salamanders) return ECoMArchitectureStyle::Draconic;
	if (RaceTag == CoMTags::Race::LavaNaga)    return ECoMArchitectureStyle::Draconic;

	// ── Demonic style ────────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::Demonkin)    return ECoMArchitectureStyle::Demonic;
	if (RaceTag == CoMTags::Race::Hellbound)   return ECoMArchitectureStyle::Demonic;
	if (RaceTag == CoMTags::Race::Ferrovians)  return ECoMArchitectureStyle::Demonic;
	if (RaceTag == CoMTags::Race::Ashguard)    return ECoMArchitectureStyle::Demonic;
	if (RaceTag == CoMTags::Race::ImpSwarms)   return ECoMArchitectureStyle::Demonic;
	if (RaceTag == CoMTags::Race::SoulForgers) return ECoMArchitectureStyle::Demonic;
	if (RaceTag == CoMTags::Race::Ashen)       return ECoMArchitectureStyle::Demonic;
	if (RaceTag == CoMTags::Race::Obsidians)   return ECoMArchitectureStyle::Demonic;

	// ── Aquatic style ────────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::Merfolk)       return ECoMArchitectureStyle::Aquatic;
	if (RaceTag == CoMTags::Race::DeepOnes)      return ECoMArchitectureStyle::Aquatic;
	if (RaceTag == CoMTags::Race::Tideshifters)  return ECoMArchitectureStyle::Aquatic;
	if (RaceTag == CoMTags::Race::EtherSwimmers) return ECoMArchitectureStyle::Aquatic;
	if (RaceTag == CoMTags::Race::Lizardmen)     return ECoMArchitectureStyle::Aquatic;

	// ── Fey style ────────────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::Sprites)       return ECoMArchitectureStyle::Fey;
	if (RaceTag == CoMTags::Race::Pixies)        return ECoMArchitectureStyle::Fey;
	if (RaceTag == CoMTags::Race::Satyrs)        return ECoMArchitectureStyle::Fey;
	if (RaceTag == CoMTags::Race::Dryads)        return ECoMArchitectureStyle::Fey;
	if (RaceTag == CoMTags::Race::Changelings)   return ECoMArchitectureStyle::Fey;
	if (RaceTag == CoMTags::Race::FeyRootkin)    return ECoMArchitectureStyle::Fey;
	if (RaceTag == CoMTags::Race::MushroomFolk)  return ECoMArchitectureStyle::Fey;
	if (RaceTag == CoMTags::Race::Myconids)      return ECoMArchitectureStyle::Fey;
	if (RaceTag == CoMTags::Race::Sporekin)      return ECoMArchitectureStyle::Fey;

	// ── Ethereal style ───────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::Wraiths)        return ECoMArchitectureStyle::Ethereal;
	if (RaceTag == CoMTags::Race::Phantoms)       return ECoMArchitectureStyle::Ethereal;
	if (RaceTag == CoMTags::Race::Mistborn)       return ECoMArchitectureStyle::Ethereal;
	if (RaceTag == CoMTags::Race::Thoughteaters)  return ECoMArchitectureStyle::Ethereal;
	if (RaceTag == CoMTags::Race::Spectrels)      return ECoMArchitectureStyle::Ethereal;
	if (RaceTag == CoMTags::Race::EchoWraiths)    return ECoMArchitectureStyle::Ethereal;
	if (RaceTag == CoMTags::Race::MemoryDevourers) return ECoMArchitectureStyle::Ethereal;
	if (RaceTag == CoMTags::Race::EchoFolk)       return ECoMArchitectureStyle::Ethereal;
	if (RaceTag == CoMTags::Race::Mindflayers)    return ECoMArchitectureStyle::Ethereal;
	if (RaceTag == CoMTags::Race::Dreamweavers)   return ECoMArchitectureStyle::Ethereal;

	// ── Abyssal style ────────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::Maleficii)      return ECoMArchitectureStyle::Abyssal;
	if (RaceTag == CoMTags::Race::Gorethians)     return ECoMArchitectureStyle::Abyssal;
	if (RaceTag == CoMTags::Race::Plagueborn)     return ECoMArchitectureStyle::Abyssal;
	if (RaceTag == CoMTags::Race::Voidlings)      return ECoMArchitectureStyle::Abyssal;
	if (RaceTag == CoMTags::Race::Ashwalkers)     return ECoMArchitectureStyle::Abyssal;
	if (RaceTag == CoMTags::Race::ChaosSpawn)     return ECoMArchitectureStyle::Abyssal;
	if (RaceTag == CoMTags::Race::VoidStalkers)   return ECoMArchitectureStyle::Abyssal;

	// ── Crystalline style ────────────────────────────────────────────────
	if (RaceTag == CoMTags::Race::Crystallines)   return ECoMArchitectureStyle::Crystalline;
	if (RaceTag == CoMTags::Race::Sylphs)         return ECoMArchitectureStyle::Crystalline;
	if (RaceTag == CoMTags::Race::Voidwalkers)    return ECoMArchitectureStyle::Crystalline;

	// ── Verdantis races (insectoid/plant) ────────────────────────────────
	if (RaceTag == CoMTags::Race::Chithari)       return ECoMArchitectureStyle::Fey;
	if (RaceTag == CoMTags::Race::Myrmidons)      return ECoMArchitectureStyle::Orcish;

	// Default fallback
	return ECoMArchitectureStyle::Human;
}

// =====================================================================
// GetBuildingsForRace — return all racial buildings for a given race
// =====================================================================

TArray<FName> CoMBuildingDatabase::GetBuildingsForRace(FGameplayTag RaceTag)
{
	EnsureInitialized();

	TArray<FName> Result;
	for (const auto& Pair : BuildingTable)
	{
		if (Pair.Value.RaceRequirement.IsValid() && Pair.Value.RaceRequirement == RaceTag)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}
