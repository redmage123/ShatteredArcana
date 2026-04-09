#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCitySubsystem.generated.h"

class UCoMUnitSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCityFounded, int32, CityID, int32, OwnerWizardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCityDestroyed, int32, CityID, int32, FormerOwnerWizardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCityRebelled, int32, CityID, int32, FormerOwnerWizardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBuildingCompleted, int32, CityID, int32, BuildingID, int32, OwnerWizardIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCityPopulationChanged, int32, CityID, int32, OldPop, int32, NewPop);

/**
 * Data for a single city enchantment instance (buff or curse applied to a city).
 */
// (struct defined in CoMStructs.h)

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
	int32 CurrentBuildingID = -1; // -1 = nothing in production

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 BuildingProgress = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 GarrisonArmyID = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoMEnchantmentInstance> CityEnchantments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHasLightSource = false; // Underdark: required for full productivity

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 WallLevel = 0; // 0=none, 1=palisade, 2=stone, 3=fortress
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

	/** All cities owned by a given wizard. */
	TArray<const FCoMCityData*> GetCitiesForWizard(int32 WizardIndex) const;

	/** All cities on a given plane (any layer). */
	TArray<const FCoMCityData*> GetCitiesOnPlane(ECoMPlane Plane) const;

	// -----------------------------------------------------------------
	// Production Queue
	// -----------------------------------------------------------------

	/**
	 * Set the building currently in the production queue.
	 * @return true if the building ID is valid and the city doesn't already have it.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cities")
	bool SetBuildingQueue(int32 CityID, int32 BuildingID);

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
	FOnBuildingCompleted OnBuildingCompleted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Cities")
	FOnCityPopulationChanged OnCityPopulationChanged;

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

	/** Process building construction for a single city. */
	void ProcessBuilding(FCoMCityData& City);

	/** Handle city rebellion when unrest reaches 10. */
	void HandleRebellion(FCoMCityData& City);

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	UPROPERTY()
	TMap<int32, FCoMCityData> AllCities;

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