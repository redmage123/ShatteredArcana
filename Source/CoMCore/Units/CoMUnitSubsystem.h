#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMFixedPoint.h"
#include "CoMUnitSubsystem.generated.h"

class UCoMPathfinder;
class UCoMWorldMapSubsystem;
class UCoMWeatherSubsystem;
class UCoMUnitSpecDataAsset;
class UCoMCitySubsystem;
class UCoMAudioSubsystem;

UCLASS()
class COMCORE_API UCoMUnitSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Unit Lifecycle ---

	/** Spawn a unit from a spec, placing it at the given location. Returns the new UnitID. */
	int32 SpawnUnit(int32 SpecID, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position, int32 OwnerWizard);

	/**
	 * Spawn a unit by FName-keyed SpecID (matching the names registered in
	 * CoMUnitDatabase: "Settler", "HighMen_Infantry", "Hero_Fighter", etc.).
	 * Preferred over int32 SpawnUnit, which only works when SpecID has been
	 * registered under the stringified-integer name (legacy path).
	 */
	UFUNCTION(BlueprintCallable, Category = "Units")
	int32 SpawnUnitByName(FName SpecID, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position, int32 OwnerWizard);

	/** Remove a unit from the world and any army it belongs to. */
	void DespawnUnit(int32 UnitID);

	// --- Army Lifecycle ---

	/** Create an empty army group at the given location. Returns the new ArmyID. */
	int32 CreateArmy(int32 OwnerWizard, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position);

	/** Add a unit to an army. Fails if the army is already at MAX_ARMY_SIZE. */
	bool AddUnitToArmy(int32 UnitID, int32 ArmyID);

	/** Remove a unit from an army. Fails if the unit is not in the army. */
	bool RemoveUnitFromArmy(int32 UnitID, int32 ArmyID);

	/** Merge Source into Dest. Fails if combined size exceeds MAX_ARMY_SIZE. Source is destroyed on success. */
	bool MergeArmies(int32 SourceArmyID, int32 DestArmyID);

	/**
	 * Split the listed units out of ArmyID into a new army at the same position.
	 * Returns the new ArmyID, or INDEX_NONE on failure.
	 */
	int32 SplitArmy(int32 ArmyID, const TArray<int32>& UnitIDsToSplit);

	// --- Settlement ---

	/**
	 * Found a city using a settler unit. The settler is consumed (removed from its army).
	 * Returns the new city ID, or -1 on failure.
	 * Validates: settler exists, tile is valid, distance from other cities, terrain suitability.
	 */
	UFUNCTION(BlueprintCallable, Category = "Units")
	int32 FoundCityWithSettler(int32 ArmyId, int32 SettlerUnitId);

	/** If the army contains an engineer, build a road (RoadLevel = 1) on the
	 *  current tile (idempotent — won't lower an existing higher level).
	 *  Returns true if a road was upgraded/built. */
	UFUNCTION(BlueprintCallable, Category = "Units")
	bool BuildRoadAtArmy(int32 ArmyId);

	/** If the army contains an engineer and the current tile has a resource and
	 *  no existing mine, build an outpost mine owned by the army's wizard (no
	 *  parent city, so it works far outside any city's working radius). */
	UFUNCTION(BlueprintCallable, Category = "Units")
	bool BuildMineAtArmy(int32 ArmyId);

	// --- Movement ---

	/**
	 * Move an army toward Destination, consuming movement points via pathfinder
	 * costs. bAllowUnexplored lets the route cross tiles the owner hasn't
	 * revealed yet — required for AI armies, which would otherwise be unable to
	 * path to any goal in the fog (settlers never reached founding sites).
	 * Terrain passability is still respected via per-tile move cost.
	 */
	void MoveArmy(int32 ArmyID, FIntPoint Destination, bool bAllowUnexplored = false);

	/** End-of-turn movement processing: restore movement points, resolve any pending encounters. */
	void ProcessMovementTurn();

	// --- Auto-Explore ---

	/** Enable or disable auto-explore for an army. */
	UFUNCTION(BlueprintCallable, Category = "Units|AutoExplore")
	void SetAutoExplore(int32 ArmyId, bool bEnable);

	/** Check if an army is currently auto-exploring. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Units|AutoExplore")
	bool IsAutoExploring(int32 ArmyId) const;

	// --- Queries ---

	const FCoMUnitInstance* GetUnit(int32 UnitID) const;
	const FCoMArmyGroup* GetArmy(int32 ArmyID) const;

	/** Mutable accessors for subsystems that need to modify army/unit state. */
	FCoMArmyGroup* GetArmyMutable(int32 ArmyID);
	FCoMUnitInstance* GetUnitMutable(int32 UnitID);

	/** Set the settler flag on a unit by ID. */
	void SetUnitSettlerFlag(int32 UnitId, bool bSettler);

	/** Set the race tag on a unit by ID. */
	void SetUnitRaceTag(int32 UnitId, FGameplayTag Tag);

	/**
	 * Apply damage to a unit. If HP drops to 0 the unit is removed from its army
	 * and despawned. Returns true if the unit was killed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Units")
	bool ApplyDamage(int32 UnitID, int32 Amount);

	/** Heal a unit, capped at MaxHP. Returns the actual amount healed. */
	UFUNCTION(BlueprintCallable, Category = "Units")
	int32 ApplyHeal(int32 UnitID, int32 Amount);

	TArray<const FCoMArmyGroup*> GetArmiesAtPosition(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position) const;
	TArray<const FCoMArmyGroup*> GetArmiesForWizard(int32 WizardIndex) const;

	// ── Save/Load Export/Import ───────────────────────────────────────────

	/** Export all units and armies for save serialization. */
	void ExportAll(TArray<FCoMUnitInstance>& OutUnits, TArray<FCoMArmyGroup>& OutArmies,
	               int32& OutNextUnitID, int32& OutNextArmyID) const;

	/** Import units and armies from save data (clears existing state). */
	void ImportAll(const TArray<FCoMUnitInstance>& InUnits, const TArray<FCoMArmyGroup>& InArmies,
	               int32 InNextUnitID, int32 InNextArmyID);

private:
	/** Resolve the UCoMUnitSpecDataAsset for a given SpecID. */
	const UCoMUnitSpecDataAsset* ResolveSpec(int32 SpecID) const;

	/** Compute the movement speed of an army (slowest unit). */
	FFixed64 ComputeArmyMovementSpeed(const FCoMArmyGroup& Army) const;

	/** Compute terrain movement cost for a tile, accounting for flying, weather, ley lines. */
	FFixed64 ComputeMoveCost(const FCoMArmyGroup& Army, ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Tile) const;

	/** Check whether an army has any flying unit flags set for all its members. */
	bool IsArmyFullyFlying(const FCoMArmyGroup& Army) const;

	/** Check whether an army has at least one burrowing unit. */
	bool ArmyHasBurrower(const FCoMArmyGroup& Army) const;

	/** Remove ArmyID from the army that contains UnitID (internal helper, no validation). */
	void Internal_RemoveUnitFromArmy(int32 UnitID, FCoMArmyGroup& Army);

	/** Wrap an X coordinate for the 160-wide map. */
	static int32 WrapX(int32 X);

	UPROPERTY()
	TObjectPtr<UCoMPathfinder> Pathfinder;

	UPROPERTY()
	TObjectPtr<UCoMWorldMapSubsystem> WorldMapSubsystem;

	UPROPERTY()
	TObjectPtr<UCoMWeatherSubsystem> WeatherSubsystem;

	/** Process auto-explore orders: find nearest unexplored tile, pathfind and move. */
	void ProcessAutoExplore();

	UPROPERTY()
	TSet<int32> AutoExploreArmies;

	UPROPERTY()
	TMap<int32, FCoMUnitInstance> AllUnits;

	UPROPERTY()
	TMap<int32, FCoMArmyGroup> AllArmies;

	int32 NextUnitID = 1;
	int32 NextArmyID = 1;
};