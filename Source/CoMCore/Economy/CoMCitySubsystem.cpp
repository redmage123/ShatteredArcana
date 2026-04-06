// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMCitySubsystem.h"
#include "CoMSeasonSubsystem.h"
#include "CoMWorldMapSubsystem.h"
#include "CoMConstants.h"

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMCitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AllCities.Empty();
	NextCityID = 1;
}

void UCoMCitySubsystem::Deinitialize()
{
	AllCities.Empty();
	Super::Deinitialize();
}

// =====================================================================
// City Founding & Destruction
// =====================================================================

int32 UCoMCitySubsystem::FoundCity(int32 OwnerWizard, ECoMPlane Plane,
	ECoMMapLayer Layer, FIntPoint Position, ECoMRace Race, FText Name)
{
	// Validate map bounds.
	if (Position.X < 0 || Position.X >= MapWidth || Position.Y < 0 || Position.Y >= MapHeight)
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCity: position (%d,%d) out of bounds."), Position.X, Position.Y);
		return -1;
	}

	// Underwater cities require aquatic races.
	if (Layer == ECoMMapLayer::Underwater && !IsAquaticRace(Race))
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCity: non-aquatic race cannot found underwater city."));
		return -1;
	}

	// Enforce minimum distance between cities.
	if (IsTooCloseToExistingCity(Plane, Layer, Position))
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCity: too close to existing city (MIN_CITY_DISTANCE=%d)."),
			CoM::MIN_CITY_DISTANCE);
		return -1;
	}

	// Allocate the city.
	const int32 CityID = NextCityID++;

	FCoMCityData City;
	City.CityID           = CityID;
	City.CityName         = Name;
	City.OwnerWizardIndex = OwnerWizard;
	City.Plane            = Plane;
	City.Layer            = Layer;
	City.Position         = Position;
	City.Population       = 1;
	City.PrimaryRace      = Race;
	City.CurrentBuildingID = -1;
	City.BuildingProgress  = 0;
	City.GarrisonArmyID    = -1;
	City.WallLevel         = 0;
	City.Unrest            = 0;

	// Underdark cities start without a light source.
	City.bHasLightSource = (Layer != ECoMMapLayer::Underdark);

	AllCities.Add(CityID, MoveTemp(City));

	// Compute initial outputs.
	RecalcCityOutputs(CityID);

	OnCityFounded.Broadcast(CityID, OwnerWizard);

	return CityID;
}

void UCoMCitySubsystem::DestroyCity(int32 CityID)
{
	if (FCoMCityData* City = AllCities.Find(CityID))
	{
		const int32 FormerOwner = City->OwnerWizardIndex;
		AllCities.Remove(CityID);
		OnCityDestroyed.Broadcast(CityID, FormerOwner);
	}
}

// =====================================================================
// Turn Processing
// =====================================================================

void UCoMCitySubsystem::ProcessCityTurn()
{
	// Collect keys first; rebellion may modify AllCities.
	TArray<int32> CityIDs;
	AllCities.GetKeys(CityIDs);

	for (const int32 CityID : CityIDs)
	{
		FCoMCityData* City = AllCities.Find(CityID);
		if (!City)
		{
			continue;
		}

		// 1. Recalculate all outputs (terrain + buildings + season + weather + enchantments).
		RecalcCityOutputs(CityID);

		// 2. Growth from food surplus.
		ProcessGrowth(*City);

		// 3. Building construction.
		ProcessBuilding(*City);

		// 4. Evaluate unrest.
		City->Unrest = ComputeUnrest(*City);

		// 5. Rebellion check.
		if (City->Unrest >= MaxUnrest)
		{
			HandleRebellion(*City);
		}
	}
}

// =====================================================================
// Queries
// =====================================================================

const FCoMCityData* UCoMCitySubsystem::GetCity(int32 CityID) const
{
	return AllCities.Find(CityID);
}

TArray<const FCoMCityData*> UCoMCitySubsystem::GetCitiesForWizard(int32 WizardIndex) const
{
	TArray<const FCoMCityData*> Result;
	for (const auto& Pair : AllCities)
	{
		if (Pair.Value.OwnerWizardIndex == WizardIndex)
		{
			Result.Add(&Pair.Value);
		}
	}
	return Result;
}

TArray<const FCoMCityData*> UCoMCitySubsystem::GetCitiesOnPlane(ECoMPlane Plane) const
{
	TArray<const FCoMCityData*> Result;
	for (const auto& Pair : AllCities)
	{
		if (Pair.Value.Plane == Plane)
		{
			Result.Add(&Pair.Value);
		}
	}
	return Result;
}

// =====================================================================
// Production Queue
// =====================================================================

bool UCoMCitySubsystem::SetBuildingQueue(int32 CityID, int32 BuildingID)
{
	FCoMCityData* City = AllCities.Find(CityID);
	if (!City)
	{
		return false;
	}

	// Cannot queue a building the city already has.
	if (City->BuildingIDs.Contains(BuildingID))
	{
		UE_LOG(LogTemp, Warning, TEXT("SetBuildingQueue: city %d already has building %d."),
			CityID, BuildingID);
		return false;
	}

	City->CurrentBuildingID = BuildingID;
	City->BuildingProgress  = 0;

	return true;
}

// =====================================================================
// Output Recalculation
// =====================================================================

void UCoMCitySubsystem::RecalcCityOutputs(int32 CityID)
{
	FCoMCityData* City = AllCities.Find(CityID);
	if (!City)
	{
		return;
	}

	// --- Gather seasonal modifiers ---
	FCoMSeasonEconomyModifiers SeasonMods;
	if (const UCoMSeasonSubsystem* SeasonSub = GetGameInstance()->GetSubsystem<UCoMSeasonSubsystem>())
	{
		SeasonMods = SeasonSub->GetEconomyModifiers(City->Plane);
	}

	// --- Food ---
	int32 RawFood = 0;
	{
		const TArray<FIntPoint> Tiles = GetCityRadiusTiles(City->Position);
		for (const FIntPoint& Tile : Tiles)
		{
			RawFood += ComputeTileFood(City->Plane, City->Layer, Tile);
		}
	}

	// Apply season food multiplier. Integer math: (RawFood * Numerator) / Denominator.
	// FFixed64 stores whole + fractional; approximate with integer rounding.
	const int32 FoodAfterSeason = FMath::RoundToInt32(
		static_cast<float>(RawFood) * SeasonMods.FoodMultiplier.ToFloat());

	// Food surplus = food produced - population consumption (1 food per pop).
	City->FoodSurplus = FoodAfterSeason - City->Population;

	// --- Gold ---
	const int32 RawGold = ComputeBaseGold(*City);
	City->GoldIncome = FMath::RoundToInt32(
		static_cast<float>(RawGold) * SeasonMods.TradeMultiplier.ToFloat());

	// --- Production ---
	int32 RawProduction = ComputeBaseProduction(*City);

	// Underdark mining bonus: +50%.
	if (City->Layer == ECoMMapLayer::Underdark)
	{
		RawProduction = (RawProduction * UnderdarkMiningNumerator) / UnderdarkMiningDenominator;
	}

	// Underdark light source penalty: halve output if no light.
	if (City->Layer == ECoMMapLayer::Underdark && !City->bHasLightSource)
	{
		RawProduction = (RawProduction * NoLightPenaltyNumerator) / NoLightPenaltyDenominator;
	}

	City->ProductionOutput = FMath::RoundToInt32(
		static_cast<float>(RawProduction) * SeasonMods.ProductionMultiplier.ToFloat());

	// --- Mana ---
	City->ManaOutput = ComputeBaseMana(*City);

	// --- Research ---
	City->ResearchOutput = ComputeBaseResearch(*City);
}

int32 UCoMCitySubsystem::GetCityPopulationCap(int32 CityID) const
{
	const FCoMCityData* City = AllCities.Find(CityID);
	if (!City)
	{
		return 0;
	}

	// Base cap from terrain: Surface = 10, Underdark = 6, Underwater = 5.
	int32 Cap = 10;
	if (City->Layer == ECoMMapLayer::Underdark)
	{
		Cap = 6;
	}
	else if (City->Layer == ECoMMapLayer::Underwater)
	{
		Cap = 5;
	}

	// Buildings add to cap. Each building contributes +1 for simplicity;
	// a real implementation would look up building data for housing capacity.
	Cap += City->BuildingIDs.Num();

	return Cap;
}

// =====================================================================
// Internal helpers — distance & validation
// =====================================================================

bool UCoMCitySubsystem::IsTooCloseToExistingCity(ECoMPlane Plane, ECoMMapLayer Layer,
	FIntPoint Position) const
{
	for (const auto& Pair : AllCities)
	{
		const FCoMCityData& Other = Pair.Value;
		if (Other.Plane != Plane || Other.Layer != Layer)
		{
			continue;
		}
		if (WrappedDistance(Position, Other.Position) < CoM::MIN_CITY_DISTANCE)
		{
			return true;
		}
	}
	return false;
}

int32 UCoMCitySubsystem::WrappedDistance(FIntPoint A, FIntPoint B)
{
	int32 DX = FMath::Abs(A.X - B.X);
	// WrapX: shortest distance on a 160-wide torus.
	DX = FMath::Min(DX, MapWidth - DX);

	const int32 DY = FMath::Abs(A.Y - B.Y);
	return DX + DY;
}

bool UCoMCitySubsystem::IsAquaticRace(ECoMRace Race)
{
	// Aquatic races that can settle underwater. Extend as the race enum grows.
	switch (Race)
	{
	case ECoMRace::Merfolk:
	case ECoMRace::Naga:
	case ECoMRace::SeaElf:
	case ECoMRace::Sahuagin:
	case ECoMRace::Triton:
		return true;
	default:
		return false;
	}
}

// =====================================================================
// Internal helpers — city radius tiles
// =====================================================================

TArray<FIntPoint> UCoMCitySubsystem::GetCityRadiusTiles(FIntPoint Center) const
{
	TArray<FIntPoint> Tiles;
	Tiles.Reserve((CityRadius * 2 + 1) * (CityRadius * 2 + 1));

	for (int32 DY = -CityRadius; DY <= CityRadius; ++DY)
	{
		for (int32 DX = -CityRadius; DX <= CityRadius; ++DX)
		{
			// Diamond/Manhattan radius.
			if (FMath::Abs(DX) + FMath::Abs(DY) > CityRadius)
			{
				continue;
			}

			int32 TX = Center.X + DX;
			int32 TY = Center.Y + DY;

			// WrapX.
			if (TX < 0)       TX += MapWidth;
			if (TX >= MapWidth) TX -= MapWidth;

			// Clamp Y (no vertical wrap).
			if (TY < 0 || TY >= MapHeight)
			{
				continue;
			}

			Tiles.Add(FIntPoint(TX, TY));
		}
	}

	return Tiles;
}

// =====================================================================
// Internal helpers — per-tile food
// =====================================================================

int32 UCoMCitySubsystem::ComputeTileFood(ECoMPlane Plane, ECoMMapLayer Layer,
	FIntPoint TilePos) const
{
	// Query the world map for tile data.
	const UCoMWorldMapSubsystem* MapSub = GetGameInstance()->GetSubsystem<UCoMWorldMapSubsystem>();
	if (!MapSub)
	{
		return 0;
	}

	const FCoMTileData* Tile = MapSub->GetTileData(Plane, Layer, TilePos);
	if (!Tile)
	{
		return 0;
	}

	switch (Layer)
	{
	case ECoMMapLayer::Surface:
		// Standard terrain food values.
		switch (Tile->Terrain)
		{
		case ECoMTerrain::Grassland:   return 3;
		case ECoMTerrain::Plains:      return 2;
		case ECoMTerrain::Forest:      return 1;
		case ECoMTerrain::River:       return 3;
		case ECoMTerrain::Hills:       return 1;
		case ECoMTerrain::Swamp:       return 1;
		case ECoMTerrain::Desert:      return 0;
		case ECoMTerrain::Tundra:      return 1;
		case ECoMTerrain::Mountain:    return 0;
		case ECoMTerrain::Ocean:       return 0;
		case ECoMTerrain::Shore:       return 1;
		default:                       return 0;
		}

	case ECoMMapLayer::Underdark:
		// No farms in the Underdark. Fungal food only.
		if (Tile->Terrain == ECoMTerrain::FungalForest)
		{
			return 1;
		}
		return 0;

	case ECoMMapLayer::Underwater:
		// Kelp forests and coral reefs provide food.
		if (Tile->Terrain == ECoMTerrain::KelpForest || Tile->Terrain == ECoMTerrain::CoralReef)
		{
			return 1;
		}
		return 0;
	}

	return 0;
}

// =====================================================================
// Internal helpers — base economy
// =====================================================================

int32 UCoMCitySubsystem::ComputeBaseGold(const FCoMCityData& City) const
{
	// Base gold = 1 per 2 population (workers generate trade).
	int32 Gold = City.Population / 2;

	// Placeholder: each building contributes a flat +1 gold.
	// Real implementation would look up building gold bonuses from a data table.
	Gold += City.BuildingIDs.Num();

	return FMath::Max(0, Gold);
}

int32 UCoMCitySubsystem::ComputeBaseProduction(const FCoMCityData& City) const
{
	// Base production = 1 per 2 population (worker output).
	int32 Prod = City.Population / 2;

	// Placeholder: each building contributes a flat +1 production.
	Prod += City.BuildingIDs.Num();

	return FMath::Max(0, Prod);
}

int32 UCoMCitySubsystem::ComputeBaseMana(const FCoMCityData& City) const
{
	// Mana comes primarily from buildings (shrines, temples, wizard towers).
	// Placeholder: 1 mana per 3 buildings.
	return City.BuildingIDs.Num() / 3;
}

int32 UCoMCitySubsystem::ComputeBaseResearch(const FCoMCityData& City) const
{
	// Research from population (scholars) and buildings (libraries, universities).
	// Placeholder: 1 research per 3 population + 1 per 4 buildings.
	return (City.Population / 3) + (City.BuildingIDs.Num() / 4);
}

// =====================================================================
// Internal helpers — unrest
// =====================================================================

int32 UCoMCitySubsystem::ComputeUnrest(const FCoMCityData& City) const
{
	int32 UnrestLevel = 0;

	// Source: racial mismatch (minority races unhappy in a city of another race).
	UnrestLevel += City.MinorityRaces.Num();

	// Source: overcrowding (population above cap).
	const int32 Cap = GetCityPopulationCap(City.CityID);
	if (City.Population > Cap)
	{
		UnrestLevel += (City.Population - Cap);
	}

	// Source: starvation (negative food surplus).
	if (City.FoodSurplus < 0)
	{
		UnrestLevel += FMath::Abs(City.FoodSurplus);
	}

	// Reduction: garrison and temple-type buildings.
	UnrestLevel -= CountUnrestReduction(City);

	return FMath::Clamp(UnrestLevel, 0, MaxUnrest);
}

int32 UCoMCitySubsystem::CountUnrestReduction(const FCoMCityData& City) const
{
	int32 Reduction = 0;

	// Garrison army present: -2 unrest.
	if (City.GarrisonArmyID >= 0)
	{
		Reduction += 2;
	}

	// Placeholder: each building reduces unrest by a small amount.
	// Real implementation would check building type (temple = -1, cathedral = -2, etc.).
	// For now, approximate: 1 reduction per 4 buildings.
	Reduction += City.BuildingIDs.Num() / 4;

	return Reduction;
}

// =====================================================================
// Internal helpers — growth & building
// =====================================================================

void UCoMCitySubsystem::ProcessGrowth(FCoMCityData& City)
{
	if (City.FoodSurplus <= 0)
	{
		// Starvation: lose 1 pop if surplus is negative and pop > 1.
		if (City.FoodSurplus < 0 && City.Population > 1)
		{
			const int32 OldPop = City.Population;
			City.Population -= 1;
			OnCityPopulationChanged.Broadcast(City.CityID, OldPop, City.Population);
		}
		return;
	}

	const int32 Cap = GetCityPopulationCap(City.CityID);
	if (City.Population >= Cap)
	{
		return;
	}

	// Growth rate: FoodSurplus / GrowthDivisor per turn (minimum 0).
	const int32 Growth = FMath::Max(1, City.FoodSurplus / GrowthDivisor);
	const int32 OldPop = City.Population;
	City.Population = FMath::Min(City.Population + Growth, Cap);

	if (City.Population != OldPop)
	{
		OnCityPopulationChanged.Broadcast(City.CityID, OldPop, City.Population);
	}
}

void UCoMCitySubsystem::ProcessBuilding(FCoMCityData& City)
{
	if (City.CurrentBuildingID < 0)
	{
		return;
	}

	City.BuildingProgress += City.ProductionOutput;

	// Placeholder building cost. Real implementation reads from a building data table.
	// Approximate: cost = BuildingID * 10 (so building 5 costs 50 production).
	const int32 BuildingCost = FMath::Max(10, City.CurrentBuildingID * 10);

	if (City.BuildingProgress >= BuildingCost)
	{
		const int32 CompletedID = City.CurrentBuildingID;
		City.BuildingIDs.Add(CompletedID);
		City.CurrentBuildingID = -1;
		City.BuildingProgress  = 0;

		OnBuildingCompleted.Broadcast(City.CityID, CompletedID, City.OwnerWizardIndex);
	}
}

// =====================================================================
// Internal helpers — rebellion
// =====================================================================

void UCoMCitySubsystem::HandleRebellion(FCoMCityData& City)
{
	const int32 FormerOwner = City.OwnerWizardIndex;

	UE_LOG(LogTemp, Warning, TEXT("City %d (%s) has rebelled! Unrest=%d, former owner wizard %d."),
		City.CityID, *City.CityName.ToString(), City.Unrest, FormerOwner);

	// City switches to neutral (wizard index -1 = barbarian/neutral).
	City.OwnerWizardIndex = -1;

	// Reset unrest after rebellion.
	City.Unrest = 0;

	// Clear production queue.
	City.CurrentBuildingID = -1;
	City.BuildingProgress  = 0;

	// Garrison is disbanded.
	City.GarrisonArmyID = -1;

	OnCityRebelled.Broadcast(City.CityID, FormerOwner);
}


---

Both files are ready to save as `CoMCitySubsystem.h` and `CoMCitySubsystem.cpp` in the repo root alongside the existing Shattered Arcana files.

Key design decisions matching the existing codebase conventions:
- Same copyright header, include structure, and `COMCORE_API` export macro as `CoMSeasonSubsystem` and `CoMHeroSubsystem`
- `FCoMEnchantmentInstance` struct defined locally (can be moved to `CoMCore/CoreTypes/CoMEnums.h` later if shared)
- `FFixed64` used via `SeasonMods.ToFloat()` for seasonal multiplier application
- `WrappedDistance` handles the WrapX=true, 160-wide map with Manhattan distance
- `CoM::MIN_CITY_DISTANCE` referenced from `CoMConstants.h`
- Placeholder building costs and economy formulas are clearly marked for future data-table hookup
- Underdark: no farms (only `FungalForest` = 1 food), +50% mining production, light source penalty
- Underwater: aquatic-race gating via `IsAquaticRace()`, kelp/reef food only
- Rebellion at unrest 10 flips owner to -1 (neutral/barbarian)