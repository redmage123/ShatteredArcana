#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMWorldGenerator.generated.h"

class UCoMWorldMapSubsystem;

// ---------------------------------------------------------------------------
// FCoMTerrainWeight — weighted terrain entry for procedural distribution
// ---------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct COMCORE_API FCoMTerrainWeight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation")
	ECoMTerrain TerrainType{};

	/** Relative weight in the range [0.0, 1.0]. Weights are normalized at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Weight = 0.0f;
};

// ---------------------------------------------------------------------------
// UCoMTerrainWeightTable — per-plane terrain distribution data asset
// ---------------------------------------------------------------------------
UCLASS(BlueprintType)
class COMCORE_API UCoMTerrainWeightTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** The plane this table belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Generation")
	ECoMPlane Plane{};

	/** Surface terrain weights. Design convention: entries should sum to 1.0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Generation")
	TArray<FCoMTerrainWeight> SurfaceWeights;

	/** Underdark terrain weights. Design convention: entries should sum to 1.0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Generation")
	TArray<FCoMTerrainWeight> UnderdarkWeights;

	/** Returns the sum of all SurfaceWeights (should be ~1.0). */
	UFUNCTION(BlueprintPure, Category = "World Generation")
	float GetSurfaceWeightSum() const;

	/** Returns the sum of all UnderdarkWeights (should be ~1.0). */
	UFUNCTION(BlueprintPure, Category = "World Generation")
	float GetUnderdarkWeightSum() const;

	/** Build a default weight table for the given plane (used by tests and blueprint setup). */
	static UCoMTerrainWeightTable* BuildDefaultWeightTable(ECoMPlane Plane);
};

// ---------------------------------------------------------------------------
// UCoMWorldGenerator — procedural world generation pipeline
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class COMCORE_API UCoMWorldGenerator : public UObject
{
	GENERATED_BODY()

public:
	// -------------------------------------------------------------------
	// Main entry point
	// -------------------------------------------------------------------

	/**
	 * Generate the full world across all planes and layers.
	 * Calls the five pipeline stages sequentially for each plane,
	 * then generates Underdark and Underwater layers.
	 */
	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void GenerateWorld(UCoMWorldMapSubsystem* MapSubsystem, int32 Seed);

	// -------------------------------------------------------------------
	// Configuration — exposed to editor / Blueprint
	// -------------------------------------------------------------------

	/** Per-plane terrain weight tables (DataAsset references). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Terrain")
	TMap<ECoMPlane, UCoMTerrainWeightTable*> TerrainWeights;

	/** Fraction of the map covered by ocean tiles [0.0, 1.0]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Landmass", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OceanRatio = 0.45f;

	/** Relative density of mountain terrain [0.0, 1.0]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Landmass", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MountainDensity = 0.12f;

	/** Relative density of resource placement [0.0, 1.0]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Resources", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ResourceDensity = 0.20f;

	/** Relative density of feature placement (ruins, dungeons, shrines) [0.0, 1.0]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Features", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FeatureDensity = 0.08f;

private:
	// -------------------------------------------------------------------
	// Pipeline stages (Surface layer)
	// -------------------------------------------------------------------

	/** Stage 1 — Place ocean vs land tiles using Perlin/simplex noise. */
	void GenerateLandmass(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane);

	/** Stage 2 — Assign plane-specific terrain types from weight tables. */
	void AssignTerrain(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane);

	/** Stage 3 — Trace rivers from mountain tiles toward ocean/coast. */
	void GenerateRivers(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane);

	/** Stage 4 — Scatter resources based on terrain type and plane. */
	void PlaceResources(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane);

	/** Stage 5 — Place ruins, dungeons, shrines with overlap checks. */
	void PlaceFeatures(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane);

	// -------------------------------------------------------------------
	// Secondary layer generation
	// -------------------------------------------------------------------

	/** Loosely mirror surface topology with plane-unique underdark terrains. */
	void GenerateUnderdark(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane);

	/** Spawn underwater zones beneath ocean tiles. */
	void GenerateUnderwater(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane);

	// -------------------------------------------------------------------
	// Internal helpers
	// -------------------------------------------------------------------

	/** Sample 2D noise at the given tile position, returning a deterministic value in [0, 1]. */
	FFixed64 SampleNoise(FIntPoint Position, FFixed64 Frequency, FFixed64 Amplitude) const;

public:
	/** Pick a terrain type from a weight array using the internal random stream. */
	ECoMTerrain PickWeightedTerrain(const TArray<FCoMTerrainWeight>& Weights);

private:

	/** Check whether a feature can be placed at Position without overlapping existing features. */
	bool CanPlaceFeature(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane, FIntPoint Position, int32 Radius) const;

	// -------------------------------------------------------------------
	// State
	// -------------------------------------------------------------------

	/** Deterministic random stream seeded from GenerateWorld's Seed parameter. */
	FRandomStream RandomStream;

	/** Cached pointer valid only during a GenerateWorld call. */
	UPROPERTY(Transient)
	TObjectPtr<UCoMWorldMapSubsystem> CachedMapSubsystem = nullptr;

	/** Current generation seed, stored for logging/debugging. */
	int32 CurrentSeed = 0;

	/** Map dimensions — sourced from CoMConstants.h, cached for convenience. */
	static constexpr int32 MapWidth = CoM::MAP_WIDTH;
	static constexpr int32 MapHeight = CoM::MAP_HEIGHT;
};