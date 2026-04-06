// Copyright Mythforge Studios. All Rights Reserved.
// COM-S3-T2: Per-Plane Terrain Distribution DataAssets
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMTerrainWeightDataAsset.generated.h"

/**
 * FCoMTerrainWeightEntry
 *
 * One weighted entry in a per-plane terrain distribution table.
 * Weight is un-normalized; the generator normalises all entries in a table at sample time.
 * Latitude and altitude ranges are in [0, 1] — 0 = equator / sea-level, 1 = pole / peak.
 * A tile must fall inside [MinLatitude, MaxLatitude] AND [MinAltitude, MaxAltitude] to be
 * eligible; if no entry matches the tile the generator falls back to hardcoded logic.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMTerrainWeightEntry
{
	GENERATED_BODY()

	/** Terrain type this entry represents. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	ECoMTerrain Terrain = ECoMTerrain::Grassland;

	/** Relative sampling weight (must be > 0 to have any effect). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	/** Minimum normalised latitude for this terrain to be eligible (0 = equator). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Range",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinLatitude = 0.0f;

	/** Maximum normalised latitude for this terrain to be eligible (1 = pole). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Range",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxLatitude = 1.0f;

	/** Minimum normalised altitude for this terrain to be eligible (0 = sea-level). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Range",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinAltitude = 0.0f;

	/** Maximum normalised altitude for this terrain to be eligible (1 = max-height). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Range",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxAltitude = 1.0f;
};

/**
 * UCoMTerrainWeightDataAsset
 *
 * Defines the terrain distribution tables for one ECoMPlane across all three map layers
 * (Surface, Underdark, Underwater).  Consumed by UCoMWorldGenerator::DistributeTerrain()
 * when a matching asset for the plane-in-question is provided via the PlaneWeights array.
 *
 * When no asset is present for a plane (or the array is empty), the generator falls back
 * to the hardcoded per-plane switch in GetSurfaceLandTerrain / GetUnderdarkTerrain.
 *
 * Asset naming convention: one asset per plane, named after the plane enum value
 * (e.g. "TW_Aurelith", "TW_Feywild").  The Plane field must match.
 *
 * Primary asset type registered in DefaultGame.ini: "CoMTerrainWeight".
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMTerrainWeightDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ─── Plane identity ──────────────────────────────────────────────────────

	/** Which plane this asset describes.  Must be consistent with the asset's FName. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane")
	ECoMPlane Plane = ECoMPlane::Aurelith;

	// ─── Per-layer weight tables ─────────────────────────────────────────────

	/**
	 * Weighted terrain entries for the Surface layer (land tiles only — ocean tiles
	 * are always assigned via GetOceanTerrain and are not affected by this table).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
	TArray<FCoMTerrainWeightEntry> SurfaceWeights;

	/**
	 * Weighted terrain entries for the Underdark layer.
	 * If empty the generator falls back to GetUnderdarkTerrain().
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Underdark")
	TArray<FCoMTerrainWeightEntry> UnderdarkWeights;

	/**
	 * Weighted terrain entries for the Underwater layer.
	 * If empty the generator leaves the default OceanFloor assigned in Stage 1.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Underwater")
	TArray<FCoMTerrainWeightEntry> UnderwaterWeights;
};
