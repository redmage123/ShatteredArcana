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
	RngStream.Initialize(0x44726167);
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
	Dragon.DragonTypeID = FName(*FString::FromInt(TypeID));
	Dragon.PersonalName = FText::GetEmpty(); // to be named by caller or event
	Dragon.Role         = ECoMDragonRole::Wild;
	Dragon.Age          = ECoMDragonAge::Young;
	Dragon.TurnsAlive   = 0;
	Dragon.UnitID       = -1; // assigned when placed on map via UCoMUnitSubsystem
	Dragon.DomainID     = -1;
	Dragon.HoardGold    = 0;
	Dragon.HoardMana    = 0;

	// Store spawn position as lair position.
	Dragon.LairPosition = LairPosition;

	// Random personality values.
	Dragon.TerritorialAggression = Rng.RandRange(20, 80);

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

	// Set the domain's plane from the dragon's spawn data.
	// Note: SpawnDragon stores Plane implicitly via the domain;
	// since FCoMDragonInstance does not carry a Plane field,
	// we look up any existing domain, or default to Aurelith.
	// For a newly created domain, the caller must ensure the dragon
	// is on the correct plane. We'll check if the dragon already has
	// a domain we can reference, otherwise default.
	Domain.Plane = ECoMPlane::Aurelith; // default
	if (Dragon->DomainID >= 0)
	{
		if (const FCoMDragonDomain* ExistingDomain = Domains.Find(Dragon->DomainID))
		{
			Domain.Plane = ExistingDomain->Plane;
		}
	}

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
	Egg.DragonTypeID    = Parent->DragonTypeID;
	Egg.ParentDragonID  = ParentDragonID;
	Egg.PossessorWizardIndex = PossessorWizard;
	Egg.IncubationTurnsRemaining = 15;
	Egg.HatchingManaCost = 50; // base cost; type-specific overrides later
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
			if (Egg.IncubationTurnsRemaining > 0)
			{
				return -1;
			}

			// Spawn the hatchling at the parent dragon's position and plane.
			FRandomStream HatchRng(EggID * 7919); // deterministic seed from egg ID
			ECoMPlane HatchPlane = ECoMPlane::Aurelith;
			FIntPoint HatchPosition = FIntPoint(0, 0);
			const FCoMDragonInstance* ParentDragon = Dragons.Find(Egg.ParentDragonID);
			if (ParentDragon)
			{
				HatchPosition = ParentDragon->LairPosition;
				if (ParentDragon->DomainID >= 0)
				{
					if (const FCoMDragonDomain* ParentDomain = Domains.Find(ParentDragon->DomainID))
					{
						HatchPlane = ParentDomain->Plane;
					}
				}
			}
			const int32 NewDragonID = SpawnDragon(FCString::Atoi(*Egg.DragonTypeID.ToString()), HatchPlane, HatchPosition, HatchRng);

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
		Pair.Value.TurnsAlive++;
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
		if (Ruler->TurnsAlive > 0 && (Ruler->TurnsAlive % CoMDragonConstants::DomainExpansionInterval) == 0)
		{
			Domain.InfluenceRadius++;
			Domain.ClaimedTiles = ComputeClaimedTiles(Domain.LairPosition, Domain.InfluenceRadius);
			OnDragonDomainExpanded.Broadcast(Domain.DomainID, Domain.InfluenceRadius);
		}
	}

	// 3. Tick egg incubation.
	for (FCoMDragonEgg& Egg : Eggs)
	{
		if (Egg.IncubationTurnsRemaining > 0) Egg.IncubationTurnsRemaining--;
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
				const int32 WrappedX = ((Center.X + DX) % CoM::MAP_WIDTH + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
				const int32 ClampedY = FMath::Clamp(Center.Y + DY, 0, CoM::MAP_HEIGHT - 1);
				Tiles.Add(FIntPoint(WrappedX, ClampedY));
			}
		}
	}

	return Tiles;
}
