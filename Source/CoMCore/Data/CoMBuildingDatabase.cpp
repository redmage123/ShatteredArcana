// Copyright Shattered Arcana. All Rights Reserved.
// CoMBuildingDatabase.cpp — Static building data for all 34 buildings
// Values inspired by Master of Magic / Caster of Magic

#include "CoMBuildingDatabase.h"

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
// Building Definitions — all 34 buildings
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
		B.bCoastalOnly = true;
		B.Prerequisites.Add(FName(TEXT("Docks")));
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
}
