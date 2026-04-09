// Copyright Mythforge Studios. All Rights Reserved.
// CoMAITacticalExecutor.h -- Translates AI strategy into concrete subsystem commands.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoMAITypes.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMAITacticalExecutor.generated.h"

class UCoMCitySubsystem;
class UCoMUnitSubsystem;
class UCoMDiplomacySubsystem;
class UCoMMagicSubsystem;
class UCoMWorldMapSubsystem;
struct FCoMCityData;
struct FCoMArmyGroup;

/**
 * UCoMAITacticalExecutor
 *
 * Given a strategy from the planner, issues concrete orders to the game's subsystems:
 *   - City production queues (buildings, units)
 *   - Army movement, merging, attacking
 *   - Diplomatic proposals and declarations
 *   - Research allocation and spell casting
 */
UCLASS()
class COMAI_API UCoMAITacticalExecutor : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Execute one full AI turn for WizardId based on the provided strategy.
	 * Calls each sub-manager in sequence: cities, armies, diplomacy, research, magic.
	 */
	void ExecuteTurn(int32 WizardId, const FCoMAIStrategy& Strategy, ECoMAIDifficulty Difficulty = ECoMAIDifficulty::Normal);

private:
	// ---- City management --------------------------------------------------

	/** Set build queues for all cities owned by WizardId. */
	void ManageCities(int32 WizardId, const FCoMAIStrategy& Strategy);

	/** Choose a building ID for a city based on priority. Returns -1 if none needed. */
	int32 ChooseBuildingForCity(const FCoMCityData& City, const FCoMAIStrategy& Strategy) const;

	// ---- Army management --------------------------------------------------

	/** Move, merge, and attack with all armies. */
	void ManageArmies(int32 WizardId, const FCoMAIStrategy& Strategy);

	/** Compute the total combat power of an army stack. */
	float ComputeArmyPower(const FCoMArmyGroup* Army, UCoMUnitSubsystem* UnitSub) const;

	/** Find the closest enemy army or city to attack. Returns target position or (-1,-1). */
	FIntPoint FindAttackTarget(int32 WizardId, const FCoMArmyGroup* Army,
	                           UCoMUnitSubsystem* UnitSub, UCoMCitySubsystem* CitySub) const;

	/** Find the most threatened friendly city to defend. Returns position or (-1,-1). */
	FIntPoint FindDefenseTarget(int32 WizardId, const FCoMAIStrategy& Strategy,
	                            UCoMCitySubsystem* CitySub) const;

	// ---- Diplomacy management ---------------------------------------------

	/** Propose treaties, declare wars, accept peace based on strategy. */
	void ManageDiplomacy(int32 WizardId, const FCoMAIStrategy& Strategy);

	// ---- Research management ----------------------------------------------

	/** Allocate mana to research and pick a spell to research if idle. */
	void ManageResearch(int32 WizardId, const FCoMAIStrategy& Strategy);

	// ---- Magic management -------------------------------------------------

	/** Cast beneficial global spells or enchantments. */
	void ManageMagic(int32 WizardId, const FCoMAIStrategy& Strategy);

	// ---- Settlement management --------------------------------------------

	/** Check if an army contains a settler unit. Returns the settler's UnitID or -1. */
	int32 FindSettlerInArmy(const FCoMArmyGroup* Army, UCoMUnitSubsystem* UnitSub) const;

	/** Find the best tile for a settler to found a city on the given plane. */
	FIntPoint FindBestSettlerTarget(int32 WizardId, ECoMPlane Plane,
	                                UCoMCitySubsystem* CitySub, UCoMWorldMapSubsystem* MapSub) const;

	/** Queue settler production in a suitable city if needed. */
	void ConsiderSettlerProduction(int32 WizardId, const FCoMAIStrategy& Strategy,
	                               UCoMCitySubsystem* CitySub);

	// ---- Helpers ----------------------------------------------------------

	/** Get the GameInstance (walks outer chain if needed). */
	UGameInstance* ResolveGameInstance() const;

	/** Wrapped Manhattan distance for the 160-wide map. */
	static int32 WrappedDistance(FIntPoint A, FIntPoint B);

	/** Cached difficulty for the current turn (set by ExecuteTurn). */
	ECoMAIDifficulty CachedDifficulty = ECoMAIDifficulty::Normal;
};
