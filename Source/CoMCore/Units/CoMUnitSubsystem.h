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

	// --- Movement ---

	/** Move an army toward Destination, consuming movement points via pathfinder costs. */
	void MoveArmy(int32 ArmyID, FIntPoint Destination);

	/** End-of-turn movement processing: restore movement points, resolve any pending encounters. */
	void ProcessMovementTurn();

	// --- Queries ---

	const FCoMUnitInstance* GetUnit(int32 UnitID) const;
	const FCoMArmyGroup* GetArmy(int32 ArmyID) const;

	TArray<const FCoMArmyGroup*> GetArmiesAtPosition(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position) const;
	TArray<const FCoMArmyGroup*> GetArmiesForWizard(int32 WizardIndex) const;

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

	UPROPERTY()
	TMap<int32, FCoMUnitInstance> AllUnits;

	UPROPERTY()
	TMap<int32, FCoMArmyGroup> AllArmies;

	int32 NextUnitID = 1;
	int32 NextArmyID = 1;
};