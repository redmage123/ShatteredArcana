// Copyright Mythforge Studios. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes/CoMConstants.h"
#include "CoreTypes/CoMEnums.h"
#include "CoMWorldGenTypes.generated.h"

// ─── Generation Parameters ────────────────────────────────────────────────────

/** Tunable generation knobs passed to UCoMWorldGenerator::GenerateWorld(). */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMWorldGenParams
{
	GENERATED_BODY()

	/** Target percentage of land tiles per surface layer (1–99). Default 60 → ~40% ocean. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen", meta=(ClampMin=1, ClampMax=99))
	int32 LandFractionPct = 60;

	/** Min rivers carved per plane surface layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen", meta=(ClampMin=0))
	int32 MinRiversPerPlane = 3;

	/** Max rivers carved per plane surface layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen", meta=(ClampMin=0))
	int32 MaxRiversPerPlane = 8;

	/** Resource node density on land surface tiles, as a percentage (1–20). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen", meta=(ClampMin=1, ClampMax=20))
	int32 ResourceDensityPct = 5;

	/** Min map-feature sites (ruins, dungeons, shrines) per plane surface layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen", meta=(ClampMin=0))
	int32 MinSitesPerPlane = 10;

	/** Max map-feature sites per plane surface layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen", meta=(ClampMin=0))
	int32 MaxSitesPerPlane = 30;

	/** Ley-line chains placed per plane. Each chain spans 3–7 node tiles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen", meta=(ClampMin=0))
	int32 LeyLinesPerPlane = 5;

	/** Portal tiles placed per plane surface layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen", meta=(ClampMin=0))
	int32 PortalsPerPlane = 2;
};

// ─── World Data Output ────────────────────────────────────────────────────────

/**
 * FCoMWorldData — generation metadata returned by UCoMWorldGenerator::GenerateWorld().
 *
 * The generated tile data lives in UCoMWorldMapSubsystem (the authoritative world state).
 * This struct carries the seed and summary counters needed for save/load, QA, and
 * Gameplay Dev hand-off (S3-T2/T3/T4).
 *
 * Usage: store FCoMWorldData.Seed in save game so world can be regenerated deterministically.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMWorldData
{
	GENERATED_BODY()

	/** Seed used to produce this world. Persist this for save/load determinism. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen")
	int32 Seed = 0;

	/** True when all 5 pipeline stages completed without error. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen")
	bool bIsValid = false;

	// Summary counters (for QA, tests, and UI).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen") int32 TotalPortals      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen") int32 TotalLeyLineNodes = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen") int32 TotalSites        = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen") int32 TotalRiverTiles   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldGen") int32 TotalResourceTiles= 0;
};
