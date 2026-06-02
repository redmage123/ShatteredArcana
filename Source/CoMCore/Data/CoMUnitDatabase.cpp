// Copyright Shattered Arcana. All Rights Reserved.
// CoMUnitDatabase.cpp — Static unit spec data for 15 races + special units
// Values inspired by Master of Magic / Caster of Magic racial unit rosters

#include "CoMUnitDatabase.h"

TMap<FName, FCoMUnitSpecInfo> CoMUnitDatabase::UnitTable;
FCoMUnitSpecInfo CoMUnitDatabase::DefaultUnit;
bool CoMUnitDatabase::bInitialized = false;

void CoMUnitDatabase::RegisterUnit(FCoMUnitSpecInfo&& Info)
{
	UnitTable.Add(Info.SpecID, MoveTemp(Info));
}

void CoMUnitDatabase::EnsureInitialized()
{
	if (!bInitialized)
	{
		DefaultUnit.SpecID = FName(TEXT("Unknown"));
		DefaultUnit.DisplayName = FText::FromString(TEXT("Unknown Unit"));
		DefaultUnit.ProductionCost = 50;
		DefaultUnit.HitPoints = 4;
		DefaultUnit.MeleeAttack = 3;
		DefaultUnit.Defense = 2;
		InitializeDatabase();
		bInitialized = true;
	}
}

const FCoMUnitSpecInfo& CoMUnitDatabase::GetUnitSpec(FName SpecID)
{
	EnsureInitialized();
	if (const FCoMUnitSpecInfo* Found = UnitTable.Find(SpecID))
	{
		return *Found;
	}
	return DefaultUnit;
}

TArray<FName> CoMUnitDatabase::GetAllUnitSpecIDs()
{
	EnsureInitialized();
	TArray<FName> Result;
	UnitTable.GetKeys(Result);
	return Result;
}

TArray<FName> CoMUnitDatabase::GetUnitsForRace(const FString& RaceTag)
{
	EnsureInitialized();
	TArray<FName> Result;
	for (const auto& Pair : UnitTable)
	{
		if (Pair.Value.RaceTag == RaceTag)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

TArray<FName> CoMUnitDatabase::GetUnitsRequiringBuilding(const FString& BuildingTag)
{
	EnsureInitialized();
	TArray<FName> Result;
	for (const auto& Pair : UnitTable)
	{
		if (Pair.Value.RequiredBuildingTag == BuildingTag)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

bool CoMUnitDatabase::Contains(FName SpecID)
{
	EnsureInitialized();
	return UnitTable.Contains(SpecID);
}

void CoMUnitDatabase::InitializeDatabase()
{
	// ═════════════════════════════════════════════════════════════════
	// SPECIAL UNITS
	// ═════════════════════════════════════════════════════════════════

	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Settler"));
		U.DisplayName = FText::FromString(TEXT("Settler"));
		U.RaceTag = TEXT("Any");
		U.HitPoints = 1;
		U.MeleeAttack = 0;
		U.RangedAttack = 0;
		U.Defense = 0;
		U.Resistance = 0;
		U.Movement = 2;
		U.Figures = 1;
		U.ProductionCost = 60;
		U.UpkeepGold = 0;
		U.UpkeepFood = 1;
		U.bSettler = true;
		U.CategoryTag = TEXT("Settler");
		U.RequiredBuildingTag = TEXT("");
		RegisterUnit(MoveTemp(U));
	}

	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Hero_Fighter"));
		U.DisplayName = FText::FromString(TEXT("Hero (Fighter)"));
		U.RaceTag = TEXT("Any");
		U.HitPoints = 10;
		U.MeleeAttack = 6;
		U.RangedAttack = 0;
		U.Defense = 4;
		U.Resistance = 5;
		U.Movement = 3;
		U.Figures = 1;
		U.ProductionCost = 0; // Heroes are hired, not produced
		U.UpkeepGold = 5;
		U.UpkeepFood = 1;
		U.bHero = true;
		U.CategoryTag = TEXT("Hero");
		RegisterUnit(MoveTemp(U));
	}

	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Hero_Magician"));
		U.DisplayName = FText::FromString(TEXT("Hero (Magician)"));
		U.RaceTag = TEXT("Any");
		U.HitPoints = 6;
		U.MeleeAttack = 2;
		U.RangedAttack = 5;
		U.Defense = 2;
		U.Resistance = 8;
		U.Movement = 2;
		U.Figures = 1;
		U.RangedShots = 4;
		U.ProductionCost = 0;
		U.UpkeepGold = 5;
		U.UpkeepFood = 1;
		U.UpkeepMana = 3;
		U.bHero = true;
		U.CategoryTag = TEXT("Hero");
		RegisterUnit(MoveTemp(U));
	}

	// ═════════════════════════════════════════════════════════════════
	// RACIAL UNITS — 15 races, 3 unit types each
	// Base: Infantry (HP4/Atk3/Def2), Cavalry (HP6/Atk4/Def3), Ranged (HP3/Atk1/Rng4/Def1)
	// ═════════════════════════════════════════════════════════════════

	// ────────────────────────────────────────────────────────────────
	// 1. HIGH MEN — Disciplined, balanced. +1 defense
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("HighMen_Infantry"));
		U.DisplayName = FText::FromString(TEXT("High Men Swordsmen"));
		U.RaceTag = TEXT("Race.HighMen");
		U.HitPoints = 4; U.MeleeAttack = 3; U.Defense = 3; U.Resistance = 4;
		U.Movement = 2; U.Figures = 6; U.ProductionCost = 20;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("HighMen_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("High Men Knights"));
		U.RaceTag = TEXT("Race.HighMen");
		U.HitPoints = 6; U.MeleeAttack = 4; U.Defense = 4; U.Resistance = 4;
		U.Movement = 3; U.Figures = 4; U.ProductionCost = 40;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("HighMen_Ranged"));
		U.DisplayName = FText::FromString(TEXT("High Men Bowmen"));
		U.RaceTag = TEXT("Race.HighMen");
		U.HitPoints = 3; U.MeleeAttack = 1; U.RangedAttack = 4; U.Defense = 2; U.Resistance = 4;
		U.Movement = 2; U.Figures = 6; U.RangedShots = 8; U.ProductionCost = 30;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 2. HIGH ELVES — Longbow masters. +1 ranged, +1 resistance
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("HighElves_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Elven Spearmen"));
		U.RaceTag = TEXT("Race.HighElves");
		U.HitPoints = 4; U.MeleeAttack = 3; U.Defense = 2; U.Resistance = 5;
		U.Movement = 2; U.Figures = 6; U.ProductionCost = 20;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("HighElves_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Elven Stag Riders"));
		U.RaceTag = TEXT("Race.HighElves");
		U.HitPoints = 6; U.MeleeAttack = 4; U.Defense = 3; U.Resistance = 5;
		U.Movement = 3; U.Figures = 4; U.ProductionCost = 40;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("HighElves_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Elven Longbowmen"));
		U.RaceTag = TEXT("Race.HighElves");
		U.HitPoints = 3; U.MeleeAttack = 1; U.RangedAttack = 5; U.Defense = 1; U.Resistance = 5;
		U.Movement = 2; U.Figures = 6; U.RangedShots = 8; U.ProductionCost = 30;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 3. DWARVES — Heavy armor, slow. +2 defense, -1 movement, +1 resistance
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Dwarves_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Dwarven Hammerers"));
		U.RaceTag = TEXT("Race.Dwarves");
		U.HitPoints = 5; U.MeleeAttack = 3; U.Defense = 4; U.Resistance = 5;
		U.Movement = 1; U.Figures = 6; U.ProductionCost = 25;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Dwarves_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Dwarven Boar Riders"));
		U.RaceTag = TEXT("Race.Dwarves");
		U.HitPoints = 7; U.MeleeAttack = 4; U.Defense = 5; U.Resistance = 5;
		U.Movement = 2; U.Figures = 4; U.ProductionCost = 45;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Dwarves_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Dwarven Crossbowmen"));
		U.RaceTag = TEXT("Race.Dwarves");
		U.HitPoints = 3; U.MeleeAttack = 1; U.RangedAttack = 4; U.Defense = 3; U.Resistance = 5;
		U.Movement = 1; U.Figures = 6; U.RangedShots = 6; U.ProductionCost = 35;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 4. DRACONIANS — Dragon heritage. +1 melee, cavalry flies
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Draconians_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Draconian Warriors"));
		U.RaceTag = TEXT("Race.Draconians");
		U.HitPoints = 4; U.MeleeAttack = 4; U.Defense = 2; U.Resistance = 4;
		U.Movement = 2; U.Figures = 6; U.ProductionCost = 25;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Draconians_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Dragon Riders"));
		U.RaceTag = TEXT("Race.Draconians");
		U.HitPoints = 7; U.MeleeAttack = 5; U.Defense = 3; U.Resistance = 4;
		U.Movement = 4; U.Figures = 2; U.ProductionCost = 60;
		U.UpkeepGold = 3; U.UpkeepFood = 2;
		U.bFlying = true;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Draconians_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Draconian Fire Hurlers"));
		U.RaceTag = TEXT("Race.Draconians");
		U.HitPoints = 3; U.MeleeAttack = 2; U.RangedAttack = 4; U.Defense = 1; U.Resistance = 4;
		U.Movement = 2; U.Figures = 6; U.RangedShots = 6; U.ProductionCost = 35;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 5. DARK ELVES — Shadow blades. +1 ranged, +1 melee
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("DarkElves_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Dark Elf Nightblades"));
		U.RaceTag = TEXT("Race.DarkElves");
		U.HitPoints = 4; U.MeleeAttack = 4; U.Defense = 2; U.Resistance = 5;
		U.Movement = 2; U.Figures = 6; U.ProductionCost = 25;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("DarkElves_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Dark Elf Shadow Stalkers"));
		U.RaceTag = TEXT("Race.DarkElves");
		U.HitPoints = 6; U.MeleeAttack = 5; U.Defense = 3; U.Resistance = 5;
		U.Movement = 3; U.Figures = 4; U.ProductionCost = 45;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("DarkElves_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Dark Elf Warlocks"));
		U.RaceTag = TEXT("Race.DarkElves");
		U.HitPoints = 3; U.MeleeAttack = 2; U.RangedAttack = 5; U.Defense = 1; U.Resistance = 6;
		U.Movement = 2; U.Figures = 6; U.RangedShots = 4; U.ProductionCost = 35;
		U.UpkeepGold = 1; U.UpkeepFood = 1; U.UpkeepMana = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 6. DEMONS — Aggressive, reckless. +2 melee, -1 defense
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Demons_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Demon Hellspawn"));
		U.RaceTag = TEXT("Race.Demons");
		U.HitPoints = 5; U.MeleeAttack = 5; U.Defense = 1; U.Resistance = 4;
		U.Movement = 2; U.Figures = 6; U.ProductionCost = 25;
		U.UpkeepGold = 1; U.UpkeepFood = 0; U.UpkeepMana = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Demons_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Demon Dread Knights"));
		U.RaceTag = TEXT("Race.Demons");
		U.HitPoints = 7; U.MeleeAttack = 6; U.Defense = 2; U.Resistance = 4;
		U.Movement = 3; U.Figures = 4; U.ProductionCost = 50;
		U.UpkeepGold = 2; U.UpkeepFood = 0; U.UpkeepMana = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Demons_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Demon Hellfire Casters"));
		U.RaceTag = TEXT("Race.Demons");
		U.HitPoints = 3; U.MeleeAttack = 2; U.RangedAttack = 5; U.Defense = 0; U.Resistance = 5;
		U.Movement = 2; U.Figures = 4; U.RangedShots = 4; U.ProductionCost = 40;
		U.UpkeepGold = 1; U.UpkeepFood = 0; U.UpkeepMana = 2;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 7. MERFOLK — Aquatic resilience. +1 HP, swimming
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Merfolk_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Merfolk Tidewarriors"));
		U.RaceTag = TEXT("Race.Merfolk");
		U.HitPoints = 5; U.MeleeAttack = 3; U.Defense = 2; U.Resistance = 3;
		U.Movement = 2; U.Figures = 6; U.ProductionCost = 20;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.bSwimming = true;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Merfolk_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Merfolk Leviathan Riders"));
		U.RaceTag = TEXT("Race.Merfolk");
		U.HitPoints = 7; U.MeleeAttack = 4; U.Defense = 3; U.Resistance = 3;
		U.Movement = 4; U.Figures = 2; U.ProductionCost = 45;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.bSwimming = true;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Merfolk_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Merfolk Coral Slingers"));
		U.RaceTag = TEXT("Race.Merfolk");
		U.HitPoints = 4; U.MeleeAttack = 1; U.RangedAttack = 4; U.Defense = 1; U.Resistance = 3;
		U.Movement = 2; U.Figures = 6; U.RangedShots = 8; U.ProductionCost = 30;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.bSwimming = true;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 8. HALFLINGS — Small but lucky. +2 resistance, -1 HP
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Halflings_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Halfling Militia"));
		U.RaceTag = TEXT("Race.Halflings");
		U.HitPoints = 3; U.MeleeAttack = 2; U.Defense = 2; U.Resistance = 6;
		U.Movement = 2; U.Figures = 8; U.ProductionCost = 15;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Halflings_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Halfling Pony Riders"));
		U.RaceTag = TEXT("Race.Halflings");
		U.HitPoints = 5; U.MeleeAttack = 3; U.Defense = 3; U.Resistance = 6;
		U.Movement = 3; U.Figures = 4; U.ProductionCost = 35;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Halflings_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Halfling Slingers"));
		U.RaceTag = TEXT("Race.Halflings");
		U.HitPoints = 2; U.MeleeAttack = 1; U.RangedAttack = 5; U.Defense = 1; U.Resistance = 6;
		U.Movement = 2; U.Figures = 8; U.RangedShots = 8; U.ProductionCost = 25;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 9. ORCS — Brutal warriors. +1 melee, +1 HP, -1 resistance
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Orcs_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Orc Berserkers"));
		U.RaceTag = TEXT("Race.Orcs");
		U.HitPoints = 5; U.MeleeAttack = 4; U.Defense = 2; U.Resistance = 2;
		U.Movement = 2; U.Figures = 6; U.ProductionCost = 20;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Orcs_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Orc Warg Riders"));
		U.RaceTag = TEXT("Race.Orcs");
		U.HitPoints = 7; U.MeleeAttack = 5; U.Defense = 2; U.Resistance = 2;
		U.Movement = 3; U.Figures = 4; U.ProductionCost = 40;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Orcs_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Orc Rock Throwers"));
		U.RaceTag = TEXT("Race.Orcs");
		U.HitPoints = 4; U.MeleeAttack = 2; U.RangedAttack = 3; U.Defense = 1; U.Resistance = 2;
		U.Movement = 2; U.Figures = 6; U.RangedShots = 6; U.ProductionCost = 25;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 10. GNOLLS — Fast pack fighters. +1 movement, +1 melee
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Gnolls_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Gnoll Pack Warriors"));
		U.RaceTag = TEXT("Race.Gnolls");
		U.HitPoints = 4; U.MeleeAttack = 4; U.Defense = 1; U.Resistance = 2;
		U.Movement = 3; U.Figures = 6; U.ProductionCost = 18;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Gnolls_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Gnoll Hyena Riders"));
		U.RaceTag = TEXT("Race.Gnolls");
		U.HitPoints = 6; U.MeleeAttack = 5; U.Defense = 2; U.Resistance = 2;
		U.Movement = 4; U.Figures = 4; U.ProductionCost = 38;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Gnolls_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Gnoll Javelin Throwers"));
		U.RaceTag = TEXT("Race.Gnolls");
		U.HitPoints = 3; U.MeleeAttack = 2; U.RangedAttack = 3; U.Defense = 1; U.Resistance = 2;
		U.Movement = 3; U.Figures = 6; U.RangedShots = 6; U.ProductionCost = 25;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 11. LIZARDMEN — Amphibious, tough. +1 HP, +1 defense, swimming
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Lizardmen_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Lizardmen Warriors"));
		U.RaceTag = TEXT("Race.Lizardmen");
		U.HitPoints = 5; U.MeleeAttack = 3; U.Defense = 3; U.Resistance = 3;
		U.Movement = 2; U.Figures = 6; U.ProductionCost = 22;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.bSwimming = true;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Lizardmen_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Lizardmen Raptor Riders"));
		U.RaceTag = TEXT("Race.Lizardmen");
		U.HitPoints = 7; U.MeleeAttack = 4; U.Defense = 4; U.Resistance = 3;
		U.Movement = 3; U.Figures = 4; U.ProductionCost = 42;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.bSwimming = true;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Lizardmen_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Lizardmen Blowgunners"));
		U.RaceTag = TEXT("Race.Lizardmen");
		U.HitPoints = 4; U.MeleeAttack = 1; U.RangedAttack = 3; U.Defense = 2; U.Resistance = 3;
		U.Movement = 2; U.Figures = 6; U.RangedShots = 8; U.ProductionCost = 28;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.bSwimming = true;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 12. UNDEAD — Non-corporeal ranged, no food upkeep. +1 resistance
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Undead_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Skeleton Warriors"));
		U.RaceTag = TEXT("Race.Undead");
		U.HitPoints = 3; U.MeleeAttack = 3; U.Defense = 2; U.Resistance = 5;
		U.Movement = 2; U.Figures = 8; U.ProductionCost = 15;
		U.UpkeepGold = 0; U.UpkeepFood = 0; U.UpkeepMana = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Undead_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Death Knights"));
		U.RaceTag = TEXT("Race.Undead");
		U.HitPoints = 6; U.MeleeAttack = 5; U.Defense = 4; U.Resistance = 6;
		U.Movement = 3; U.Figures = 2; U.ProductionCost = 50;
		U.UpkeepGold = 0; U.UpkeepFood = 0; U.UpkeepMana = 2;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Undead_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Wraiths"));
		U.RaceTag = TEXT("Race.Undead");
		U.HitPoints = 4; U.MeleeAttack = 2; U.RangedAttack = 4; U.Defense = 1; U.Resistance = 7;
		U.Movement = 3; U.Figures = 4; U.RangedShots = 4; U.ProductionCost = 40;
		U.UpkeepGold = 0; U.UpkeepFood = 0; U.UpkeepMana = 2;
		U.bNonCorporeal = true; U.bFlying = true;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 13. TROLLS — Massive regenerators. +3 HP, +1 melee, -1 movement
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Trolls_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Troll Brutes"));
		U.RaceTag = TEXT("Race.Trolls");
		U.HitPoints = 7; U.MeleeAttack = 4; U.Defense = 2; U.Resistance = 3;
		U.Movement = 1; U.Figures = 4; U.ProductionCost = 30;
		U.UpkeepGold = 1; U.UpkeepFood = 2;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Trolls_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Troll War Beasts"));
		U.RaceTag = TEXT("Race.Trolls");
		U.HitPoints = 10; U.MeleeAttack = 5; U.Defense = 3; U.Resistance = 3;
		U.Movement = 2; U.Figures = 2; U.ProductionCost = 55;
		U.UpkeepGold = 2; U.UpkeepFood = 2;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Trolls_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Troll Boulder Chuckers"));
		U.RaceTag = TEXT("Race.Trolls");
		U.HitPoints = 6; U.MeleeAttack = 2; U.RangedAttack = 5; U.Defense = 1; U.Resistance = 3;
		U.Movement = 1; U.Figures = 4; U.RangedShots = 4; U.ProductionCost = 40;
		U.UpkeepGold = 1; U.UpkeepFood = 2;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 14. NOMADS — Fast and versatile. +1 movement, +1 ranged
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Nomads_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Nomad Raiders"));
		U.RaceTag = TEXT("Race.Nomads");
		U.HitPoints = 4; U.MeleeAttack = 3; U.Defense = 2; U.Resistance = 3;
		U.Movement = 3; U.Figures = 6; U.ProductionCost = 18;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Nomads_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Nomad Horsemen"));
		U.RaceTag = TEXT("Race.Nomads");
		U.HitPoints = 6; U.MeleeAttack = 4; U.Defense = 3; U.Resistance = 3;
		U.Movement = 4; U.Figures = 4; U.ProductionCost = 38;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Nomads_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Nomad Horse Archers"));
		U.RaceTag = TEXT("Race.Nomads");
		U.HitPoints = 3; U.MeleeAttack = 1; U.RangedAttack = 5; U.Defense = 1; U.Resistance = 3;
		U.Movement = 3; U.Figures = 6; U.RangedShots = 8; U.ProductionCost = 32;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ────────────────────────────────────────────────────────────────
	// 15. BEASTMEN — Fierce, tough. +1 melee, +1 HP
	// ────────────────────────────────────────────────────────────────
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Beastmen_Infantry"));
		U.DisplayName = FText::FromString(TEXT("Beastmen Warriors"));
		U.RaceTag = TEXT("Race.Beastmen");
		U.HitPoints = 5; U.MeleeAttack = 4; U.Defense = 2; U.Resistance = 3;
		U.Movement = 2; U.Figures = 6; U.ProductionCost = 22;
		U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Infantry"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Beastmen_Cavalry"));
		U.DisplayName = FText::FromString(TEXT("Beastmen Centaurs"));
		U.RaceTag = TEXT("Race.Beastmen");
		U.HitPoints = 7; U.MeleeAttack = 5; U.Defense = 3; U.Resistance = 3;
		U.Movement = 3; U.Figures = 4; U.ProductionCost = 42;
		U.UpkeepGold = 2; U.UpkeepFood = 1;
		U.CategoryTag = TEXT("Cavalry"); U.RequiredBuildingTag = TEXT("Stable");
		RegisterUnit(MoveTemp(U));
	}
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(TEXT("Beastmen_Ranged"));
		U.DisplayName = FText::FromString(TEXT("Beastmen Shamans"));
		U.RaceTag = TEXT("Race.Beastmen");
		U.HitPoints = 4; U.MeleeAttack = 2; U.RangedAttack = 4; U.Defense = 1; U.Resistance = 4;
		U.Movement = 2; U.Figures = 4; U.RangedShots = 4; U.ProductionCost = 35;
		U.UpkeepGold = 1; U.UpkeepFood = 1; U.UpkeepMana = 1;
		U.CategoryTag = TEXT("Ranged"); U.RequiredBuildingTag = TEXT("Barracks");
		RegisterUnit(MoveTemp(U));
	}

	// ── Engineers (race-gated road builders, MoM-style) ───────────────────────
	// Only a few races can train engineers (matches MoM lore — settled,
	// industrious civilisations). Light combat stats, the point is their
	// BuildRoad capability via bEngineer.
	auto AddEngineer = [&](const TCHAR* SpecId, const TCHAR* Name, const TCHAR* RaceTag)
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(SpecId);
		U.DisplayName = FText::FromString(Name);
		U.RaceTag = RaceTag;
		U.HitPoints = 4; U.MeleeAttack = 2; U.Defense = 3; U.Resistance = 5;
		U.Movement = 2; U.Figures = 4;
		U.ProductionCost = 60; U.UpkeepGold = 1; U.UpkeepFood = 1;
		U.bEngineer = true;
		U.CategoryTag = TEXT("Engineer");
		U.RequiredBuildingTag = TEXT(""); // recruitable from any city of the right race
		RegisterUnit(MoveTemp(U));
	};
	AddEngineer(TEXT("Dwarves_Engineer"),   TEXT("Dwarven Engineers"),     TEXT("Race.Dwarves"));
	AddEngineer(TEXT("HighMen_Engineer"),   TEXT("High Men Engineers"),    TEXT("Race.HighMen"));
	AddEngineer(TEXT("Beastmen_Engineer"),  TEXT("Beastmen Engineers"),    TEXT("Race.Beastmen"));
	AddEngineer(TEXT("Lizardmen_Engineer"), TEXT("Lizardmen Engineers"),   TEXT("Race.Lizardmen"));

	// ── Summoned creatures (conjured by Summon spells, not recruited) ─────────
	// Single powerful figures with mana upkeep and no food cost. Mapped from
	// summon spells by realm + rarity in UCoMMagicSubsystem::ResolveSpell.
	auto AddSummon = [&](const TCHAR* Id, const TCHAR* Name, int32 HP, int32 Melee,
		int32 Ranged, int32 Def, int32 Res, int32 Move, int32 Figures, int32 Mana,
		bool bFly, bool bGhost)
	{
		FCoMUnitSpecInfo U;
		U.SpecID = FName(Id);
		U.DisplayName = FText::FromString(Name);
		U.RaceTag = TEXT("Summoned");
		U.HitPoints = HP; U.MeleeAttack = Melee; U.RangedAttack = Ranged;
		U.Defense = Def; U.Resistance = Res; U.Movement = Move; U.Figures = Figures;
		U.RangedShots = (Ranged > 0) ? 2 : 0;
		U.ProductionCost = 0; U.UpkeepGold = 0; U.UpkeepFood = 0; U.UpkeepMana = Mana;
		U.bFlying = bFly; U.bNonCorporeal = bGhost;
		U.CategoryTag = TEXT("Summoned"); U.RequiredBuildingTag = TEXT("");
		RegisterUnit(MoveTemp(U));
	};

	//        Id                       Name               HP Mel Rng Def Res Mov Fig Mana Fly  Ghost
	AddSummon(TEXT("Summon_Skeleton"),       TEXT("Skeletons"),        6,  4, 0, 2, 5, 2, 3, 1, false, false);
	AddSummon(TEXT("Summon_PhantomWarrior"), TEXT("Phantom Warriors"), 8,  5, 0, 3, 7, 2, 1, 1, false, true);
	AddSummon(TEXT("Summon_WarBear"),        TEXT("War Bears"),        12, 7, 0, 4, 5, 3, 2, 2, false, false);
	AddSummon(TEXT("Summon_Hellhound"),      TEXT("Hell Hounds"),      10, 6, 0, 3, 5, 4, 2, 2, false, false);
	AddSummon(TEXT("Summon_GuardianSpirit"), TEXT("Guardian Spirit"),  10, 5, 0, 5, 8, 2, 1, 2, false, false);
	AddSummon(TEXT("Summon_Gargoyle"),       TEXT("Gargoyle"),         16, 8, 0, 9, 6, 3, 1, 4, true,  false);
	AddSummon(TEXT("Summon_Wraith"),         TEXT("Wraith"),           18, 9, 0, 5, 9, 3, 1, 4, true,  true);
	AddSummon(TEXT("Summon_Angel"),          TEXT("Angel"),            20, 10,0, 8, 10,3, 1, 5, true,  false);
	AddSummon(TEXT("Summon_Drake"),          TEXT("Great Drake"),      30, 14,8, 8, 9, 4, 1, 6, true,  false);

	// ── Life additions ────────────────────────────────────────────────────
	AddSummon(TEXT("Summon_Unicorns"),       TEXT("Unicorns"),          6,  7, 0, 4, 8, 3, 4, 3, false, false);
	AddSummon(TEXT("Summon_ArchAngel"),      TEXT("Arch Angel"),        30, 16, 0, 9, 11, 3, 1, 9, true,  false);

	// ── Death additions ───────────────────────────────────────────────────
	AddSummon(TEXT("Summon_Ghouls"),         TEXT("Ghouls"),            4,  4, 0, 3, 4, 2, 4, 1, false, false);
	AddSummon(TEXT("Summon_Werewolves"),     TEXT("Werewolves"),        10, 9, 0, 3, 5, 3, 2, 3, false, false);
	AddSummon(TEXT("Summon_NightStalker"),   TEXT("Night Stalker"),     16, 8, 0, 4, 8, 3, 1, 4, false, false);
	AddSummon(TEXT("Summon_ShadowDemons"),   TEXT("Shadow Demons"),     6,  5, 0, 3, 8, 3, 4, 4, true,  true);
	AddSummon(TEXT("Summon_DemonLord"),      TEXT("Demon Lord"),        30, 18, 0, 8, 10, 3, 1, 10, false, false);
	AddSummon(TEXT("Summon_DeathKnights"),   TEXT("Death Knights"),     12, 12, 0, 6, 9, 4, 4, 8, false, false);

	// ── Chaos additions ───────────────────────────────────────────────────
	AddSummon(TEXT("Summon_FireElemental"),  TEXT("Fire Elemental"),    12, 8, 0, 3, 6, 2, 1, 2, true,  false);
	AddSummon(TEXT("Summon_FireGiant"),      TEXT("Fire Giant"),        20, 12, 8, 5, 7, 3, 1, 5, false, false);
	AddSummon(TEXT("Summon_DoomBat"),        TEXT("Doom Bat"),          14, 11, 0, 4, 6, 5, 1, 4, true,  false);
	AddSummon(TEXT("Summon_Chimera"),        TEXT("Chimera"),           18, 12, 0, 5, 7, 3, 1, 5, true,  false);
	AddSummon(TEXT("Summon_Efreet"),         TEXT("Efreet"),            18, 10, 8, 5, 9, 3, 1, 6, true,  false);
	AddSummon(TEXT("Summon_Hydra"),          TEXT("Hydra"),             22, 14, 0, 6, 8, 2, 1, 7, false, false);
	AddSummon(TEXT("Summon_ChaosSpawn"),     TEXT("Chaos Spawn"),       20, 12, 0, 4, 8, 2, 1, 6, false, false);

	// ── Nature additions ──────────────────────────────────────────────────
	AddSummon(TEXT("Summon_Sprites"),        TEXT("Sprites"),           1,  2, 5, 5, 7, 4, 4, 1, true,  false);
	AddSummon(TEXT("Summon_Cockatrices"),    TEXT("Cockatrices"),       6,  6, 0, 4, 7, 3, 3, 4, true,  false);
	AddSummon(TEXT("Summon_Basilisk"),       TEXT("Basilisk"),          16, 10, 0, 4, 6, 1, 1, 4, false, false);
	AddSummon(TEXT("Summon_StoneGiant"),     TEXT("Stone Giant"),       22, 12, 6, 6, 7, 2, 1, 5, false, false);
	AddSummon(TEXT("Summon_EarthElemental"), TEXT("Earth Elemental"),   20, 12, 0, 5, 7, 1, 1, 5, false, false);
	AddSummon(TEXT("Summon_Gorgons"),        TEXT("Gorgons"),           14, 9, 0, 6, 9, 2, 2, 6, false, false);
	AddSummon(TEXT("Summon_Behemoth"),       TEXT("Behemoth"),          28, 14, 0, 5, 8, 2, 1, 7, false, false);
	AddSummon(TEXT("Summon_Colossus"),       TEXT("Colossus"),          32, 15, 0, 7, 10, 2, 1, 8, false, false);
	AddSummon(TEXT("Summon_GreatWyrm"),      TEXT("Great Wyrm"),        35, 18, 0, 8, 10, 3, 1, 11, false, false);

	// ── Sorcery additions ─────────────────────────────────────────────────
	AddSummon(TEXT("Summon_PhantomBeast"),   TEXT("Phantom Beast"),     10, 7, 0, 5, 7, 3, 1, 3, false, true);
	AddSummon(TEXT("Summon_AirElemental"),   TEXT("Air Elemental"),     10, 8, 0, 4, 6, 5, 1, 3, true,  false);
	AddSummon(TEXT("Summon_Nagas"),          TEXT("Nagas"),             8,  6, 0, 4, 8, 3, 2, 4, false, false);
	AddSummon(TEXT("Summon_StormGiant"),     TEXT("Storm Giant"),       22, 12, 10, 6, 9, 3, 1, 7, false, false);
	AddSummon(TEXT("Summon_Djinn"),          TEXT("Djinn"),             20, 10, 8, 5, 10, 4, 1, 7, true,  false);
	AddSummon(TEXT("Summon_SkyDrake"),       TEXT("Sky Drake"),         35, 18, 8, 8, 11, 5, 1, 12, true,  false);

	// ── Arcane additions ──────────────────────────────────────────────────
	AddSummon(TEXT("Summon_MagicSpirit"),    TEXT("Magic Spirit"),      8,  4, 0, 3, 6, 3, 1, 1, true,  true);
	AddSummon(TEXT("Summon_FloatingIsland"), TEXT("Floating Island"),   12, 0, 0, 6, 8, 3, 1, 2, true,  false);
}
