// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMDragonSubsystem.h"

// =====================================================================
// Fixed-point constants
// =====================================================================

namespace CoMDragonConstants
{
	static const int32 DomainExpansionInterval = 10; // expand every 10 turns
}

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMDragonSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RngStream.Initialize(FPlatformTime::Cycles());
}

void UCoMDragonSubsystem::Deinitialize()
{
	Dragons.Empty();
	Domains.Empty();
	Eggs.Empty();
	Super::Deinitialize();
}

// =====================================================================
// Dragons
// =====================================================================

int32 UCoMDragonSubsystem::AllocateDragonID()
{
	return NextDragonID++;
}

int32 UCoMDragonSubsystem::AllocateDomainID()
{
	return NextDomainID++;
}

int32 UCoMDragonSubsystem::AllocateEggID()
{
	return NextEggID++;
}

int32 UCoMDragonSubsystem::SpawnDragon(int32 TypeID, ECoMPlane Plane, FIntPoint LairPosition, FRandomStream& Rng)
{
	const int32 ID = AllocateDragonID();

	FCoMDragonInstance Dragon;
	Dragon.DragonID     = ID;
	Dragon.TypeID       = TypeID;
	Dragon.PersonalName = FName(NAME_None); // to be named by caller or event
	Dragon.Role         = ECoMDragonRole::Wild;
	Dragon.Age          = 0;
	Dragon.UnitID       = -1; // assigned when placed on map via UCoMUnitSubsystem
	Dragon.DomainID     = -1;
	Dragon.HoardGold    = FFixed64(0, 0);
	Dragon.HoardMana    = FFixed64(0, 0);

	// Store spawn position as lair position.
	Dragon.LairPosition = LairPosition;

	// Random personality values.
	Dragon.TerritorialAggression = FFixed64(Rng.RandRange(20, 80), 0);

	Dragons.Add(ID, Dragon);
	return ID;
}

void UCoMDragonSubsystem::CreateDragonDomain(int32 DragonID, int32 InfluenceRadius)
{
	FCoMDragonInstance* Dragon = Dragons.Find(DragonID);
	if (!Dragon)
	{
		return;
	}

	const int32 DomID = AllocateDomainID();

	FCoMDragonDomain Domain;
	Domain.DomainID        = DomID;
	Domain.RulerDragonID   = DragonID;
	Domain.InfluenceRadius = InfluenceRadius;
	Domain.LairPosition    = Dragon->LairPosition;

	// Compute initial claimed tiles.
	Domain.ClaimedTiles = ComputeClaimedTiles(Domain.LairPosition, InfluenceRadius);

	Domains.Add(DomID, Domain);

	// Link the dragon to this domain and promote to DomainRuler.
	Dragon->DomainID = DomID;
	Dragon->Role     = ECoMDragonRole::DomainRuler;
}

const FCoMDragonInstance* UCoMDragonSubsystem::GetDragon(int32 DragonID) const
{
	return Dragons.Find(DragonID);
}

const FCoMDragonDomain* UCoMDragonSubsystem::GetDomain(int32 DomainID) const
{
	return Domains.Find(DomainID);
}

TArray<const FCoMDragonDomain*> UCoMDragonSubsystem::GetDomainsOnPlane(ECoMPlane Plane) const
{
	TArray<const FCoMDragonDomain*> Result;
	for (const auto& Pair : Domains)
	{
		if (Pair.Value.Plane == Plane)
		{
			Result.Add(&Pair.Value);
		}
	}
	return Result;
}

// =====================================================================
// Dragon Eggs
// =====================================================================

int32 UCoMDragonSubsystem::LayDragonEgg(int32 ParentDragonID, int32 PossessorWizard)
{
	const FCoMDragonInstance* Parent = Dragons.Find(ParentDragonID);
	if (!Parent)
	{
		return -1;
	}

	const int32 ID = AllocateEggID();

	FCoMDragonEgg Egg;
	Egg.EggID           = ID;
	Egg.DragonTypeID    = Parent->TypeID;
	Egg.ParentDragonID  = ParentDragonID;
	Egg.PossessorWizard = PossessorWizard;
	Egg.IncubationTurns = 0;
	Egg.HatchingManaCost = FFixed64(50, 0); // base cost; type-specific overrides later
	Egg.bImprints       = false;

	Eggs.Add(Egg);
	return ID;
}

int32 UCoMDragonSubsystem::HatchEgg(int32 EggID)
{
	for (int32 i = 0; i < Eggs.Num(); ++i)
	{
		if (Eggs[i].EggID == EggID)
		{
			FCoMDragonEgg& Egg = Eggs[i];

			// Require a minimum incubation period (10 turns).
			if (Egg.IncubationTurns < 10)
			{
				return -1;
			}

			// Spawn the hatchling.
			FRandomStream HatchRng(EggID * 7919); // deterministic seed from egg ID
			const int32 NewDragonID = SpawnDragon(Egg.DragonTypeID, Egg.HatchingRealm, FIntPoint(0, 0), HatchRng);

			if (NewDragonID >= 0)
			{
				FCoMDragonInstance* Hatchling = Dragons.Find(NewDragonID);
				if (Hatchling)
				{
					// If the egg imprints, the hatchling becomes a companion.
					if (Egg.bImprints)
					{
						Hatchling->Role = ECoMDragonRole::Companion;
					}
					else
					{
						Hatchling->Role = ECoMDragonRole::Summon;
					}
				}

				OnDragonEggHatched.Broadcast(EggID, NewDragonID);
			}

			Eggs.RemoveAt(i);
			return NewDragonID;
		}
	}

	return -1;
}

// =====================================================================
// Dragon Turn Processing
// =====================================================================

void UCoMDragonSubsystem::ProcessDragonTurn()
{
	// 1. Age all dragons.
	for (auto& Pair : Dragons)
	{
		Pair.Value.Age++;
	}

	// 2. Expand domains periodically.
	for (auto& Pair : Domains)
	{
		FCoMDragonDomain& Domain = Pair.Value;
		const FCoMDragonInstance* Ruler = Dragons.Find(Domain.RulerDragonID);
		if (!Ruler)
		{
			continue;
		}

		// Domain rulers expand influence every N turns based on age.
		if (Ruler->Age > 0 && (Ruler->Age % CoMDragonConstants::DomainExpansionInterval) == 0)
		{
			Domain.InfluenceRadius++;
			Domain.ClaimedTiles = ComputeClaimedTiles(Domain.LairPosition, Domain.InfluenceRadius);
			OnDragonDomainExpanded.Broadcast(Domain.DomainID, Domain.InfluenceRadius);
		}
	}

	// 3. Tick egg incubation.
	for (FCoMDragonEgg& Egg : Eggs)
	{
		Egg.IncubationTurns++;
	}
}

// =====================================================================
// Internal helpers
// =====================================================================

TArray<FIntPoint> UCoMDragonSubsystem::ComputeClaimedTiles(FIntPoint Center, int32 Radius)
{
	TArray<FIntPoint> Tiles;
	const int32 RadiusSq = Radius * Radius;

	for (int32 DX = -Radius; DX <= Radius; ++DX)
	{
		for (int32 DY = -Radius; DY <= Radius; ++DY)
		{
			if ((DX * DX + DY * DY) <= RadiusSq)
			{
				Tiles.Add(FIntPoint(Center.X + DX, Center.Y + DY));
			}
		}
	}

	return Tiles;
}
