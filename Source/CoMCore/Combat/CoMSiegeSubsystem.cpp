// Copyright Shattered Arcana. All Rights Reserved.
#include "CoMSiegeSubsystem.h"

void UCoMSiegeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AllSieges.Empty();
	NextSiegeID = 1;
	RngStream.Initialize(FPlatformTime::Cycles());
}

void UCoMSiegeSubsystem::Deinitialize()
{
	AllSieges.Empty();
	Super::Deinitialize();
}

int32 UCoMSiegeSubsystem::BeginSiege(int32 AttackerArmyID, int32 DefenderCityID)
{
	// Check city not already under siege
	if (IsCityUnderSiege(DefenderCityID))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Siege] City %d already under siege"), DefenderCityID);
		return -1;
	}

	const int32 SiegeID = NextSiegeID++;

	FCoMSiegeState Siege;
	Siege.SiegeID = SiegeID;
	Siege.AttackerArmyGroup = AttackerArmyID;
	Siege.DefenderCity = DefenderCityID;
	Siege.Phase = ECoMSiegePhase::Approach;
	Siege.TurnsUnderSiege = 0;
	Siege.CityFoodReserves = 50; // Default food reserves
	Siege.CityMorale = 100;
	Siege.bSupplyLinesCut = false;

	// Wall HP based on city wall level (TODO: query from CitySubsystem)
	// Palisade=50, Stone=100, Fortress=200
	Siege.WallMaxHP = 100; // Default stone walls
	Siege.WallHP = Siege.WallMaxHP;
	Siege.GatehouseHP = Siege.WallMaxHP / 2;

	AllSieges.Add(SiegeID, Siege);

	UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d begun: army %d vs city %d"), SiegeID, AttackerArmyID, DefenderCityID);
	return SiegeID;
}

void UCoMSiegeSubsystem::EndSiege(int32 SiegeID, bool bAttackerVictory)
{
	if (AllSieges.Contains(SiegeID))
	{
		UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d ended: %s"), SiegeID,
			bAttackerVictory ? TEXT("attacker victory") : TEXT("defender held"));
		AllSieges.Remove(SiegeID);
	}
}

void UCoMSiegeSubsystem::ProcessSiegeTurn()
{
	TArray<int32> SiegeIDs;
	AllSieges.GetKeys(SiegeIDs);

	for (const int32 SiegeID : SiegeIDs)
	{
		FCoMSiegeState* Siege = AllSieges.Find(SiegeID);
		if (!Siege) continue;

		Siege->TurnsUnderSiege++;

		// Phase progression
		if (Siege->Phase == ECoMSiegePhase::Approach && Siege->TurnsUnderSiege >= 1)
		{
			Siege->Phase = ECoMSiegePhase::Bombardment;
		}

		// Bombardment: siege equipment damages walls
		if (Siege->Phase == ECoMSiegePhase::Bombardment)
		{
			// TODO: check weather — Blizzard/Rain halve bombardment damage
			int32 TotalDamage = 0;
			for (int32 EquipID : Siege->SiegeEquipmentIDs)
			{
				// Catapults: 5 damage to walls, Rams: 10 to gatehouse
				// Simplified: all equipment does 5 to walls
				TotalDamage += 5;
			}

			// At least 1 damage per turn even without equipment (sappers)
			TotalDamage = FMath::Max(1, TotalDamage);

			Siege->WallHP = FMath::Max(0, Siege->WallHP - TotalDamage);

			// Gatehouse damage from rams
			int32 RamDamage = Siege->SiegeEquipmentIDs.Num() > 0 ? 3 : 0;
			Siege->GatehouseHP = FMath::Max(0, Siege->GatehouseHP - RamDamage);

			// Check for breach
			if (Siege->WallHP <= 0 || Siege->GatehouseHP <= 0)
			{
				Siege->Phase = ECoMSiegePhase::Breach;
				if (Siege->WallHP <= 0)
				{
					Siege->BreachPoints.Add(Siege->TurnsUnderSiege); // Use turn as breach ID
				}
				UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: BREACH! WallHP=%d GateHP=%d"),
					SiegeID, Siege->WallHP, Siege->GatehouseHP);
			}
		}

		// Starvation
		int32 FoodLoss = Siege->bSupplyLinesCut ? 3 : 1;
		Siege->CityFoodReserves = FMath::Max(0, Siege->CityFoodReserves - FoodLoss);

		if (Siege->CityFoodReserves <= 0)
		{
			if (Siege->Phase != ECoMSiegePhase::Starvation && Siege->Phase != ECoMSiegePhase::Surrender)
			{
				Siege->Phase = ECoMSiegePhase::Starvation;
			}
			// Morale drops 2/turn when starving
			Siege->CityMorale = FMath::Max(0, Siege->CityMorale - 2);
		}

		// Auto-surrender at morale 0
		if (Siege->CityMorale <= 0)
		{
			Siege->Phase = ECoMSiegePhase::Surrender;
			UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: city morale 0, auto-surrender"), SiegeID);
			EndSiege(SiegeID, true);
			continue;
		}

		// Morale decay from siege duration
		if (Siege->TurnsUnderSiege > 5)
		{
			Siege->CityMorale = FMath::Max(0, Siege->CityMorale - 1);
		}
	}
}

const FCoMSiegeState* UCoMSiegeSubsystem::GetSiege(int32 SiegeID) const
{
	return AllSieges.Find(SiegeID);
}

TArray<const FCoMSiegeState*> UCoMSiegeSubsystem::GetActiveSieges() const
{
	TArray<const FCoMSiegeState*> Result;
	for (const auto& Pair : AllSieges)
	{
		Result.Add(&Pair.Value);
	}
	return Result;
}

bool UCoMSiegeSubsystem::IsCityUnderSiege(int32 CityID) const
{
	for (const auto& Pair : AllSieges)
	{
		if (Pair.Value.DefenderCity == CityID)
		{
			return true;
		}
	}
	return false;
}

void UCoMSiegeSubsystem::DeploySiegeEquipment(int32 SiegeID, int32 EquipmentUnitID)
{
	FCoMSiegeState* Siege = AllSieges.Find(SiegeID);
	if (!Siege)
	{
		return;
	}

	if (!Siege->SiegeEquipmentIDs.Contains(EquipmentUnitID))
	{
		Siege->SiegeEquipmentIDs.Add(EquipmentUnitID);
		UE_LOG(LogTemp, Log, TEXT("[Siege] Equipment %d deployed to siege %d"), EquipmentUnitID, SiegeID);
	}
}

void UCoMSiegeSubsystem::AttemptAssault(int32 SiegeID)
{
	FCoMSiegeState* Siege = AllSieges.Find(SiegeID);
	if (!Siege)
	{
		return;
	}

	// TODO: check weather — Thunderstorm blocks assault

	Siege->Phase = ECoMSiegePhase::Assault;

	// Determine defender advantage multiplier
	float DefenderMult = 1.0f;

	if (Siege->BreachPoints.Num() == 0 && Siege->GatehouseHP > 0)
	{
		// No breach, no gatehouse down — 3x defender advantage (very costly assault)
		DefenderMult = 3.0f;
	}
	else if (Siege->BreachPoints.Num() == 0)
	{
		// Gatehouse down but walls up — siege towers might help
		// Check if any siege towers deployed (simplified: check equipment count > 2)
		if (Siege->SiegeEquipmentIDs.Num() > 2)
		{
			DefenderMult = 1.5f; // Siege towers reduce advantage
		}
		else
		{
			DefenderMult = 2.5f;
		}
	}
	else
	{
		// Breach exists — normal combat
		DefenderMult = 1.0f;
	}

	UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: assault attempted! Defender mult=%.1f, breaches=%d"),
		SiegeID, DefenderMult, Siege->BreachPoints.Num());

	// TODO: trigger tactical combat with DefenderMult applied to defender stats
	// For now, resolve based on simple comparison
	// Actual combat resolution will be wired in the Combat subsystem (Phase 8)
}

bool UCoMSiegeSubsystem::OfferSurrender(int32 SiegeID)
{
	FCoMSiegeState* Siege = AllSieges.Find(SiegeID);
	if (!Siege)
	{
		return false;
	}

	// Surrender acceptance based on morale and food
	float AcceptChance = 0.0f;

	if (Siege->CityMorale < 10)
	{
		AcceptChance = 0.80f;
	}
	else if (Siege->CityMorale < 20 && Siege->CityFoodReserves < 5)
	{
		AcceptChance = 0.50f;
	}
	else if (Siege->CityMorale < 30 && Siege->CityFoodReserves <= 0)
	{
		AcceptChance = 0.30f;
	}

	// Roll
	const float Roll = RngStream.FRand();
	const bool bAccepted = Roll < AcceptChance;

	UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: surrender offered. Morale=%d, Food=%d, Chance=%.0f%%, %s"),
		SiegeID, Siege->CityMorale, Siege->CityFoodReserves, AcceptChance * 100.0f,
		bAccepted ? TEXT("ACCEPTED") : TEXT("REJECTED"));

	if (bAccepted)
	{
		Siege->Phase = ECoMSiegePhase::Surrender;
		EndSiege(SiegeID, true);
	}

	return bAccepted;
}

void UCoMSiegeSubsystem::CutSupplyLines(int32 SiegeID)
{
	FCoMSiegeState* Siege = AllSieges.Find(SiegeID);
	if (Siege && !Siege->bSupplyLinesCut)
	{
		Siege->bSupplyLinesCut = true;
		UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: supply lines CUT — starvation accelerated"), SiegeID);
	}
}
