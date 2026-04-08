// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMResourceSubsystem.h"
#include "CoMCore/CoreTypes/CoMConstants.h"

static constexpr int32 MINE_BUILD_TURNS = 3;
static constexpr int32 MINE_MAX_LEVEL   = 3;

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMResourceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AllMines.Empty();
	AllTradeRoutes.Empty();
	AllOutbreaks.Empty();
	NextMineID     = 1;
	NextRouteID    = 1;
	NextOutbreakID = 1;
	ResourceRng.Initialize(12345);
}

void UCoMResourceSubsystem::Deinitialize()
{
	AllMines.Empty();
	AllTradeRoutes.Empty();
	AllOutbreaks.Empty();
	Super::Deinitialize();
}

// =====================================================================
// ID allocation
// =====================================================================

int32 UCoMResourceSubsystem::AllocateMineID()
{
	return NextMineID++;
}

int32 UCoMResourceSubsystem::AllocateRouteID()
{
	return NextRouteID++;
}

int32 UCoMResourceSubsystem::AllocateOutbreakID()
{
	return NextOutbreakID++;
}

// =====================================================================
// Mining
// =====================================================================

int32 UCoMResourceSubsystem::BuildMine(int32 CityID, ECoMPlane Plane, FIntPoint Position)
{
	// Check if tile already has a mine.
	for (const auto& Pair : AllMines)
	{
		if (Pair.Value.Plane == Plane && Pair.Value.Position == Position)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Resource] Tile (%d,%d) already has a mine"), Position.X, Position.Y);
			return -1;
		}
	}

	const int32 MineID = AllocateMineID();

	FCoMMineData Mine;
	Mine.MineID         = MineID;
	Mine.Position       = Position;
	Mine.Plane          = Plane;
	Mine.Layer          = ECoMMapLayer::Surface;
	Mine.ResourceType   = ECoMResource::Iron; // TODO: read from tile data
	Mine.OwnerCityID    = CityID;
	Mine.MineLevel      = 0; // Under construction
	Mine.WorkersAssigned = 0;
	Mine.bExhausted     = false;
	Mine.TurnsUntilBuilt = MINE_BUILD_TURNS;

	AllMines.Add(MineID, Mine);

	UE_LOG(LogTemp, Log, TEXT("[Resource] Mine %d construction started at (%d,%d) for city %d"),
		MineID, Position.X, Position.Y, CityID);

	return MineID;
}

void UCoMResourceSubsystem::ProcessMineTurn()
{
	TArray<int32> MineIDs;
	AllMines.GetKeys(MineIDs);

	for (const int32 MineID : MineIDs)
	{
		FCoMMineData* Mine = AllMines.Find(MineID);
		if (!Mine)
		{
			continue;
		}

		// Construction phase.
		if (Mine->TurnsUntilBuilt > 0)
		{
			Mine->TurnsUntilBuilt--;
			if (Mine->TurnsUntilBuilt == 0)
			{
				Mine->MineLevel = 1;
				OnMineCompleted.Broadcast(MineID, Mine->OwnerCityID);
				UE_LOG(LogTemp, Log, TEXT("[Resource] Mine %d construction complete"), MineID);
			}
			continue;
		}

		// Skip exhausted mines.
		if (Mine->bExhausted)
		{
			continue;
		}

		// Depletion check: 2% chance per turn per mine level.
		const int32 DepletionChance = Mine->MineLevel * 2;
		if (ResourceRng.RandRange(1, 100) <= DepletionChance)
		{
			Mine->bExhausted = true;
			OnMineExhausted.Broadcast(MineID, Mine->ResourceType);
			UE_LOG(LogTemp, Log, TEXT("[Resource] Mine %d exhausted!"), MineID);
			continue;
		}

		// Deep mine breach: level 3 surface mines have 5% chance to breach into Underdark.
		if (Mine->MineLevel >= MINE_MAX_LEVEL && Mine->Layer == ECoMMapLayer::Surface)
		{
			if (CheckUnderdarkBreach(*Mine))
			{
				OnUnderdarkBreach.Broadcast(MineID, Mine->Position);
				UE_LOG(LogTemp, Warning, TEXT("[Resource] Mine %d BREACHED into Underdark at (%d,%d)!"),
					MineID, Mine->Position.X, Mine->Position.Y);
			}
		}
	}
}

const FCoMMineData* UCoMResourceSubsystem::GetMine(int32 MineID) const
{
	return AllMines.Find(MineID);
}

TArray<int32> UCoMResourceSubsystem::GetMinesForCity(int32 CityID) const
{
	TArray<int32> Result;
	for (const auto& Pair : AllMines)
	{
		if (Pair.Value.OwnerCityID == CityID)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

int32 UCoMResourceSubsystem::GetResourceYield(int32 MineID) const
{
	const FCoMMineData* Mine = AllMines.Find(MineID);
	if (!Mine || Mine->bExhausted || Mine->TurnsUntilBuilt > 0)
	{
		return 0;
	}

	// Base yield per level: level 1=2, level 2=4, level 3=7.
	int32 Yield = 0;
	switch (Mine->MineLevel)
	{
	case 1:  Yield = 2; break;
	case 2:  Yield = 4; break;
	case 3:  Yield = 7; break;
	default: Yield = Mine->MineLevel * 2; break;
	}

	// Workers bonus: +1 per worker assigned (up to mine level).
	Yield += FMath::Min(Mine->WorkersAssigned, Mine->MineLevel);

	return Yield;
}

// =====================================================================
// Trade Routes
// =====================================================================

int32 UCoMResourceSubsystem::EstablishTradeRoute(int32 SourceCityID, int32 DestCityID, ECoMResource Good)
{
	if (SourceCityID == DestCityID)
	{
		return -1;
	}

	// Check for duplicate route with same good (using ECoMTradeGood comparison).
	// NOTE: The header accepts ECoMResource but the struct stores ECoMTradeGood.
	// For now we store a default trade good. A future pass should harmonize these types.
	for (const FCoMTradeRoute& Route : AllTradeRoutes)
	{
		if ((Route.SourceCityID == SourceCityID && Route.DestCityID == DestCityID) ||
		    (Route.SourceCityID == DestCityID && Route.DestCityID == SourceCityID))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Resource] Duplicate trade route between cities %d and %d"),
				SourceCityID, DestCityID);
			return -1;
		}
	}

	const int32 RouteID = AllocateRouteID();

	FCoMTradeRoute Route;
	Route.RouteID      = RouteID;
	Route.SourceCityID = SourceCityID;
	Route.DestCityID   = DestCityID;
	Route.Good         = ECoMTradeGood::Grain; // Default; resource-to-good mapping TBD
	Route.Quantity     = 1;
	Route.RouteLength  = 10; // TODO: compute from pathfinder
	Route.bInterplanar = false; // TODO: check if cities are on different planes
	Route.RiskFactor   = FFixed64(0.05f);
	Route.GoldPerTurn  = 0; // Computed during ProcessTradeTurn
	Route.TurnsActive  = 0;
	Route.Status       = ECoMTradeRouteStatus::Active;

	AllTradeRoutes.Add(Route);

	UE_LOG(LogTemp, Log, TEXT("[Resource] Trade route %d established: city %d -> city %d"),
		RouteID, SourceCityID, DestCityID);

	return RouteID;
}

void UCoMResourceSubsystem::CancelTradeRoute(int32 RouteID)
{
	for (int32 i = AllTradeRoutes.Num() - 1; i >= 0; --i)
	{
		if (AllTradeRoutes[i].RouteID == RouteID)
		{
			AllTradeRoutes.RemoveAt(i);
			break;
		}
	}
}

void UCoMResourceSubsystem::ProcessTradeTurn()
{
	for (FCoMTradeRoute& Route : AllTradeRoutes)
	{
		Route.TurnsActive++;

		// Calculate revenue based on good type.
		const FFixed64 Revenue = ComputeTradeRevenue(Route);
		Route.GoldPerTurn = Revenue.ToInt32();

		// Raid risk check.
		const FFixed64 RaidChance = ComputeRaidChance(Route);
		const int32 RaidRoll = ResourceRng.RandRange(1, 100);
		if (RaidRoll <= RaidChance.ToInt32())
		{
			const int32 GoldLost = Route.GoldPerTurn / 2;
			Route.GoldPerTurn -= GoldLost;
			OnTradeRouteRaided.Broadcast(Route.RouteID, GoldLost, Route.RiskFactor);
			UE_LOG(LogTemp, Log, TEXT("[Resource] Trade route %d raided! Lost %d gold"),
				Route.RouteID, GoldLost);
		}

		// Disease spread: if source or dest city has an outbreak, spread chance via trade.
		for (const FCoMDiseaseOutbreak& Outbreak : AllOutbreaks)
		{
			if (Outbreak.bQuarantined)
			{
				continue;
			}

			if (Outbreak.CityID == Route.SourceCityID || Outbreak.CityID == Route.DestCityID)
			{
				const int32 TargetCity = (Outbreak.CityID == Route.SourceCityID)
					? Route.DestCityID : Route.SourceCityID;

				// Check if target already has this disease.
				bool bAlreadyInfected = false;
				for (const FCoMDiseaseOutbreak& Other : AllOutbreaks)
				{
					if (Other.CityID == TargetCity && Other.Type == Outbreak.Type)
					{
						bAlreadyInfected = true;
						break;
					}
				}

				if (!bAlreadyInfected)
				{
					// SpreadChance is FFixed64 (0-1 range); compare against random roll.
					const int32 SpreadRoll = ResourceRng.RandRange(1, 100);
					const int32 SpreadPct = FMath::Max(1, static_cast<int32>(Outbreak.SpreadChance.ToFloat() * 100.0f));
					if (SpreadRoll <= SpreadPct)
					{
						StartOutbreak(TargetCity, Outbreak.Type);
						OnDiseaseSpread.Broadcast(Outbreak.OutbreakID, Outbreak.CityID, TargetCity);
					}
				}
			}
		}
	}
}

const FCoMTradeRoute* UCoMResourceSubsystem::GetTradeRoute(int32 RouteID) const
{
	for (const FCoMTradeRoute& Route : AllTradeRoutes)
	{
		if (Route.RouteID == RouteID)
		{
			return &Route;
		}
	}
	return nullptr;
}

// =====================================================================
// Disease
// =====================================================================

int32 UCoMResourceSubsystem::StartOutbreak(int32 CityID, ECoMDiseaseType Type)
{
	// Check for existing outbreak of same type in same city.
	for (const FCoMDiseaseOutbreak& Existing : AllOutbreaks)
	{
		if (Existing.CityID == CityID && Existing.Type == Type)
		{
			return Existing.OutbreakID;
		}
	}

	const int32 OutbreakID = AllocateOutbreakID();

	FCoMDiseaseOutbreak Outbreak;
	Outbreak.OutbreakID  = OutbreakID;
	Outbreak.Type        = Type;
	Outbreak.CityID      = CityID;
	Outbreak.Severity    = 1;
	Outbreak.TurnsActive = 0;
	Outbreak.bQuarantined = false;

	// SpreadChance is FFixed64 (0-1 range).
	Outbreak.SpreadChance = GetBaseDiseaseSpreadChance(Type);

	AllOutbreaks.Add(Outbreak);

	UE_LOG(LogTemp, Warning, TEXT("[Resource] Disease outbreak %d (type %d) in city %d"),
		OutbreakID, static_cast<int32>(Type), CityID);

	return OutbreakID;
}

void UCoMResourceSubsystem::DeclareQuarantine(int32 CityID)
{
	int32 RoutesStopped = 0;

	// Quarantine all outbreaks in the city.
	for (FCoMDiseaseOutbreak& Outbreak : AllOutbreaks)
	{
		if (Outbreak.CityID == CityID)
		{
			Outbreak.bQuarantined = true;
			// Halve spread chance.
			Outbreak.SpreadChance = Outbreak.SpreadChance * FFixed64::Half();
		}
	}

	// Suspend trade routes involving this city.
	for (FCoMTradeRoute& Route : AllTradeRoutes)
	{
		if (Route.SourceCityID == CityID || Route.DestCityID == CityID)
		{
			Route.GoldPerTurn = 0;
			RoutesStopped++;
		}
	}

	OnQuarantineDeclared.Broadcast(CityID, RoutesStopped);
	UE_LOG(LogTemp, Log, TEXT("[Resource] Quarantine declared on city %d, %d routes suspended"),
		CityID, RoutesStopped);
}

void UCoMResourceSubsystem::ProcessDiseaseTurn()
{
	for (int32 i = AllOutbreaks.Num() - 1; i >= 0; --i)
	{
		FCoMDiseaseOutbreak& Outbreak = AllOutbreaks[i];
		Outbreak.TurnsActive++;

		// Severity increases over time (caps at 10).
		if (Outbreak.TurnsActive % 3 == 0 && Outbreak.Severity < 10)
		{
			Outbreak.Severity++;
		}

		// Natural recovery: at severity 1-2 with quarantine, 10% chance to resolve per turn.
		if (Outbreak.bQuarantined && Outbreak.Severity <= 2)
		{
			if (ResourceRng.RandRange(1, 100) <= 10)
			{
				UE_LOG(LogTemp, Log, TEXT("[Resource] Outbreak %d resolved naturally"), Outbreak.OutbreakID);
				AllOutbreaks.RemoveAt(i);
				continue;
			}
		}

		// Long outbreaks (20+ turns) gradually lose severity.
		if (Outbreak.TurnsActive > 20 && Outbreak.Severity > 1)
		{
			if (ResourceRng.RandRange(1, 100) <= 15)
			{
				Outbreak.Severity--;
			}
		}

		// Apply disease effects to the city.
		ApplyDiseaseEffects(Outbreak);
	}
}

TArray<FCoMDiseaseOutbreak> UCoMResourceSubsystem::GetOutbreaksForCity(int32 CityID) const
{
	TArray<FCoMDiseaseOutbreak> Result;
	for (const FCoMDiseaseOutbreak& Outbreak : AllOutbreaks)
	{
		if (Outbreak.CityID == CityID)
		{
			Result.Add(Outbreak);
		}
	}
	return Result;
}

// =====================================================================
// Internal helpers
// =====================================================================

int32 UCoMResourceSubsystem::GetBaseYield(ECoMResource Resource)
{
	switch (Resource)
	{
	case ECoMResource::Iron:       return 3;
	case ECoMResource::Coal:       return 4;
	case ECoMResource::GoldOre:    return 5;
	case ECoMResource::Gems:       return 6;
	case ECoMResource::Silver:     return 4;
	case ECoMResource::Mithril:    return 7;
	case ECoMResource::Adamantium: return 8;
	default:                       return 2;
	}
}

FFixed64 UCoMResourceSubsystem::GetLevelMultiplier(int32 MineLevel)
{
	switch (MineLevel)
	{
	case 1:  return FFixed64(1);
	case 2:  return FFixed64(1.5f);
	case 3:  return FFixed64(2);
	default: return FFixed64(1);
	}
}

FFixed64 UCoMResourceSubsystem::ComputeTradeRevenue(const FCoMTradeRoute& Route) const
{
	// Base price by good type.
	int32 BasePrice = 5;
	switch (Route.Good)
	{
	case ECoMTradeGood::Grain:              BasePrice = 3;  break;
	case ECoMTradeGood::Lumber:             BasePrice = 4;  break;
	case ECoMTradeGood::Stone:              BasePrice = 4;  break;
	case ECoMTradeGood::Steel:              BasePrice = 8;  break;
	case ECoMTradeGood::Wine:               BasePrice = 10; break;
	case ECoMTradeGood::Jewelry:            BasePrice = 15; break;
	case ECoMTradeGood::Spices:             BasePrice = 12; break;
	case ECoMTradeGood::Silk:               BasePrice = 14; break;
	case ECoMTradeGood::MagicalArtifacts:   BasePrice = 25; break;
	case ECoMTradeGood::AncientRelics:      BasePrice = 30; break;
	default:                                BasePrice = 5;  break;
	}

	// Distance factor: longer routes earn less per unit.
	FFixed64 DistFactor = FFixed64(1);
	if (Route.RouteLength > 10)
	{
		DistFactor = FFixed64(10) / FFixed64(Route.RouteLength);
	}

	// Interplanar bonus: 2x.
	const int32 PlanarMult = Route.bInterplanar ? 2 : 1;

	return FFixed64(BasePrice) * FFixed64(Route.Quantity) * DistFactor * FFixed64(PlanarMult);
}

FFixed64 UCoMResourceSubsystem::ComputeRaidChance(const FCoMTradeRoute& Route) const
{
	// 5-15% for routes through unguarded territory; interplanar routes have higher risk.
	if (Route.bInterplanar)
	{
		return FFixed64(15);
	}
	return FFixed64(5);
}

bool UCoMResourceSubsystem::CheckUnderdarkBreach(const FCoMMineData& Mine) const
{
	// 5% chance for level-3 surface mines.
	if (Mine.MineLevel >= MINE_MAX_LEVEL && Mine.Layer == ECoMMapLayer::Surface)
	{
		return ResourceRng.RandRange(1, 100) <= 5;
	}
	return false;
}

FFixed64 UCoMResourceSubsystem::GetBaseDiseaseSpreadChance(ECoMDiseaseType Type)
{
	// Returns a 0-1 range FFixed64 spread chance.
	switch (Type)
	{
	case ECoMDiseaseType::CommonPlague:  return FFixed64(0.25f);
	case ECoMDiseaseType::RedPox:        return FFixed64(0.05f);
	case ECoMDiseaseType::ShadowBlight:  return FFixed64(0.10f);
	case ECoMDiseaseType::SporeLung:     return FFixed64(0.30f);
	case ECoMDiseaseType::FireFever:     return FFixed64(0.15f);
	case ECoMDiseaseType::VoidSickness:  return FFixed64(0.08f);
	case ECoMDiseaseType::UndeathPlague: return FFixed64(0.20f);
	case ECoMDiseaseType::CursedBlood:   return FFixed64(0.15f);
	default:                             return FFixed64(0.10f);
	}
}

void UCoMResourceSubsystem::ApplyDiseaseEffects(const FCoMDiseaseOutbreak& Outbreak)
{
	// Placeholder: disease effects (population loss, production penalty) would be
	// applied via the city subsystem. Currently logged for future integration.
	if (Outbreak.Severity >= 5)
	{
		UE_LOG(LogTemp, Log, TEXT("[Resource] Disease %d severity %d in city %d - high impact"),
			Outbreak.OutbreakID, Outbreak.Severity, Outbreak.CityID);
	}
}
