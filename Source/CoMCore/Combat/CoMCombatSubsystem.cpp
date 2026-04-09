// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMCombatSubsystem.h"

// =====================================================================
// Fixed-point constants
// =====================================================================


namespace CoMCombatConstants
{
	static const int32    MaxCombatRounds = 50;

	// Base hit chance: (30 + attack_value) %
	static const FFixed64 BaseHitChance = FFixed64::FromRaw(1966080);

	// Defense block: (defense_value * 3) %
	static const FFixed64 DefenseBlockMultiplier = FFixed64::FromRaw(196608);

	// XP per enemy unit killed, awarded to each survivor
	static const int32    XPPerKill = 2;

	// Hero tier attack bonuses (additive to attack rolls)
	static const FFixed64 BonusAdventurer = FFixed64::FromRaw(3277);    // +5%  (0.05)
	static const FFixed64 BonusHero = FFixed64::FromRaw(6554);          // +10% (0.10)
	static const FFixed64 BonusChampion = FFixed64::FromRaw(9830);      // +15% (0.15)
	static const FFixed64 BonusDemigod = FFixed64::FromRaw(16384);      // +25% (0.25)

	// Terrain defense modifiers
	static const FFixed64 TerrainMountain = FFixed64::FromRaw(19661);   // +30% defense
	static const FFixed64 TerrainForest = FFixed64::FromRaw(6554);      // +10% defense
	static const FFixed64 TerrainHills = FFixed64::FromRaw(9830);       // +15% defense
	static const FFixed64 TerrainSwamp = FFixed64::FromRaw(-6554);      // -10% defense (unstable ground)
	static const FFixed64 TerrainDesert = FFixed64::FromRaw(3277);      // +5% defense (elevation)

	// Weather modifiers (applied to ranged attack chance)
	static const FFixed64 WeatherStorm = FFixed64::FromRaw(-19661);     // -30% ranged
	static const FFixed64 WeatherRain = FFixed64::FromRaw(-9830);       // -15% ranged
	static const FFixed64 WeatherFog = FFixed64::FromRaw(-13107);       // -20% ranged

	// Percentage constants
	static const FFixed64 Zero = FFixed64::FromRaw(0);
	static const FFixed64 One = FFixed64::FromRaw(65536);
}

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMCombatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RngStream.Initialize(FPlatformTime::Cycles());
}

void UCoMCombatSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// =====================================================================
// Terrain & Weather Modifiers (static lookups)
// =====================================================================

FFixed64 UCoMCombatSubsystem::GetTerrainModifier(ECoMTerrain TerrainType)
{
	switch (TerrainType)
	{
	case ECoMTerrain::Mountains:    return CoMCombatConstants::TerrainMountain;
	case ECoMTerrain::Forest:      return CoMCombatConstants::TerrainForest;
	case ECoMTerrain::Hills:       return CoMCombatConstants::TerrainHills;
	case ECoMTerrain::Swamp:       return CoMCombatConstants::TerrainSwamp;
	case ECoMTerrain::Desert:      return CoMCombatConstants::TerrainDesert;
	default:                       return CoMCombatConstants::Zero;
	}
}

FFixed64 UCoMCombatSubsystem::GetWeatherModifier(ECoMWeatherType WeatherType)
{
	switch (WeatherType)
	{
	// --- Standard weather ---
	case ECoMWeatherType::Clear:        return CoMCombatConstants::Zero;
	case ECoMWeatherType::Rain:         return CoMCombatConstants::WeatherRain;
	case ECoMWeatherType::HeavyRain:    return CoMCombatConstants::WeatherStorm;
	case ECoMWeatherType::Thunderstorm: return CoMCombatConstants::WeatherStorm;
	case ECoMWeatherType::Snow:         return CoMCombatConstants::WeatherRain;   // moderate visibility reduction
	case ECoMWeatherType::Blizzard:     return CoMCombatConstants::WeatherStorm;
	case ECoMWeatherType::Fog:          return CoMCombatConstants::WeatherFog;
	case ECoMWeatherType::DenseFog:     return CoMCombatConstants::WeatherFog;    // heavy visibility loss
	case ECoMWeatherType::Sandstorm:    return CoMCombatConstants::WeatherFog;
	case ECoMWeatherType::Monsoon:      return CoMCombatConstants::WeatherStorm;  // extreme conditions

	// --- No combat penalty (cosmetic / event / non-obscuring) ---
	case ECoMWeatherType::Drought:      return CoMCombatConstants::Zero;
	case ECoMWeatherType::WildHunt:     return CoMCombatConstants::Zero;          // event, not weather
	case ECoMWeatherType::BlossomGale:  return CoMCombatConstants::Zero;          // cosmetic

	// --- Infernyx plane ---
	case ECoMWeatherType::AshStorm:     return CoMCombatConstants::WeatherStorm;  // severe ash obscuration
	case ECoMWeatherType::AshFall_Inf:  return CoMCombatConstants::WeatherFog;    // ash reduces visibility
	case ECoMWeatherType::FireRain:     return CoMCombatConstants::WeatherFog;    // fire haze
	case ECoMWeatherType::SoulSmog:     return CoMCombatConstants::WeatherFog;    // smog reduces visibility
	case ECoMWeatherType::HellGust:     return CoMCombatConstants::WeatherStorm;  // ranged attacks -2

	// --- Aethermist / Ethereal planes ---
	case ECoMWeatherType::VoidRift:     return CoMCombatConstants::WeatherFog;    // spatial distortion

	// --- Verdantis plane ---
	case ECoMWeatherType::SporeCloud:   return CoMCombatConstants::WeatherFog;    // spore haze
	case ECoMWeatherType::LivingStorm:  return CoMCombatConstants::WeatherStorm;  // animate storm

	// --- Noctharion plane ---
	case ECoMWeatherType::ShadowVeil:   return CoMCombatConstants::WeatherFog;    // supernatural darkness
	case ECoMWeatherType::CrystalRain:  return CoMCombatConstants::WeatherRain;   // crystal shards

	// --- Abyssal plane ---
	case ECoMWeatherType::ChaosStorm:   return CoMCombatConstants::WeatherStorm;  // wild magic surges
	case ECoMWeatherType::BloodRain:    return CoMCombatConstants::WeatherRain;   // visibility -3
	case ECoMWeatherType::FleshGale:    return CoMCombatConstants::WeatherFog;    // morale + visibility

	// --- Feywild plane ---
	case ECoMWeatherType::FeyMist:      return CoMCombatConstants::WeatherRain;   // light obscuring
	case ECoMWeatherType::TrueNamingStorm: return CoMCombatConstants::WeatherStorm; // magical storm

	default:                            return CoMCombatConstants::Zero;
	}
}

// =====================================================================
// Hero Tier Attack Bonus
// =====================================================================

FFixed64 UCoMCombatSubsystem::GetHeroTierAttackBonus(ECoMHeroTier Tier)
{
	switch (Tier)
	{
	case ECoMHeroTier::Adventurer: return CoMCombatConstants::BonusAdventurer;
	case ECoMHeroTier::Hero:       return CoMCombatConstants::BonusHero;
	case ECoMHeroTier::Champion:   return CoMCombatConstants::BonusChampion;
	case ECoMHeroTier::Demigod:    return CoMCombatConstants::BonusDemigod;
	default:                       return CoMCombatConstants::Zero;
	}
}

FFixed64 UCoMCombatSubsystem::GetHeroCommandBonus(int32 ArmyID) const
{
	// Scan army units for the highest-tier hero.
	TArray<FCoMCombatUnitState> Units = BuildCombatUnits(ArmyID);

	FFixed64 BestBonus = CoMCombatConstants::Zero;
	for (const FCoMCombatUnitState& Unit : Units)
	{
		if (Unit.bIsHero)
		{
			FFixed64 Bonus = GetHeroTierAttackBonus(Unit.HeroTier);
			if (Bonus > BestBonus)
			{
				BestBonus = Bonus;
			}
		}
	}

	return BestBonus;
}

// =====================================================================
// Army Power Calculation
// =====================================================================

FFixed64 UCoMCombatSubsystem::CalculateArmyPower(int32 ArmyID) const
{
	TArray<FCoMCombatUnitState> Units = BuildCombatUnits(ArmyID);

	FFixed64 TotalPower = CoMCombatConstants::Zero;
	for (const FCoMCombatUnitState& Unit : Units)
	{
		// Power = (MeleeAttack + RangedAttack) * FiguresRemaining + Defense * HP
		FFixed64 AttackPower = (Unit.MeleeAttack + Unit.RangedAttack) * FFixed64(Unit.FiguresRemaining);
		FFixed64 DefPower    = Unit.Defense * FFixed64(Unit.HitPoints);
		TotalPower = TotalPower + AttackPower + DefPower;
	}

	// Add hero command bonus as a multiplier
	FFixed64 HeroBonus = GetHeroCommandBonus(ArmyID);
	if (HeroBonus > CoMCombatConstants::Zero)
	{
		TotalPower = TotalPower * (CoMCombatConstants::One + HeroBonus);
	}

	return TotalPower;
}

// =====================================================================
// Build Combat Unit Snapshots
// =====================================================================

TArray<FCoMCombatUnitState> UCoMCombatSubsystem::BuildCombatUnits(int32 ArmyID) const
{
	TArray<FCoMCombatUnitState> Result;

	// TODO: Query UCoMArmySubsystem for the unit roster of ArmyID,
	// then query UCoMUnitSubsystem for each unit's stats. For now we
	// return an empty array — callers that need test data should
	// populate FCoMCombatUnitState arrays directly.

	return Result;
}

// =====================================================================
// Encounter Detection
// =====================================================================

TArray<FIntPoint> UCoMCombatSubsystem::DetectEncounters() const
{
	TArray<FIntPoint> Encounters;

	// TODO: Query UCoMArmySubsystem for all army positions, group by tile,
	// then check ownership (different wizards = hostile). Each pair of
	// hostile armies on the same tile produces a TPair stored as FIntPoint
	// (X = AttackerArmyID, Y = DefenderArmyID). Lower-ID army is attacker
	// by convention.

	return Encounters;
}

// =====================================================================
// Resolve All Encounters
// =====================================================================

void UCoMCombatSubsystem::ResolveAllEncounters(int32 CurrentTurn)
{
	TArray<FIntPoint> Encounters = DetectEncounters();

	for (const FIntPoint& Pair : Encounters)
	{
		FCoMCombatResult Result = ResolveAutoCombat(Pair.X, Pair.Y, CurrentTurn);
		OnCombatResolved.Broadcast(Result);
	}
}

// =====================================================================
// Core Combat Resolution Algorithm
// =====================================================================

FCoMCombatResult UCoMCombatSubsystem::ResolveAutoCombat(
	int32 AttackerArmyID,
	int32 DefenderArmyID,
	int32 CurrentTurn)
{
	FCoMCombatResult Result;
	Result.bAutoResolved = true;

	// Build combat-unit snapshots for both sides.
	TArray<FCoMCombatUnitState> AttackerUnits = BuildCombatUnits(AttackerArmyID);
	TArray<FCoMCombatUnitState> DefenderUnits = BuildCombatUnits(DefenderArmyID);

	// Immediate victory if one side is empty.
	if (AttackerUnits.Num() == 0 && DefenderUnits.Num() == 0)
	{
		Result.CombatRounds = 0;
		return Result; // draw — both empty
	}
	if (DefenderUnits.Num() == 0)
	{
		Result.WinnerWizardID = AttackerUnits.Num() > 0 ? AttackerUnits[0].OwnerWizardID : INDEX_NONE;
		Result.CombatRounds   = 0;
		return Result;
	}
	if (AttackerUnits.Num() == 0)
	{
		Result.WinnerWizardID = DefenderUnits[0].OwnerWizardID;
		Result.CombatRounds   = 0;
		return Result;
	}

	// Determine modifiers.
	// TODO: look up the tile terrain from UCoMWorldMapSubsystem. For now,
	// callers can inject terrain via the direct combat methods or tests
	// can exercise ExecuteAttackRound directly.
	FFixed64 DefenseTerrainMod = CoMCombatConstants::Zero;
	FFixed64 WeatherRangedMod  = CoMCombatConstants::Zero;

	// Hero bonuses (best hero in each army).
	FFixed64 AttackerHeroBonus = CoMCombatConstants::Zero;
	FFixed64 DefenderHeroBonus = CoMCombatConstants::Zero;
	for (const FCoMCombatUnitState& U : AttackerUnits)
	{
		if (U.bIsHero)
		{
			FFixed64 B = GetHeroTierAttackBonus(U.HeroTier);
			if (B > AttackerHeroBonus) AttackerHeroBonus = B;
		}
	}
	for (const FCoMCombatUnitState& U : DefenderUnits)
	{
		if (U.bIsHero)
		{
			FFixed64 B = GetHeroTierAttackBonus(U.HeroTier);
			if (B > DefenderHeroBonus) DefenderHeroBonus = B;
		}
	}

	// Track total kills per side for XP.
	int32 AttackerKillCount = 0;
	int32 DefenderKillCount = 0;

	// === Combat loop: up to MaxCombatRounds ===
	for (int32 Round = 1; Round <= CoMCombatConstants::MaxCombatRounds; ++Round)
	{
		Result.CombatRounds = Round;

		// --- Attacker phase: attacker units attack defender units ---
		ExecuteAttackRound(AttackerUnits, DefenderUnits,
			AttackerHeroBonus, DefenseTerrainMod, WeatherRangedMod);

		// Remove dead defenders.
		TArray<int32> DeadDefenders = RemoveDeadUnits(DefenderUnits);
		Result.DefenderCasualties.Append(DeadDefenders);
		AttackerKillCount += DeadDefenders.Num();

		// Check if defenders are wiped out.
		if (DefenderUnits.Num() == 0)
		{
			Result.WinnerWizardID = AttackerUnits[0].OwnerWizardID;
			break;
		}

		// --- Defender counterattack phase ---
		ExecuteAttackRound(DefenderUnits, AttackerUnits,
			DefenderHeroBonus, CoMCombatConstants::Zero, WeatherRangedMod);

		// Remove dead attackers.
		TArray<int32> DeadAttackers = RemoveDeadUnits(AttackerUnits);
		Result.AttackerCasualties.Append(DeadAttackers);
		DefenderKillCount += DeadAttackers.Num();

		// Check if attackers are wiped out.
		if (AttackerUnits.Num() == 0)
		{
			Result.WinnerWizardID = DefenderUnits[0].OwnerWizardID;
			break;
		}
	}

	// === Award XP to survivors ===
	// Each surviving unit gets XPPerKill * number of enemy units killed in battle.
	for (const FCoMCombatUnitState& U : AttackerUnits)
	{
		Result.XPGained.Add(U.UnitID, CoMCombatConstants::XPPerKill * AttackerKillCount);
	}
	for (const FCoMCombatUnitState& U : DefenderUnits)
	{
		Result.XPGained.Add(U.UnitID, CoMCombatConstants::XPPerKill * DefenderKillCount);
	}

	return Result;
}

// =====================================================================
// Execute Attack Round
// =====================================================================

void UCoMCombatSubsystem::ExecuteAttackRound(
	TArray<FCoMCombatUnitState>& Attackers,
	TArray<FCoMCombatUnitState>& Defenders,
	FFixed64 AttackHeroBonus,
	FFixed64 DefenseTerrainMod,
	FFixed64 WeatherRangedMod)
{
	if (Defenders.Num() == 0)
	{
		return;
	}

	for (const FCoMCombatUnitState& Attacker : Attackers)
	{
		if (Attacker.HitPoints <= 0)
		{
			continue;
		}

		// Select attack stat: ranged if the unit is ranged, else melee.
		FFixed64 AttackStat = Attacker.bIsRanged ? Attacker.RangedAttack : Attacker.MeleeAttack;

		// Number of attack dice = attack stat * figures remaining.
		int32 AttackDice = static_cast<int32>(
			(AttackStat * FFixed64(Attacker.FiguresRemaining)).ToInt32());

		if (AttackDice <= 0)
		{
			continue;
		}

		// Hit chance per die: (30 + attack_value) %
		// With hero bonus: multiply by (1 + hero_bonus)
		FFixed64 HitChancePct = CoMCombatConstants::BaseHitChance + AttackStat;

		// Hero bonus: adds percentage points (e.g. +10% means multiply hit chance by 1.10)
		if (AttackHeroBonus > CoMCombatConstants::Zero)
		{
			HitChancePct = HitChancePct * (CoMCombatConstants::One + AttackHeroBonus);
		}

		// Weather penalty for ranged units.
		if (Attacker.bIsRanged && WeatherRangedMod < CoMCombatConstants::Zero)
		{
			// WeatherRangedMod is negative, e.g. -0.30 for storms.
			// Apply: HitChance *= (1 + weatherMod) which reduces it.
			HitChancePct = HitChancePct * (CoMCombatConstants::One + WeatherRangedMod);
		}

		// Clamp hit chance to [1, 95].
		int32 HitChanceInt = FMath::Clamp(static_cast<int32>(HitChancePct.ToInt32()), 1, 95);

		// Choose a target defender (round-robin distribution).
		int32 TargetIndex = RngStream.RandRange(0, Defenders.Num() - 1);
		FCoMCombatUnitState& Target = Defenders[TargetIndex];

		// Defender block chance: (defense * 3) %
		// Terrain modifier: defense * (1 + terrainMod)
		FFixed64 EffectiveDefense = Target.Defense;
		if (DefenseTerrainMod != CoMCombatConstants::Zero)
		{
			EffectiveDefense = EffectiveDefense * (CoMCombatConstants::One + DefenseTerrainMod);
		}
		FFixed64 BlockChancePct = EffectiveDefense * CoMCombatConstants::DefenseBlockMultiplier;
		int32 BlockChanceInt = FMath::Clamp(static_cast<int32>(BlockChancePct.ToInt32()), 0, 90);

		// Roll each attack die.
		int32 TotalHits = 0;
		for (int32 Die = 0; Die < AttackDice; ++Die)
		{
			const int32 AttackRoll = RngStream.RandRange(0, 99);
			if (AttackRoll < HitChanceInt)
			{
				// Hit — now defender rolls to block.
				const int32 DefenseRoll = RngStream.RandRange(0, 99);
				if (DefenseRoll >= BlockChanceInt)
				{
					// Unblocked hit — 1 HP damage.
					TotalHits++;
				}
			}
		}

		// Apply damage.
		Target.HitPoints -= TotalHits;

		// Reduce figures proportionally (simplified: each figure = 1 HP).
		if (Target.HitPoints < Target.FiguresRemaining)
		{
			Target.FiguresRemaining = FMath::Max(0, Target.HitPoints);
		}
	}
}

// =====================================================================
// Remove Dead Units
// =====================================================================

TArray<int32> UCoMCombatSubsystem::RemoveDeadUnits(TArray<FCoMCombatUnitState>& Units)
{
	TArray<int32> DeadIDs;

	for (int32 i = Units.Num() - 1; i >= 0; --i)
	{
		if (Units[i].HitPoints <= 0)
		{
			DeadIDs.Add(Units[i].UnitID);
			Units.RemoveAt(i);
		}
	}

	return DeadIDs;
}
