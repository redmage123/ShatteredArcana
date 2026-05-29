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
	ResourceRng.Initialize(0x5265736F);
}

void UCoMResourceSubsystem::ReseedRandom(int32 MasterSeed)
{
	ResourceRng.Initialize(MasterSeed ^ 0x5265736F);
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

int32 UCoMResourceSubsystem::BuildMine(int32 CityID, ECoMPlane Plane, FIntPoint Position, ECoMResource TileResource)
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
	Mine.ResourceType   = TileResource;
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

int32 UCoMResourceSubsystem::BuildMineForWizard(int32 WizardId, ECoMPlane Plane,
	FIntPoint Position, ECoMResource TileResource)
{
	// Refuse if the tile already has a mine.
	for (const auto& Pair : AllMines)
	{
		if (Pair.Value.Plane == Plane && Pair.Value.Position == Position)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Resource] Tile (%d,%d) already has a mine"),
				Position.X, Position.Y);
			return -1;
		}
	}

	const int32 MineID = AllocateMineID();

	FCoMMineData Mine;
	Mine.MineID            = MineID;
	Mine.Position          = Position;
	Mine.Plane             = Plane;
	Mine.Layer             = ECoMMapLayer::Surface;
	Mine.ResourceType      = TileResource;
	Mine.OwnerCityID       = -1;       // outpost mine — no parent city
	Mine.OwnerWizardIndex  = WizardId; // owned directly by the wizard
	Mine.MineLevel         = 0;
	Mine.WorkersAssigned   = 0;
	Mine.bExhausted        = false;
	Mine.TurnsUntilBuilt   = MINE_BUILD_TURNS;

	AllMines.Add(MineID, Mine);

	UE_LOG(LogTemp, Log, TEXT("[Resource] Outpost mine %d built at (%d,%d) for wizard %d"),
		MineID, Position.X, Position.Y, WizardId);

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

bool UCoMResourceSubsystem::HasMineAt(ECoMPlane Plane, FIntPoint Position) const
{
	for (const auto& Pair : AllMines)
	{
		const FCoMMineData& M = Pair.Value;
		if (M.Plane == Plane && M.Position == Position) { return true; }
	}
	return false;
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
// Resource -> TradeGood Mapping
// =====================================================================

static ECoMTradeGood ResourceToTradeGood(ECoMResource Resource)
{
	switch (Resource)
	{
		// Metals / Ores -> Steel (refined metal goods)
		case ECoMResource::Iron:            return ECoMTradeGood::Steel;
		case ECoMResource::Coal:            return ECoMTradeGood::Steel;
		case ECoMResource::Adamantium:      return ECoMTradeGood::Steel;
		case ECoMResource::Orichalcon:      return ECoMTradeGood::Steel;
		case ECoMResource::SoulSteel:       return ECoMTradeGood::Steel;
		case ECoMResource::HellgoldAlloy:   return ECoMTradeGood::Steel;
		case ECoMResource::AshenIron:        return ECoMTradeGood::Steel;
		case ECoMResource::AbyssalIron:      return ECoMTradeGood::Steel;
		case ECoMResource::Deepstone:        return ECoMTradeGood::Stone;
		case ECoMResource::ShadowOre:        return ECoMTradeGood::Steel;
		case ECoMResource::PhaseMetal:       return ECoMTradeGood::Steel;
		case ECoMResource::AncientAlloy:     return ECoMTradeGood::Steel;

		// Precious metals / gems -> Jewelry
		case ECoMResource::GoldOre:         return ECoMTradeGood::Jewelry;
		case ECoMResource::Gems:            return ECoMTradeGood::Jewelry;
		case ECoMResource::Silver:          return ECoMTradeGood::Jewelry;
		case ECoMResource::Pearls:          return ECoMTradeGood::Jewelry;

		// Magical minerals -> AlchemicalReagents
		case ECoMResource::Mithril:         return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::Quartz:          return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::ShadowQuartz:    return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::Moonstone:       return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::LivingCrystal:   return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::Nightshade:      return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::Brimstone:       return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::EmberCrystal:    return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::Brimite:         return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::Bloodite:        return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::DemonBlood:      return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::BoneShards:      return ECoMTradeGood::AlchemicalReagents;
		case ECoMResource::PactStone:       return ECoMTradeGood::AlchemicalReagents;

		// Magical artifacts / high-magic resources -> MagicalArtifacts
		case ECoMResource::Aetherium:        return ECoMTradeGood::MagicalArtifacts;
		case ECoMResource::Voidstone:        return ECoMTradeGood::MagicalArtifacts;
		case ECoMResource::ChaosCrystal_Aby: return ECoMTradeGood::MagicalArtifacts;
		case ECoMResource::ChaosOre:         return ECoMTradeGood::MagicalArtifacts;
		case ECoMResource::VoidEssence_Aby:  return ECoMTradeGood::MagicalArtifacts;
		case ECoMResource::Thoughtstone:     return ECoMTradeGood::MagicalArtifacts;

		// Spirit / ethereal resources -> AetherDust
		case ECoMResource::SpiritGlass:      return ECoMTradeGood::AetherDust;
		case ECoMResource::Dreamweave:       return ECoMTradeGood::AetherDust;
		case ECoMResource::MemoryCrystal:    return ECoMTradeGood::AetherDust;
		case ECoMResource::SpiritDust:       return ECoMTradeGood::AetherDust;
		case ECoMResource::PhaseGlass:       return ECoMTradeGood::AetherDust;
		case ECoMResource::DreamThread:      return ECoMTradeGood::AetherDust;
		case ECoMResource::FaeDust:          return ECoMTradeGood::AetherDust;

		// Shadow / dark resources -> ShadowEssence
		case ECoMResource::AmberEssence:     return ECoMTradeGood::ShadowEssence;
		case ECoMResource::TimewornAmber:    return ECoMTradeGood::ShadowEssence;
		case ECoMResource::Glowmoss:         return ECoMTradeGood::ShadowEssence;

		// Nature / wood resources -> Lumber
		case ECoMResource::Lifewood:         return ECoMTradeGood::LivingSap;
		case ECoMResource::FeyWood:          return ECoMTradeGood::Lumber;
		case ECoMResource::Darkwood:         return ECoMTradeGood::Lumber;
		case ECoMResource::MoonbloomPetal:   return ECoMTradeGood::Spices;

		// Fire / lava resources -> MoltenGlass
		case ECoMResource::Fireglass:        return ECoMTradeGood::MoltenGlass;
		case ECoMResource::MagmaCore:        return ECoMTradeGood::MoltenGlass;

		// Animal / beast resources -> ExoticBeasts
		case ECoMResource::Horses:           return ECoMTradeGood::ExoticBeasts;

		// Underwater / coral -> AncientRelics
		case ECoMResource::DeepCoral:        return ECoMTradeGood::AncientRelics;
		case ECoMResource::PressureCrystals: return ECoMTradeGood::AncientRelics;

		// Dragon-related -> DragonScale
		// (no direct resource maps here, but keeping the pattern)

		default: return ECoMTradeGood::Grain; // fallback for unmapped resources
	}
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
	Route.Good         = ResourceToTradeGood(Good);
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
	struct FPendingOutbreak { int32 CityID; ECoMDiseaseType Type; int32 SourceOutbreakID; int32 SourceCityID; };
	TArray<FPendingOutbreak> PendingOutbreaks;

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
		PendingOutbreaks.Empty();
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
					const int32 SpreadRoll = ResourceRng.RandRange(1, 100);
					const int32 SpreadPct = FMath::Max(1, static_cast<int32>(Outbreak.SpreadChance.ToFloat() * 100.0f));
					if (SpreadRoll <= SpreadPct)
					{
						PendingOutbreaks.Add({TargetCity, Outbreak.Type, Outbreak.OutbreakID, Outbreak.CityID});
					}
				}
			}
		}
		for (const FPendingOutbreak& Pending : PendingOutbreaks)
		{
			StartOutbreak(Pending.CityID, Pending.Type);
			OnDiseaseSpread.Broadcast(Pending.SourceOutbreakID, Pending.SourceCityID, Pending.CityID);
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
	// ── Common metals (production-focused) ───────────────────────────
	case ECoMResource::Iron:       return 3;  // Iron mine: 3 production
	case ECoMResource::Coal:       return 4;  // Coal mine: 4 production (fuel bonus)

	// ── Precious metals (gold-focused) ──────────────────────────────
	case ECoMResource::GoldOre:    return 5;  // Gold mine: 5 gold
	case ECoMResource::Gems:       return 6;  // Gem mine: 6 gold
	case ECoMResource::Silver:     return 4;  // Silver mine: 4 gold

	// ── Magical materials (hybrid production + mana) ────────────────
	case ECoMResource::Mithril:    return 4;  // Mithril mine: 2 production + 2 mana (total value 4)
	case ECoMResource::Adamantium: return 5;  // Adamantium mine: 5 production

	// ── Aurelith-specific ───────────────────────────────────────────
	case ECoMResource::Quartz:     return 3;  // Crystal mine: mapped to mana

	// ── Noctharion-specific ─────────────────────────────────────────
	case ECoMResource::Orichalcon:  return 6; // Rare magical metal
	case ECoMResource::ShadowQuartz: return 4; // Shadow magic focus

	// ── Verdantis-specific ──────────────────────────────────────────
	case ECoMResource::Lifewood:    return 3;  // Living wood
	case ECoMResource::AmberEssence: return 4; // Magical amber
	case ECoMResource::Moonstone:   return 5;  // Moon-aspected crystal
	case ECoMResource::LivingCrystal: return 5; // Self-growing crystal

	// ── Infernyx-specific ───────────────────────────────────────────
	case ECoMResource::Fireglass:   return 4;  // Volcanic glass
	case ECoMResource::Brimstone:   return 3;  // Sulfur/alchemical
	case ECoMResource::MagmaCore:   return 6;  // Concentrated fire mana
	case ECoMResource::ChaosOre:    return 5;  // Chaos-infused metal

	// ── Aethermist-specific ─────────────────────────────────────────
	case ECoMResource::Aetherium:    return 7; // Pure magical resource
	case ECoMResource::SpiritGlass:  return 4; // Spirit-realm crystal
	case ECoMResource::Voidstone:    return 6; // Anti-magic stone
	case ECoMResource::Dreamweave:   return 5; // Dream-realm material

	// ── Abyssal-specific ────────────────────────────────────────────
	case ECoMResource::DemonBlood:       return 5; // Demon essence
	case ECoMResource::ChaosCrystal_Aby: return 6; // Chaotic crystal: 4 mana
	case ECoMResource::BoneShards:       return 3; // Undeath material
	case ECoMResource::VoidEssence_Aby:  return 7; // Void power

	// ── Infernyx Iron Quarter ───────────────────────────────────────
	case ECoMResource::SoulSteel:    return 6; // Soul-forged metal
	case ECoMResource::HellgoldAlloy: return 5; // Infernal alloy
	case ECoMResource::PactStone:    return 4; // Contract-bound stone
	case ECoMResource::AshenIron:    return 3; // Volcanic iron

	// ── Ethereal-specific ───────────────────────────────────────────
	case ECoMResource::MemoryCrystal: return 5; // Crystallized thought
	case ECoMResource::SpiritDust:   return 3;  // Spirit residue
	case ECoMResource::PhaseGlass:   return 4;  // Phase-shifting glass
	case ECoMResource::DreamThread:  return 4;  // Dream-realm thread

	// ── Feywild-specific ────────────────────────────────────────────
	case ECoMResource::FeyWood:       return 3; // Enchanted wood
	case ECoMResource::MoonbloomPetal: return 4; // Magical flower
	case ECoMResource::FaeDust:       return 5; // Fey essence
	case ECoMResource::TimewornAmber: return 6; // Time-aspected amber

	// ── Underdark-specific ──────────────────────────────────────────
	case ECoMResource::Deepstone:    return 3;  // Deep rock
	case ECoMResource::Glowmoss:     return 2;  // Bioluminescent
	case ECoMResource::ShadowOre:    return 4;  // Shadow metal
	case ECoMResource::EmberCrystal: return 5;  // Fire crystal
	case ECoMResource::PhaseMetal:   return 6;  // Phase-shifting metal
	case ECoMResource::Darkwood:     return 3;  // Underground wood
	case ECoMResource::Brimite:      return 4;  // Sulphuric mineral
	case ECoMResource::Thoughtstone: return 5;  // Psychic mineral
	case ECoMResource::Bloodite:     return 4;  // Blood-infused mineral
	case ECoMResource::AncientAlloy: return 7;  // Lost civilization metal

	// ── Underwater-specific ─────────────────────────────────────────
	case ECoMResource::Pearls:           return 5; // Precious
	case ECoMResource::DeepCoral:        return 3; // Building material
	case ECoMResource::AbyssalIron:      return 4; // Deep ocean iron
	case ECoMResource::PressureCrystals: return 6; // High-pressure crystals

	// ── Animals / other ─────────────────────────────────────────────
	case ECoMResource::Horses:     return 3; // Cavalry resource
	case ECoMResource::Nightshade: return 4; // Alchemical ingredient

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

bool UCoMResourceSubsystem::CheckUnderdarkBreach(const FCoMMineData& Mine)
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
