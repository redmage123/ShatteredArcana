// Copyright Mythforge Studios. All Rights Reserved.
// CoMAIStrategicPlanner.h -- Evaluates game state and produces high-level strategy for AI wizards.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoMAITypes.h"
#include "CoMAIStrategicPlanner.generated.h"

class UCoMCitySubsystem;
class UCoMUnitSubsystem;
class UCoMDiplomacySubsystem;
class UCoMMagicSubsystem;
class UCoMTurnSubsystem;
struct FCoMDiplomaticPersonality;

/**
 * UCoMAIStrategicPlanner
 *
 * Analyses the full game state for a single AI wizard and produces an FCoMAIStrategy
 * that describes priorities and resource allocation for the coming turn. The tactical
 * executor then translates the strategy into concrete subsystem commands.
 *
 * Factors considered:
 *   - Military balance (our army power vs each rival)
 *   - Economic strength (city count, gold/mana income)
 *   - Threat assessment (wars, enemy armies near our cities)
 *   - Diplomatic personality (aggressiveness, scholarly nature, etc.)
 *   - Game phase (early = expand, mid = build, late = conquer)
 */
UCLASS()
class COMAI_API UCoMAIStrategicPlanner : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Evaluate the strategic situation for WizardId and return a strategy struct.
	 * @param WizardId  Index into the WizardStates array.
	 * @return Strategy with priorities, budgets, threat lists.
	 */
	FCoMAIStrategy EvaluateStrategy(int32 WizardId, ECoMAIDifficulty Difficulty = ECoMAIDifficulty::Normal);

private:
	// ---- Sub-evaluations --------------------------------------------------

	/** Sum the combat power of all armies owned by WizardId. */
	float ComputeMilitaryPower(int32 WizardId, UCoMUnitSubsystem* UnitSub) const;

	/** Count cities and sum gold/mana income for WizardId. */
	void ComputeEconomicStrength(int32 WizardId, UCoMCitySubsystem* CitySub,
	                              int32& OutCityCount, int32& OutGoldIncome,
	                              int32& OutManaIncome) const;

	/** Identify which wizards are actively threatening us (at war or armies near our cities). */
	TArray<int32> AssessThreats(int32 WizardId,
	                            UCoMDiplomacySubsystem* DiploSub,
	                            UCoMUnitSubsystem* UnitSub,
	                            UCoMCitySubsystem* CitySub) const;

	/** Identify allied wizard indices. */
	TArray<int32> IdentifyAllies(int32 WizardId, UCoMDiplomacySubsystem* DiploSub) const;

	/** Estimate the game phase based on turn number, city count, and known spells. */
	int32 EstimateGamePhase(int32 CurrentTurn, int32 CityCount,
	                         UCoMMagicSubsystem* MagicSub, int32 WizardId) const;

	/** Pick a city to expand toward (-1 if none appropriate). */
	int32 ChooseExpansionTarget(int32 WizardId, UCoMCitySubsystem* CitySub) const;

	/** Determine the top priority from the strategic analysis. */
	ECoMAIPriority DeterminePriority(int32 GamePhase, float MilitaryRatio,
	                                  int32 CityCount, int32 ThreatCount,
	                                  const FCoMDiplomaticPersonality& Personality) const;

	/** Compute budget split (military/research/economy) based on priority and personality. */
	void ComputeBudgets(ECoMAIPriority Priority, const FCoMDiplomaticPersonality& Personality,
	                     float& OutMilitary, float& OutResearch, float& OutEconomy) const;

	/** Check if an enemy army is within ThreatRadius tiles of any of our cities. */
	bool IsArmyNearOurCities(const TArray<const struct FCoMCityData*>& OurCities,
	                          FIntPoint ArmyPos, int32 ThreatRadius) const;

	/** Wrapped Manhattan distance for the 160-wide map. */
	static int32 WrappedDistance(FIntPoint A, FIntPoint B);
};
