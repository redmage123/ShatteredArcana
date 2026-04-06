// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMResourceSubsystem.h"
#include "CoMCore/CoreTypes/CoMConstants.h"

static constexpr int32 MAP_WIDTH = 160;
static constexpr int32 MINE_BUILD_TURNS = 3;
static constexpr int32 MINE_MAX_LEVEL = 3;

// ═══════════════════════════════════════════════════════════════════════
// Subsystem lifecycle
// ═══════════════════════════════════════════════════════════════════════

void UCoMResourceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AllMines.Empty();
	AllTradeRoutes.Empty();
	AllOutbreaks.Empty();
	NextMineID = 1;
	NextRouteID = 1;
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

// ═══════════════════════════════════════════════════════════════════════
// Mining
// ═══════════════════════════════════════════════════════════════════════

int32 UCoMResourceSubsystem::BuildMine(int32 CityID, ECoMPlane Plane, FIntPoint Position)
{
	// Check if tile already has a mine
	for (const auto& Pair : AllMines)
	{
		if (Pair.Value.Plane == Plane &&
		    Pair.Value.Position == Position)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Resource] Tile (%d,%d) already has a mine"), Position.X, Position.Y);
			return -1;
		}
	}

	const int32 MineID = NextMineID++;

	FCoMMineData Mine;
	Mine.MineID = MineID;
	Mine.Position = Position;
	Mine.Plane = Plane;
	Mine.ResourceType = ECoMResource::Iron; // TODO: read from tile data
	Mine.OwnerCityID = CityID;
	Mine.MineLevel = 0; // Under construction
	Mine.WorkersAssigned = 0;
	Mine.bExhausted = false;
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
		if (!Mine) continue;

		// Construction phase
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

		// Skip exhausted mines
		if (Mine->bExhausted) continue;

		// Extraction happens in city turn processing (yield queries)

		// Depletion check: 2% chance per turn per mine level
		const int32 DepletionChance = Mine->MineLevel * 2;
		if (ResourceRng.RandRange(1, 100) <= DepletionChance)
		{
			Mine->bExhausted = true;
			OnMineExhausted.Broadcast(MineID, Mine->ResourceType);
			UE_LOG(LogTemp, Log, TEXT("[Resource] Mine %d exhausted!"), MineID);
			continue;
		}

		// Deep mine breach: level 3 mines on surface have 5% chance to breach into Underdark
		if (Mine->MineLevel >= MINE_MAX_LEVEL)
		{
			if (ResourceRng.RandRange(1, 100) <= 5)
			{
				OnUnderdarkBreach.Broadcast(MineID, Mine->Position);
				UE_LOG(LogTemp, Warning, TEXT("[Resource] Mine %d BREACHED into Underdark at (%d,%d)!"),
					MineID, Mine->Position.X, Mine->Position.Y);
				// TODO: create portal from surface to underdark at this position
			}
		}
	}
}

const FCoMMineData* UCoMResourceSubsystem::GetMine(int32 MineID) const
{
	return AllMines.Find(MineID);
}

TArray<const FCoMMineData*> UCoMResourceSubsystem::GetMinesForCity(int32 CityID) const
{
	TArray<const FCoMMineData*> Result;
	for (const auto& Pair : AllMines)
	{
		if (Pair.Value.OwnerCityID == CityID)
		{
			Result.Add(&Pair.Value);
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

	// Base yield per level: level 1=2, level 2=4, level 3=7
	int32 Yield = 0;
	switch (Mine->MineLevel)
	{
	case 1: Yield = 2; break;
	case 2: Yield = 4; break;
	case 3: Yield = 7; break;
	default: Yield = Mine->MineLevel * 2; break;
	}

	// Workers bonus: +1 per worker assigned (up to mine level)
	Yield += FMath::Min(Mine->WorkersAssigned, Mine->MineLevel);

	return Yield;
}

// ═══════════════════════════════════════════════════════════════════════
// Trade Routes
// ═══════════════════════════════════════════════════════════════════════

int32 UCoMResourceSubsystem::EstablishTradeRoute(int32 SourceCityID, int32 DestCityID, ECoMResource Good)
{
	if (SourceCityID == DestCityID)
	{
		return -1;
	}

	// Check for duplicate route
	for (const FCoMTradeRoute& Route : AllTradeRoutes)
	{
		if ((Route.SourceCity == SourceCityID && Route.DestCity == DestCityID) ||
		    (Route.SourceCity == DestCityID && Route.DestCity == SourceCityID))
		{
			if (Route.Good == Good)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Resource] Duplicate trade route"));
				return -1;
			}
		}
	}

	const int32 RouteID = NextRouteID++;

	FCoMTradeRoute Route;
	Route.RouteID = RouteID;
	Route.SourceCity = SourceCityID;
	Route.DestCity = DestCityID;
	Route.Good = Good;
	Route.Quantity = 1; // Base quantity
	Route.RouteLength = 10; // TODO: compute from pathfinder
	Route.bInterplanar = false; // TODO: check if cities are on different planes
	Route.RiskFactor = FFixed64(0); // Computed during ProcessTradeTurn
	Route.GoldPerTurn = 0; // Computed during ProcessTradeTurn
	Route.TurnsActive = 0;

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

		// Calculate revenue
		// Base price varies by good type (simplified)
		int32 BasePrice = 5;
		switch (Route.Good)
		{
		case ECoMResource::Gold:     BasePrice = 10; break;
		case ECoMResource::Gems:     BasePrice = 15; break;
		case ECoMResource::Mithril:  BasePrice = 20; break;
		case ECoMResource::Adamantium: BasePrice = 30; break;
		case ECoMResource::Iron:     BasePrice = 5; break;
		case ECoMResource::Wood:     BasePrice = 3; break;
		case ECoMResource::Crystals: BasePrice = 12; break;
		default: BasePrice = 5; break;
		}

		// Distance factor: longer routes earn less per unit
		const float DistFactor = 1.0f / FMath::Max(1.0f, float(Route.RouteLength) / 10.0f);

		// Interplanar bonus: 2x
		const float PlanarMult = Route.bInterplanar ? 2.0f : 1.0f;

		Route.GoldPerTurn = FMath::RoundToInt32(BasePrice * Route.Quantity * DistFactor * PlanarMult);

		// Raid risk: 5-15% for routes through unguarded territory
		// Interplanar routes have higher risk
		const int32 BaseRisk = Route.bInterplanar ? 15 : 5;
		if (ResourceRng.RandRange(1, 100) <= BaseRisk)
		{
			const int32 GoldLost = Route.GoldPerTurn / 2;
			Route.GoldPerTurn -= GoldLost;
			OnTradeRouteRaided.Broadcast(Route.RouteID, GoldLost, FFixed64(BaseRisk));
			UE_LOG(LogTemp, Log, TEXT("[Resource] Trade route %d raided! Lost %d gold"), Route.RouteID, GoldLost);
		}

		// Disease spread: if source or dest city has an outbreak, spread chance via trade
		for (FCoMDiseaseOutbreak& Outbreak : AllOutbreaks)
		{
			if (Outbreak.bQuarantined) continue;

			if (Outbreak.CityID == Route.SourceCity || Outbreak.CityID == Route.DestCity)
			{
				const int32 TargetCity = (Outbreak.CityID == Route.SourceCity)
					? Route.DestCity : Route.SourceCity;

				// Check if target already has this disease
				bool bAlreadyInfected = false;
				for (const FCoMDiseaseOutbreak& Other : AllOutbreaks)
				{
					if (Other.CityID == TargetCity && Other.Type == Outbreak.Type)
					{
						bAlreadyInfected = true;
						break;
					}
				}

				if (!bAlreadyInfected && ResourceRng.RandRange(1, 100) <= Outbreak.SpreadChance)
				{
					StartOutbreak(TargetCity, Outbreak.Type);
					OnDiseaseSpread.Broadcast(Outbreak.OutbreakID, Outbreak.CityID, TargetCity);
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

TArray<const FCoMTradeRoute*> UCoMResourceSubsystem::GetRoutesForCity(int32 CityID) const
{
	TArray<const FCoMTradeRoute*> Result;
	for (const FCoMTradeRoute& Route : AllTradeRoutes)
	{
		if (Route.SourceCity == CityID || Route.DestCity == CityID)
		{
			Result.Add(&Route);
		}
	}
	return Result;
}

// ═══════════════════════════════════════════════════════════════════════
// Disease
// ═══════════════════════════════════════════════════════════════════════

int32 UCoMResourceSubsystem::StartOutbreak(int32 CityID, ECoMDiseaseType Type)
{
	// Check for existing outbreak of same type in same city
	for (const FCoMDiseaseOutbreak& Existing : AllOutbreaks)
	{
		if (Existing.CityID == CityID && Existing.Type == Type)
		{
			return Existing.OutbreakID; // Already exists
		}
	}

	const int32 OutbreakID = NextOutbreakID++;

	FCoMDiseaseOutbreak Outbreak;
	Outbreak.OutbreakID = OutbreakID;
	Outbreak.Type = Type;
	Outbreak.CityID = CityID;
	Outbreak.Severity = 1;
	Outbreak.TurnsActive = 0;
	Outbreak.bQuarantined = false;

	// Spread chance varies by disease type
	switch (Type)
	{
	case ECoMDiseaseType::Plague:         Outbreak.SpreadChance = 25; break;
	case ECoMDiseaseType::Famine:         Outbreak.SpreadChance = 5; break;
	case ECoMDiseaseType::BloodRot:       Outbreak.SpreadChance = 15; break;
	case ECoMDiseaseType::SporeBlight:    Outbreak.SpreadChance = 30; break;
	case ECoMDiseaseType::ShadowWasting:  Outbreak.SpreadChance = 10; break;
	case ECoMDiseaseType::ManaCorruption: Outbreak.SpreadChance = 20; break;
	case ECoMDiseaseType::VoidTaint:      Outbreak.SpreadChance = 8; break;
	case ECoMDiseaseType::DemonicPox:     Outbreak.SpreadChance = 20; break;
	default: Outbreak.SpreadChance = 15; break;
	}

	AllOutbreaks.Add(Outbreak);

	UE_LOG(LogTemp, Warning, TEXT("[Resource] Disease outbreak %d (type %d) in city %d"),
		OutbreakID, static_cast<int32>(Type), CityID);

	return OutbreakID;
}

void UCoMResourceSubsystem::DeclareQuarantine(int32 CityID)
{
	int32 RoutesStopped = 0;

	// Quarantine all outbreaks in the city
	for (FCoMDiseaseOutbreak& Outbreak : AllOutbreaks)
	{
		if (Outbreak.CityID == CityID)
		{
			Outbreak.bQuarantined = true;
			Outbreak.SpreadChance = FMath::Max(1, Outbreak.SpreadChance / 2); // Halve spread
		}
	}

	// Suspend trade routes involving this city
	for (FCoMTradeRoute& Route : AllTradeRoutes)
	{
		if (Route.SourceCity == CityID || Route.DestCity == CityID)
		{
			Route.GoldPerTurn = 0; // No revenue during quarantine
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

		// Severity increases over time (caps at 10)
		if (Outbreak.TurnsActive % 3 == 0 && Outbreak.Severity < 10)
		{
			Outbreak.Severity++;
		}

		// Natural recovery: at severity 1-2 with quarantine, 10% chance to resolve per turn
		if (Outbreak.bQuarantined && Outbreak.Severity <= 2)
		{
			if (ResourceRng.RandRange(1, 100) <= 10)
			{
				UE_LOG(LogTemp, Log, TEXT("[Resource] Outbreak %d resolved naturally"), Outbreak.OutbreakID);
				AllOutbreaks.RemoveAt(i);
				continue;
			}
		}

		// Long outbreaks (20+ turns) gradually lose severity
		if (Outbreak.TurnsActive > 20 && Outbreak.Severity > 1)
		{
			if (ResourceRng.RandRange(1, 100) <= 15)
			{
				Outbreak.Severity--;
			}
		}
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
