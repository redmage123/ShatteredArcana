// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMCitySubsystem.h"
#include "CoMCore/World/CoMSeasonSubsystem.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMCore/CoreTypes/CoMGameplayTags.h"
#include "CoMCore/Data/CoMBuildingDataAsset.h"
#include "CoMCore/Data/CoMUnitSpecDataAsset.h"
#include "CoMCore/Data/CoMRaceDataAsset.h"
#include "Engine/AssetManager.h"

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
	ECoMMapLayer Layer, FIntPoint Position, FGameplayTag RaceTag, FText Name)
{
	// Validate map bounds.
	if (Position.X < 0 || Position.X >= MapWidth || Position.Y < 0 || Position.Y >= MapHeight)
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCity: position (%d,%d) out of bounds."), Position.X, Position.Y);
		return -1;
	}

	// Underwater cities require aquatic races.
	if (Layer == ECoMMapLayer::Underwater && !IsAquaticRace(RaceTag))
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
	City.PrimaryRaceTag    = RaceTag;
	City.CurrentBuildingID   = -1;
	City.BuildingProgress    = 0;
	City.AccumulatedProduction = 0;
	City.GarrisonArmyID      = -1;
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
// Settler Production
// =====================================================================

int32 UCoMCitySubsystem::ProduceSettler(int32 CityId)
{
	FCoMCityData* City = AllCities.Find(CityId);
	if (!City)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProduceSettler: invalid CityId %d"), CityId);
		return -1;
	}

	// Population must be at least 2 (can't depopulate below 1).
	if (City->Population < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProduceSettler: city %d population %d is too low (need >= 2)"),
			CityId, City->Population);
		return -1;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return -1;
	}

	UCoMUnitSubsystem* UnitSub = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (!UnitSub)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProduceSettler: no UnitSubsystem"));
		return -1;
	}

	// Spawn a settler unit (SpecID 0 = generic settler placeholder).
	const int32 SettlerUnitId = UnitSub->SpawnUnit(
		0, City->Plane, City->Layer, City->Position, City->OwnerWizardIndex);

	if (SettlerUnitId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProduceSettler: SpawnUnit failed for city %d"), CityId);
		return -1;
	}

	// Mark the spawned unit as a settler and set its race tag via the subsystem's public API.
	UnitSub->SetUnitSettlerFlag(SettlerUnitId, true);
	UnitSub->SetUnitRaceTag(SettlerUnitId, City->PrimaryRaceTag);

	// Create a new army at the city's position containing just the settler.
	const int32 ArmyId = UnitSub->CreateArmy(
		City->OwnerWizardIndex, City->Plane, City->Layer, City->Position);
	UnitSub->AddUnitToArmy(SettlerUnitId, ArmyId);

	// Reduce city population by 1.
	const int32 OldPop = City->Population;
	City->Population -= 1;
	OnCityPopulationChanged.Broadcast(City->CityID, OldPop, City->Population);

	UE_LOG(LogTemp, Log, TEXT("ProduceSettler: city %d produced settler unit %d in army %d (pop %d -> %d)"),
		CityId, SettlerUnitId, ArmyId, OldPop, City->Population);

	return SettlerUnitId;
}

// =====================================================================
// Validation
// =====================================================================

bool UCoMCitySubsystem::CanFoundCityAt(ECoMPlane Plane, ECoMMapLayer Layer,
	FIntPoint Position, FGameplayTag RaceTag) const
{
	// Check map bounds.
	if (Position.X < 0 || Position.X >= MapWidth || Position.Y < 0 || Position.Y >= MapHeight)
	{
		return false;
	}

	// Underwater requires aquatic race.
	if (Layer == ECoMMapLayer::Underwater && !IsAquaticRace(RaceTag))
	{
		return false;
	}

	// Check minimum distance from existing cities.
	if (IsTooCloseToExistingCity(Plane, Layer, Position))
	{
		return false;
	}

	// Check terrain suitability.
	const UCoMWorldMapSubsystem* MapSub = GetGameInstance()->GetSubsystem<UCoMWorldMapSubsystem>();
	if (MapSub)
	{
		const FCoMTileData* Tile = MapSub->GetTileAtPos(Plane, Layer, Position);
		if (Tile)
		{
			// Cannot found cities on ocean (unless aquatic), mountains, or volcanic terrain.
			if (Tile->Terrain == ECoMTerrain::Ocean || Tile->Terrain == ECoMTerrain::VoidOcean ||
				Tile->Terrain == ECoMTerrain::MistOcean || Tile->Terrain == ECoMTerrain::SulfurSeas)
			{
				// Ocean tiles: only aquatic races can found here (and only on Underwater layer).
				if (!IsAquaticRace(RaceTag))
				{
					return false;
				}
			}

			if (Tile->Terrain == ECoMTerrain::Mountains || Tile->Terrain == ECoMTerrain::DarkMountains ||
				Tile->Terrain == ECoMTerrain::RootMountains || Tile->Terrain == ECoMTerrain::BasaltMountains)
			{
				return false;
			}

			if (Tile->Terrain == ECoMTerrain::Volcano || Tile->Terrain == ECoMTerrain::UnderwaterVolcano ||
				Tile->Terrain == ECoMTerrain::VolcanicChain)
			{
				return false;
			}

			if (Tile->Terrain == ECoMTerrain::DeepTrench || Tile->Terrain == ECoMTerrain::VoidRifts ||
				Tile->Terrain == ECoMTerrain::DeepChasm || Tile->Terrain == ECoMTerrain::ShadowRift)
			{
				return false;
			}
		}
	}

	return true;
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

		// 3. Production queue processing (buildings and units).
		ProcessCityProduction(*City);

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
// Production Queue (new multi-item system)
// =====================================================================

void UCoMCitySubsystem::AddToQueue(int32 CityId, FName ItemID, bool bIsUnit)
{
	FCoMCityData* City = AllCities.Find(CityId);
	if (!City)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddToQueue: invalid CityId %d"), CityId);
		return;
	}

	FCoMProductionItem Item;
	Item.ItemID = ItemID;
	Item.bIsUnit = bIsUnit;

	if (bIsUnit)
	{
		Item.ProductionCost = LookupUnitCost(ItemID);
	}
	else
	{
		// Don't allow queueing a building the city already has.
		if (CityHasBuilding(*City, ItemID))
		{
			UE_LOG(LogTemp, Warning, TEXT("AddToQueue: city %d already has building %s"),
				CityId, *ItemID.ToString());
			return;
		}
		Item.ProductionCost = LookupBuildingCost(ItemID);
	}

	// Estimate turns remaining.
	const int32 ProdPerTurn = FMath::Max(1, City->ProductionOutput);
	int32 RemainingCost = Item.ProductionCost;

	// Account for production already accumulated and costs of prior queue items.
	for (int32 i = 0; i < City->ProductionQueue.Num(); ++i)
	{
		if (i == 0)
		{
			RemainingCost += (City->ProductionQueue[i].ProductionCost - City->AccumulatedProduction);
		}
		else
		{
			RemainingCost += City->ProductionQueue[i].ProductionCost;
		}
	}
	if (City->ProductionQueue.Num() == 0)
	{
		// This will be the first item — no prior cost to account for.
	}
	Item.TurnsRemaining = FMath::CeilToInt32(static_cast<float>(RemainingCost) / ProdPerTurn);

	City->ProductionQueue.Add(MoveTemp(Item));

	UE_LOG(LogTemp, Log, TEXT("AddToQueue: city %d queued %s '%s' (cost %d)"),
		CityId, bIsUnit ? TEXT("unit") : TEXT("building"), *ItemID.ToString(),
		City->ProductionQueue.Last().ProductionCost);
}

void UCoMCitySubsystem::RemoveFromQueue(int32 CityId, int32 QueueIndex)
{
	FCoMCityData* City = AllCities.Find(CityId);
	if (!City)
	{
		return;
	}

	if (!City->ProductionQueue.IsValidIndex(QueueIndex))
	{
		return;
	}

	City->ProductionQueue.RemoveAt(QueueIndex);

	// If we removed the front item, reset accumulated production.
	if (QueueIndex == 0)
	{
		City->AccumulatedProduction = 0;
	}
}

void UCoMCitySubsystem::MoveInQueue(int32 CityId, int32 FromIndex, int32 ToIndex)
{
	FCoMCityData* City = AllCities.Find(CityId);
	if (!City)
	{
		return;
	}

	if (!City->ProductionQueue.IsValidIndex(FromIndex) ||
		!City->ProductionQueue.IsValidIndex(ToIndex) ||
		FromIndex == ToIndex)
	{
		return;
	}

	FCoMProductionItem MovedItem = City->ProductionQueue[FromIndex];
	City->ProductionQueue.RemoveAt(FromIndex);
	City->ProductionQueue.Insert(MoveTemp(MovedItem), ToIndex);

	// If the front item changed, reset accumulated production.
	if (FromIndex == 0 || ToIndex == 0)
	{
		City->AccumulatedProduction = 0;
	}
}

void UCoMCitySubsystem::ClearQueue(int32 CityId)
{
	FCoMCityData* City = AllCities.Find(CityId);
	if (!City)
	{
		return;
	}

	City->ProductionQueue.Empty();
	City->AccumulatedProduction = 0;
}

TArray<FCoMProductionItem> UCoMCitySubsystem::GetQueue(int32 CityId) const
{
	const FCoMCityData* City = AllCities.Find(CityId);
	if (!City)
	{
		return TArray<FCoMProductionItem>();
	}
	return City->ProductionQueue;
}

// =====================================================================
// Availability Queries
// =====================================================================

TArray<FName> UCoMCitySubsystem::GetAvailableBuildings(int32 CityId) const
{
	TArray<FName> Result;

	const FCoMCityData* City = AllCities.Find(CityId);
	if (!City)
	{
		return Result;
	}

	UAssetManager& AM = UAssetManager::Get();
	const FPrimaryAssetType BuildingType(TEXT("CoMBuilding"));
	TArray<FPrimaryAssetId> AssetList;
	AM.GetPrimaryAssetIdList(BuildingType, AssetList);

	// Collect IDs already in queue (to avoid duplicate queueing).
	TSet<FName> QueuedBuildingIDs;
	for (const FCoMProductionItem& QItem : City->ProductionQueue)
	{
		if (!QItem.bIsUnit)
		{
			QueuedBuildingIDs.Add(QItem.ItemID);
		}
	}

	for (const FPrimaryAssetId& AssetId : AssetList)
	{
		FSoftObjectPath Path = AM.GetPrimaryAssetPath(AssetId);
		const UCoMBuildingDataAsset* Building = Cast<UCoMBuildingDataAsset>(Path.ResolveObject());
		if (!Building)
		{
			continue;
		}

		// Skip if already built.
		if (CityHasBuilding(*City, Building->BuildingID))
		{
			continue;
		}

		// Skip if already in queue.
		if (QueuedBuildingIDs.Contains(Building->BuildingID))
		{
			continue;
		}

		// Check prerequisites: all required buildings must already be built.
		bool bPrereqsMet = true;
		for (const FName& ReqID : Building->RequiredBuildingIDs)
		{
			if (!CityHasBuilding(*City, ReqID))
			{
				bPrereqsMet = false;
				break;
			}
		}

		if (!bPrereqsMet)
		{
			continue;
		}

		Result.Add(Building->BuildingID);
	}

	// Add settler option if population >= 2.
	if (City->Population >= 2)
	{
		Result.Add(FName(TEXT("Settler")));
	}

	return Result;
}

TArray<FName> UCoMCitySubsystem::GetAvailableUnits(int32 CityId) const
{
	TArray<FName> Result;

	const FCoMCityData* City = AllCities.Find(CityId);
	if (!City)
	{
		return Result;
	}

	UAssetManager& AM = UAssetManager::Get();
	TSet<FName> EnabledUnitSpecIDs;

	// 1. Gather units enabled by the city's buildings.
	const FPrimaryAssetType BuildingType(TEXT("CoMBuilding"));
	TArray<FPrimaryAssetId> BuildingAssets;
	AM.GetPrimaryAssetIdList(BuildingType, BuildingAssets);

	for (const FPrimaryAssetId& AssetId : BuildingAssets)
	{
		FSoftObjectPath Path = AM.GetPrimaryAssetPath(AssetId);
		const UCoMBuildingDataAsset* Building = Cast<UCoMBuildingDataAsset>(Path.ResolveObject());
		if (!Building)
		{
			continue;
		}

		// Only consider buildings the city actually has.
		if (!CityHasBuilding(*City, Building->BuildingID))
		{
			continue;
		}

		for (const TSoftObjectPtr<UCoMUnitSpecDataAsset>& UnitRef : Building->EnabledUnits)
		{
			if (const UCoMUnitSpecDataAsset* UnitSpec = UnitRef.Get())
			{
				EnabledUnitSpecIDs.Add(UnitSpec->UnitSpecID);
			}
		}
	}

	// 2. Add racial units if the race data asset can be resolved.
	const FPrimaryAssetType RaceType(TEXT("CoMRace"));
	TArray<FPrimaryAssetId> RaceAssets;
	AM.GetPrimaryAssetIdList(RaceType, RaceAssets);

	for (const FPrimaryAssetId& AssetId : RaceAssets)
	{
		FSoftObjectPath Path = AM.GetPrimaryAssetPath(AssetId);
		const UCoMRaceDataAsset* Race = Cast<UCoMRaceDataAsset>(Path.ResolveObject());
		if (!Race || Race->RaceTag != City->PrimaryRaceTag)
		{
			continue;
		}

		for (const TSoftObjectPtr<UCoMUnitSpecDataAsset>& UnitRef : Race->UniqueUnits)
		{
			if (const UCoMUnitSpecDataAsset* UnitSpec = UnitRef.Get())
			{
				EnabledUnitSpecIDs.Add(UnitSpec->UnitSpecID);
			}
		}
		break; // Found the matching race.
	}

	Result = EnabledUnitSpecIDs.Array();
	return Result;
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

bool UCoMCitySubsystem::IsAquaticRace(FGameplayTag RaceTag)
{
	return RaceTag == CoMTags::Race::Merfolk
		|| RaceTag == CoMTags::Race::DeepOnes
		|| RaceTag == CoMTags::Race::Tideshifters
		|| RaceTag == CoMTags::Race::LavaNaga
		|| RaceTag == CoMTags::Race::EtherSwimmers;
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

	UnrestLevel += City.MinorityRaceTags.Num();

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

	const int32 BuildingCost = 50; // TODO: look up from building data asset

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
// Internal helpers -- production queue processing
// =====================================================================

void UCoMCitySubsystem::ProcessCityProduction(FCoMCityData& City)
{
	// If the queue is empty, fall back to legacy single-item processing.
	if (City.ProductionQueue.Num() == 0)
	{
		ProcessBuilding(City);
		return;
	}

	const int32 ProdPerTurn = FMath::Max(1, City.ProductionOutput);
	City.AccumulatedProduction += ProdPerTurn;

	// Process completions — loop in case production is high enough to finish multiple items.
	while (City.ProductionQueue.Num() > 0)
	{
		FCoMProductionItem& CurrentItem = City.ProductionQueue[0];

		if (City.AccumulatedProduction < CurrentItem.ProductionCost)
		{
			break; // Not enough yet.
		}

		// Item completed — carry over excess production.
		const int32 Overflow = City.AccumulatedProduction - CurrentItem.ProductionCost;

		if (CurrentItem.bIsUnit)
		{
			// Spawn the recruited unit.
			const int32 UnitId = SpawnRecruitedUnit(City, CurrentItem.ItemID);
			if (UnitId != INDEX_NONE)
			{
				OnUnitRecruited.Broadcast(City.CityID, CurrentItem.ItemID, City.OwnerWizardIndex);

				const FText Msg = FText::FromString(FString::Printf(
					TEXT("%s recruited in %s!"),
					*CurrentItem.ItemID.ToString(), *City.CityName.ToString()));
				OnProductionNotification.Broadcast(City.CityID, Msg);
			}
		}
		else
		{
			// Special case: Settler.
			if (CurrentItem.ItemID == FName(TEXT("Settler")))
			{
				ProduceSettler(City.CityID);

				const FText Msg = FText::FromString(FString::Printf(
					TEXT("Settler produced in %s!"), *City.CityName.ToString()));
				OnProductionNotification.Broadcast(City.CityID, Msg);
			}
			else
			{
				// Complete a building.
				const int32 IntID = BuildingNameToID(CurrentItem.ItemID);
				City.BuildingIDs.Add(IntID);

				OnBuildingCompleted.Broadcast(City.CityID, IntID, City.OwnerWizardIndex);

				const FText Msg = FText::FromString(FString::Printf(
					TEXT("%s completed in %s!"),
					*CurrentItem.ItemID.ToString(), *City.CityName.ToString()));
				OnProductionNotification.Broadcast(City.CityID, Msg);
			}
		}

		City.ProductionQueue.RemoveAt(0);
		City.AccumulatedProduction = Overflow;
	}

	// Update TurnsRemaining estimates for remaining items.
	if (ProdPerTurn > 0)
	{
		int32 CumulativeCost = 0;
		for (int32 i = 0; i < City.ProductionQueue.Num(); ++i)
		{
			if (i == 0)
			{
				CumulativeCost = City.ProductionQueue[i].ProductionCost - City.AccumulatedProduction;
			}
			else
			{
				CumulativeCost += City.ProductionQueue[i].ProductionCost;
			}
			City.ProductionQueue[i].TurnsRemaining =
				FMath::CeilToInt32(static_cast<float>(FMath::Max(0, CumulativeCost)) / ProdPerTurn);
		}
	}
}

int32 UCoMCitySubsystem::LookupBuildingCost(FName BuildingID) const
{
	UAssetManager& AM = UAssetManager::Get();
	const FPrimaryAssetType BuildingType(TEXT("CoMBuilding"));
	TArray<FPrimaryAssetId> AssetList;
	AM.GetPrimaryAssetIdList(BuildingType, AssetList);

	for (const FPrimaryAssetId& AssetId : AssetList)
	{
		FSoftObjectPath Path = AM.GetPrimaryAssetPath(AssetId);
		if (const UCoMBuildingDataAsset* Building = Cast<UCoMBuildingDataAsset>(Path.ResolveObject()))
		{
			if (Building->BuildingID == BuildingID)
			{
				return Building->ProductionCost;
			}
		}
	}

	// Settler special case.
	if (BuildingID == FName(TEXT("Settler")))
	{
		return 80;
	}

	UE_LOG(LogTemp, Warning, TEXT("LookupBuildingCost: unknown building '%s', using default 50"), *BuildingID.ToString());
	return 50;
}

int32 UCoMCitySubsystem::LookupUnitCost(FName UnitSpecID) const
{
	UAssetManager& AM = UAssetManager::Get();
	const FPrimaryAssetType UnitSpecType(TEXT("CoMUnitSpec"));
	TArray<FPrimaryAssetId> AssetList;
	AM.GetPrimaryAssetIdList(UnitSpecType, AssetList);

	for (const FPrimaryAssetId& AssetId : AssetList)
	{
		FSoftObjectPath Path = AM.GetPrimaryAssetPath(AssetId);
		if (const UCoMUnitSpecDataAsset* Spec = Cast<UCoMUnitSpecDataAsset>(Path.ResolveObject()))
		{
			if (Spec->UnitSpecID == UnitSpecID)
			{
				return Spec->ProductionCost;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("LookupUnitCost: unknown unit '%s', using default 50"), *UnitSpecID.ToString());
	return 50;
}

bool UCoMCitySubsystem::CityHasBuilding(const FCoMCityData& City, FName BuildingID) const
{
	const int32 IntID = BuildingNameToID(BuildingID);
	return City.BuildingIDs.Contains(IntID);
}

int32 UCoMCitySubsystem::BuildingNameToID(FName BuildingID)
{
	// Use FName's internal index as a stable int32 identifier.
	// This bridges the int32 BuildingIDs array with FName-based data assets.
	return GetTypeHash(BuildingID);
}

int32 UCoMCitySubsystem::SpawnRecruitedUnit(const FCoMCityData& City, FName UnitSpecID)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return INDEX_NONE;
	}

	UCoMUnitSubsystem* UnitSub = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (!UnitSub)
	{
		return INDEX_NONE;
	}

	// Resolve the SpecID int32 from the FName.
	UAssetManager& AM = UAssetManager::Get();
	const FPrimaryAssetType UnitSpecType(TEXT("CoMUnitSpec"));
	TArray<FPrimaryAssetId> AssetList;
	AM.GetPrimaryAssetIdList(UnitSpecType, AssetList);

	int32 ResolvedSpecID = 0;
	for (const FPrimaryAssetId& AssetId : AssetList)
	{
		FSoftObjectPath Path = AM.GetPrimaryAssetPath(AssetId);
		if (const UCoMUnitSpecDataAsset* Spec = Cast<UCoMUnitSpecDataAsset>(Path.ResolveObject()))
		{
			if (Spec->UnitSpecID == UnitSpecID)
			{
				// UCoMUnitSubsystem::SpawnUnit takes an int32 SpecID.
				// Use the type hash as a mapping, matching the resolver pattern.
				ResolvedSpecID = GetTypeHash(UnitSpecID);
				break;
			}
		}
	}

	const int32 UnitId = UnitSub->SpawnUnit(
		ResolvedSpecID, City.Plane, City.Layer, City.Position, City.OwnerWizardIndex);

	if (UnitId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnRecruitedUnit: SpawnUnit failed for '%s' in city %d"),
			*UnitSpecID.ToString(), City.CityID);
		return INDEX_NONE;
	}

	// Add to garrison army if one exists, otherwise create one.
	if (City.GarrisonArmyID >= 0)
	{
		UnitSub->AddUnitToArmy(UnitId, City.GarrisonArmyID);
	}
	else
	{
		const int32 ArmyId = UnitSub->CreateArmy(
			City.OwnerWizardIndex, City.Plane, City.Layer, City.Position);
		UnitSub->AddUnitToArmy(UnitId, ArmyId);
	}

	return UnitId;
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
	City.ProductionQueue.Empty();
	City.AccumulatedProduction = 0;
	City.GarrisonArmyID = -1;

	OnCityRebelled.Broadcast(City.CityID, FormerOwner);
}

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load Export/Import
// ─────────────────────────────────────────────────────────────────────────────

void UCoMCitySubsystem::ExportAll(TArray<FCoMCityData>& OutCities, int32& OutNextCityID) const
{
	OutCities.Empty();
	OutCities.Reserve(AllCities.Num());
	for (const auto& Pair : AllCities)
	{
		OutCities.Add(Pair.Value);
	}
	OutNextCityID = NextCityID;
}

void UCoMCitySubsystem::ImportAll(const TArray<FCoMCityData>& InCities, int32 InNextCityID)
{
	AllCities.Empty();
	for (const FCoMCityData& City : InCities)
	{
		AllCities.Add(City.CityID, City);
	}
	NextCityID = InNextCityID;
	UE_LOG(LogTemp, Log, TEXT("[CitySubsystem] ImportAll: %d cities imported."), AllCities.Num());
}
