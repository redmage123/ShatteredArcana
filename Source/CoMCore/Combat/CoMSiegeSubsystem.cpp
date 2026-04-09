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
	Siege.AttackerArmyGroupID = AttackerArmyID;
	Siege.DefenderCityID = DefenderCityID;
	Siege.Phase = ECoMSiegePhase::Approach;
	Siege.TurnsUnderSiege = 0;
	Siege.CityFoodReserves = CoM::DEFAULT_FOOD_RESERVES;
	Siege.CityMorale = CoM::DEFAULT_MORALE;
	Siege.bSupplyLinesCut = false;

	// Wall HP based on city wall level (TODO: query from CitySubsystem)
	// Palisade=50, Stone=100, Fortress=200
	Siege.WallMaxHP = CoM::DEFAULT_WALL_HP;
	Siege.WallHP = Siege.WallMaxHP;
	Siege.GatehouseHP = Siege.WallMaxHP / 2;

	AllSieges.Add(SiegeID, Siege);

	OnSiegeStarted.Broadcast(SiegeID, DefenderCityID);

	UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d begun: army %d vs city %d"), SiegeID, AttackerArmyID, DefenderCityID);
	return SiegeID;
}

void UCoMSiegeSubsystem::EndSiege(int32 SiegeID, bool bAttackerVictory)
{
	if (AllSieges.Contains(SiegeID))
	{
		OnSiegeEnded.Broadcast(SiegeID, bAttackerVictory);

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
		if (Siege->Phase == ECoMSiegePhase::Approach && Siege->TurnsUnderSiege >= 2)
		{
			Siege->Phase = ECoMSiegePhase::Encirclement;
			OnSiegePhaseChanged.Broadcast(SiegeID, ECoMSiegePhase::Encirclement);
		}
		else if (Siege->Phase == ECoMSiegePhase::Encirclement && Siege->TurnsUnderSiege >= 4)
		{
			Siege->Phase = ECoMSiegePhase::Bombardment;
			OnSiegePhaseChanged.Broadcast(SiegeID, ECoMSiegePhase::Bombardment);
		}

		// Bombardment: siege equipment damages walls
		if (Siege->Phase == ECoMSiegePhase::Bombardment)
		{
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
				Siege->Phase = ECoMSiegePhase::Breakthrough;
				OnSiegePhaseChanged.Broadcast(SiegeID, ECoMSiegePhase::Breakthrough);

				if (Siege->WallHP <= 0)
				{
					const int32 BreachSeg = Siege->TurnsUnderSiege;
					Siege->BreachPoints.Add(FIntPoint(BreachSeg, 0));
					OnWallBreached.Broadcast(SiegeID, BreachSeg);
				}
				UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: BREACH! WallHP=%d GateHP=%d"),
					SiegeID, Siege->WallHP, Siege->GatehouseHP);
			}
		}

		// Starvation
		const int32 FoodLoss = Siege->bSupplyLinesCut
			? CoM::STARVATION_RATE_CUT
			: CoM::STARVATION_RATE_NORMAL;
		Siege->CityFoodReserves = FMath::Max(0, Siege->CityFoodReserves - FoodLoss);

		if (Siege->CityFoodReserves <= 0)
		{
			if (Siege->Phase != ECoMSiegePhase::Assault && Siege->Phase != ECoMSiegePhase::Surrender)
			{
				Siege->Phase = ECoMSiegePhase::Assault;
				OnSiegePhaseChanged.Broadcast(SiegeID, ECoMSiegePhase::Assault);
			}
			// Morale drops when starving
			Siege->CityMorale = FMath::Max(0, Siege->CityMorale - CoM::MORALE_DECAY_RATE);
		}

		// Auto-surrender at morale 0
		if (Siege->CityMorale <= 0)
		{
			Siege->Phase = ECoMSiegePhase::Surrender;
			OnSiegeSurrender.Broadcast(SiegeID);
			UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: city morale 0, auto-surrender"), SiegeID);
			EndSiege(SiegeID, true);
			continue;
		}

		// Morale decay from siege duration
		if (Siege->TurnsUnderSiege > CoM::MORALE_DECAY_THRESHOLD_TURNS)
		{
			Siege->CityMorale = FMath::Max(0, Siege->CityMorale - 1);
		}
	}
}

const FCoMSiegeState* UCoMSiegeSubsystem::GetSiege(int32 SiegeID) const
{
	return AllSieges.Find(SiegeID);
}

TArray<FCoMSiegeState> UCoMSiegeSubsystem::GetAllSieges() const
{
	TArray<FCoMSiegeState> Result;
	AllSieges.GenerateValueArray(Result);
	return Result;
}

bool UCoMSiegeSubsystem::IsCityUnderSiege(int32 CityID) const
{
	for (const auto& Pair : AllSieges)
	{
		if (Pair.Value.DefenderCityID == CityID)
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

	Siege->Phase = ECoMSiegePhase::Assault;
	OnSiegePhaseChanged.Broadcast(SiegeID, ECoMSiegePhase::Assault);

	// Determine defender advantage multiplier
	FFixed64 DefenderMult = FFixed64(1);

	if (Siege->BreachPoints.Num() == 0 && Siege->GatehouseHP > 0)
	{
		DefenderMult = FFixed64(3);
	}
	else if (Siege->BreachPoints.Num() == 0)
	{
		if (Siege->SiegeEquipmentIDs.Num() > 2)
		{
			DefenderMult = FFixed64::FromRaw(98304); // 1.5
		}
		else
		{
			DefenderMult = FFixed64::FromRaw(163840); // 2.5
		}
	}
	else
	{
		DefenderMult = FFixed64(1);
	}

	UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: assault attempted! Defender mult=%d, breaches=%d"),
		SiegeID, DefenderMult.ToInt32(), Siege->BreachPoints.Num());

	// TODO: trigger tactical combat with DefenderMult applied to defender stats
}

bool UCoMSiegeSubsystem::OfferSurrender(int32 SiegeID)
{
	FCoMSiegeState* Siege = AllSieges.Find(SiegeID);
	if (!Siege)
	{
		return false;
	}

	// Surrender acceptance based on morale and food
	int32 AcceptChance = 0;

	if (Siege->CityMorale < 10)
	{
		AcceptChance = 80;
	}
	else if (Siege->CityMorale < 20 && Siege->CityFoodReserves < 5)
	{
		AcceptChance = 50;
	}
	else if (Siege->CityMorale < 30 && Siege->CityFoodReserves <= 0)
	{
		AcceptChance = 30;
	}

	// Roll
	int32 Roll = RngStream.RandRange(0, 99);
	const bool bAccepted = Roll < AcceptChance;

	UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: surrender offered. Morale=%d, Food=%d, Chance=%d%%, %s"),
		SiegeID, Siege->CityMorale, Siege->CityFoodReserves, AcceptChance,
		bAccepted ? TEXT("ACCEPTED") : TEXT("REJECTED"));

	if (bAccepted)
	{
		Siege->Phase = ECoMSiegePhase::Surrender;
		OnSiegeSurrender.Broadcast(SiegeID);
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
		UE_LOG(LogTemp, Log, TEXT("[Siege] Siege %d: supply lines CUT -- starvation accelerated"), SiegeID);
	}
}

// ---------------------------------------------------------------------------
// Private helpers (stubs for future wiring)
// ---------------------------------------------------------------------------

void UCoMSiegeSubsystem::ProcessBombardment(FCoMSiegeState& Siege)
{
	// Handled inline in ProcessSiegeTurn for now.
}

void UCoMSiegeSubsystem::ProcessStarvation(FCoMSiegeState& Siege)
{
	// Handled inline in ProcessSiegeTurn for now.
}

void UCoMSiegeSubsystem::UpdatePhase(FCoMSiegeState& Siege)
{
	// Handled inline in ProcessSiegeTurn for now.
}

FFixed64 UCoMSiegeSubsystem::GetWeatherBombardmentFactor(int32 CityID) const
{
	// TODO: query weather subsystem
	return FFixed64(1);
}

bool UCoMSiegeSubsystem::IsAssaultBlockedByWeather(int32 CityID) const
{
	// TODO: query weather subsystem for thunderstorms
	return false;
}

bool UCoMSiegeSubsystem::HasSiegeTowers(const FCoMSiegeState& Siege) const
{
	// TODO: check equipment types when UnitSubsystem is wired
	return Siege.SiegeEquipmentIDs.Num() > 2;
}

void UCoMSiegeSubsystem::InitializeWallHP(FCoMSiegeState& Siege, int32 CityID)
{
	// TODO: query CitySubsystem for wall level
	Siege.WallMaxHP = CoM::DEFAULT_WALL_HP;
	Siege.WallHP = Siege.WallMaxHP;
	Siege.GatehouseHP = Siege.WallMaxHP / 2;
}
