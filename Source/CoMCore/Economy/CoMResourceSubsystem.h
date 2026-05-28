// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMFixedPoint.h"
#include "CoMResourceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMineExhausted, int32, MineID, ECoMResource, ResourceType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMineCompleted, int32, MineID, int32, CityID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnderdarkBreach, int32, MineID, FIntPoint, Position);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTradeRouteRaided, int32, RouteID, int32, GoldLost, FFixed64, RiskFactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDiseaseSpread, int32, OutbreakID, int32, SourceCityID, int32, TargetCityID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuarantineDeclared, int32, CityID, int32, ActiveRoutesStopped);

/**
 * Manages mine construction, resource extraction with depletion,
 * trade route management, and disease spread along trade routes.
 */
UCLASS()
class COMCORE_API UCoMResourceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Re-seed this subsystem's RNG from a per-game master seed so playtests vary
	 *  by seed and stay reproducible. Mixes a per-subsystem salt so each stream is
	 *  independent. Called from UCoMPlaytestSubsystem::BootstrapGame. */
	void ReseedRandom(int32 MasterSeed);

	// -----------------------------------------------------------------
	// Mining
	// -----------------------------------------------------------------

	/**
	 * Begin construction of a mine at the given tile.
	 * Returns the new MineID, or -1 on failure (no resource, tile occupied).
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Resources")
	int32 BuildMine(int32 CityID, ECoMPlane Plane, FIntPoint Position, ECoMResource TileResource = ECoMResource::Iron);

	/** Engineer-built outpost mine: owned by a wizard directly with no parent
	 *  city, so it can sit on a resource tile far outside any city's working
	 *  radius. Returns the new MineID, or -1 on failure. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Resources")
	int32 BuildMineForWizard(int32 WizardId, ECoMPlane Plane, FIntPoint Position, ECoMResource TileResource = ECoMResource::Iron);

	/** Advance mine construction, extract resources, check depletion. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Resources")
	void ProcessMineTurn();

	/** Return mine data by ID, or nullptr if not found. */
	const FCoMMineData* GetMine(int32 MineID) const;

	/** All mines owned by the given city. */

	/**
	 * Compute the resource yield for a mine this turn.
	 * Based on mine level, workers assigned, and enchantments.
	 * Returns 0 for exhausted or under-construction mines.
	 */
	int32 GetResourceYield(int32 MineID) const;

	// -----------------------------------------------------------------
	// Trade
	// -----------------------------------------------------------------

	/**
	 * Establish a trade route between two cities for the given good.
	 * Returns RouteID, or -1 on failure.
	 */
	int32 EstablishTradeRoute(int32 SourceCityID, int32 DestCityID, ECoMResource Good);

	/** Cancel an active trade route. */
	void CancelTradeRoute(int32 RouteID);

	/** Calculate revenue, check raid risk, spread disease along routes. */
	void ProcessTradeTurn();

	/** Return trade route by ID, or nullptr. */
	const FCoMTradeRoute* GetTradeRoute(int32 RouteID) const;

	/** All trade routes involving the given city (source or dest). */

	// -----------------------------------------------------------------
	// Disease
	// -----------------------------------------------------------------

	/**
	 * Start a disease outbreak in the given city.
	 * Returns OutbreakID.
	 */
	int32 StartOutbreak(int32 CityID, ECoMDiseaseType Type);

	/** Declare quarantine on a city. Stops trade routes, reduces spread. */
	void DeclareQuarantine(int32 CityID);

	/** Advance disease: spread along trade routes, increase severity, check cure. */
	void ProcessDiseaseTurn();

	TArray<int32> GetMinesForCity(int32 CityID) const;

	/** All active outbreaks in the given city. */
	TArray<FCoMDiseaseOutbreak> GetOutbreaksForCity(int32 CityID) const;

	// -----------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Resources")
	FOnMineExhausted OnMineExhausted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Resources")
	FOnMineCompleted OnMineCompleted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Resources")
	FOnUnderdarkBreach OnUnderdarkBreach;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Resources")
	FOnTradeRouteRaided OnTradeRouteRaided;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Resources")
	FOnDiseaseSpread OnDiseaseSpread;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Resources")
	FOnQuarantineDeclared OnQuarantineDeclared;

private:
	// -----------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------

	/** Base yield for a resource type at mine level 1. */
	static int32 GetBaseYield(ECoMResource Resource);

	/** Yield multiplier for the given mine level (1-3). */
	static FFixed64 GetLevelMultiplier(int32 MineLevel);

	/** Compute gold revenue for a trade route this turn. */
	FFixed64 ComputeTradeRevenue(const FCoMTradeRoute& Route) const;

	/** Compute raid chance for a route (5-15% range, higher for interplanar). */
	FFixed64 ComputeRaidChance(const FCoMTradeRoute& Route) const;

	/** Check whether a level-3 surface mine near mountains should breach. */
	bool CheckUnderdarkBreach(const FCoMMineData& Mine);

	/** Get the base spread chance for a disease type. */
	static FFixed64 GetBaseDiseaseSpreadChance(ECoMDiseaseType Type);

	/** Apply disease effects to a city based on type and severity. */
	void ApplyDiseaseEffects(const FCoMDiseaseOutbreak& Outbreak);

	/** Allocate sequential IDs. */
	int32 AllocateMineID();
	int32 AllocateRouteID();
	int32 AllocateOutbreakID();

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	UPROPERTY()
	TMap<int32, FCoMMineData> AllMines;

	UPROPERTY()
	TArray<FCoMTradeRoute> AllTradeRoutes;

	UPROPERTY()
	TArray<FCoMDiseaseOutbreak> AllOutbreaks;

	FRandomStream ResourceRng;

	int32 NextMineID     = 1;
	int32 NextRouteID    = 1;
	int32 NextOutbreakID = 1;
};
