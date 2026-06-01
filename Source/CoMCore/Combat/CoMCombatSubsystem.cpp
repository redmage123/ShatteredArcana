// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMCombatSubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Data/CoMUnitDatabase.h"
#include "CoMCore/Framework/CoMGameInstance.h"
#include "CoMCore/Items/CoMItemSubsystem.h"
#include "CoMCore/TacticalCombat/CoMTacticalCombatSubsystem.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"
#include "Kismet/GameplayStatics.h"

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

void UCoMCombatSubsystem::ReseedRandom(int32 MasterSeed)
{
	// Default Initialize() uses FPlatformTime::Cycles() (non-reproducible);
	// this gives the playtest a deterministic, seed-varying combat stream.
	RngStream.Initialize(MasterSeed ^ 0x436F6D62);
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

	UCoMUnitSubsystem* UnitSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>() : nullptr;
	if (!UnitSub)
	{
		return Result;
	}

	const FCoMArmyGroup* Army = UnitSub->GetArmy(ArmyID);
	if (!Army)
	{
		return Result;
	}

	for (const int32 UnitID : Army->UnitIDs)
	{
		const FCoMUnitInstance* UnitInst = UnitSub->GetUnit(UnitID);
		if (!UnitInst)
		{
			continue;
		}

		FCoMCombatUnitState CombatUnit;
		CombatUnit.UnitID        = UnitID;
		CombatUnit.OwnerWizardID = UnitInst->OwnerWizardIndex;
		CombatUnit.bIsHero       = UnitInst->bIsHero;

		// Pull real stats from the unit database
		if (CoMUnitDatabase::Contains(UnitInst->SpecID))
		{
			const FCoMUnitSpecInfo& Spec = CoMUnitDatabase::GetUnitSpec(UnitInst->SpecID);
			CombatUnit.MeleeAttack       = FFixed64(Spec.MeleeAttack);
			CombatUnit.RangedAttack      = FFixed64(Spec.RangedAttack);
			CombatUnit.Defense           = FFixed64(Spec.Defense);
			CombatUnit.HitPoints         = UnitInst->CurrentHP;
			CombatUnit.MaxHP             = Spec.HitPoints;
			CombatUnit.FiguresRemaining  = Spec.Figures;
			CombatUnit.bIsRanged         = (Spec.RangedAttack > 0);
		}
		else
		{
			// Fallback to unit instance data with defaults
			CombatUnit.MeleeAttack       = FFixed64(3);
			CombatUnit.RangedAttack      = FFixed64(0);
			CombatUnit.Defense           = FFixed64(3);
			CombatUnit.HitPoints         = UnitInst->CurrentHP;
			CombatUnit.MaxHP             = UnitInst->MaxHP;
			CombatUnit.FiguresRemaining  = 1;
			CombatUnit.bIsRanged         = false;
		}

		// Fold equipped magical items onto the hero's combat stats.
		if (CombatUnit.bIsHero)
		{
			if (UCoMItemSubsystem* Items = GetGameInstance()->GetSubsystem<UCoMItemSubsystem>())
			{
				const TArray<FCoMItemInstance> Equipped = Items->GetHeroEquipment(UnitID);
				int32 AtkBonus = 0;
				int32 DefBonus = 0;
				int32 HPBonus  = 0;
				for (const FCoMItemInstance& Item : Equipped)
				{
					for (const FCoMItemPower& P : Item.Powers)
					{
						if (P.Type == ECoMItemPowerType::StatBonus)
						{
							if      (P.Key == TEXT("Attack"))  AtkBonus += P.Magnitude;
							else if (P.Key == TEXT("Defense")) DefBonus += P.Magnitude;
							else if (P.Key == TEXT("HP"))      HPBonus  += P.Magnitude;
						}
						else if (P.Type == ECoMItemPowerType::GrantSkill)
						{
							CombatUnit.ItemSkills.AddUnique(P.Key);
						}
					}
				}
				if (AtkBonus != 0)
				{
					CombatUnit.MeleeAttack  = CombatUnit.MeleeAttack  + FFixed64(AtkBonus);
					if (CombatUnit.bIsRanged)
					{
						CombatUnit.RangedAttack = CombatUnit.RangedAttack + FFixed64(AtkBonus);
					}
				}
				if (DefBonus != 0)
				{
					CombatUnit.Defense = CombatUnit.Defense + FFixed64(DefBonus);
				}
				if (HPBonus != 0)
				{
					CombatUnit.MaxHP    += HPBonus;
					CombatUnit.HitPoints = FMath::Min(CombatUnit.HitPoints + HPBonus, CombatUnit.MaxHP);
				}
			}
		}

		Result.Add(CombatUnit);
	}

	return Result;
}

// =====================================================================
// Encounter Detection
// =====================================================================

TArray<FIntPoint> UCoMCombatSubsystem::DetectEncounters() const
{
	TArray<FIntPoint> Encounters;

	UCoMUnitSubsystem* UnitSub = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>()
		: nullptr;
	if (!UnitSub) return Encounters;

	// Bucket armies by (Plane, Layer, Position) so we only compare within
	// the same tile. Different wizards on the same tile = hostile encounter.
	// Encoded as int64: (PlaneByte << 56) | (LayerByte << 48) | (X << 24) | Y.
	struct FArmyRef
	{
		int32 ArmyID    = -1;
		int32 WizardIdx = -1;
		int32 UnitCount = 0;
	};
	TMap<int64, TArray<FArmyRef>> Buckets;

	int32 TotalArmies = 0;
	for (const auto& Pair : UnitSub->GetAllArmies())
	{
		++TotalArmies;
		const FCoMArmyGroup& Army = Pair.Value;
		// Skip empty stacks (e.g. mid-disband or just-spawned settlers).
		if (Army.UnitIDs.Num() == 0) continue;
		const int64 Key =
			(static_cast<int64>(static_cast<uint8>(Army.Plane)) << 56) |
			(static_cast<int64>(static_cast<uint8>(Army.Layer)) << 48) |
			(static_cast<int64>(Army.Position.X & 0xFFFFFF) << 24) |
			(static_cast<int64>(Army.Position.Y & 0xFFFFFF));
		FArmyRef Ref;
		Ref.ArmyID    = Pair.Key;
		Ref.WizardIdx = Army.OwnerWizardIndex;
		Ref.UnitCount = Army.UnitIDs.Num();
		Buckets.FindOrAdd(Key).Add(Ref);
	}

	// Emit one encounter per hostile pair sharing a tile. Lower-ArmyID is
	// the attacker by convention (matches the StartTacticalBattle expectation).
	int32 SharedTiles = 0;
	for (const auto& KV : Buckets)
	{
		const TArray<FArmyRef>& Bucket = KV.Value;
		if (Bucket.Num() < 2) continue;
		++SharedTiles;
		for (int32 i = 0; i < Bucket.Num(); ++i)
		{
			for (int32 j = i + 1; j < Bucket.Num(); ++j)
			{
				if (Bucket[i].WizardIdx == Bucket[j].WizardIdx) continue;
				const int32 A = FMath::Min(Bucket[i].ArmyID, Bucket[j].ArmyID);
				const int32 B = FMath::Max(Bucket[i].ArmyID, Bucket[j].ArmyID);
				Encounters.Add(FIntPoint(A, B));
			}
		}
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("DetectEncounters: armies=%d, shared-tiles=%d, encounters=%d"),
		TotalArmies, SharedTiles, Encounters.Num());

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
		const int32 AttackerArmyID = Pair.X;
		const int32 DefenderArmyID = Pair.Y;

		// Determine wizard ownership of each army.
		int32 AttackerWizard = CoM::WIZARD_INDEX_NONE;
		int32 DefenderWizard = CoM::WIZARD_INDEX_NONE;

		if (UCoMUnitSubsystem* UnitSub = GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>())
		{
			if (const FCoMArmyGroup* AtkArmy = UnitSub->GetArmy(AttackerArmyID))
			{
				AttackerWizard = AtkArmy->OwnerWizardIndex;
			}
			if (const FCoMArmyGroup* DefArmy = UnitSub->GetArmy(DefenderArmyID))
			{
				DefenderWizard = DefArmy->OwnerWizardIndex;
			}
		}

		// If the human player is involved, open the tactical combat map.
		const bool bPlayerInvolved =
			(HumanPlayerWizardIndex >= 0) &&
			(AttackerWizard == HumanPlayerWizardIndex || DefenderWizard == HumanPlayerWizardIndex);

		if (bPlayerInvolved)
		{
			// Store pending battle state for the pre-battle choice UI.
			PendingAttackerArmyID = AttackerArmyID;
			PendingDefenderArmyID = DefenderArmyID;
			PendingAttackerWizard = AttackerWizard;
			PendingDefenderWizard = DefenderWizard;

			// Broadcast so the UI can show the pre-battle popup.
			OnPreBattleChoice.Broadcast(AttackerArmyID, DefenderArmyID);

			// Only one tactical battle at a time — remaining encounters auto-resolve next turn.
			return;
		}

		// AI-vs-AI: try the tactical headless sim first (proper grid combat
		// driven by the tactical AI). Falls back to round-robin auto-combat
		// if the tactical subsystem isn't available or returns no winner.
		FCoMCombatResult Result;
		bool bResolved = false;
		if (UCoMTacticalCombatSubsystem* Tac =
			GetGameInstance()->GetSubsystem<UCoMTacticalCombatSubsystem>())
		{
			FCoMCombatContext Ctx;
			Ctx.ParticipatingArmyGroupIDs.Add(AttackerArmyID);
			Ctx.ParticipatingArmyGroupIDs.Add(DefenderArmyID);
			Ctx.AttackerWizardIndex = AttackerWizard;
			Ctx.DefenderWizardIndex = DefenderWizard;
			// Headless sim needs a non-empty ReturnMapName for IsValid(). We
			// never actually open the tactical map in this branch, so any
			// sentinel works — use the currently-loaded map name.
			Ctx.ReturnMapName = FName(TEXT("HeadlessSim"));
			if (UCoMUnitSubsystem* UnitSub2 = GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>())
			{
				if (const FCoMArmyGroup* Atk = UnitSub2->GetArmy(AttackerArmyID))
				{
					Ctx.OriginTile = Atk->Position;
					Ctx.OriginPlane = Atk->Plane;
				}
			}
			if (Ctx.IsValid())
			{
				Result = Tac->RunHeadlessBattle(Ctx, /*MaxRounds=*/30);
				bResolved = (Result.WinnerWizardID != INDEX_NONE)
					|| Result.AttackerCasualties.Num() > 0
					|| Result.DefenderCasualties.Num() > 0;
			}
		}
		if (!bResolved)
		{
			Result = ResolveAutoCombat(AttackerArmyID, DefenderArmyID, CurrentTurn);
		}
		OnCombatResolved.Broadcast(Result);
	}
}

void UCoMCombatSubsystem::StartTacticalBattle(
	int32 AttackerArmyID, int32 DefenderArmyID,
	int32 AttackerWizard, int32 DefenderWizard)
{
	UCoMGameInstance* GI = Cast<UCoMGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("StartTacticalBattle: No UCoMGameInstance."));
		return;
	}

	// Populate CombatContext.
	FCoMCombatContext& Ctx = GI->CombatContext;
	Ctx.Reset();
	Ctx.ParticipatingArmyGroupIDs.Add(AttackerArmyID);
	Ctx.ParticipatingArmyGroupIDs.Add(DefenderArmyID);
	Ctx.AttackerWizardIndex = AttackerWizard;
	Ctx.DefenderWizardIndex = DefenderWizard;

	// Look up the army's position for OriginTile.
	if (UCoMUnitSubsystem* UnitSub = GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>())
	{
		if (const FCoMArmyGroup* AtkArmy = UnitSub->GetArmy(AttackerArmyID))
		{
			Ctx.OriginTile = AtkArmy->Position;
			Ctx.OriginPlane = AtkArmy->Plane;
		}
	}

	// Return map: the currently loaded level.
	Ctx.ReturnMapName = FName(*GI->GetWorld()->GetMapName());

	// Open the tactical combat map.
	UE_LOG(LogTemp, Log, TEXT("Transitioning to tactical combat: %s (Atk wizard %d vs Def wizard %d)"),
	       *TacticalCombatMapName.ToString(), AttackerWizard, DefenderWizard);

	UGameplayStatics::OpenLevel(GI->GetWorld(), TacticalCombatMapName);
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

	for (FCoMCombatUnitState& Attacker : Attackers)
	{
		if (Attacker.HitPoints <= 0)
		{
			continue;
		}

		// Item-skill quick lookup table (constant per attacker per round).
		const bool bFlameBlade     = Attacker.ItemSkills.Contains(FName(TEXT("flame_blade")));
		const bool bFrostBlade     = Attacker.ItemSkills.Contains(FName(TEXT("frost_blade")));
		const bool bLightningBlade = Attacker.ItemSkills.Contains(FName(TEXT("lightning_blade")));
		const bool bHolyAvenger    = Attacker.ItemSkills.Contains(FName(TEXT("holy_avenger")));
		const bool bVampiric       = Attacker.ItemSkills.Contains(FName(TEXT("vampiric")));
		const bool bStoningTouch   = Attacker.ItemSkills.Contains(FName(TEXT("stoning_touch")));
		const bool bDestruction    = Attacker.ItemSkills.Contains(FName(TEXT("destruction")));

		// MoM convention: blade-type skills don't stack; pick the strongest source.
		// Each blade adds +1 damage per unblocked hit; we cap to one bonus.
		const bool bAnyBlade = bFlameBlade || bFrostBlade || bLightningBlade;
		const int32 PerHitBonus = (bAnyBlade ? 1 : 0) + (bHolyAvenger ? 1 : 0);

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

		// Lightning Blade halves armor for block purposes.
		if (bLightningBlade)
		{
			BlockChancePct = BlockChancePct * FFixed64(0.5f);
		}
		int32 BlockChanceInt = FMath::Clamp(static_cast<int32>(BlockChancePct.ToInt32()), 0, 90);

		// Roll each attack die. Track damage and instant-kill triggers.
		int32 TotalDamage = 0;
		bool bInstantKill = false;
		for (int32 Die = 0; Die < AttackDice; ++Die)
		{
			const int32 AttackRoll = RngStream.RandRange(0, 99);
			if (AttackRoll < HitChanceInt)
			{
				// Hit — now defender rolls to block.
				const int32 DefenseRoll = RngStream.RandRange(0, 99);
				if (DefenseRoll >= BlockChanceInt)
				{
					// Unblocked hit — 1 HP base + per-hit skill bonuses.
					TotalDamage += 1 + PerHitBonus;

					// Stoning Touch / Destruction: per-hit chance to wipe the target.
					if (bDestruction && RngStream.RandRange(0, 99) < 5)       { bInstantKill = true; }
					else if (bStoningTouch && RngStream.RandRange(0, 99) < 10){ bInstantKill = true; }
				}
			}
		}

		if (bInstantKill)
		{
			TotalDamage = Target.HitPoints; // reduce target to 0
		}

		// Apply damage.
		Target.HitPoints -= TotalDamage;

		// Reduce figures proportionally (simplified: each figure = 1 HP).
		if (Target.HitPoints < Target.FiguresRemaining)
		{
			Target.FiguresRemaining = FMath::Max(0, Target.HitPoints);
		}

		// Vampiric: heal the attacker for half the damage dealt (rounded down),
		// capped at MaxHP. Lifesteal only triggers on real damage, not overkill.
		if (bVampiric && TotalDamage > 0)
		{
			const int32 Heal = TotalDamage / 2;
			if (Heal > 0)
			{
				Attacker.HitPoints = FMath::Min(Attacker.HitPoints + Heal, Attacker.MaxHP);
			}
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

// =====================================================================
// Quick-Resolve Battle (army power + random roll, no tactical map)
// =====================================================================

FCoMCombatResult UCoMCombatSubsystem::QuickResolveBattle(int32 AttackerArmyId, int32 DefenderArmyId)
{
	// Use the full auto-combat engine for a faithful result.
	// The current turn is approximated from the game state.
	int32 CurrentTurn = 0;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
		{
			CurrentTurn = TurnSub->GetCurrentTurn();
		}
	}

	FCoMCombatResult Result = ResolveAutoCombat(AttackerArmyId, DefenderArmyId, CurrentTurn);
	Result.bAutoResolved = true;

	UE_LOG(LogTemp, Log, TEXT("QuickResolveBattle: Army %d vs Army %d — Winner wizard %d, %d rounds"),
		AttackerArmyId, DefenderArmyId, Result.WinnerWizardID, Result.CombatRounds);

	OnCombatResolved.Broadcast(Result);
	return Result;
}

// =====================================================================
// Resolve Pending Battle (called by UI after player chooses)
// =====================================================================

void UCoMCombatSubsystem::ResolvePendingBattle(bool bAutoResolve)
{
	if (PendingAttackerArmyID == INDEX_NONE || PendingDefenderArmyID == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResolvePendingBattle: No pending battle."));
		return;
	}

	const int32 AtkID = PendingAttackerArmyID;
	const int32 DefID = PendingDefenderArmyID;
	const int32 AtkWiz = PendingAttackerWizard;
	const int32 DefWiz = PendingDefenderWizard;

	// Clear pending state.
	PendingAttackerArmyID = INDEX_NONE;
	PendingDefenderArmyID = INDEX_NONE;
	PendingAttackerWizard = INDEX_NONE;
	PendingDefenderWizard = INDEX_NONE;

	if (bAutoResolve)
	{
		QuickResolveBattle(AtkID, DefID);
	}
	else
	{
		StartTacticalBattle(AtkID, DefID, AtkWiz, DefWiz);
	}
}
