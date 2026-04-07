#include "CoMWeatherSubsystem.h"

const TArray<FCoMWeatherZone> UCoMWeatherSubsystem::EmptyZones = {};

// ---------------------------------------------------------------------------
// Subsystem lifecycle
// ---------------------------------------------------------------------------

void UCoMWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCoMWeatherSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UCoMWeatherSubsystem::InitializeWeather(int32 Seed)
{
	WeatherRng.Initialize(Seed);

	WeatherStates.Empty();

	const ECoMPlane AllPlanes[] = {
		ECoMPlane::Aurelith,
		ECoMPlane::Infernyx,
		ECoMPlane::Verdantis,
		ECoMPlane::Noctharion,
		ECoMPlane::Aethermist,
		ECoMPlane::Abyssal,
		ECoMPlane::Ethereal,
		ECoMPlane::Feywild
	};

	for (ECoMPlane Plane : AllPlanes)
	{
		FCoMWeatherState& State = WeatherStates.Add(Plane);
		State.SeasonTurnCounter = 0;
		State.CurrentSeason = ECoMSeason::Spring;
		State.ActiveZones.Empty();
	}
}

void UCoMWeatherSubsystem::ProcessWeatherTurn()
{
	for (auto& Pair : WeatherStates)
	{
		ECoMPlane Plane = Pair.Key;
		FCoMWeatherState& State = Pair.Value;

		// Advance season counter
		State.SeasonTurnCounter++;

		// Drift and age every active zone
		for (int32 i = State.ActiveZones.Num() - 1; i >= 0; --i)
		{
			FCoMWeatherZone& Zone = State.ActiveZones[i];

			// Move center
			int32 NewX = Zone.Center.X + Zone.MovementDirection.X * Zone.MovementSpeed;
			int32 NewY = Zone.Center.Y + Zone.MovementDirection.Y * Zone.MovementSpeed;

			// WrapX
			NewX = ((NewX % MAP_WIDTH) + MAP_WIDTH) % MAP_WIDTH;
			// ClampY
			NewY = FMath::Clamp(NewY, 0, MAP_HEIGHT - 1);

			Zone.Center.X = NewX;
			Zone.Center.Y = NewY;

			Zone.TurnsRemaining--;

			if (Zone.TurnsRemaining <= 0)
			{
				State.ActiveZones.RemoveAt(i);
			}
		}

		// Roll for new front spawn
		const int32 SpawnChance = GetBaseSpawnChancePercent(State.CurrentSeason);
		const int32 Roll = WeatherRng.RandRange(1, 100);
		if (Roll <= SpawnChance)
		{
			GenerateWeatherFront(Plane);
		}
	}
}

ECoMWeatherType UCoMWeatherSubsystem::GetWeatherAtTile(ECoMPlane Plane, int32 X, int32 Y) const
{
	const FCoMWeatherState* State = WeatherStates.Find(Plane);
	if (!State)
	{
		return ECoMWeatherType::Clear;
	}

	ECoMWeatherType BestType = ECoMWeatherType::Clear;
	FFixed64 BestIntensity = FFixed64(0);

	for (const FCoMWeatherZone& Zone : State->ActiveZones)
	{
		const int32 Dist = ManhattanDistWrapX(X, Y, Zone.Center.X, Zone.Center.Y);
		if (Dist <= Zone.Radius)
		{
			if (Zone.Intensity > BestIntensity)
			{
				BestIntensity = Zone.Intensity;
				BestType = Zone.Type;
			}
		}
	}

	return BestType;
}

FFixed64 UCoMWeatherSubsystem::GetWeatherIntensity(ECoMPlane Plane, int32 X, int32 Y) const
{
	const FCoMWeatherState* State = WeatherStates.Find(Plane);
	if (!State)
	{
		return FFixed64(0);
	}

	FFixed64 BestIntensity = FFixed64(0);

	for (const FCoMWeatherZone& Zone : State->ActiveZones)
	{
		const int32 Dist = ManhattanDistWrapX(X, Y, Zone.Center.X, Zone.Center.Y);
		if (Dist <= Zone.Radius)
		{
			if (Zone.Intensity > BestIntensity)
			{
				BestIntensity = Zone.Intensity;
			}
		}
	}

	return BestIntensity;
}

FCoMWeatherEffects UCoMWeatherSubsystem::GetWeatherEffects(ECoMWeatherType Type, FFixed64 Intensity)
{
	FCoMWeatherEffects Fx;

	switch (Type)
	{
	case ECoMWeatherType::Clear:
		break;

	case ECoMWeatherType::Rain:
		Fx.MovementCostDelta = 1;
		Fx.RangedAttackDelta = -1;
		Fx.SightRangeDelta   = -1;
		Fx.SpecialFlags      = FCoMWeatherEffects::Flag_RoadWash;
		break;

	case ECoMWeatherType::HeavyRain:
		Fx.MovementCostDelta = 1;
		Fx.RangedAttackDelta = -2;
		Fx.SightRangeDelta   = -2;
		Fx.SpecialFlags      = FCoMWeatherEffects::Flag_RoadWash | FCoMWeatherEffects::Flag_RiversFlood;
		break;

	case ECoMWeatherType::Thunderstorm:
		Fx.MovementCostDelta = 1;
		Fx.bFlyingGrounded   = true;
		Fx.RangedAttackDelta = -2;
		Fx.SightRangeDelta   = -2;
		Fx.SpecialFlags      = FCoMWeatherEffects::Flag_LightningStrike | FCoMWeatherEffects::Flag_RiversFlood;
		break;

	case ECoMWeatherType::Snow:
		Fx.MovementCostDelta = 1;
		Fx.RangedAttackDelta = -1;
		Fx.SightRangeDelta   = -1;
		Fx.AttritionDamagePerTurn = FFixed64(0) + (Intensity * FFixed64(1)) / FFixed64(2); // half intensity
		break;

	case ECoMWeatherType::Blizzard:
		Fx.MovementCostDelta = 2;
		Fx.RangedAttackDelta = -2;
		Fx.SightRangeDelta   = -3;
		Fx.bFlyingGrounded   = true;
		Fx.AttritionDamagePerTurn = FFixed64(1);
		break;

	case ECoMWeatherType::Fog:
		Fx.SightRangeDelta   = -2;
		Fx.RangedAttackDelta = -1;
		Fx.AmbushChanceMult  = FFixed64(2);
		break;

	case ECoMWeatherType::DenseFog:
		Fx.SightRangeDelta   = -4;
		Fx.RangedAttackDelta = -2;
		Fx.AmbushChanceMult  = FFixed64(3);
		break;

	case ECoMWeatherType::Sandstorm:
		Fx.MovementCostDelta = 1;
		Fx.RangedAttackDelta = -2;
		Fx.SightRangeDelta   = -3;
		Fx.AttritionDamagePerTurn = Intensity / FFixed64(2);
		break;

	case ECoMWeatherType::AshStorm:
		Fx.MovementCostDelta = 2;
		Fx.RangedAttackDelta = -2;
		Fx.SightRangeDelta   = -3;
		Fx.AttritionDamagePerTurn = FFixed64(1);
		break;

	case ECoMWeatherType::ManaStorm:
		Fx.MovementCostDelta = 1;
		Fx.SightRangeDelta   = -2;
		Fx.MeleeAttackDelta  = -1;
		Fx.RangedAttackDelta = -1;
		break;

	case ECoMWeatherType::SporeCloud:
		Fx.MovementCostDelta = 1;
		Fx.SightRangeDelta   = -2;
		Fx.AttritionDamagePerTurn = Intensity / FFixed64(2);
		Fx.AmbushChanceMult  = FFixed64(2); // FFixed64(1.5) if supported
		break;

	case ECoMWeatherType::ShadowVeil:
		Fx.SightRangeDelta   = -3;
		Fx.AmbushChanceMult  = FFixed64(2);
		Fx.MeleeAttackDelta  = -1;
		break;

	case ECoMWeatherType::FireRain:
		Fx.bFlyingGrounded   = true;
		Fx.RangedAttackDelta = -1;
		Fx.AttritionDamagePerTurn = FFixed64(2);
		break;

	case ECoMWeatherType::EtherealWind:
		Fx.MovementCostDelta = -1; // speeds movement
		Fx.bFlyingGrounded   = true;
		Fx.SightRangeDelta   = -1;
		break;

	case ECoMWeatherType::Drought:
		Fx.AttritionDamagePerTurn = Intensity / FFixed64(2);
		Fx.SightRangeDelta   = 1; // clear skies, better sight
		break;

	case ECoMWeatherType::VolcanicEruption:
		Fx.MovementCostDelta = 3;
		Fx.bFlyingGrounded   = true;
		Fx.RangedAttackDelta = -3;
		Fx.SightRangeDelta   = -4;
		Fx.AttritionDamagePerTurn = FFixed64(3);
		Fx.SpecialFlags      = FCoMWeatherEffects::Flag_VolcanicTileDestruction | FCoMWeatherEffects::Flag_LightningStrike;
		break;

	default:
		break;
	}

	return Fx;
}

void UCoMWeatherSubsystem::GenerateWeatherFront(ECoMPlane Plane)
{
	FCoMWeatherState* State = WeatherStates.Find(Plane);
	if (!State)
	{
		return;
	}

	ECoMWeatherType ChosenType = PickWeatherTypeForPlane(Plane);

	FCoMWeatherZone Zone;
	Zone.Type            = ChosenType;
	Zone.Center.X        = WeatherRng.RandRange(0, MAP_WIDTH - 1);
	Zone.Center.Y        = WeatherRng.RandRange(0, MAP_HEIGHT - 1);
	Zone.Radius          = WeatherRng.RandRange(5, 15);
	Zone.TurnsRemaining  = WeatherRng.RandRange(3, 8);
	Zone.Intensity       = FFixed64(WeatherRng.RandRange(50, 100)) / FFixed64(100);

	// Random drift direction: -1, 0, or 1 per axis
	Zone.MovementDirection = FVector2D(WeatherRng.RandRange(-1, 1);
	Zone.MovementDirection.Y = WeatherRng.RandRange(-1, 1);
	Zone.MovementSpeed       = WeatherRng.RandRange(0, 2);

	State->ActiveZones.Add(Zone);
}

const TArray<FCoMWeatherZone>& UCoMWeatherSubsystem::GetActiveZones(ECoMPlane Plane) const
{
	const FCoMWeatherState* State = WeatherStates.Find(Plane);
	if (!State)
	{
		return EmptyZones;
	}
	return State->ActiveZones;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int32 UCoMWeatherSubsystem::ManhattanDistWrapX(int32 X1, int32 Y1, int32 X2, int32 Y2) const
{
	int32 DX = FMath::Abs(X1 - X2);
	if (DX > MAP_WIDTH / 2)
	{
		DX = MAP_WIDTH - DX;
	}
	const int32 DY = FMath::Abs(Y1 - Y2);
	return DX + DY;
}

int32 UCoMWeatherSubsystem::GetBaseSpawnChancePercent(ECoMSeason Season) const
{
	switch (Season)
	{
	case ECoMSeason::Spring: return 30;
	case ECoMSeason::Summer: return 20;
	case ECoMSeason::Autumn: return 35;
	case ECoMSeason::Winter: return 40;
	default:                 return 25;
	}
}

ECoMWeatherType UCoMWeatherSubsystem::PickWeatherTypeForPlane(ECoMPlane Plane)
{
	const TArray<FWeightedWeather>& Table = GetPlaneWeatherTable(Plane);

	int32 TotalWeight = 0;
	for (const FWeightedWeather& Entry : Table)
	{
		TotalWeight += Entry.Weight;
	}

	int32 Roll = WeatherRng.RandRange(1, TotalWeight);
	for (const FWeightedWeather& Entry : Table)
	{
		Roll -= Entry.Weight;
		if (Roll <= 0)
		{
			return Entry.Type;
		}
	}

	return ECoMWeatherType::Clear;
}

const TArray<UCoMWeatherSubsystem::FWeightedWeather>& UCoMWeatherSubsystem::GetPlaneWeatherTable(ECoMPlane Plane)
{
	// Aurelith: temperate, classic weather
	static const TArray<FWeightedWeather> Aurelith = {
		{ ECoMWeatherType::Rain,          25 },
		{ ECoMWeatherType::HeavyRain,     15 },
		{ ECoMWeatherType::Thunderstorm,  15 },
		{ ECoMWeatherType::Snow,          15 },
		{ ECoMWeatherType::Blizzard,       5 },
		{ ECoMWeatherType::Fog,           15 },
		{ ECoMWeatherType::DenseFog,       5 },
		{ ECoMWeatherType::Drought,        5 },
	};

	// Infernyx: volcanic hellscape
	static const TArray<FWeightedWeather> Infernyx = {
		{ ECoMWeatherType::AshStorm,          30 },
		{ ECoMWeatherType::FireRain,          25 },
		{ ECoMWeatherType::VolcanicEruption,  15 },
		{ ECoMWeatherType::Drought,           15 },
		{ ECoMWeatherType::Sandstorm,         10 },
		{ ECoMWeatherType::DenseFog,           5 },
	};

	// Verdantis: overgrown jungle/swamp
	static const TArray<FWeightedWeather> Verdantis = {
		{ ECoMWeatherType::SporeCloud,    25 },
		{ ECoMWeatherType::Rain,          25 },
		{ ECoMWeatherType::HeavyRain,     20 },
		{ ECoMWeatherType::Fog,           15 },
		{ ECoMWeatherType::DenseFog,      10 },
		{ ECoMWeatherType::Thunderstorm,   5 },
	};

	// Noctharion: dark, shadowy
	static const TArray<FWeightedWeather> Noctharion = {
		{ ECoMWeatherType::ShadowVeil,    30 },
		{ ECoMWeatherType::DenseFog,      25 },
		{ ECoMWeatherType::Fog,           20 },
		{ ECoMWeatherType::Rain,          10 },
		{ ECoMWeatherType::ManaStorm,     10 },
		{ ECoMWeatherType::Thunderstorm,   5 },
	};

	// Aethermist: celestial/arcane
	static const TArray<FWeightedWeather> Aethermist = {
		{ ECoMWeatherType::ManaStorm,      30 },
		{ ECoMWeatherType::EtherealWind,   30 },
		{ ECoMWeatherType::Thunderstorm,   15 },
		{ ECoMWeatherType::Fog,            10 },
		{ ECoMWeatherType::Snow,           10 },
		{ ECoMWeatherType::Blizzard,        5 },
	};

	// Abyssal: deep ocean/abyss
	static const TArray<FWeightedWeather> Abyssal = {
		{ ECoMWeatherType::HeavyRain,     25 },
		{ ECoMWeatherType::Thunderstorm,  25 },
		{ ECoMWeatherType::DenseFog,      20 },
		{ ECoMWeatherType::ManaStorm,     15 },
		{ ECoMWeatherType::Fog,           10 },
		{ ECoMWeatherType::ShadowVeil,     5 },
	};

	// Ethereal: industrial/construct
	static const TArray<FWeightedWeather> Ethereal = {
		{ ECoMWeatherType::AshStorm,      25 },
		{ ECoMWeatherType::Sandstorm,     20 },
		{ ECoMWeatherType::DenseFog,      20 },
		{ ECoMWeatherType::Fog,           15 },
		{ ECoMWeatherType::Thunderstorm,  10 },
		{ ECoMWeatherType::ManaStorm,     10 },
	};

	// Feywild: whimsical, magical
	static const TArray<FWeightedWeather> Feywild = {
		{ ECoMWeatherType::EtherealWind,  25 },
		{ ECoMWeatherType::ManaStorm,     20 },
		{ ECoMWeatherType::Fog,           15 },
		{ ECoMWeatherType::Rain,          15 },
		{ ECoMWeatherType::SporeCloud,    10 },
		{ ECoMWeatherType::Snow,          10 },
		{ ECoMWeatherType::ShadowVeil,     5 },
	};

	switch (Plane)
	{
	case ECoMPlane::Aurelith:    return Aurelith;
	case ECoMPlane::Infernyx:    return Infernyx;
	case ECoMPlane::Verdantis:   return Verdantis;
	case ECoMPlane::Noctharion: return Noctharion;
	case ECoMPlane::Aethermist:   return Aethermist;
	case ECoMPlane::Abyssal:    return Abyssal;
	case ECoMPlane::Ethereal:    return Ethereal;
	case ECoMPlane::Feywild:     return Feywild;
	default:                     return Aurelith;
	}
}