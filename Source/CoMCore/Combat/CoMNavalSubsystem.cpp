// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMNavalSubsystem.h"
#include "CoMWorldMapSubsystem.h"
#include "CoMSeasonSubsystem.h"

// =====================================================================
// Fixed-point constants
// =====================================================================

namespace CoMNavalConstants
{
	static const FFixed64 Zero(0, 0);
	static const FFixed64 One(1, 0);
	static const FFixed64 OceanMoveCost(1, 0);
	static const int32    SeaMonstersPerPlaneMin = 1;
	static const int32    SeaMonstersPerPlaneMax = 3;
	static const int32    SeaMonsterRespawnTurns = 20;
	static const FFixed64 SeaMonsterMinPower(30, 0);
	static const FFixed64 SeaMonsterMaxPower(80, 0);
}

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMNavalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCoMNavalSubsystem::Deinitialize()
{
	AllFleets.Empty();
	Blockades.Empty();
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
	const int32 ID = AllocateFleetID();

	FCoMFleet Fleet;
	Fleet.FleetID      = ID;
	Fleet.OwnerWizard  = OwnerWizard;
	Fleet.Plane        = Plane;
	Fleet.Position     = Position;
	Fleet.Formation    = ECoMFleetFormation::Line;
	Fleet.Destination  = Position;
	Fleet.bMoving      = false;

	AllFleets.Add(ID, Fleet);
	return ID;
}

void UCoMNavalSubsystem::AddShipToFleet(int32 FleetID, int32 ShipUnitID)
{
	FCoMFleet* Fleet = AllFleets.Find(FleetID);
	if (!Fleet)
	{
		return;
	}

	Fleet->ShipUnitIDs.AddUnique(ShipUnitID);
}

void UCoMNavalSubsystem::RemoveShipFromFleet(int32 FleetID, int32 ShipUnitID)
{
	FCoMFleet* Fleet = AllFleets.Find(FleetID);
	if (!Fleet)
	{
		return;
	}

	Fleet->ShipUnitIDs.Remove(ShipUnitID);
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

	Fleet->Destination = Destination;
	Fleet->bMoving     = true;
}

const FCoMFleet* UCoMNavalSubsystem::GetFleet(int32 FleetID) const
{
	return AllFleets.Find(FleetID);
}

TArray<const FCoMFleet*> UCoMNavalSubsystem::GetFleetsForWizard(int32 WizardIndex) const
{
	TArray<const FCoMFleet*> Result;
	for (const auto& Pair : AllFleets)
	{
		if (Pair.Value.OwnerWizard == WizardIndex)
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

	const int32 ID = AllocateBlockadeID();

	FCoMBlockade Blockade;
	Blockade.BlockadeID   = ID;
	Blockade.FleetID      = FleetID;
	Blockade.TargetCityID = TargetCityID;
	Blockade.TurnsActive  = 0;

	Blockades.Add(Blockade);
	return ID;
}

void UCoMNavalSubsystem::LiftBlockade(int32 BlockadeID)
{
	for (int32 i = Blockades.Num() - 1; i >= 0; --i)
	{
		if (Blockades[i].BlockadeID == BlockadeID)
		{
			Blockades.RemoveAt(i);
			return;
		}
	}
}

bool UCoMNavalSubsystem::IsPortBlockaded(int32 CityID) const
{
	for (const FCoMBlockade& Blockade : Blockades)
	{
		if (Blockade.TargetCityID == CityID)
		{
			// Verify the blockading fleet still exists.
			if (AllFleets.Contains(Blockade.FleetID))
			{
				return true;
			}
		}
	}
	return false;
}

// =====================================================================
// WrapX helper
// =====================================================================

static int32 WrapX(int32 X)
{
	if (X < 0)
	{
		X += CoM::MAP_WIDTH;
	}
	else if (X >= CoM::MAP_WIDTH)
	{
		X -= CoM::MAP_WIDTH;
	}
	return X;
}

static int32 WrapXDelta(int32 From, int32 To)
{
	int32 DX = To - From;
	if (DX > CoM::MAP_WIDTH / 2)
	{
		DX -= CoM::MAP_WIDTH;
	}
	else if (DX < -CoM::MAP_WIDTH / 2)
	{
		DX += CoM::MAP_WIDTH;
	}
	return DX;
}

// =====================================================================
// Sea Monsters
// =====================================================================

void UCoMNavalSubsystem::SpawnSeaMonsters(ECoMPlane Plane, FRandomStream& Rng)
{
	const int32 Count = Rng.RandRange(
		CoMNavalConstants::SeaMonstersPerPlaneMin,
		CoMNavalConstants::SeaMonstersPerPlaneMax
	);

	for (int32 i = 0; i < Count; ++i)
	{
		FCoMSeaMonster Monster;
		Monster.MonsterID   = AllocateMonsterID();
		Monster.Plane       = Plane;
		Monster.Position    = FIntPoint(
			Rng.RandRange(0, CoM::MAP_WIDTH - 1),
			Rng.RandRange(0, CoM::MAP_HEIGHT - 1)
		);
		Monster.CombatPower = FFixed64(
			Rng.RandRange(
				static_cast<int32>(CoMNavalConstants::SeaMonsterMinPower.GetWhole()),
				static_cast<int32>(CoMNavalConstants::SeaMonsterMaxPower.GetWhole())
			), 0
		);
		Monster.bAlive       = true;
		Monster.RespawnTurns = 0;

		SeaMonsters.Add(Monster);
	}
}

void UCoMNavalSubsystem::TickSeaMonsterRespawns()
{
	for (FCoMSeaMonster& Monster : SeaMonsters)
	{
		if (!Monster.bAlive)
		{
			Monster.RespawnTurns++;

			if (Monster.RespawnTurns >= CoMNavalConstants::SeaMonsterRespawnTurns)
			{
				Monster.bAlive       = true;
				Monster.RespawnTurns = 0;

				// Reposition randomly on the same plane.
				FRandomStream Rng(Monster.MonsterID * 7919 + Monster.RespawnTurns);
				Monster.Position = FIntPoint(
					Rng.RandRange(0, CoM::MAP_WIDTH - 1),
					Rng.RandRange(0, CoM::MAP_HEIGHT - 1)
				);
			}
		}
	}
}

bool UCoMNavalSubsystem::CheckSeaMonsterEncounter(const FCoMFleet& Fleet)
{
	for (FCoMSeaMonster& Monster : SeaMonsters)
	{
		if (Monster.bAlive &&
			Monster.Plane == Fleet.Plane &&
			Monster.Position == Fleet.Position)
		{
			// Sea monster encountered — resolve combat.
			// Fleet combat power is approximated by ship count * 10.
			const FFixed64 FleetPower = FFixed64(Fleet.ShipUnitIDs.Num() * 10, 0);

			if (FleetPower > Monster.CombatPower)
			{
				// Fleet wins: monster dies and enters respawn.
				Monster.bAlive       = false;
				Monster.RespawnTurns = 0;
				OnSeaMonsterDefeated.Broadcast(Monster.MonsterID, Fleet.FleetID);
			}
			else
			{
				// Monster wins: fleet takes losses (handled by caller via delegate).
				OnFleetDamagedByMonster.Broadcast(Fleet.FleetID, Monster.MonsterID);
			}
			return true;
		}
	}
	return false;
}

// =====================================================================
// Turn Processing
// =====================================================================

void UCoMNavalSubsystem::ProcessNavalTurn()
{
	// 1. Move fleets toward destinations (one tile per turn, ocean only).
	for (auto& Pair : AllFleets)
	{
		FCoMFleet& Fleet = Pair.Value;
		if (!Fleet.bMoving)
		{
			continue;
		}

		if (Fleet.Position == Fleet.Destination)
		{
			Fleet.bMoving = false;
			continue;
		}

		// Compute the next step toward the destination using WrapX.
		const int32 DX = WrapXDelta(Fleet.Position.X, Fleet.Destination.X);
		const int32 DY = Fleet.Destination.Y - Fleet.Position.Y;

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

		FIntPoint NextPos;
		NextPos.X = WrapX(Fleet.Position.X + StepX);
		NextPos.Y = FMath::Clamp(Fleet.Position.Y + StepY, 0, CoM::MAP_HEIGHT - 1);

		// TODO: Verify next tile is ocean via UCoMWorldMapSubsystem.
		// For now, accept the move. Production code should query the map
		// and reject moves onto land tiles.

		Fleet.Position = NextPos;

		// Check arrival.
		if (Fleet.Position == Fleet.Destination)
		{
			Fleet.bMoving = false;
		}
	}

	// 2. Check sea monster encounters for all moving fleets.
	for (auto& Pair : AllFleets)
	{
		CheckSeaMonsterEncounter(Pair.Value);
	}

	// 3. Tick blockade durations.
	for (FCoMBlockade& Blockade : Blockades)
	{
		Blockade.TurnsActive++;
	}

	// Remove blockades whose fleet no longer exists.
	for (int32 i = Blockades.Num() - 1; i >= 0; --i)
	{
		if (!AllFleets.Contains(Blockades[i].FleetID))
		{
			Blockades.RemoveAt(i);
		}
	}

	// 4. Tick sea monster respawns.
	TickSeaMonsterRespawns();
}
