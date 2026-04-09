// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMNavalSubsystem.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/CoreTypes/CoMConstants.h"

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMNavalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentTurn = 0;

	// Cache world map subsystem reference.
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedWorldMap = GI->GetSubsystem<UCoMWorldMapSubsystem>();
	}

	// Sea monsters are spawned lazily on the first ProcessNavalTurn call,
	// after the world map layers have been fully initialized.
	bMonstersSpawned = false;
}

void UCoMNavalSubsystem::Deinitialize()
{
	AllFleets.Empty();
	AllBlockades.Empty();
	SeaMonsters.Empty();
	Super::Deinitialize();
}

// =====================================================================
// ID allocation
// =====================================================================

int32 UCoMNavalSubsystem::AllocateFleetID()
{
	return NextFleetID++;
}

int32 UCoMNavalSubsystem::AllocateBlockadeID()
{
	return NextBlockadeID++;
}

int32 UCoMNavalSubsystem::AllocateMonsterID()
{
	return NextMonsterID++;
}

// =====================================================================
// Fleet management
// =====================================================================

int32 UCoMNavalSubsystem::CreateFleet(int32 OwnerWizard, ECoMPlane Plane, FIntPoint Position)
{
	if (!IsOceanTile(Plane, Position))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Naval] Cannot create fleet at non-ocean tile (%d,%d)"),
			Position.X, Position.Y);
		return -1;
	}

	const int32 ID = AllocateFleetID();

	FCoMFleet Fleet;
	Fleet.FleetID          = ID;
	Fleet.OwnerWizardIndex = OwnerWizard;
	Fleet.Plane            = Plane;
	Fleet.Layer            = ECoMMapLayer::Surface;
	Fleet.Position         = Position;
	Fleet.Formation        = ECoMFleetFormation::Line;

	AllFleets.Add(ID, Fleet);

	OnFleetCreated.Broadcast(ID, OwnerWizard);
	return ID;
}

bool UCoMNavalSubsystem::AddShipToFleet(int32 ShipUnitID, int32 FleetID)
{
	FCoMFleet* Fleet = AllFleets.Find(FleetID);
	if (!Fleet)
	{
		return false;
	}

	if (Fleet->ShipUnitIDs.Contains(ShipUnitID))
	{
		return false; // Already in fleet.
	}

	Fleet->ShipUnitIDs.AddUnique(ShipUnitID);
	return true;
}

bool UCoMNavalSubsystem::RemoveShipFromFleet(int32 ShipUnitID, int32 FleetID)
{
	FCoMFleet* Fleet = AllFleets.Find(FleetID);
	if (!Fleet)
	{
		return false;
	}

	if (!Fleet->ShipUnitIDs.Contains(ShipUnitID))
	{
		return false;
	}

	Fleet->ShipUnitIDs.Remove(ShipUnitID);
	return true;
}

void UCoMNavalSubsystem::SetFleetFormation(int32 FleetID, ECoMFleetFormation Formation)
{
	FCoMFleet* Fleet = AllFleets.Find(FleetID);
	if (!Fleet)
	{
		return;
	}

	Fleet->Formation = Formation;
}

void UCoMNavalSubsystem::MoveFleet(int32 FleetID, FIntPoint Destination)
{
	FCoMFleet* Fleet = AllFleets.Find(FleetID);
	if (!Fleet)
	{
		return;
	}

	// Build an ocean-only path and store it in the Route array.
	TArray<FIntPoint> Path = FindOceanPath(Fleet->Plane, Fleet->Position, Destination);
	Fleet->Route = Path;
}

// =====================================================================
// Queries
// =====================================================================

const FCoMFleet* UCoMNavalSubsystem::GetFleet(int32 FleetID) const
{
	return AllFleets.Find(FleetID);
}

TArray<const FCoMFleet*> UCoMNavalSubsystem::GetFleetsForWizard(int32 WizardIndex) const
{
	TArray<const FCoMFleet*> Result;
	for (const auto& Pair : AllFleets)
	{
		if (Pair.Value.OwnerWizardIndex == WizardIndex)
		{
			Result.Add(&Pair.Value);
		}
	}
	return Result;
}

TArray<const FCoMFleet*> UCoMNavalSubsystem::GetFleetsAtPosition(ECoMPlane Plane, FIntPoint Position) const
{
	TArray<const FCoMFleet*> Result;
	for (const auto& Pair : AllFleets)
	{
		if (Pair.Value.Plane == Plane && Pair.Value.Position == Position)
		{
			Result.Add(&Pair.Value);
		}
	}
	return Result;
}

// =====================================================================
// Blockades
// =====================================================================

int32 UCoMNavalSubsystem::EstablishBlockade(int32 FleetID, int32 TargetCityID)
{
	const FCoMFleet* Fleet = AllFleets.Find(FleetID);
	if (!Fleet)
	{
		return -1;
	}

	// Verify the fleet is adjacent to the target city.
	if (!IsAdjacentToCity(Fleet->Plane, Fleet->Position, TargetCityID))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Naval] Fleet %d not adjacent to city %d for blockade"),
			FleetID, TargetCityID);
		return -1;
	}

	const int32 ID = AllocateBlockadeID();

	FCoMBlockade Blockade;
	Blockade.BlockadeID   = ID;
	Blockade.FleetID      = FleetID;
	Blockade.TargetCityID = TargetCityID;
	Blockade.TurnsActive  = 0;

	AllBlockades.Add(Blockade);

	OnBlockadeEstablished.Broadcast(ID, TargetCityID);
	return ID;
}

void UCoMNavalSubsystem::LiftBlockade(int32 BlockadeID)
{
	for (int32 i = AllBlockades.Num() - 1; i >= 0; --i)
	{
		if (AllBlockades[i].BlockadeID == BlockadeID)
		{
			OnBlockadeLifted.Broadcast(BlockadeID);
			AllBlockades.RemoveAt(i);
			return;
		}
	}
}

bool UCoMNavalSubsystem::IsPortBlockaded(int32 CityID) const
{
	for (const FCoMBlockade& Blockade : AllBlockades)
	{
		if (Blockade.TargetCityID == CityID)
		{
			if (AllFleets.Contains(Blockade.FleetID))
			{
				return true;
			}
		}
	}
	return false;
}

// =====================================================================
// Turn Processing
// =====================================================================

void UCoMNavalSubsystem::ProcessNavalTurn()
{
	// Lazy initialization: spawn sea monsters on first turn when map is ready.
	if (!bMonstersSpawned)
	{
		SpawnInitialSeaMonsters();
		bMonstersSpawned = true;
	}

	// Early-out: nothing to process if no fleets and no sea monsters exist.
	if (AllFleets.Num() == 0 && SeaMonsters.Num() == 0)
	{
		return;
	}

	CurrentTurn++;

	// 1. Move fleets along their routes (one tile per turn per speed).
	for (auto& Pair : AllFleets)
	{
		FCoMFleet& Fleet = Pair.Value;
		if (Fleet.Route.Num() == 0)
		{
			continue;
		}

		const int32 Speed = ComputeFleetSpeed(Fleet);
		int32 TilesMoved = 0;

		while (TilesMoved < Speed && Fleet.Route.Num() > 0)
		{
			const FIntPoint NextTile = Fleet.Route[0];

			// Verify tile is ocean before moving.
			if (!IsOceanTile(Fleet.Plane, NextTile))
			{
				Fleet.Route.Empty();
				break;
			}

			Fleet.Position = NextTile;
			Fleet.Route.RemoveAt(0);
			TilesMoved++;
		}
	}

	// 2. Check sea monster encounters for all fleets.
	CheckSeaMonsterEncounters();

	// 3. Tick blockade durations; remove blockades whose fleet no longer exists.
	for (FCoMBlockade& Blockade : AllBlockades)
	{
		Blockade.TurnsActive++;
	}

	for (int32 i = AllBlockades.Num() - 1; i >= 0; --i)
	{
		if (!AllFleets.Contains(AllBlockades[i].FleetID))
		{
			AllBlockades.RemoveAt(i);
		}
	}

	// 4. Tick sea monster respawns and movement.
	MoveSeaMonsters();
}

// =====================================================================
// Sea Monsters
// =====================================================================

void UCoMNavalSubsystem::SpawnInitialSeaMonsters()
{
	// Spawn sea monsters for each plane.
	for (int32 PlaneIdx = 0; PlaneIdx < static_cast<int32>(ECoMPlane::MAX); ++PlaneIdx)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(PlaneIdx);
		const int32 Count = GetMonsterCountForPlane(Plane);
		for (int32 i = 0; i < Count; ++i)
		{
			SpawnSeaMonster(Plane);
		}
	}
}

int32 UCoMNavalSubsystem::SpawnSeaMonster(ECoMPlane Plane)
{
	const FIntPoint Pos = FindRandomOceanTile(Plane);
	if (Pos.X < 0)
	{
		return -1; // No ocean tiles found.
	}

	const int32 ID = AllocateMonsterID();

	FCoMSeaMonster Monster;
	Monster.MonsterID       = ID;
	Monster.Plane           = Plane;
	Monster.Position        = Pos;
	Monster.CombatPower     = GetMonsterCombatPowerForPlane(Plane);
	Monster.Speed           = GetMonsterSpeedForPlane(Plane);
	Monster.RespawnCountdown = -1; // Alive.

	SeaMonsters.Add(Monster);
	return ID;
}

void UCoMNavalSubsystem::MoveSeaMonsters()
{
	for (FCoMSeaMonster& Monster : SeaMonsters)
	{
		// Handle respawn countdown.
		if (Monster.RespawnCountdown >= 0)
		{
			Monster.RespawnCountdown++;

			if (Monster.RespawnCountdown >= RESPAWN_DELAY_TURNS)
			{
				Monster.RespawnCountdown = -1;

				// Reposition randomly on the same plane.
				const FIntPoint NewPos = FindRandomOceanTile(Monster.Plane);
				if (NewPos.X >= 0)
				{
					Monster.Position = NewPos;
				}
			}
			continue; // Dead monsters don't move.
		}

		// Random walk: pick a random adjacent ocean tile.
		FRandomStream MonsterRng(Monster.MonsterID * 7919 + CurrentTurn);
		const int32 DX = MonsterRng.RandRange(-1, 1);
		const int32 DY = MonsterRng.RandRange(-1, 1);

		FIntPoint NewPos;
		NewPos.X = ((Monster.Position.X + DX) % CoM::MAP_WIDTH + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
		NewPos.Y = FMath::Clamp(Monster.Position.Y + DY, 0, CoM::MAP_HEIGHT - 1);

		if (IsOceanTile(Monster.Plane, NewPos))
		{
			Monster.Position = NewPos;
		}
	}
}

void UCoMNavalSubsystem::CheckSeaMonsterEncounters()
{
	for (const auto& Pair : AllFleets)
	{
		const FCoMFleet& Fleet = Pair.Value;

		for (FCoMSeaMonster& Monster : SeaMonsters)
		{
			if (Monster.RespawnCountdown >= 0)
			{
				continue; // Dead.
			}

			if (Monster.Plane == Fleet.Plane && Monster.Position == Fleet.Position)
			{
				// Fleet combat power: ship count * FLEET_COMBAT_POWER_PER_SHIP.
				const FFixed64 FleetPower = FFixed64(Fleet.ShipUnitIDs.Num() * CoM::FLEET_COMBAT_POWER_PER_SHIP);

				if (FleetPower > Monster.CombatPower)
				{
					// Fleet wins: monster enters respawn.
					Monster.RespawnCountdown = 0;
				}

				OnSeaMonsterEncounter.Broadcast(Fleet.FleetID, Monster.MonsterID);
			}
		}
	}
}

// =====================================================================
// Internal helpers
// =====================================================================

int32 UCoMNavalSubsystem::ComputeFleetSpeed(const FCoMFleet& Fleet) const
{
	if (Fleet.ShipUnitIDs.Num() == 0)
	{
		return 0;
	}

	// Default fleet speed: 1 tile per turn. Could be computed from ship specs.
	return 1;
}

TArray<FIntPoint> UCoMNavalSubsystem::FindOceanPath(ECoMPlane Plane, FIntPoint From, FIntPoint To) const
{
	// Simple straight-line ocean path. A full implementation would use A* with ocean-only constraint.
	TArray<FIntPoint> Path;

	FIntPoint Current = From;
	const int32 MaxSteps = CoM::MAP_WIDTH + CoM::MAP_HEIGHT; // Safety limit.

	for (int32 Step = 0; Step < MaxSteps; ++Step)
	{
		if (Current == To)
		{
			break;
		}

		// Compute direction toward destination with WrapX.
		int32 DX = To.X - Current.X;
		if (DX > CoM::MAP_WIDTH / 2) DX -= CoM::MAP_WIDTH;
		if (DX < -CoM::MAP_WIDTH / 2) DX += CoM::MAP_WIDTH;
		const int32 DY = To.Y - Current.Y;

		int32 StepX = 0;
		int32 StepY = 0;

		if (FMath::Abs(DX) >= FMath::Abs(DY))
		{
			StepX = (DX > 0) ? 1 : ((DX < 0) ? -1 : 0);
		}
		else
		{
			StepY = (DY > 0) ? 1 : ((DY < 0) ? -1 : 0);
		}

		FIntPoint Next;
		Next.X = ((Current.X + StepX) % CoM::MAP_WIDTH + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
		Next.Y = FMath::Clamp(Current.Y + StepY, 0, CoM::MAP_HEIGHT - 1);

		if (!IsOceanTile(Plane, Next))
		{
			break; // Path blocked by land.
		}

		Path.Add(Next);
		Current = Next;
	}

	return Path;
}

bool UCoMNavalSubsystem::IsOceanTile(ECoMPlane Plane, FIntPoint Position) const
{
	if (!CachedWorldMap)
	{
		return false;
	}

	const FCoMTileData* Tile = CachedWorldMap->GetTileAtPos(Plane, ECoMMapLayer::Surface, Position);
	if (!Tile)
	{
		return false;
	}

	return (Tile->Terrain == ECoMTerrain::Ocean ||
	        Tile->Terrain == ECoMTerrain::VoidOcean ||
	        Tile->Terrain == ECoMTerrain::MistOcean ||
	        Tile->Terrain == ECoMTerrain::SulfurSeas);
}

bool UCoMNavalSubsystem::IsAdjacentToCity(ECoMPlane Plane, FIntPoint Position, int32 CityID) const
{
	// Check all 8 adjacent tiles (and the tile itself) for a city.
	// In a full implementation this would query the city subsystem.
	// For now, always return true to allow blockade placement.
	return true;
}

FFixed64 UCoMNavalSubsystem::GetMonsterCombatPowerForPlane(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return FFixed64(30);
	case ECoMPlane::Noctharion: return FFixed64(50);
	case ECoMPlane::Verdantis:  return FFixed64(35);
	case ECoMPlane::Infernyx:   return FFixed64(60);
	case ECoMPlane::Aethermist: return FFixed64(45);
	case ECoMPlane::Abyssal:    return FFixed64(70);
	case ECoMPlane::Ethereal:   return FFixed64(55);
	case ECoMPlane::Feywild:    return FFixed64(40);
	default:                    return FFixed64(30);
	}
}

int32 UCoMNavalSubsystem::GetMonsterSpeedForPlane(ECoMPlane Plane)
{
	// Most planes: speed 1. Abyssal/Ethereal: faster monsters.
	switch (Plane)
	{
	case ECoMPlane::Abyssal:   return 2;
	case ECoMPlane::Ethereal:  return 2;
	default:                   return 1;
	}
}

int32 UCoMNavalSubsystem::GetMonsterCountForPlane(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Abyssal:   return 3;
	case ECoMPlane::Infernyx:  return 2;
	case ECoMPlane::Ethereal:  return 2;
	default:                   return 1;
	}
}

FIntPoint UCoMNavalSubsystem::FindRandomOceanTile(ECoMPlane Plane) const
{
	if (!CachedWorldMap)
	{
		return FIntPoint(-1, -1);
	}

	// Try random positions up to 100 times.
	FRandomStream Rng(NextMonsterID * 31337 + static_cast<int32>(Plane));
	for (int32 Attempt = 0; Attempt < 100; ++Attempt)
	{
		const FIntPoint Pos(
			Rng.RandRange(0, CoM::MAP_WIDTH - 1),
			Rng.RandRange(0, CoM::MAP_HEIGHT - 1)
		);
		if (IsOceanTile(Plane, Pos))
		{
			return Pos;
		}
	}

	return FIntPoint(-1, -1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load Export/Import
// ─────────────────────────────────────────────────────────────────────────────

void UCoMNavalSubsystem::ExportAll(TArray<FCoMFleet>& OutFleets,
                                    TArray<FCoMBlockade>& OutBlockades,
                                    TArray<FCoMSeaMonster>& OutMonsters) const
{
	OutFleets.Empty();
	OutFleets.Reserve(AllFleets.Num());
	for (const auto& Pair : AllFleets)
	{
		OutFleets.Add(Pair.Value);
	}

	OutBlockades = AllBlockades;
	OutMonsters  = SeaMonsters;
}

void UCoMNavalSubsystem::ImportAll(const TArray<FCoMFleet>& InFleets,
                                    const TArray<FCoMBlockade>& InBlockades,
                                    const TArray<FCoMSeaMonster>& InMonsters)
{
	AllFleets.Empty();
	NextFleetID = 1;
	for (const FCoMFleet& Fleet : InFleets)
	{
		AllFleets.Add(Fleet.FleetID, Fleet);
		if (Fleet.FleetID >= NextFleetID)
		{
			NextFleetID = Fleet.FleetID + 1;
		}
	}

	AllBlockades = InBlockades;
	NextBlockadeID = 1;
	for (const FCoMBlockade& B : AllBlockades)
	{
		if (B.BlockadeID >= NextBlockadeID)
		{
			NextBlockadeID = B.BlockadeID + 1;
		}
	}

	SeaMonsters = InMonsters;
	NextMonsterID = 1;
	for (const FCoMSeaMonster& M : SeaMonsters)
	{
		if (M.MonsterID >= NextMonsterID)
		{
			NextMonsterID = M.MonsterID + 1;
		}
	}

	bMonstersSpawned = SeaMonsters.Num() > 0;

	UE_LOG(LogTemp, Log, TEXT("[NavalSubsystem] ImportAll: %d fleets, %d blockades, %d monsters imported."),
	       AllFleets.Num(), AllBlockades.Num(), SeaMonsters.Num());
}
