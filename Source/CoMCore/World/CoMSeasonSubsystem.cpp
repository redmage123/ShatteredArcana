// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMSeasonSubsystem.h"

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMSeasonSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BuildDefaultConfigs();
	InitializeSeasons();
}

void UCoMSeasonSubsystem::Deinitialize()
{
	PlaneSeasons.Empty();
	PlaneConfigs.Empty();
	Super::Deinitialize();
}

// =====================================================================
// Public API
// =====================================================================

void UCoMSeasonSubsystem::InitializeSeasons()
{
	PlaneSeasons.Empty();

	for (const auto& Pair : PlaneConfigs)
	{
		FCoMSeasonalState State;
		State.CurrentSeason    = ECoMSeason::Spring;
		State.TurnsIntoSeason  = 0;
		State.TurnsPerSeason   = Pair.Value.TurnsPerYear / 4;
		PlaneSeasons.Add(Pair.Key, State);
	}
}

void UCoMSeasonSubsystem::AdvanceTurn()
{
	for (auto& Pair : PlaneSeasons)
	{
		const ECoMPlane Plane = Pair.Key;
		FCoMSeasonalState& State = Pair.Value;

		State.TurnsIntoSeason++;

		if (State.TurnsIntoSeason >= State.TurnsPerSeason)
		{
			const ECoMSeason OldSeason = State.CurrentSeason;
			State.CurrentSeason   = GetNextSeason(OldSeason);
			State.TurnsIntoSeason = 0;

			// Re-derive TurnsPerSeason in case config was hot-reloaded.
			State.TurnsPerSeason = GetTurnsPerSeason(Plane);

			OnSeasonChanged.Broadcast(Plane, OldSeason, State.CurrentSeason);
		}
	}
}

ECoMSeason UCoMSeasonSubsystem::GetCurrentSeason(ECoMPlane Plane) const
{
	if (const FCoMSeasonalState* State = PlaneSeasons.Find(Plane))
	{
		return State->CurrentSeason;
	}
	return ECoMSeason::Spring;
}

int32 UCoMSeasonSubsystem::GetTurnsIntoSeason(ECoMPlane Plane) const
{
	if (const FCoMSeasonalState* State = PlaneSeasons.Find(Plane))
	{
		return State->TurnsIntoSeason;
	}
	return 0;
}

int32 UCoMSeasonSubsystem::GetTurnsUntilNextSeason(ECoMPlane Plane) const
{
	if (const FCoMSeasonalState* State = PlaneSeasons.Find(Plane))
	{
		return FMath::Max(0, State->TurnsPerSeason - State->TurnsIntoSeason);
	}
	return 0;
}

FCoMSeasonEconomyModifiers UCoMSeasonSubsystem::GetEconomyModifiers(ECoMPlane Plane) const
{
	const ECoMSeason Season = GetCurrentSeason(Plane);

	if (const FCoMPlaneSeasonConfig* Cfg = PlaneConfigs.Find(Plane))
	{
		if (const FCoMSeasonEconomyModifiers* Mods = Cfg->SeasonModifiers.Find(Season))
		{
			return *Mods;
		}
	}

	return MakeDefaultModifiers();
}

// =====================================================================
// Internal helpers
// =====================================================================

ECoMSeason UCoMSeasonSubsystem::GetNextSeason(ECoMSeason Current)
{
	const uint8 Next = (static_cast<uint8>(Current) + 1) % 4;
	return static_cast<ECoMSeason>(Next);
}

int32 UCoMSeasonSubsystem::GetTurnsPerSeason(ECoMPlane Plane) const
{
	if (const FCoMPlaneSeasonConfig* Cfg = PlaneConfigs.Find(Plane))
	{
		return Cfg->TurnsPerYear / 4;
	}
	return 12; // fallback: 48/4
}

FCoMSeasonEconomyModifiers UCoMSeasonSubsystem::MakeDefaultModifiers()
{
	FCoMSeasonEconomyModifiers Mods;
	Mods.FoodMultiplier       = FFixed64(1, 0);
	Mods.TradeMultiplier      = FFixed64(1, 0);
	Mods.ProductionMultiplier = FFixed64(1, 0);
	Mods.bBuildingAllowed     = true;
	return Mods;
}

// =====================================================================
// Default per-plane configurations
// =====================================================================

void UCoMSeasonSubsystem::BuildDefaultConfigs()
{
	PlaneConfigs.Empty();

	auto MakeMods = [](FFixed64 Food, FFixed64 Trade, FFixed64 Production, bool bBuilding) -> FCoMSeasonEconomyModifiers
	{
		FCoMSeasonEconomyModifiers M;
		M.FoodMultiplier       = Food;
		M.TradeMultiplier      = Trade;
		M.ProductionMultiplier = Production;
		M.bBuildingAllowed     = bBuilding;
		return M;
	};

	// Shorthand for FFixed64 literals.
	const FFixed64 F0_3(0, 3000);  // 0.3
	const FFixed64 F0_5(0, 5000);  // 0.5
	const FFixed64 F1_0(1, 0);     // 1.0
	const FFixed64 F1_25(1, 2500); // 1.25
	const FFixed64 F1_5(1, 5000);  // 1.5
	const FFixed64 F2_0(2, 0);     // 2.0

	// -----------------------------------------------------------------
	// Aurelith — 48 turns/year (12/season)
	// Spring: food x1.25. Summer: baseline. Autumn: trade x1.25, food x1.5. Winter: food x0.5.
	// -----------------------------------------------------------------
	{
		FCoMPlaneSeasonConfig Cfg;
		Cfg.TurnsPerYear = 48;

		Cfg.SeasonModifiers.Add(ECoMSeason::Spring, MakeMods(F1_25, F1_0,  F1_0, true));
		Cfg.SeasonModifiers.Add(ECoMSeason::Summer, MakeMods(F1_0,  F1_0,  F1_0, true));
		Cfg.SeasonModifiers.Add(ECoMSeason::Autumn, MakeMods(F1_5,  F1_25, F1_0, true));
		Cfg.SeasonModifiers.Add(ECoMSeason::Winter, MakeMods(F0_5,  F1_0,  F1_0, true));

		PlaneConfigs.Add(ECoMPlane::Aurelith, Cfg);
	}

	// -----------------------------------------------------------------
	// Noctharion — 40 turns/year (10/season)
	// Waxing Shadow / Waning / Eclipse / Void
	// Eclipse: Death magic x1.5. Void: all magic x0.5.
	// -----------------------------------------------------------------
	{
		FCoMPlaneSeasonConfig Cfg;
		Cfg.TurnsPerYear = 40;

		Cfg.SeasonModifiers.Add(ECoMSeason::Spring, MakeMods(F1_0, F1_0, F1_0, true)); // Waxing Shadow
		Cfg.SeasonModifiers.Add(ECoMSeason::Summer, MakeMods(F1_0, F1_0, F1_0, true)); // Waning

		// Eclipse: Death magic x1.5
		{
			FCoMSeasonEconomyModifiers Eclipse = MakeMods(F1_0, F1_0, F1_0, true);
			Eclipse.MagicSchoolBonuses.Add(ECoMSpellRealm::Death, F1_5);
			Cfg.SeasonModifiers.Add(ECoMSeason::Autumn, Eclipse);
		}
		// Void: all magic x0.5
		{
			FCoMSeasonEconomyModifiers Void = MakeMods(F1_0, F1_0, F1_0, true);
			Void.MagicSchoolBonuses.Add(ECoMSpellRealm::Life,    F0_5);
			Void.MagicSchoolBonuses.Add(ECoMSpellRealm::Death,   F0_5);
			Void.MagicSchoolBonuses.Add(ECoMSpellRealm::Nature,  F0_5);
			Void.MagicSchoolBonuses.Add(ECoMSpellRealm::Chaos,   F0_5);
			Void.MagicSchoolBonuses.Add(ECoMSpellRealm::Sorcery, F0_5);
			Void.MagicSchoolBonuses.Add(ECoMSpellRealm::Glamour, F0_5);
			Cfg.SeasonModifiers.Add(ECoMSeason::Winter, Void);
		}

		PlaneConfigs.Add(ECoMPlane::Noctharion, Cfg);
	}

	// -----------------------------------------------------------------
	// Verdantis — 52 turns/year (13/season)
	// Bloom / Growth / Harvest / Dormancy
	// Bloom: Nature magic x1.5, food x1.5. Dormancy: food x0.3.
	// -----------------------------------------------------------------
	{
		FCoMPlaneSeasonConfig Cfg;
		Cfg.TurnsPerYear = 52;

		// Bloom
		{
			FCoMSeasonEconomyModifiers Bloom = MakeMods(F1_5, F1_0, F1_0, true);
			Bloom.MagicSchoolBonuses.Add(ECoMSpellRealm::Nature, F1_5);
			Cfg.SeasonModifiers.Add(ECoMSeason::Spring, Bloom);
		}
		Cfg.SeasonModifiers.Add(ECoMSeason::Summer, MakeMods(F1_0, F1_0, F1_0, true)); // Growth
		Cfg.SeasonModifiers.Add(ECoMSeason::Autumn, MakeMods(F1_0, F1_0, F1_0, true)); // Harvest

		// Dormancy
		Cfg.SeasonModifiers.Add(ECoMSeason::Winter, MakeMods(F0_3, F1_0, F1_0, true));

		PlaneConfigs.Add(ECoMPlane::Verdantis, Cfg);
	}

	// -----------------------------------------------------------------
	// Infernyx — 36 turns/year (9/season)
	// Eruption / Smolder / Dormant / Ember
	// Eruption: Chaos magic x2.0, bBuildingAllowed=false. Dormant: production x1.5.
	// -----------------------------------------------------------------
	{
		FCoMPlaneSeasonConfig Cfg;
		Cfg.TurnsPerYear = 36;

		// Eruption
		{
			FCoMSeasonEconomyModifiers Eruption = MakeMods(F1_0, F1_0, F1_0, /*bBuilding=*/false);
			Eruption.MagicSchoolBonuses.Add(ECoMSpellRealm::Chaos, F2_0);
			Cfg.SeasonModifiers.Add(ECoMSeason::Spring, Eruption);
		}
		Cfg.SeasonModifiers.Add(ECoMSeason::Summer, MakeMods(F1_0, F1_0, F1_0, true)); // Smolder

		// Dormant: production x1.5
		Cfg.SeasonModifiers.Add(ECoMSeason::Autumn, MakeMods(F1_0, F1_0, F1_5, true));

		Cfg.SeasonModifiers.Add(ECoMSeason::Winter, MakeMods(F1_0, F1_0, F1_0, true)); // Ember

		PlaneConfigs.Add(ECoMPlane::Infernyx, Cfg);
	}

	// -----------------------------------------------------------------
	// Aethermist — 44 turns/year (11/season)
	// Convergence / Drift / Null / Resonance
	// Convergence: Sorcery magic x1.5, trade (portal cost proxy) x0.5. Null: all magic x0.5.
	// -----------------------------------------------------------------
	{
		FCoMPlaneSeasonConfig Cfg;
		Cfg.TurnsPerYear = 44;

		// Convergence
		{
			FCoMSeasonEconomyModifiers Conv = MakeMods(F1_0, F0_5, F1_0, true);
			Conv.MagicSchoolBonuses.Add(ECoMSpellRealm::Sorcery, F1_5);
			Cfg.SeasonModifiers.Add(ECoMSeason::Spring, Conv);
		}
		Cfg.SeasonModifiers.Add(ECoMSeason::Summer, MakeMods(F1_0, F1_0, F1_0, true)); // Drift

		// Null: all magic x0.5
		{
			FCoMSeasonEconomyModifiers Null = MakeMods(F1_0, F1_0, F1_0, true);
			Null.MagicSchoolBonuses.Add(ECoMSpellRealm::Life,    F0_5);
			Null.MagicSchoolBonuses.Add(ECoMSpellRealm::Death,   F0_5);
			Null.MagicSchoolBonuses.Add(ECoMSpellRealm::Nature,  F0_5);
			Null.MagicSchoolBonuses.Add(ECoMSpellRealm::Chaos,   F0_5);
			Null.MagicSchoolBonuses.Add(ECoMSpellRealm::Sorcery, F0_5);
			Null.MagicSchoolBonuses.Add(ECoMSpellRealm::Glamour, F0_5);
			Cfg.SeasonModifiers.Add(ECoMSeason::Autumn, Null);
		}
		Cfg.SeasonModifiers.Add(ECoMSeason::Winter, MakeMods(F1_0, F1_0, F1_0, true)); // Resonance

		PlaneConfigs.Add(ECoMPlane::Aethermist, Cfg);
	}

	// -----------------------------------------------------------------
	// Abyssal — 48 turns/year (12/season)
	// Hunger / Feast / Madness / Calm
	// Feast: food x2.0. Madness: Chaos magic x2.0.
	// -----------------------------------------------------------------
	{
		FCoMPlaneSeasonConfig Cfg;
		Cfg.TurnsPerYear = 48;

		Cfg.SeasonModifiers.Add(ECoMSeason::Spring, MakeMods(F1_0, F1_0, F1_0, true)); // Hunger

		// Feast
		Cfg.SeasonModifiers.Add(ECoMSeason::Summer, MakeMods(F2_0, F1_0, F1_0, true));

		// Madness: Chaos magic x2.0
		{
			FCoMSeasonEconomyModifiers Madness = MakeMods(F1_0, F1_0, F1_0, true);
			Madness.MagicSchoolBonuses.Add(ECoMSpellRealm::Chaos, F2_0);
			Cfg.SeasonModifiers.Add(ECoMSeason::Autumn, Madness);
		}
		Cfg.SeasonModifiers.Add(ECoMSeason::Winter, MakeMods(F1_0, F1_0, F1_0, true)); // Calm

		PlaneConfigs.Add(ECoMPlane::Abyssal, Cfg);
	}

	// -----------------------------------------------------------------
	// Ethereal — 60 turns/year (15/season)
	// Thin Veil / Thick Veil / Crossing / Silence
	// Thin Veil: spirit (Life) magic x2.0.
	// -----------------------------------------------------------------
	{
		FCoMPlaneSeasonConfig Cfg;
		Cfg.TurnsPerYear = 60;

		// Thin Veil
		{
			FCoMSeasonEconomyModifiers ThinVeil = MakeMods(F1_0, F1_0, F1_0, true);
			ThinVeil.MagicSchoolBonuses.Add(ECoMSpellRealm::Life, F2_0);
			Cfg.SeasonModifiers.Add(ECoMSeason::Spring, ThinVeil);
		}
		Cfg.SeasonModifiers.Add(ECoMSeason::Summer, MakeMods(F1_0, F1_0, F1_0, true)); // Thick Veil
		Cfg.SeasonModifiers.Add(ECoMSeason::Autumn, MakeMods(F1_0, F1_0, F1_0, true)); // Crossing
		Cfg.SeasonModifiers.Add(ECoMSeason::Winter, MakeMods(F1_0, F1_0, F1_0, true)); // Silence

		PlaneConfigs.Add(ECoMPlane::Ethereal, Cfg);
	}

	// -----------------------------------------------------------------
	// Feywild — 32 turns/year (8/season)
	// Revel / Hunt / Court / Dream
	// Revel: Glamour magic x2.0. Hunt: (movement bonus handled elsewhere).
	// -----------------------------------------------------------------
	{
		FCoMPlaneSeasonConfig Cfg;
		Cfg.TurnsPerYear = 32;

		// Revel
		{
			FCoMSeasonEconomyModifiers Revel = MakeMods(F1_0, F1_0, F1_0, true);
			Revel.MagicSchoolBonuses.Add(ECoMSpellRealm::Glamour, F2_0);
			Cfg.SeasonModifiers.Add(ECoMSeason::Spring, Revel);
		}
		Cfg.SeasonModifiers.Add(ECoMSeason::Summer, MakeMods(F1_0, F1_0, F1_0, true)); // Hunt
		Cfg.SeasonModifiers.Add(ECoMSeason::Autumn, MakeMods(F1_0, F1_0, F1_0, true)); // Court
		Cfg.SeasonModifiers.Add(ECoMSeason::Winter, MakeMods(F1_0, F1_0, F1_0, true)); // Dream

		PlaneConfigs.Add(ECoMPlane::Feywild, Cfg);
	}
}
