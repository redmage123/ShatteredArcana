#include "CoMUnitSubsystem.h"
#include "CoMUnitSubsystem.h"
#include "CoMPathfinder.h"
#include "CoMWorldMapSubsystem.h"
#include "CoMWeatherSubsystem.h"
#include "CoMUnitSpecDataAsset.h"
#include "CoMConstants.h"
#include "Engine/AssetManager.h"

static constexpr int32 MAP_WIDTH = 160;
static constexpr int32 MAP_HEIGHT = 100;

void UCoMUnitSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	WorldMapSubsystem = GI->GetSubsystem<UCoMWorldMapSubsystem>();
	WeatherSubsystem = GI->GetSubsystem<UCoMWeatherSubsystem>();

	// Pathfinder is expected to be created and registered externally or as a subsystem dependency.
	// Acquire reference when first needed if not yet set.
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
		UE_LOG(LogTemp, Warning, TEXT("UCoMUnitSubsystem::SpawnUnit — unknown SpecID %d"), SpecID);
		return INDEX_NONE;
	}

	const int32 UnitID = NextUnitID++;

	FCoMUnitInstance& Unit = AllUnits.Add(UnitID);
	Unit.UnitID = UnitID;
	Unit.SpecID = SpecID;
	Unit.MaxHP = Spec->HPBase;
	Unit.CurrentHP = Unit.MaxHP;
	Unit.Experience = FFixed64(0);
	Unit.Level = 1;
	Unit.Position = FIntPoint(WrapX(Position.X), FMath::Clamp(Position.Y, 0, MAP_HEIGHT - 1));

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
	Army.ArmyID = ArmyID;
	Army.OwnerWizardIndex = OwnerWizard;
	Army.Plane = Plane;
	Army.Layer = Layer;
	Army.Position = FIntPoint(WrapX(Position.X), FMath::Clamp(Position.Y, 0, MAP_HEIGHT - 1));
	Army.HeroUnitID = INDEX_NONE;

	return ArmyID;
}

bool UCoMUnitSubsystem::AddUnitToArmy(int32 UnitID, int32 ArmyID)
{
	FCoMArmyGroup* Army = AllArmies.Find(ArmyID);
	if (!Army)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddUnitToArmy — invalid ArmyID %d"), ArmyID);
		return false;
	}

	if (!AllUnits.Contains(UnitID))
	{
		UE_LOG(LogTemp, Warning, TEXT("AddUnitToArmy — invalid UnitID %d"), UnitID);
		return false;
	}

	if (Army->UnitIDs.Num() >= CoM::MAX_ARMY_SIZE)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddUnitToArmy — army %d already at max size"), ArmyID);
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
	FCoMArmyGroup* Dest = AllArmies.Find(DestArmyID);

	if (!Source || !Dest)
	{
		UE_LOG(LogTemp, Warning, TEXT("MergeArmies — invalid army ID(s)"));
		return false;
	}

	if (Source->OwnerWizardIndex != Dest->OwnerWizardIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("MergeArmies — armies belong to different wizards"));
		return false;
	}

	const int32 CombinedSize = Source->UnitIDs.Num() + Dest->UnitIDs.Num();
	if (CombinedSize > CoM::MAX_ARMY_SIZE)
	{
		UE_LOG(LogTemp, Warning, TEXT("MergeArmies — combined size %d exceeds MAX_ARMY_SIZE"), CombinedSize);
		return false;
	}

	for (int32 UID : Source->UnitIDs)
	{
		Dest->UnitIDs.Add(UID);
	}

	// Preserve hero from source if dest has none.
	if (Dest->HeroUnitID == INDEX_NONE && Source->HeroUnitID != INDEX_NONE)
	{
		Dest->HeroUnitID = Source->HeroUnitID;
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
		// Cannot split zero units or split all units (would leave original empty).
		return INDEX_NONE;
	}

	// Validate all requested units belong to the army.
	for (int32 UID : UnitIDsToSplit)
	{
		if (!Army->UnitIDs.Contains(UID))
		{
			UE_LOG(LogTemp, Warning, TEXT("SplitArmy — unit %d not in army %d"), UID, ArmyID);
			return INDEX_NONE;
		}
	}

	const int32 NewArmyID = CreateArmy(Army->OwnerWizardIndex, Army->Plane, Army->Layer, Army->Position);

	// Re-fetch pointer; CreateArmy may have rehashed the map.
	Army = AllArmies.Find(ArmyID);
	FCoMArmyGroup* NewArmy = AllArmies.Find(NewArmyID);

	for (int32 UID : UnitIDsToSplit)
	{
		Army->UnitIDs.Remove(UID);
		NewArmy->UnitIDs.Add(UID);

		// Transfer hero designation if the hero moves to the new army.
		if (Army->HeroUnitID == UID)
		{
			NewArmy->HeroUnitID = UID;
			Army->HeroUnitID = INDEX_NONE;
		}
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

	if (!Pathfinder)
	{
		UE_LOG(LogTemp, Error, TEXT("MoveArmy — Pathfinder is null"));
		return;
	}

	// Ask pathfinder for the tile sequence.
	TArray<FIntPoint> Path;
	const bool bFound = Pathfinder->FindPath(Army->Position, Destination, Army->Plane, Army->Layer, Path);
	if (!bFound || Path.Num() == 0)
	{
		return;
	}

	FFixed64 MovementBudget = ComputeArmyMovementSpeed(*Army);

	for (const FIntPoint& Tile : Path)
	{
		FFixed64 TileCost = ComputeMoveCost(*Army, Army->Plane, Army->Layer, Tile);

		if (TileCost > MovementBudget)
		{
			break;
		}

		MovementBudget = MovementBudget - TileCost;
		Army->Position = Tile;

		// Sync individual unit positions.
		for (int32 UID : Army->UnitIDs)
		{
			if (FCoMUnitInstance* Unit = AllUnits.Find(UID))
			{
				Unit->Position = Tile;
			}
		}
	}
}

void UCoMUnitSubsystem::ProcessMovementTurn()
{
	// Restore movement budgets (no persistent budget field yet — movement speed is recomputed
	// each move call). This hook is the place for future per-army budget tracking.

	// Resolve encounters: detect armies of different wizards on the same tile.
	TMap<uint64, TArray<int32>> TileArmies; // key = packed (Plane, Layer, X, Y)

	for (auto& Pair : AllArmies)
	{
		const FCoMArmyGroup& Army = Pair.Value;
		// Pack: Plane(3 bits) | Layer(2 bits) | X(16 bits) | Y(16 bits) into 64 bits.
		const uint64 Key =
			(static_cast<uint64>(Army.Plane) << 34) |
			(static_cast<uint64>(Army.Layer) << 32) |
			(static_cast<uint64>(static_cast<uint32>(Army.Position.X)) << 16) |
			static_cast<uint64>(static_cast<uint32>(Army.Position.Y));

		TileArmies.FindOrAdd(Key).Add(Army.ArmyID);
	}

	for (auto& Pair : TileArmies)
	{
		const TArray<int32>& ArmyIDs = Pair.Value;
		if (ArmyIDs.Num() < 2)
		{
			continue;
		}

		// Check for opposing wizards.
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
			UE_LOG(LogTemp, Log, TEXT("ProcessMovementTurn — encounter detected at tile, %d armies from %d wizards"), ArmyIDs.Num(), Wizards.Num());
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
	// Look up from the asset manager by SpecID.
	UAssetManager& AM = UAssetManager::Get();
	const FPrimaryAssetType UnitSpecType(TEXT("CoMUnitSpec"));
	TArray<FPrimaryAssetId> AssetList;
	AM.GetPrimaryAssetIdList(UnitSpecType, AssetList);

	for (const FPrimaryAssetId& AssetId : AssetList)
	{
		FSoftObjectPath Path = AM.GetPrimaryAssetPath(AssetId);
		if (const UCoMUnitSpecDataAsset* Spec = Cast<UCoMUnitSpecDataAsset>(Path.ResolveObject()))
		{
			if (Spec->SpecID == SpecID)
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

		const UCoMUnitSpecDataAsset* Spec = ResolveSpec(Unit->SpecID);
		if (!Spec)
		{
			continue;
		}

		if (Spec->Movement < Slowest)
		{
			Slowest = Spec->Movement;
		}
	}

	return (Slowest == FFixed64(999)) ? FFixed64(0) : Slowest;
}

FFixed64 UCoMUnitSubsystem::ComputeMoveCost(const FCoMArmyGroup& Army, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Tile) const
{
	FFixed64 BaseCost = FFixed64(1);

	// Query terrain cost from world map.
	if (WorldMapSubsystem)
	{
		BaseCost = WorldMapSubsystem->GetTerrainMovementCost(Plane, Layer, Tile);
	}

	// Flying armies pay half terrain cost.
	if (IsArmyFullyFlying(Army))
	{
		BaseCost = BaseCost * FFixed64(0.5);
	}

	// Ley line tiles give 0.5x cost.
	if (WorldMapSubsystem && WorldMapSubsystem->IsLeyLineTile(Plane, Layer, Tile))
	{
		BaseCost = BaseCost * FFixed64(0.5);
	}

	// Weather modifier.
	if (WeatherSubsystem)
	{
		FFixed64 WeatherMod = WeatherSubsystem->GetMovementModifier(Plane, Tile);
		BaseCost = BaseCost * WeatherMod;
	}

	// Minimum cost of a small positive value to prevent zero-cost movement.
	const FFixed64 MinCost = FFixed64(0.25);
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
		if (!Unit)
		{
			return false;
		}

		const UCoMUnitSpecDataAsset* Spec = ResolveSpec(Unit->SpecID);
		if (!Spec || !Spec->bFlying)
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
		if (!Unit)
		{
			continue;
		}

		const UCoMUnitSpecDataAsset* Spec = ResolveSpec(Unit->SpecID);
		if (Spec && Spec->bBurrowing)
		{
			return true;
		}
	}

	return false;
}

void UCoMUnitSubsystem::Internal_RemoveUnitFromArmy(int32 UnitID, FCoMArmyGroup& Army)
{
	Army.UnitIDs.Remove(UnitID);

	if (Army.HeroUnitID == UnitID)
	{
		Army.HeroUnitID = INDEX_NONE;
	}
}

int32 UCoMUnitSubsystem::WrapX(int32 X)
{
	return ((X % MAP_WIDTH) + MAP_WIDTH) % MAP_WIDTH;
}
```