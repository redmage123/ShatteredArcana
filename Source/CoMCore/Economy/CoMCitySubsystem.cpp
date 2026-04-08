// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMCitySubsystem.h"
#include "CoMCore/World/CoMSeasonSubsystem.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/CoreTypes/CoMConstants.h"

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
	City.CityID            = CityID;
	City.CityName          = Name;
	City.OwnerWizardIndex  = OwnerWizard;
	City.Plane             = Plane;
	City.Layer             = Layer;
	City.Position          = Position;
	City.Population        = 1;
	City.PrimaryRace       = Race;
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
	TArray<int32> CityIDs;
	AllCities.GetKeys(CityIDs);

	for (const int32 CityID : CityIDs)
	{
		FCoMCityData* City = AllCities.Find(CityID);
		if (!City)
		{
			continue;
		}

		// 1. Recalculate all outputs.
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

	const int32 FoodAfterSeason = FMath::RoundToInt32(
		static_cast<float>(RawFood) * SeasonMods.FoodMultiplier.ToFloat());

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

	int32 Cap = 10;
	if (City->Layer == ECoMMapLayer::Underdark)
	{
		Cap = 6;
	}
	else if (City->Layer == ECoMMapLayer::Underwater)
	{
		Cap = 5;
	}

	Cap += City->BuildingIDs.Num();

	return Cap;
}

// =====================================================================
// Internal helpers -- distance & validation
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
	DX = FMath::Min(DX, MapWidth - DX);

	const int32 DY = FMath::Abs(A.Y - B.Y);
	return DX + DY;
}

bool UCoMCitySubsystem::IsAquaticRace(ECoMRace Race)
{
	switch (Race)
	{
	case ECoMRace::Merfolk:
		return true;
	default:
		return false;
	}
}

// =====================================================================
// Internal helpers -- city radius tiles
// =====================================================================

TArray<FIntPoint> UCoMCitySubsystem::GetCityRadiusTiles(FIntPoint Center) const
{
	TArray<FIntPoint> Tiles;
	Tiles.Reserve((CityRadius * 2 + 1) * (CityRadius * 2 + 1));

	for (int32 DY = -CityRadius; DY <= CityRadius; ++DY)
	{
		for (int32 DX = -CityRadius; DX <= CityRadius; ++DX)
		{
			if (FMath::Abs(DX) + FMath::Abs(DY) > CityRadius)
			{
				continue;
			}

			int32 TX = Center.X + DX;
			int32 TY = Center.Y + DY;

			if (TX < 0)        TX += MapWidth;
			if (TX >= MapWidth) TX -= MapWidth;

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
// Internal helpers -- per-tile food
// =====================================================================

int32 UCoMCitySubsystem::ComputeTileFood(ECoMPlane Plane, ECoMMapLayer Layer,
	FIntPoint TilePos) const
{
	const UCoMWorldMapSubsystem* MapSub = GetGameInstance()->GetSubsystem<UCoMWorldMapSubsystem>();
	if (!MapSub)
	{
		return 0;
	}

	// Use GetTileAtPos which takes FIntPoint (not GetTile which takes X, Y separately).
	const FCoMTileData* Tile = MapSub->GetTileAtPos(Plane, Layer, TilePos);
	if (!Tile)
	{
		return 0;
	}

	switch (Layer)
	{
	case ECoMMapLayer::Surface:
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
		case ECoMTerrain::Mountains:   return 0;
		case ECoMTerrain::Ocean:       return 0;
		case ECoMTerrain::Shore:       return 1;
		default:                       return 0;
		}

	case ECoMMapLayer::Underdark:
		if (Tile->Terrain == ECoMTerrain::FungalGrove)
		{
			return 1;
		}
		return 0;

	case ECoMMapLayer::Underwater:
		if (Tile->Terrain == ECoMTerrain::KelpForest || Tile->Terrain == ECoMTerrain::CoralReef)
		{
			return 1;
		}
		return 0;

	default:
		return 0;
	}
}

// =====================================================================
// Internal helpers -- base economy
// =====================================================================

int32 UCoMCitySubsystem::ComputeBaseGold(const FCoMCityData& City) const
{
	int32 Gold = City.Population / 2;
	Gold += City.BuildingIDs.Num();
	return FMath::Max(0, Gold);
}

int32 UCoMCitySubsystem::ComputeBaseProduction(const FCoMCityData& City) const
{
	int32 Prod = City.Population / 2;
	Prod += City.BuildingIDs.Num();
	return FMath::Max(0, Prod);
}

int32 UCoMCitySubsystem::ComputeBaseMana(const FCoMCityData& City) const
{
	return City.BuildingIDs.Num() / 3;
}

int32 UCoMCitySubsystem::ComputeBaseResearch(const FCoMCityData& City) const
{
	return (City.Population / 3) + (City.BuildingIDs.Num() / 4);
}

// =====================================================================
// Internal helpers -- unrest
// =====================================================================

int32 UCoMCitySubsystem::ComputeUnrest(const FCoMCityData& City) const
{
	int32 UnrestLevel = 0;

	UnrestLevel += City.MinorityRaces.Num();

	const int32 Cap = GetCityPopulationCap(City.CityID);
	if (City.Population > Cap)
	{
		UnrestLevel += (City.Population - Cap);
	}

	if (City.FoodSurplus < 0)
	{
		UnrestLevel += FMath::Abs(City.FoodSurplus);
	}

	UnrestLevel -= CountUnrestReduction(City);

	return FMath::Clamp(UnrestLevel, 0, MaxUnrest);
}

int32 UCoMCitySubsystem::CountUnrestReduction(const FCoMCityData& City) const
{
	int32 Reduction = 0;

	if (City.GarrisonArmyID >= 0)
	{
		Reduction += 2;
	}

	Reduction += City.BuildingIDs.Num() / 4;

	return Reduction;
}

// =====================================================================
// Internal helpers -- growth & building
// =====================================================================

void UCoMCitySubsystem::ProcessGrowth(FCoMCityData& City)
{
	if (City.FoodSurplus <= 0)
	{
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
// Internal helpers -- rebellion
// =====================================================================

void UCoMCitySubsystem::HandleRebellion(FCoMCityData& City)
{
	const int32 FormerOwner = City.OwnerWizardIndex;

	UE_LOG(LogTemp, Warning, TEXT("City %d (%s) has rebelled! Unrest=%d, former owner wizard %d."),
		City.CityID, *City.CityName.ToString(), City.Unrest, FormerOwner);

	City.OwnerWizardIndex = -1;
	City.Unrest = 0;
	City.CurrentBuildingID = -1;
	City.BuildingProgress  = 0;
	City.GarrisonArmyID = -1;

	OnCityRebelled.Broadcast(City.CityID, FormerOwner);
}
