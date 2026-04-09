// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMFixedPoint.h"
#include "CoMNavalSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFleetCreated, int32, FleetID, int32, OwnerWizard);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBlockadeEstablished, int32, BlockadeID, int32, TargetCityID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBlockadeLifted, int32, BlockadeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSeaMonsterEncounter, int32, FleetID, int32, MonsterID);

/**
 * Blockade placed by a fleet against a coastal city.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMBlockade
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 BlockadeID = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 FleetID = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 TargetCityID = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 TurnsActive = 0;
};

/**
 * A roaming sea monster group on a given plane.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMSeaMonster
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 MonsterID = -1;

	UPROPERTY(BlueprintReadOnly)
	ECoMPlane Plane;

	UPROPERTY(BlueprintReadOnly)
	FIntPoint Position = FIntPoint(-1, -1);

	/** Combat power of this monster group. */
	UPROPERTY(BlueprintReadOnly)
	FFixed64 CombatPower = FFixed64(0);

	/** Speed in tiles per turn. */
	UPROPERTY(BlueprintReadOnly)
	int32 Speed = 1;

	/** If killed, counts turns until respawn. -1 means alive. */
	UPROPERTY(BlueprintReadOnly)
	int32 RespawnCountdown = -1;
};

class UCoMWorldMapSubsystem;
class UCoMPathfinder;

/**
 * Manages fleet assembly, ocean movement, sea lane routing,
 * blockade zones, and sea monster encounters across all 8 planes.
 */
UCLASS()
class COMCORE_API UCoMNavalSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -----------------------------------------------------------------
	// Fleet Assembly
	// -----------------------------------------------------------------

	/** Create a new empty fleet at the given ocean position. Returns FleetID (-1 on failure). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Naval")
	int32 CreateFleet(int32 OwnerWizard, ECoMPlane Plane, FIntPoint Position);

	/** Add a ship unit to an existing fleet. Returns false if fleet not found or ship already assigned. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Naval")
	bool AddShipToFleet(int32 ShipUnitID, int32 FleetID);

	/** Remove a ship unit from a fleet. Returns false if not found. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Naval")
	bool RemoveShipFromFleet(int32 ShipUnitID, int32 FleetID);

	/** Set the combat formation for a fleet. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Naval")
	void SetFleetFormation(int32 FleetID, ECoMFleetFormation Formation);

	// -----------------------------------------------------------------
	// Fleet Movement
	// -----------------------------------------------------------------

	/** Plot an ocean-only route and begin moving a fleet toward a destination. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Naval")
	void MoveFleet(int32 FleetID, FIntPoint Destination);

	// -----------------------------------------------------------------
	// Turn Processing
	// -----------------------------------------------------------------

	/** Advance all fleets along routes, tick blockades, move and spawn sea monsters. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Naval")
	void ProcessNavalTurn();

	// -----------------------------------------------------------------
	// Blockades
	// -----------------------------------------------------------------

	/** Establish a blockade on a coastal city using the given fleet. Returns BlockadeID (-1 on failure). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Naval")
	int32 EstablishBlockade(int32 FleetID, int32 TargetCityID);

	/** Lift an existing blockade by ID. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Naval")
	void LiftBlockade(int32 BlockadeID);

	/** Check whether a city's port is currently blockaded. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Naval")
	bool IsPortBlockaded(int32 CityID) const;

	// -----------------------------------------------------------------
	// Queries
	// -----------------------------------------------------------------

	/** Return a fleet by ID, or nullptr. */
	const FCoMFleet* GetFleet(int32 FleetID) const;

	/** All fleets owned by a given wizard. */
	TArray<const FCoMFleet*> GetFleetsForWizard(int32 WizardIndex) const;

	/** All fleets at the given map position on a plane. */
	TArray<const FCoMFleet*> GetFleetsAtPosition(ECoMPlane Plane, FIntPoint Position) const;

	// -----------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Naval")
	FOnFleetCreated OnFleetCreated;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Naval")
	FOnBlockadeEstablished OnBlockadeEstablished;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Naval")
	FOnBlockadeLifted OnBlockadeLifted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Naval")
	FOnSeaMonsterEncounter OnSeaMonsterEncounter;

private:
	// -----------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------

	/** Allocate the next sequential fleet ID. */
	int32 AllocateFleetID();

	/** Allocate the next sequential blockade ID. */
	int32 AllocateBlockadeID();

	/** Allocate the next sequential monster ID. */
	int32 AllocateMonsterID();

	/** Compute the fleet's effective speed (minimum ship speed in the fleet). */
	int32 ComputeFleetSpeed(const FCoMFleet& Fleet) const;

	/** Find an ocean-only path between two points on the same plane. */
	TArray<FIntPoint> FindOceanPath(ECoMPlane Plane, FIntPoint From, FIntPoint To) const;

	/** Check whether a tile is ocean. */
	bool IsOceanTile(ECoMPlane Plane, FIntPoint Position) const;

	/** Check whether a position is adjacent to a city (for blockade validation). */
	bool IsAdjacentToCity(ECoMPlane Plane, FIntPoint Position, int32 CityID) const;

	/** Spawn initial sea monsters for all planes. */
	void SpawnInitialSeaMonsters();

	/** Spawn a single sea monster group on the given plane. */
	int32 SpawnSeaMonster(ECoMPlane Plane);

	/** Move sea monsters randomly along ocean tiles. */
	void MoveSeaMonsters();

	/** Check fleet encounters with sea monsters. */
	void CheckSeaMonsterEncounters();

	/** Get hardcoded combat power for sea monsters on a given plane. */
	static FFixed64 GetMonsterCombatPowerForPlane(ECoMPlane Plane);

	/** Get hardcoded monster speed for a given plane. */
	static int32 GetMonsterSpeedForPlane(ECoMPlane Plane);

	/** Get the number of initial sea monster groups for a plane (1-3). */
	static int32 GetMonsterCountForPlane(ECoMPlane Plane);

	/** Find a random ocean tile on the given plane. Returns (-1,-1) if none found. */
	FIntPoint FindRandomOceanTile(ECoMPlane Plane) const;

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	UPROPERTY()
	TMap<int32, FCoMFleet> AllFleets;

	UPROPERTY()
	TArray<FCoMBlockade> AllBlockades;

	UPROPERTY()
	TArray<FCoMSeaMonster> SeaMonsters;

	int32 NextFleetID    = 1;
	int32 NextBlockadeID = 1;
	int32 NextMonsterID  = 1;

	/** Cached reference to the world map subsystem. */
	UPROPERTY()
	TObjectPtr<UCoMWorldMapSubsystem> CachedWorldMap;

	/** Cached pathfinder instance for ocean routing. */
	UPROPERTY()
	TObjectPtr<UCoMPathfinder> OceanPathfinder;

	/** Global turn counter for respawn tracking. */
	int32 CurrentTurn = 0;

	/** Whether initial sea monsters have been spawned (lazy init). */
	bool bMonstersSpawned = false;

	/** Starvation tracking: CityID -> turns blockaded with negative food. */
	TMap<int32, int32> StarvationCounters;

	static constexpr int32 RESPAWN_DELAY_TURNS       = 20;
	static constexpr int32 STARVATION_POP_LOSS_TURNS = 3;
};
