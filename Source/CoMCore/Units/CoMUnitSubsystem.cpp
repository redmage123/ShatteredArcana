// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMUnitSubsystem.h"
#include "CoMCore/World/CoMPathfinder.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/World/CoMLeyPortalSubsystem.h"
#include "CoMCore/World/CoMSiteEncounterSubsystem.h"
#include "CoMCore/World/CoMWeatherSubsystem.h"
#include "CoMCore/Data/CoMUnitSpecDataAsset.h"
#include "CoMCore/Data/CoMUnitDatabase.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Audio/CoMAudioSubsystem.h"
#include "CoMCore/World/CoMFogOfWarSubsystem.h"
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
	Pathfinder = NewObject<UCoMPathfinder>(this);
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

int32 UCoMUnitSubsystem::SpawnUnitByName(FName SpecID, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position, int32 OwnerWizard)
{
	if (!CoMUnitDatabase::Contains(SpecID))
	{
		UE_LOG(LogTemp, Warning, TEXT("UCoMUnitSubsystem::SpawnUnitByName - unknown SpecID %s"),
			*SpecID.ToString());
		return INDEX_NONE;
	}

	const int32 UnitID = NextUnitID++;
	FCoMUnitInstance& Unit = AllUnits.Add(UnitID);
	Unit.UnitID           = UnitID;
	Unit.SpecID           = SpecID;
	Unit.OwnerWizardIndex = OwnerWizard;
	Unit.Experience       = 0;
	Unit.Level            = 1;

	const FCoMUnitSpecInfo& DBSpec = CoMUnitDatabase::GetUnitSpec(SpecID);
	Unit.CurrentHP = DBSpec.HitPoints;
	Unit.MaxHP     = DBSpec.HitPoints;
	Unit.bFlying   = DBSpec.bFlying;
	Unit.bIsHero   = DBSpec.bHero;
	Unit.bIsSettler= DBSpec.bSettler;

	return UnitID;
}

int32 UCoMUnitSubsystem::SpawnUnit(int32 SpecID, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position, int32 OwnerWizard)
{
	const FName SpecFName = FName(*FString::FromInt(SpecID));

	// Try static C++ database first
	bool bFromDatabase = CoMUnitDatabase::Contains(SpecFName);

	// Then try editor-authored data asset
	const UCoMUnitSpecDataAsset* Spec = bFromDatabase ? nullptr : ResolveSpec(SpecID);

	if (!bFromDatabase && !Spec)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCoMUnitSubsystem::SpawnUnit - unknown SpecID %d"), SpecID);
		return INDEX_NONE;
	}

	const int32 UnitID = NextUnitID++;

	FCoMUnitInstance& Unit = AllUnits.Add(UnitID);
	Unit.UnitID           = UnitID;
	Unit.SpecID           = SpecFName;
	Unit.OwnerWizardIndex = OwnerWizard;
	Unit.Experience       = 0;
	Unit.Level            = 1;

	if (bFromDatabase)
	{
		const FCoMUnitSpecInfo& DBSpec = CoMUnitDatabase::GetUnitSpec(SpecFName);
		Unit.CurrentHP = DBSpec.HitPoints;
		Unit.MaxHP     = DBSpec.HitPoints;
		Unit.bFlying   = DBSpec.bFlying;
		Unit.bIsHero   = DBSpec.bHero;
		Unit.bIsSettler = DBSpec.bSettler;
	}
	else
	{
		Unit.CurrentHP = Spec->HitPoints;
		Unit.MaxHP     = Spec->HitPoints;
		Unit.bFlying   = Spec->bFlying;
		Unit.bIsHero   = Spec->bHero;
	}

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
// Settlement
// ---------------------------------------------------------------------------

int32 UCoMUnitSubsystem::FoundCityWithSettler(int32 ArmyId, int32 SettlerUnitId)
{
	// 1. Find the army.
	FCoMArmyGroup* Army = AllArmies.Find(ArmyId);
	if (!Army)
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCityWithSettler: invalid ArmyId %d"), ArmyId);
		return -1;
	}

	// 2. Verify the settler unit is in this army.
	if (!Army->UnitIDs.Contains(SettlerUnitId))
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCityWithSettler: unit %d not in army %d"), SettlerUnitId, ArmyId);
		return -1;
	}

	// 3. Validate bIsSettler.
	FCoMUnitInstance* Settler = AllUnits.Find(SettlerUnitId);
	if (!Settler || !Settler->bIsSettler)
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCityWithSettler: unit %d is not a settler"), SettlerUnitId);
		return -1;
	}

	// 4. Get position, plane, layer from the army.
	const ECoMPlane Plane = Army->Plane;
	const ECoMMapLayer Layer = Army->Layer;
	const FIntPoint Position = Army->Position;
	const int32 OwnerWizard = Army->OwnerWizardIndex;
	const FGameplayTag RaceTag = Settler->RaceTag;

	// 5. Get city subsystem.
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCityWithSettler: no GameInstance"));
		return -1;
	}

	UCoMCitySubsystem* CitySub = GI->GetSubsystem<UCoMCitySubsystem>();
	if (!CitySub)
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCityWithSettler: no CitySubsystem"));
		return -1;
	}

	// 6. Check if we can found a city here.
	if (!CitySub->CanFoundCityAt(Plane, Layer, Position, RaceTag))
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCityWithSettler: cannot found city at (%d,%d)"),
			Position.X, Position.Y);
		return -1;
	}

	// 7. Generate a city name and found the city.
	const FText CityName = FText::FromString(
		FString::Printf(TEXT("City_%d_%d"), Position.X, Position.Y));

	const int32 NewCityId = CitySub->FoundCity(OwnerWizard, Plane, Layer, Position, RaceTag, CityName);
	if (NewCityId < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FoundCityWithSettler: FoundCity failed"));
		return -1;
	}

	// 8. Remove the settler from the army.
	const bool bSettlerWasOnlyUnit = (Army->UnitIDs.Num() == 1);
	Internal_RemoveUnitFromArmy(SettlerUnitId, *Army);
	AllUnits.Remove(SettlerUnitId);

	// If the settler was the only unit, disband the army.
	if (bSettlerWasOnlyUnit)
	{
		AllArmies.Remove(ArmyId);
	}

	// 9. Play audio notification if available.
	UCoMAudioSubsystem* AudioSub = GI->GetSubsystem<UCoMAudioSubsystem>();
	if (AudioSub)
	{
		AudioSub->PlaySFX(FName(TEXT("CityFounded")),
			FVector(static_cast<float>(Position.X), static_cast<float>(Position.Y), 0.f));
	}

	UE_LOG(LogTemp, Log, TEXT("FoundCityWithSettler: wizard %d founded city %d at (%d,%d) on plane %d"),
		OwnerWizard, NewCityId, Position.X, Position.Y, static_cast<int32>(Plane));

	return NewCityId;
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------

void UCoMUnitSubsystem::MoveArmy(int32 ArmyID, FIntPoint Destination, bool bAllowUnexplored)
{
	FCoMArmyGroup* Army = AllArmies.Find(ArmyID);
	if (!Army || Army->UnitIDs.Num() == 0)
	{
		return;
	}

	Destination.X = WrapX(Destination.X);
	Destination.Y = FMath::Clamp(Destination.Y, 0, MAP_HEIGHT - 1);

	// Lazy-resolve cached subsystems — Initialize() may have run before
	// WorldMapSubsystem was ready, leaving the cache null forever.
	if (!WorldMapSubsystem)
	{
		if (UGameInstance* GIL = GetGameInstance())
		{
			WorldMapSubsystem = GIL->GetSubsystem<UCoMWorldMapSubsystem>();
		}
	}
	if (!Pathfinder)
	{
		Pathfinder = NewObject<UCoMPathfinder>(this);
	}

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
	Request.bAllowUnexplored = bAllowUnexplored;

	// FindPath bails immediately on a null LeyPortal subsystem (even when portals
	// aren't used), so we MUST pass it — passing nullptr meant every path request
	// failed and no army ever moved.
	UCoMLeyPortalSubsystem* LeyPortals = nullptr;
	if (UGameInstance* GIL = GetGameInstance())
	{
		LeyPortals = GIL->GetSubsystem<UCoMLeyPortalSubsystem>();
	}

	FCoMPathResult Result = Pathfinder->FindPath(WorldMapSubsystem.Get(), LeyPortals, Request);
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

void UCoMUnitSubsystem::SetAutoExplore(int32 ArmyId, bool bEnable)
{
	if (bEnable)
	{
		AutoExploreArmies.Add(ArmyId);
	}
	else
	{
		AutoExploreArmies.Remove(ArmyId);
	}
}

bool UCoMUnitSubsystem::IsAutoExploring(int32 ArmyId) const
{
	return AutoExploreArmies.Contains(ArmyId);
}

void UCoMUnitSubsystem::ProcessAutoExplore()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UCoMFogOfWarSubsystem* FoW = GI->GetSubsystem<UCoMFogOfWarSubsystem>();
	if (!FoW) return;

	// Iterate over a copy since we may remove entries.
	TSet<int32> CurrentAutoExplore = AutoExploreArmies;

	for (int32 ArmyId : CurrentAutoExplore)
	{
		FCoMArmyGroup* Army = AllArmies.Find(ArmyId);
		if (!Army || Army->UnitIDs.Num() == 0)
		{
			AutoExploreArmies.Remove(ArmyId);
			continue;
		}

		// Find the nearest unexplored tile within a reasonable search radius.
		const int32 SearchRadius = 30;
		FIntPoint BestTile = FIntPoint(-1, -1);
		int32 BestDist = INT_MAX;

		for (int32 dy = -SearchRadius; dy <= SearchRadius; ++dy)
		{
			for (int32 dx = -SearchRadius; dx <= SearchRadius; ++dx)
			{
				const int32 TileX = WrapX(Army->Position.X + dx);
				const int32 TileY = FMath::Clamp(Army->Position.Y + dy, 0, MAP_HEIGHT - 1);
				const FIntPoint Candidate(TileX, TileY);

				if (!FoW->IsTileExplored(Army->OwnerWizardIndex, Army->Plane, Candidate, Army->Layer))
				{
					const int32 Dist = FMath::Abs(dx) + FMath::Abs(dy);
					if (Dist < BestDist)
					{
						BestDist = Dist;
						BestTile = Candidate;
					}
				}
			}
		}

		if (BestTile.X < 0)
		{
			// No unexplored tiles found — cancel auto-explore.
			AutoExploreArmies.Remove(ArmyId);
			UE_LOG(LogTemp, Log, TEXT("Auto-explore: Army %d has no reachable unexplored tiles, cancelling."), ArmyId);
			continue;
		}

		// Move toward the unexplored tile.
		MoveArmy(ArmyId, BestTile);
	}
}

void UCoMUnitSubsystem::ProcessMovementTurn()
{
	// Tick down per-army encounter cooldowns so damaged armies regain the
	// ability to engage sites/nodes after the rest period set by a loss.
	for (auto& Pair : AllArmies)
	{
		if (Pair.Value.EncounterCooldown > 0)
		{
			Pair.Value.EncounterCooldown--;
		}
	}

	// Process auto-explore orders before regular movement resolution.
	ProcessAutoExplore();

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

	// Site / guarded-mana-node encounters: any army standing on a site or
	// guarded node fights its guards and (on win) loots / unlocks.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMSiteEncounterSubsystem* Sites = GI->GetSubsystem<UCoMSiteEncounterSubsystem>())
		{
			Sites->ProcessArmyArrivals();
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

FCoMArmyGroup* UCoMUnitSubsystem::GetArmyMutable(int32 ArmyID)
{
	return AllArmies.Find(ArmyID);
}

FCoMUnitInstance* UCoMUnitSubsystem::GetUnitMutable(int32 UnitID)
{
	return AllUnits.Find(UnitID);
}

void UCoMUnitSubsystem::SetUnitSettlerFlag(int32 UnitId, bool bSettler)
{
	if (FCoMUnitInstance* Unit = AllUnits.Find(UnitId))
	{
		Unit->bIsSettler = bSettler;
	}
}

void UCoMUnitSubsystem::SetUnitRaceTag(int32 UnitId, FGameplayTag Tag)
{
	if (FCoMUnitInstance* Unit = AllUnits.Find(UnitId))
	{
		Unit->RaceTag = Tag;
	}
}

bool UCoMUnitSubsystem::ApplyDamage(int32 UnitID, int32 Amount)
{
	FCoMUnitInstance* Unit = AllUnits.Find(UnitID);
	if (!Unit || Amount <= 0) return false;
	Unit->CurrentHP -= Amount;
	if (Unit->CurrentHP <= 0)
	{
		DespawnUnit(UnitID);
		return true;
	}
	return false;
}

int32 UCoMUnitSubsystem::ApplyHeal(int32 UnitID, int32 Amount)
{
	FCoMUnitInstance* Unit = AllUnits.Find(UnitID);
	if (!Unit || Amount <= 0) return 0;
	const int32 Before = Unit->CurrentHP;
	Unit->CurrentHP = FMath::Min(Unit->CurrentHP + Amount, Unit->MaxHP);
	return Unit->CurrentHP - Before;
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

		// Units carry an FName SpecID (e.g. "Settler"). Resolve movement from the
		// FName-keyed C++ database first; fall back to an editor data asset for
		// asset-authored specs. The old code did FCString::Atoi("Settler") -> 0
		// and ResolveSpec(0) -> nullptr, so every unit was skipped and the army's
		// speed came back as 0 — MoveArmy then bailed on the first tile and no
		// army (settlers included) ever actually moved.
		FFixed64 UnitMove(0);
		if (CoMUnitDatabase::Contains(Unit->SpecID))
		{
			UnitMove = FFixed64(CoMUnitDatabase::GetUnitSpec(Unit->SpecID).Movement);
		}
		else if (const UCoMUnitSpecDataAsset* Spec = ResolveSpec(FCString::Atoi(*Unit->SpecID.ToString())))
		{
			UnitMove = FFixed64(Spec->MovePoints);
		}
		else
		{
			continue;
		}

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

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load Export/Import
// ─────────────────────────────────────────────────────────────────────────────

void UCoMUnitSubsystem::ExportAll(TArray<FCoMUnitInstance>& OutUnits, TArray<FCoMArmyGroup>& OutArmies,
                                   int32& OutNextUnitID, int32& OutNextArmyID) const
{
	OutUnits.Empty();
	OutUnits.Reserve(AllUnits.Num());
	for (const auto& Pair : AllUnits)
	{
		OutUnits.Add(Pair.Value);
	}

	OutArmies.Empty();
	OutArmies.Reserve(AllArmies.Num());
	for (const auto& Pair : AllArmies)
	{
		OutArmies.Add(Pair.Value);
	}

	OutNextUnitID = NextUnitID;
	OutNextArmyID = NextArmyID;
}

void UCoMUnitSubsystem::ImportAll(const TArray<FCoMUnitInstance>& InUnits, const TArray<FCoMArmyGroup>& InArmies,
                                   int32 InNextUnitID, int32 InNextArmyID)
{
	AllUnits.Empty();
	for (const FCoMUnitInstance& Unit : InUnits)
	{
		AllUnits.Add(Unit.UnitID, Unit);
	}

	AllArmies.Empty();
	for (const FCoMArmyGroup& Army : InArmies)
	{
		AllArmies.Add(Army.ArmyGroupID, Army);
	}

	NextUnitID = InNextUnitID;
	NextArmyID = InNextArmyID;
	UE_LOG(LogTemp, Log, TEXT("[UnitSubsystem] ImportAll: %d units, %d armies imported."),
	       AllUnits.Num(), AllArmies.Num());
}
