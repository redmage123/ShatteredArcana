#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCitySubsystem.generated.h"

/**
 * City governor focus — determines what the city auto-builds when its queue is empty.
 */
UENUM(BlueprintType)
enum class ECoMCityFocus : uint8
{
	Manual         UMETA(DisplayName = "Manual"),          // Player manages queue manually
	BalancedGrowth UMETA(DisplayName = "Balanced Growth"), // Auto-build: granary, marketplace, library, then balanced
	Military       UMETA(DisplayName = "Military"),        // Auto-build: barracks, stable, armory, then units
	Economy        UMETA(DisplayName = "Economy"),         // Auto-build: marketplace, bank, then gold buildings
	Research       UMETA(DisplayName = "Research"),        // Auto-build: library, mage tower, wizard guild
	Production     UMETA(DisplayName = "Production"),      // Auto-build: smithy, enchanter's workshop, then production buildings
	MAX UMETA(Hidden)
};

class UCoMUnitSubsystem;
class UCoMBuildingDataAsset;
class UCoMUnitSpecDataAsset;
class UCoMRaceDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCityFounded, int32, CityID, int32, OwnerWizardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCityDestroyed, int32, CityID, int32, FormerOwnerWizardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCityRebelled, int32, CityID, int32, FormerOwnerWizardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCityCaptured, int32, CityID, int32, FormerOwnerWizard, int32, NewOwnerWizard);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBuildingCompleted, int32, CityID, int32, BuildingID, int32, OwnerWizardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCityPopulationChanged, int32, CityID, int32, OldPop, int32, NewPop);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUnitRecruited, int32, CityID, FName, UnitSpecID, int32, OwnerWizardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProductionNotification, int32, CityID, FText, Message);

/**
 * Data for a single city enchantment instance (buff or curse applied to a city).
 */
// (struct defined in CoMStructs.h)

/**
 * A single item (building or unit) in a city's production queue.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMProductionItem
{
	GENERATED_BODY()

	/** Building ID (FName from BuildingDataAsset) or Unit Spec ID (FName from UnitSpecDataAsset). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	/** True = unit recruitment, false = building construction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsUnit = false;

	/** Total production cost to complete this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ProductionCost = 50;

	/** Estimated turns remaining at current production rate (recalculated each turn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TurnsRemaining = 0;
};

/**
 * Full state for a single city in the world.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMCityData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CityID = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText CityName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 OwnerWizardIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECoMPlane Plane = ECoMPlane::Aurelith;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECoMMapLayer Layer = ECoMMapLayer::Surface;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FIntPoint Position = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Population = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag PrimaryRaceTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FGameplayTag> MinorityRaceTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 FoodSurplus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 GoldIncome = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ProductionOutput = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ManaOutput = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ResearchOutput = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Unrest = 0; // 0-10; at 10 the city rebels

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<int32> BuildingIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CurrentBuildingID = -1; // -1 = nothing in production (legacy, use ProductionQueue)

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 BuildingProgress = 0; // legacy field, use AccumulatedProduction

	/** Ordered list of items to produce. Front of array = currently being built. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCoMProductionItem> ProductionQueue;

	/** Production points accumulated toward the current queue item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AccumulatedProduction = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 GarrisonArmyID = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoMEnchantmentInstance> CityEnchantments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHasLightSource = false; // Underdark: required for full productivity

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 WallLevel = 0; // 0=none, 1=palisade, 2=stone, 3=fortress

	/** Visual architecture style — set at founding based on the founding race. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoMArchitectureStyle Architecture = ECoMArchitectureStyle::Human;
};

/**
 * Manages cities across all 8 planes and 3 map layers (Surface, Underdark, Underwater).
 *
 * Surface cities use standard farming/economy. Underdark cities have no farms
 * (enhanced mining, fungal food, light source requirement). Underwater cities
 * are restricted to aquatic races only.
 *
 * Coordinates with UCoMSeasonSubsystem for seasonal economy modifiers,
 * UCoMWorldMapSubsystem for terrain/tile queries, and UCoMTerritorySubsystem
 * for tile ownership.
 */
UCLASS()
class COMCORE_API UCoMCitySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -----------------------------------------------------------------
	// City Founding & Destruction
	// -----------------------------------------------------------------

	/**
	 * Found a new city at the given location.
	 * Validates MIN_CITY_DISTANCE from existing cities, terrain suitability,
	 * and aquatic-race requirement for Underwater layer.
	 * @return CityID on success, -1 on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	int32 FoundCity(int32 OwnerWizard, ECoMPlane Plane, ECoMMapLayer Layer,
		FIntPoint Position, FGameplayTag RaceTag, FText Name);

	/** Remove a city from the world. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void DestroyCity(int32 CityID);

	// -----------------------------------------------------------------
	// Settler Production
	// -----------------------------------------------------------------

	/**
	 * Spawn a settler unit from a city. Costs 1 population.
	 * Returns the settler's unit ID, or -1 if city population is too low (must be >= 2).
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	int32 ProduceSettler(int32 CityId);

	// -----------------------------------------------------------------
	// Validation
	// -----------------------------------------------------------------

	/**
	 * Check if a city can be founded at the given position.
	 * Validates MIN_CITY_DISTANCE, terrain suitability, and layer restrictions.
	 * Returns true if the position is valid for city founding.
	 */
	UFUNCTION(BlueprintPure, Category = "CoM|Cities")
	bool CanFoundCityAt(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position, FGameplayTag RaceTag) const;

	// -----------------------------------------------------------------
	// Turn Processing
	// -----------------------------------------------------------------

	/**
	 * Process one turn for every city: growth, production, food, gold, mana,
	 * research, unrest evaluation, and building completion.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void ProcessCityTurn();

	// -----------------------------------------------------------------
	// Queries
	// -----------------------------------------------------------------

	/** Return city data by ID, or nullptr if not found. */
	const FCoMCityData* GetCity(int32 CityID) const;

	/** Mutable city data by ID, or nullptr. For systems (e.g. global enchantments)
	 *  that need to push state changes like unrest or population. */
	FCoMCityData* GetCityMutable(int32 CityID);

	/** IDs of every city in the world (all owners, all planes). */
	TArray<int32> GetAllCityIDs() const;

	/** Transfer a city to a new owner (siege conquest). Resets production and
	 *  raises unrest from the hostile takeover. Returns true on success. */
	bool CaptureCity(int32 CityID, int32 NewOwnerWizard);

	/** All cities owned by a given wizard. */
	TArray<const FCoMCityData*> GetCitiesForWizard(int32 WizardIndex) const;

	/** All cities on a given plane (any layer). */
	TArray<const FCoMCityData*> GetCitiesOnPlane(ECoMPlane Plane) const;

	// -----------------------------------------------------------------
	// Production Queue (legacy — kept for backward compatibility)
	// -----------------------------------------------------------------

	/**
	 * Set the building currently in the production queue (legacy single-item).
	 * Prefer AddToQueue for the new multi-item system.
	 * @return true if the building ID is valid and the city doesn't already have it.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	bool SetBuildingQueue(int32 CityID, int32 BuildingID);

	// -----------------------------------------------------------------
	// Production Queue (new multi-item system)
	// -----------------------------------------------------------------

	/** Add a building or unit to the end of a city's production queue. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void AddToQueue(int32 CityId, FName ItemID, bool bIsUnit);

	/** Remove an item from the queue at the given index. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void RemoveFromQueue(int32 CityId, int32 QueueIndex);

	/** Move an item within the queue (reorder). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void MoveInQueue(int32 CityId, int32 FromIndex, int32 ToIndex);

	/** Clear all items from a city's production queue. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void ClearQueue(int32 CityId);

	/** Get a copy of the city's current production queue. */
	UFUNCTION(BlueprintPure, Category = "CoM|Cities")
	TArray<FCoMProductionItem> GetQueue(int32 CityId) const;

	// -----------------------------------------------------------------
	// City Governor / Auto-Build
	// -----------------------------------------------------------------

	/** Set the auto-build focus for a city. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void SetCityFocus(int32 CityId, ECoMCityFocus Focus);

	/** Get the current auto-build focus for a city. */
	UFUNCTION(BlueprintPure, Category = "CoM|Cities")
	ECoMCityFocus GetCityFocus(int32 CityId) const;

	// -----------------------------------------------------------------
	// Rally Points
	// -----------------------------------------------------------------

	/** Set a rally point for a city. New units will auto-move toward this position. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void SetRallyPoint(int32 CityId, FIntPoint Position);

	/** Get the rally point for a city. Returns the city's own position if no rally point is set. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Cities")
	FIntPoint GetRallyPoint(int32 CityId) const;

	/** Clear the rally point for a city (units stay at the city). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void ClearRallyPoint(int32 CityId);

	// -----------------------------------------------------------------
	// Availability Queries
	// -----------------------------------------------------------------

	/** Return building IDs available to construct (not already built, prerequisites met). */
	UFUNCTION(BlueprintPure, Category = "CoM|Cities")
	TArray<FName> GetAvailableBuildings(int32 CityId) const;

	/** Return unit spec IDs available to recruit (based on city's buildings and race). */
	UFUNCTION(BlueprintPure, Category = "CoM|Cities")
	TArray<FName> GetAvailableUnits(int32 CityId) const;

	// -----------------------------------------------------------------
	// Output Recalculation
	// -----------------------------------------------------------------

	/**
	 * Recalculate food/gold/production/mana/research for a city based on
	 * terrain, buildings, population, season, weather, and enchantments.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	void RecalcCityOutputs(int32 CityID);

	/** Population cap for a city based on buildings and terrain. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Cities")
	int32 GetCityPopulationCap(int32 CityID) const;

	// -----------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Cities")
	FOnCityFounded OnCityFounded;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Cities")
	FOnCityDestroyed OnCityDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Cities")
	FOnCityRebelled OnCityRebelled;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Cities")
	FOnCityCaptured OnCityCaptured;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Cities")
	FOnBuildingCompleted OnBuildingCompleted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Cities")
	FOnCityPopulationChanged OnCityPopulationChanged;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Cities")
	FOnUnitRecruited OnUnitRecruited;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Cities")
	FOnProductionNotification OnProductionNotification;

	// ── Save/Load Export/Import ───────────────────────────────────────────

	/** Export all cities and next-ID counter for save serialization. */
	void ExportAll(TArray<FCoMCityData>& OutCities, int32& OutNextCityID) const;

	/** Import cities from save data (clears existing state). */
	void ImportAll(const TArray<FCoMCityData>& InCities, int32 InNextCityID);

private:
	// -----------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------

	/** Check if any existing city on the same plane/layer is within MIN_CITY_DISTANCE. */
	bool IsTooCloseToExistingCity(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position) const;

	/** Wrapped Manhattan distance accounting for WrapX on a 160-wide map. */
	static int32 WrappedDistance(FIntPoint A, FIntPoint B);

	/** Whether the given race is aquatic and allowed to found underwater cities. */
	static bool IsAquaticRace(FGameplayTag RaceTag);

	/** Gather food-producing tile coordinates in the city radius. */
	TArray<FIntPoint> GetCityRadiusTiles(FIntPoint Center) const;

	/** Compute food output for a single tile based on layer rules. */
	int32 ComputeTileFood(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint TilePos) const;

	/** Compute base gold from population and buildings. */
	int32 ComputeBaseGold(const FCoMCityData& City) const;

	/** Compute base production from population and buildings. */
	int32 ComputeBaseProduction(const FCoMCityData& City) const;

	/** Compute base mana output from buildings and terrain. */
	int32 ComputeBaseMana(const FCoMCityData& City) const;

	/** Compute base research output from buildings and population. */
	int32 ComputeBaseResearch(const FCoMCityData& City) const;

	/** Evaluate unrest sources and reductions, clamp to [0,10]. */
	int32 ComputeUnrest(const FCoMCityData& City) const;

	/** Count buildings that reduce unrest (temples, garrisons, etc.). */
	int32 CountUnrestReduction(const FCoMCityData& City) const;

	/** Process growth for a single city based on food surplus. */
	void ProcessGrowth(FCoMCityData& City);

	/** Process building construction for a single city (legacy single-item). */
	void ProcessBuilding(FCoMCityData& City);

	/** Process production queue for a single city (new multi-item system). */
	void ProcessCityProduction(FCoMCityData& City);

	/** Look up production cost for a building by its FName ID. Returns 50 as fallback. */
	int32 LookupBuildingCost(FName BuildingID) const;

	/** Look up production cost for a unit by its FName spec ID. Returns 50 as fallback. */
	int32 LookupUnitCost(FName UnitSpecID) const;

	/** Check if a city has a specific building (by FName). BuildingIDs stores int32, so this does a data-asset lookup. */
	bool CityHasBuilding(const FCoMCityData& City, FName BuildingID) const;

	/** Convert a building FName to its int32 hash for BuildingIDs storage. */
	static int32 BuildingNameToID(FName BuildingID);

	/** Spawn a recruited unit at the city's position. Returns the unit ID or INDEX_NONE. */
	int32 SpawnRecruitedUnit(const FCoMCityData& City, FName UnitSpecID);

	/** Handle city rebellion when unrest reaches 10. */
	void HandleRebellion(FCoMCityData& City);

	/** Auto-enqueue the next building/unit based on city focus when queue is empty. */
	void AutoEnqueueForFocus(FCoMCityData& City);

	/** Get the priority build list for a given focus. Returns building FNames in priority order. */
	static TArray<FName> GetFocusBuildingPriority(ECoMCityFocus Focus);

	/** Get the priority unit list for Military focus. Returns unit FNames to cycle. */
	static TArray<FName> GetMilitaryUnitCycle();

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	UPROPERTY()
	TMap<int32, FCoMCityData> AllCities;

	/** Per-city rally points (position where new units auto-move to). */
	UPROPERTY()
	TMap<int32, FIntPoint> CityRallyPoints;

	/** Per-city governor focus setting. */
	UPROPERTY()
	TMap<int32, ECoMCityFocus> CityFocusMap;

	int32 NextCityID = 1;

	// -----------------------------------------------------------------
	// Constants
	// -----------------------------------------------------------------

	static constexpr int32 CityRadius = 5;
	static constexpr int32 MapWidth   = 160;
	static constexpr int32 MapHeight  = 100;
	static constexpr int32 MaxUnrest  = 10;
	static constexpr int32 GrowthDivisor = 10;

	// Underdark mining production bonus: +50% (applied as multiplier 3/2).
	static constexpr int32 UnderdarkMiningNumerator   = 3;
	static constexpr int32 UnderdarkMiningDenominator = 2;

	// Underdark light source penalty: 50% productivity without light.
	static constexpr int32 NoLightPenaltyNumerator   = 1;
	static constexpr int32 NoLightPenaltyDenominator = 2;
};