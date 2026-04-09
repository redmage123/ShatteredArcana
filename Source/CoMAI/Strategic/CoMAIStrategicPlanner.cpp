// Copyright Mythforge Studios. All Rights Reserved.
// CoMAIStrategicPlanner.cpp -- Strategic evaluation for AI wizards.

#include "Strategic/CoMAIStrategicPlanner.h"

#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"
#include "Engine/GameInstance.h"

// Radius in tiles within which an enemy army is considered a threat to a city.
static constexpr int32 THREAT_RADIUS = 8;

// Early/mid/late game thresholds (turn number).
static constexpr int32 EARLY_GAME_END   = 40;
static constexpr int32 MID_GAME_END     = 120;

// Military ratio threshold below which we are in danger.
static constexpr float DANGER_RATIO    = 0.5f;
// Military ratio above which we are dominant.
static constexpr float DOMINANT_RATIO  = 2.0f;

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------

FCoMAIStrategy UCoMAIStrategicPlanner::EvaluateStrategy(int32 WizardId, ECoMAIDifficulty Difficulty)
{
	FCoMAIStrategy Strategy;

	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : Cast<UGameInstance>(GetOuter());
	if (!GI)
	{
		// Try outer chain -- the subsystem that created us should be a game instance subsystem
		for (UObject* Obj = GetOuter(); Obj != nullptr; Obj = Obj->GetOuter())
		{
			GI = Cast<UGameInstance>(Obj);
			if (GI) break;
			if (UGameInstanceSubsystem* Sub = Cast<UGameInstanceSubsystem>(Obj))
			{
				GI = Sub->GetGameInstance();
				if (GI) break;
			}
		}
	}
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("UCoMAIStrategicPlanner::EvaluateStrategy -- No GameInstance available."));
		return Strategy;
	}

	UCoMCitySubsystem*      CitySub  = GI->GetSubsystem<UCoMCitySubsystem>();
	UCoMUnitSubsystem*      UnitSub  = GI->GetSubsystem<UCoMUnitSubsystem>();
	UCoMDiplomacySubsystem* DiploSub = GI->GetSubsystem<UCoMDiplomacySubsystem>();
	UCoMMagicSubsystem*     MagicSub = GI->GetSubsystem<UCoMMagicSubsystem>();
	UCoMTurnSubsystem*      TurnSub  = GI->GetSubsystem<UCoMTurnSubsystem>();

	const int32 CurrentTurn = TurnSub ? TurnSub->GetCurrentTurn() : 1;

	// ---- Military power ---------------------------------------------------
	const float OurPower = UnitSub ? ComputeMilitaryPower(WizardId, UnitSub) : 0.0f;
	Strategy.OurMilitaryPower = OurPower;

	// Compute strongest rival's power
	float StrongestRival = 0.0f;
	for (int32 i = 0; i < CoM::MAX_WIZARDS; ++i)
	{
		if (i == WizardId) continue;
		const float RivalPower = UnitSub ? ComputeMilitaryPower(i, UnitSub) : 0.0f;
		if (RivalPower > StrongestRival)
		{
			StrongestRival = RivalPower;
		}
	}
	Strategy.StrongestEnemyPower = StrongestRival;

	// ---- Economic strength ------------------------------------------------
	int32 CityCount = 0, GoldIncome = 0, ManaIncome = 0;
	if (CitySub)
	{
		ComputeEconomicStrength(WizardId, CitySub, CityCount, GoldIncome, ManaIncome);
	}

	// ---- Threats and allies -----------------------------------------------
	Strategy.ThreatWizardIds = DiploSub && UnitSub && CitySub
		? AssessThreats(WizardId, DiploSub, UnitSub, CitySub)
		: TArray<int32>();

	Strategy.AllyWizardIds = DiploSub
		? IdentifyAllies(WizardId, DiploSub)
		: TArray<int32>();

	// ---- Game phase -------------------------------------------------------
	Strategy.GamePhaseEstimate = EstimateGamePhase(CurrentTurn, CityCount, MagicSub, WizardId);

	// ---- Expansion target -------------------------------------------------
	Strategy.ExpansionTargetCityId = CitySub ? ChooseExpansionTarget(WizardId, CitySub) : -1;

	// ---- Personality (from diplomacy subsystem) ---------------------------
	FCoMDiplomaticPersonality Personality;
	if (DiploSub)
	{
		// EvaluateProposal gives us the personality indirectly; use a default if not set.
		// We access the personality through the relation system.
		// For now fetch via a small trick: create a dummy proposal to read EvaluateProposal.
		// Better: directly read personality fields if accessible.
		Personality.Aggressiveness = 50;
		Personality.ScholarlyNature = 50;
		Personality.TradeAffinity = 50;
	}

	// ---- Determine priority -----------------------------------------------
	const float MilitaryRatio = (StrongestRival > 1.0f)
		? (OurPower / StrongestRival)
		: 10.0f; // No rivals with armies = we are safe

	Strategy.TopPriority = DeterminePriority(
		Strategy.GamePhaseEstimate, MilitaryRatio,
		CityCount, Strategy.ThreatWizardIds.Num(), Personality);

	// ---- Compute budget split ---------------------------------------------
	ComputeBudgets(Strategy.TopPriority, Personality,
	               Strategy.MilitaryBudget, Strategy.ResearchBudget, Strategy.EconomyBudget);

	UE_LOG(LogTemp, Log,
	       TEXT("CoMAI: Wizard %d strategy -- Priority=%d Phase=%d Cities=%d Power=%.0f vs %.0f Threats=%d"),
	       WizardId, (int32)Strategy.TopPriority, Strategy.GamePhaseEstimate,
	       CityCount, OurPower, StrongestRival, Strategy.ThreatWizardIds.Num());

	return Strategy;
}

// ---------------------------------------------------------------------------
// Military power -- sum unit levels * HP as a simple combat rating
// ---------------------------------------------------------------------------

float UCoMAIStrategicPlanner::ComputeMilitaryPower(int32 WizardId, UCoMUnitSubsystem* UnitSub) const
{
	float TotalPower = 0.0f;
	TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesForWizard(WizardId);
	for (const FCoMArmyGroup* Army : Armies)
	{
		if (!Army) continue;
		for (int32 UnitID : Army->UnitIDs)
		{
			const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitID);
			if (Unit)
			{
				// Simple power formula: Level * CurrentHP. Heroes are worth double.
				float UnitPower = static_cast<float>(Unit->Level * Unit->CurrentHP);
				if (Unit->bIsHero)
				{
					UnitPower *= 2.0f;
				}
				TotalPower += UnitPower;
			}
		}
	}
	return TotalPower;
}

// ---------------------------------------------------------------------------
// Economic strength
// ---------------------------------------------------------------------------

void UCoMAIStrategicPlanner::ComputeEconomicStrength(
	int32 WizardId, UCoMCitySubsystem* CitySub,
	int32& OutCityCount, int32& OutGoldIncome, int32& OutManaIncome) const
{
	OutCityCount = 0;
	OutGoldIncome = 0;
	OutManaIncome = 0;

	TArray<const FCoMCityData*> Cities = CitySub->GetCitiesForWizard(WizardId);
	OutCityCount = Cities.Num();
	for (const FCoMCityData* City : Cities)
	{
		if (!City) continue;
		OutGoldIncome += City->GoldIncome;
		OutManaIncome += City->ManaOutput;
	}
}

// ---------------------------------------------------------------------------
// Threat assessment
// ---------------------------------------------------------------------------

TArray<int32> UCoMAIStrategicPlanner::AssessThreats(
	int32 WizardId,
	UCoMDiplomacySubsystem* DiploSub,
	UCoMUnitSubsystem* UnitSub,
	UCoMCitySubsystem* CitySub) const
{
	TArray<int32> Threats;
	TArray<const FCoMCityData*> OurCities = CitySub->GetCitiesForWizard(WizardId);

	for (int32 i = 0; i < CoM::MAX_WIZARDS; ++i)
	{
		if (i == WizardId) continue;

		// At war = automatic threat
		if (DiploSub->AreAtWar(WizardId, i))
		{
			Threats.AddUnique(i);
			continue;
		}

		// Check if this wizard has armies near our cities (even if not at war)
		TArray<const FCoMArmyGroup*> TheirArmies = UnitSub->GetArmiesForWizard(i);
		for (const FCoMArmyGroup* Army : TheirArmies)
		{
			if (!Army || Army->UnitIDs.Num() == 0) continue;
			if (IsArmyNearOurCities(OurCities, Army->Position, THREAT_RADIUS))
			{
				Threats.AddUnique(i);
				break;
			}
		}
	}

	return Threats;
}

// ---------------------------------------------------------------------------
// Allies
// ---------------------------------------------------------------------------

TArray<int32> UCoMAIStrategicPlanner::IdentifyAllies(int32 WizardId,
                                                      UCoMDiplomacySubsystem* DiploSub) const
{
	return DiploSub->GetAllies(WizardId);
}

// ---------------------------------------------------------------------------
// Game phase estimation
// ---------------------------------------------------------------------------

int32 UCoMAIStrategicPlanner::EstimateGamePhase(
	int32 CurrentTurn, int32 CityCount,
	UCoMMagicSubsystem* MagicSub, int32 WizardId) const
{
	// Heuristic: combine turn number, city count, and spell count.
	int32 SpellCount = 0;
	if (MagicSub)
	{
		const FCoMWizardMagicState& Magic = MagicSub->GetWizardMagic(WizardId);
		SpellCount = Magic.KnownSpells.Num();
	}

	// Score: higher = later game
	const int32 Score = CurrentTurn + (CityCount * 5) + (SpellCount * 3);

	if (Score < EARLY_GAME_END)  return 0; // Early
	if (Score < MID_GAME_END)    return 1; // Mid
	return 2;                               // Late
}

// ---------------------------------------------------------------------------
// Expansion target
// ---------------------------------------------------------------------------

int32 UCoMAIStrategicPlanner::ChooseExpansionTarget(int32 WizardId,
                                                     UCoMCitySubsystem* CitySub) const
{
	// Look for unclaimed or enemy cities that are weakly defended.
	// For now, return -1 (no specific target) and let the tactical executor decide
	// based on fog-of-war exploration. A real implementation would scan nearby
	// tile clusters without cities and flag the best site.
	// TODO: Integrate with UCoMWorldMapSubsystem for terrain suitability scoring.
	return -1;
}

// ---------------------------------------------------------------------------
// Priority determination
// ---------------------------------------------------------------------------

ECoMAIPriority UCoMAIStrategicPlanner::DeterminePriority(
	int32 GamePhase, float MilitaryRatio,
	int32 CityCount, int32 ThreatCount,
	const FCoMDiplomaticPersonality& Personality) const
{
	// Existential danger: we are drastically outmatched and under threat
	if (MilitaryRatio < DANGER_RATIO && ThreatCount > 0)
	{
		return ECoMAIPriority::Survive;
	}

	// Early game with few cities: prioritize expansion
	if (GamePhase == 0 && CityCount < 3)
	{
		return ECoMAIPriority::Expand;
	}

	// Scholarly wizards lean toward research
	if (Personality.ScholarlyNature > 70 && GamePhase <= 1)
	{
		return ECoMAIPriority::Research;
	}

	// Aggressive wizards with military advantage: push for military
	if (Personality.Aggressiveness > 65 && MilitaryRatio > 1.2f)
	{
		return ECoMAIPriority::BuildMilitary;
	}

	// Mid game with decent cities but weak military
	if (GamePhase == 1 && MilitaryRatio < 0.8f)
	{
		return ECoMAIPriority::BuildMilitary;
	}

	// Late game with dominant military: conquer (still expressed as BuildMilitary)
	if (GamePhase == 2 && MilitaryRatio > DOMINANT_RATIO)
	{
		return ECoMAIPriority::BuildMilitary;
	}

	// Multiple active threats but not desperate: diplomacy to reduce fronts
	if (ThreatCount >= 2)
	{
		return ECoMAIPriority::Diplomacy;
	}

	// Default: grow the economy
	return ECoMAIPriority::BuildEconomy;
}

// ---------------------------------------------------------------------------
// Budget computation
// ---------------------------------------------------------------------------

void UCoMAIStrategicPlanner::ComputeBudgets(
	ECoMAIPriority Priority, const FCoMDiplomaticPersonality& Personality,
	float& OutMilitary, float& OutResearch, float& OutEconomy) const
{
	// Base budgets per priority
	switch (Priority)
	{
	case ECoMAIPriority::Expand:
		OutMilitary = 0.25f; OutResearch = 0.15f; OutEconomy = 0.60f;
		break;
	case ECoMAIPriority::BuildEconomy:
		OutMilitary = 0.20f; OutResearch = 0.25f; OutEconomy = 0.55f;
		break;
	case ECoMAIPriority::BuildMilitary:
		OutMilitary = 0.55f; OutResearch = 0.15f; OutEconomy = 0.30f;
		break;
	case ECoMAIPriority::Research:
		OutMilitary = 0.15f; OutResearch = 0.55f; OutEconomy = 0.30f;
		break;
	case ECoMAIPriority::Diplomacy:
		OutMilitary = 0.30f; OutResearch = 0.30f; OutEconomy = 0.40f;
		break;
	case ECoMAIPriority::Survive:
		OutMilitary = 0.60f; OutResearch = 0.10f; OutEconomy = 0.30f;
		break;
	default:
		OutMilitary = 0.33f; OutResearch = 0.34f; OutEconomy = 0.33f;
		break;
	}

	// Personality-based adjustments: shift up to 10% between categories
	const float AggressionShift = (static_cast<float>(Personality.Aggressiveness) - 50.0f) / 500.0f; // -0.1 to +0.1
	const float ScholarShift    = (static_cast<float>(Personality.ScholarlyNature) - 50.0f) / 500.0f;

	OutMilitary += AggressionShift;
	OutResearch += ScholarShift;
	OutEconomy  -= (AggressionShift + ScholarShift);

	// Clamp all budgets to [0.05, 0.90] and re-normalize
	OutMilitary = FMath::Clamp(OutMilitary, 0.05f, 0.90f);
	OutResearch = FMath::Clamp(OutResearch, 0.05f, 0.90f);
	OutEconomy  = FMath::Clamp(OutEconomy,  0.05f, 0.90f);

	const float Sum = OutMilitary + OutResearch + OutEconomy;
	if (Sum > 0.0f)
	{
		OutMilitary /= Sum;
		OutResearch /= Sum;
		OutEconomy  /= Sum;
	}
}

// ---------------------------------------------------------------------------
// Proximity check
// ---------------------------------------------------------------------------

bool UCoMAIStrategicPlanner::IsArmyNearOurCities(
	const TArray<const FCoMCityData*>& OurCities,
	FIntPoint ArmyPos, int32 ThreatRadius) const
{
	for (const FCoMCityData* City : OurCities)
	{
		if (!City) continue;
		if (WrappedDistance(City->Position, ArmyPos) <= ThreatRadius)
		{
			return true;
		}
	}
	return false;
}

int32 UCoMAIStrategicPlanner::WrappedDistance(FIntPoint A, FIntPoint B)
{
	int32 DX = FMath::Abs(A.X - B.X);
	if (CoM::MAP_WRAP_X)
	{
		DX = FMath::Min(DX, CoM::MAP_WIDTH - DX);
	}
	const int32 DY = FMath::Abs(A.Y - B.Y);
	return DX + DY;
}
