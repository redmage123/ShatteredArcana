// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMUnitSubsystem.h"
#include "CoMCore/World/CoMPathfinder.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/World/CoMWeatherSubsystem.h"
#include "CoMCore/Data/CoMUnitSpecDataAsset.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "Engine/AssetManager.h"

static constexpr int32 MAP_WIDTH  = CoM::MAP_WIDTH;
static constexpr int32 MAP_HEIGHT = CoM::MAP_HEIGHT;

void UCoMUnitSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	WorldMapSubsystem = GI->GetSubsystem<UCoMWorldMapSubsystem>();
	WeatherSubsystem  = GI->GetSubsystem<UCoMWeatherSubsystem>();
}

void UCoMUnitSubsystem::Deinitialize()
{
	AllUnits.Empty();
	AllArmies.Empty();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Unit Lifecycle
// ---------------------------------------------------------------------------

int32 UCoMUnitSubsystem::SpawnUnit(int32 SpecID, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position, int32 OwnerWizard)
{
	const UCoMUnitSpecDataAsset* Spec = ResolveSpec(SpecID);
	if (!Spec)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCoMUnitSubsystem::SpawnUnit - unknown SpecID %d"), SpecID);
		return INDEX_NONE;
	}

	const int32 UnitID = NextUnitID++;

	FCoMUnitInstance& Unit = AllUnits.Add(UnitID);
	Unit.UnitID           = UnitID;
	Unit.SpecID           = FName(*FString::FromInt(SpecID));
	Unit.OwnerWizardIndex = OwnerWizard;
	Unit.CurrentHP        = Spec->HitPoints;
	Unit.Experience       = 0;
	Unit.Level            = 1;
	Unit.bFlying          = Spec->bFlying;

	return UnitID;
}

void UCoMUnitSubsystem::DespawnUnit(int32 UnitID)
{
	if (!AllUnits.Contains(UnitID))
	{
		return;
	}

	// Remove from any army that references this unit.
	for (auto& Pair : AllArmies)
	{
		FCoMArmyGroup& Army = Pair.Value;
		if (Army.UnitIDs.Contains(UnitID))
		{
			Internal_RemoveUnitFromArmy(UnitID, Army);
			break;
		}
	}

	AllUnits.Remove(UnitID);
}

// ---------------------------------------------------------------------------
// Army Lifecycle
// ---------------------------------------------------------------------------

int32 UCoMUnitSubsystem::CreateArmy(int32 OwnerWizard, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position)
{
	const int32 ArmyID = NextArmyID++;

	FCoMArmyGroup& Army = AllArmies.Add(ArmyID);
	Army.ArmyGroupID      = ArmyID;
	Army.OwnerWizardIndex  = OwnerWizard;
	Army.Plane             = Plane;
	Army.Layer             = Layer;
	Army.Position          = FIntPoint(WrapX(Position.X), FMath::Clamp(Position.Y, 0, MAP_HEIGHT - 1));
	Army.MovementRemaining = 0;

	return ArmyID;
}

bool UCoMUnitSubsystem::AddUnitToArmy(int32 UnitID, int32 ArmyID)
{
	FCoMArmyGroup* Army = AllArmies.Find(ArmyID);
	if (!Army)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddUnitToArmy - invalid ArmyID %d"), ArmyID);
		return false;
	}

	if (!AllUnits.Contains(UnitID))
	{
		UE_LOG(LogTemp, Warning, TEXT("AddUnitToArmy - invalid UnitID %d"), UnitID);
		return false;
	}

	if (Army->UnitIDs.Num() >= CoM::MAX_ARMY_SIZE)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddUnitToArmy - army %d already at max size"), ArmyID);
		return false;
	}

	if (Army->UnitIDs.Contains(UnitID))
	{
		return true; // Already present.
	}

	Army->UnitIDs.Add(UnitID);
	return true;
}

bool UCoMUnitSubsystem::RemoveUnitFromArmy(int32 UnitID, int32 ArmyID)
{
	FCoMArmyGroup* Army = AllArmies.Find(ArmyID);
	if (!Army)
	{
		return false;
	}

	if (!Army->UnitIDs.Contains(UnitID))
	{
		return false;
	}

	Internal_RemoveUnitFromArmy(UnitID, *Army);
	return true;
}

bool UCoMUnitSubsystem::MergeArmies(int32 SourceArmyID, int32 DestArmyID)
{
	FCoMArmyGroup* Source = AllArmies.Find(SourceArmyID);
	FCoMArmyGroup* Dest   = AllArmies.Find(DestArmyID);

	if (!Source || !Dest)
	{
		UE_LOG(LogTemp, Warning, TEXT("MergeArmies - invalid army ID(s)"));
		return false;
	}

	if (Source->OwnerWizardIndex != Dest->OwnerWizardIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("MergeArmies - armies belong to different wizards"));
		return false;
	}

	const int32 CombinedSize = Source->UnitIDs.Num() + Dest->UnitIDs.Num();
	if (CombinedSize > CoM::MAX_ARMY_SIZE)
	{
		UE_LOG(LogTemp, Warning, TEXT("MergeArmies - combined size %d exceeds MAX_ARMY_SIZE"), CombinedSize);
		return false;
	}

	for (int32 UID : Source->UnitIDs)
	{
		Dest->UnitIDs.Add(UID);
	}

	AllArmies.Remove(SourceArmyID);
	return true;
}

int32 UCoMUnitSubsystem::SplitArmy(int32 ArmyID, const TArray<int32>& UnitIDsToSplit)
{
	FCoMArmyGroup* Army = AllArmies.Find(ArmyID);
	if (!Army)
	{
		return INDEX_NONE;
	}

	if (UnitIDsToSplit.Num() == 0 || UnitIDsToSplit.Num() >= Army->UnitIDs.Num())
	{
		return INDEX_NONE;
	}

	// Validate all requested units belong to the army.
	for (int32 UID : UnitIDsToSplit)
	{
		if (!Army->UnitIDs.Contains(UID))
		{
			UE_LOG(LogTemp, Warning, TEXT("SplitArmy - unit %d not in army %d"), UID, ArmyID);
			return INDEX_NONE;
		}
	}

	const int32 NewArmyID = CreateArmy(Army->OwnerWizardIndex, Army->Plane, Army->Layer, Army->Position);

	// Re-fetch pointers; CreateArmy may have rehashed the map.
	Army = AllArmies.Find(ArmyID);
	FCoMArmyGroup* NewArmy = AllArmies.Find(NewArmyID);

	for (int32 UID : UnitIDsToSplit)
	{
		Army->UnitIDs.Remove(UID);
		NewArmy->UnitIDs.Add(UID);
	}

	return NewArmyID;
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------

void UCoMUnitSubsystem::MoveArmy(int32 ArmyID, FIntPoint Destination)
{
	FCoMArmyGroup* Army = AllArmies.Find(ArmyID);
	if (!Army || Army->UnitIDs.Num() == 0)
	{
		return;
	}

	Destination.X = WrapX(Destination.X);
	Destination.Y = FMath::Clamp(Destination.Y, 0, MAP_HEIGHT - 1);

	if (!Pathfinder || !WorldMapSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("MoveArmy - Pathfinder or WorldMapSubsystem is null"));
		return;
	}

	// Build a path request for the pathfinder.
	FCoMPathRequest Request;
	Request.StartPlane = Army->Plane;
	Request.StartLayer = Army->Layer;
	Request.StartPos   = Army->Position;
	Request.GoalPlane  = Army->Plane;
	Request.GoalLayer  = Army->Layer;
	Request.GoalPos    = Destination;
	Request.WizardIndex    = Army->OwnerWizardIndex;
	Request.bAllowPortals  = false;
	Request.bAllowUnexplored = false;

	FCoMPathResult Result = Pathfinder->FindPath(WorldMapSubsystem.Get(), nullptr, Request);
	if (!Result.bFound || Result.Segments.Num() == 0)
	{
		return;
	}

	FFixed64 MovementBudget = ComputeArmyMovementSpeed(*Army);

	// Walk along path segments/tiles consuming movement budget.
	for (const FCoMPathSegment& Segment : Result.Segments)
	{
		for (const FIntPoint& Tile : Segment.Tiles)
		{
			FFixed64 TileCost = ComputeMoveCost(*Army, Army->Plane, Army->Layer, Tile);

			if (TileCost > MovementBudget)
			{
				return; // Out of movement.
			}

			MovementBudget = MovementBudget - TileCost;
			Army->Position = Tile;
		}
	}
}

void UCoMUnitSubsystem::ProcessMovementTurn()
{
	// Resolve encounters: detect armies of different wizards on the same tile.
	TMap<uint64, TArray<int32>> TileArmies;

	for (auto& Pair : AllArmies)
	{
		const FCoMArmyGroup& Army = Pair.Value;
		const uint64 Key =
			(static_cast<uint64>(Army.Plane) << 34) |
			(static_cast<uint64>(Army.Layer) << 32) |
			(static_cast<uint64>(static_cast<uint32>(Army.Position.X)) << 16) |
			static_cast<uint64>(static_cast<uint32>(Army.Position.Y));

		TileArmies.FindOrAdd(Key).Add(Army.ArmyGroupID);
	}

	for (auto& Pair : TileArmies)
	{
		const TArray<int32>& ArmyIDs = Pair.Value;
		if (ArmyIDs.Num() < 2)
		{
			continue;
		}

		TSet<int32> Wizards;
		for (int32 AID : ArmyIDs)
		{
			if (const FCoMArmyGroup* A = AllArmies.Find(AID))
			{
				Wizards.Add(A->OwnerWizardIndex);
			}
		}

		if (Wizards.Num() > 1)
		{
			// TODO: trigger combat encounter via combat subsystem.
			UE_LOG(LogTemp, Log, TEXT("ProcessMovementTurn - encounter detected, %d armies from %d wizards"),
				ArmyIDs.Num(), Wizards.Num());
		}
	}
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const FCoMUnitInstance* UCoMUnitSubsystem::GetUnit(int32 UnitID) const
{
	return AllUnits.Find(UnitID);
}

const FCoMArmyGroup* UCoMUnitSubsystem::GetArmy(int32 ArmyID) const
{
	return AllArmies.Find(ArmyID);
}

TArray<const FCoMArmyGroup*> UCoMUnitSubsystem::GetArmiesAtPosition(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position) const
{
	TArray<const FCoMArmyGroup*> Result;
	const int32 WrappedX = WrapX(Position.X);

	for (const auto& Pair : AllArmies)
	{
		const FCoMArmyGroup& Army = Pair.Value;
		if (Army.Plane == Plane && Army.Layer == Layer &&
			Army.Position.X == WrappedX && Army.Position.Y == Position.Y)
		{
			Result.Add(&Army);
		}
	}

	return Result;
}

TArray<const FCoMArmyGroup*> UCoMUnitSubsystem::GetArmiesForWizard(int32 WizardIndex) const
{
	TArray<const FCoMArmyGroup*> Result;

	for (const auto& Pair : AllArmies)
	{
		if (Pair.Value.OwnerWizardIndex == WizardIndex)
		{
			Result.Add(&Pair.Value);
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------
// Internals
// ---------------------------------------------------------------------------

const UCoMUnitSpecDataAsset* UCoMUnitSubsystem::ResolveSpec(int32 SpecID) const
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
			// Compare by converting the FName-based UnitSpecID to int.
			const FString SpecStr = Spec->UnitSpecID.ToString();
			if (FCString::Atoi(*SpecStr) == SpecID)
			{
				return Spec;
			}
		}
	}

	return nullptr;
}

FFixed64 UCoMUnitSubsystem::ComputeArmyMovementSpeed(const FCoMArmyGroup& Army) const
{
	FFixed64 Slowest = FFixed64(999);

	for (int32 UID : Army.UnitIDs)
	{
		const FCoMUnitInstance* Unit = AllUnits.Find(UID);
		if (!Unit)
		{
			continue;
		}

		// Parse SpecID back to int for asset lookup.
		const int32 ParsedSpecID = FCString::Atoi(*Unit->SpecID.ToString());
		const UCoMUnitSpecDataAsset* Spec = ResolveSpec(ParsedSpecID);
		if (!Spec)
		{
			continue;
		}

		const FFixed64 UnitMove = FFixed64(Spec->MovePoints);
		if (UnitMove < Slowest)
		{
			Slowest = UnitMove;
		}
	}

	return (Slowest == FFixed64(999)) ? FFixed64(0) : Slowest;
}

FFixed64 UCoMUnitSubsystem::ComputeMoveCost(const FCoMArmyGroup& Army, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Tile) const
{
	FFixed64 BaseCost = FFixed64(1);

	// Query terrain cost from world map tile data.
	if (WorldMapSubsystem)
	{
		const FCoMTileData* TileData = WorldMapSubsystem->GetTileAtPos(Plane, Layer, Tile);
		if (TileData)
		{
			BaseCost = TileData->MoveCostModifier;
		}
	}

	// Flying armies pay half terrain cost.
	if (IsArmyFullyFlying(Army))
	{
		BaseCost = BaseCost * FFixed64::Half();
	}

	// Ley line tiles give 0.5x cost.
	if (WorldMapSubsystem)
	{
		const FCoMTileData* TileData = WorldMapSubsystem->GetTileAtPos(Plane, Layer, Tile);
		if (TileData && TileData->LeyLineIDs.Num() > 0)
		{
			BaseCost = BaseCost * FFixed64::Half();
		}
	}

	// Weather modifier: look up weather effects and apply movement penalty.
	if (WeatherSubsystem)
	{
		const ECoMWeatherType Weather = WeatherSubsystem->GetWeatherAtTile(Plane, Tile.X, Tile.Y);
		const FFixed64 Intensity = WeatherSubsystem->GetWeatherIntensity(Plane, Tile.X, Tile.Y);
		const FCoMWeatherEffects Effects = UCoMWeatherSubsystem::GetWeatherEffects(Weather, Intensity);

		if (Effects.MovementPenalty > 0)
		{
			// Add movement penalty as fraction of base cost.
			BaseCost = BaseCost + FFixed64(Effects.MovementPenalty) * FFixed64::Tenth();
		}
	}

	// Minimum cost to prevent zero-cost movement.
	const FFixed64 MinCost = FFixed64::Quarter();
	if (BaseCost < MinCost)
	{
		BaseCost = MinCost;
	}

	return BaseCost;
}

bool UCoMUnitSubsystem::IsArmyFullyFlying(const FCoMArmyGroup& Army) const
{
	if (Army.UnitIDs.Num() == 0)
	{
		return false;
	}

	for (int32 UID : Army.UnitIDs)
	{
		const FCoMUnitInstance* Unit = AllUnits.Find(UID);
		if (!Unit || !Unit->bFlying)
		{
			return false;
		}
	}

	return true;
}

bool UCoMUnitSubsystem::ArmyHasBurrower(const FCoMArmyGroup& Army) const
{
	for (int32 UID : Army.UnitIDs)
	{
		const FCoMUnitInstance* Unit = AllUnits.Find(UID);
		if (Unit && Unit->MovementType == ECoMMovementType::Burrowing)
		{
			return true;
		}
	}

	return false;
}

void UCoMUnitSubsystem::Internal_RemoveUnitFromArmy(int32 UnitID, FCoMArmyGroup& Army)
{
	Army.UnitIDs.Remove(UnitID);
}

int32 UCoMUnitSubsystem::WrapX(int32 X)
{
	return ((X % MAP_WIDTH) + MAP_WIDTH) % MAP_WIDTH;
}
