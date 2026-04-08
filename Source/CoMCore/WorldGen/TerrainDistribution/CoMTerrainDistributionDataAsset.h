// OLD Phase 3 code — disabled
#if 0
// Copyright Mythforge Studios. All Rights Reserved.
// COM-S3-T2: Per-Plane Terrain Distribution DataAsset (registry for all 8 planes).
// Asset type: "CoMTerrainDistribution"
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/Data/CoMTerrainWeightDataAsset.h"
#include "CoMTerrainDistributionDataAsset.generated.h"

/**
 * UCoMTerrainDistributionDataAsset
 *
 * Top-level DataAsset that bundles per-plane terrain weight tables for ALL 8 planes.
 * One instance of this asset is referenced by the world generator; it replaces the
 * empty TArray<UCoMTerrainWeightDataAsset*>{} previously passed to DistributeTerrain().
 *
 * Each PlaneWeights entry maps to one UCoMTerrainWeightDataAsset (one per ECoMPlane).
 * Planes without an entry fall back to the hardcoded logic in UCoMWorldGenerator.
 *
 * Validation invariants (enforced in IsDataValid):
 *   - No null entries in PlaneWeights
 *   - Each plane's SurfaceWeights is non-empty and weights sum to ≥ 0.999
 *   - No weight entry has Weight < 0
 *
 * Asset naming convention: single asset "DA_TerrainDistribution" in Content/WorldGen/
 * Primary asset type: "CoMTerrainDistribution"
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMTerrainDistributionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ─── Plane weight registry ───────────────────────────────────────────────

	/**
	 * One UCoMTerrainWeightDataAsset per ECoMPlane.  Order is not required to be sorted;
	 * UCoMWorldGenerator matches by the Plane field on each asset.
	 * Missing planes fall back to hardcoded GetSurfaceLandTerrain / GetUnderdarkTerrain.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TerrainDistribution")
	TArray<TObjectPtr<UCoMTerrainWeightDataAsset>> PlaneWeights;

	// ─── Query ───────────────────────────────────────────────────────────────

	/** Returns the DataAsset for the given plane, or null if not configured. */
	UFUNCTION(BlueprintCallable, Category = "TerrainDistribution")
	UCoMTerrainWeightDataAsset* GetWeightsForPlane(ECoMPlane Plane) const;

	/** Builds a raw array of asset pointers suitable for UCoMWorldGenerator::DistributeTerrain. */
	TArray<UCoMTerrainWeightDataAsset*> BuildPlaneWeightsArray() const;

	// ─── Validation ──────────────────────────────────────────────────────────

	/**
	 * Validates all per-plane entries.  Returns true when the asset is ready for generation.
	 * Populates OutErrors with one message per violation found.
	 *
	 * Checks:
	 *   - PlaneWeights is not empty
	 *   - No null entries
	 *   - Each entry's SurfaceWeights is non-empty
	 *   - No weight < 0 in any layer
	 *   - Each plane's normalized weight sums to ≈ 1.0 (tolerance 0.001)
	 */
	bool IsDataValid(TArray<FString>& OutErrors) const;

	/**
	 * Computes the normalized weight total for a weight-entry array.
	 * For a valid table the sum of all Weight values equals 1.0.
	 * Returns 0.0 when the array is empty.
	 */
	static float ComputeWeightSum(const TArray<FCoMTerrainWeightEntry>& Entries);

	// ─── Default factory (for unit tests and editor baseline) ────────────────

	/**
	 * Builds a fully-populated UCoMTerrainDistributionDataAsset with one
	 * UCoMTerrainWeightDataAsset per ECoMPlane.  Each plane has distinct Surface,
	 * Underdark, and Underwater tables whose weights sum to exactly 1.0.
	 *
	 * Intended for unit tests (no editor/asset-registry needed) and as the
	 * baseline that a designer can override per-asset in the Content Browser.
	 *
	 * @param Outer  Outer object for the new assets; pass null for transient.
	 */
	static UCoMTerrainDistributionDataAsset* CreateDefaults(UObject* Outer = nullptr);

private:
	/** Populate a single UCoMTerrainWeightDataAsset with plane-specific default tables. */
	static UCoMTerrainWeightDataAsset* MakePlaneAsset(ECoMPlane Plane, UObject* Outer);
};


#endif
