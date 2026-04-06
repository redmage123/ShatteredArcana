// Copyright Mythforge Studios. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WorldGen/CoMWorldGenTypes.h"
#include "CoMWorldGenerator.generated.h"

class UCoMWorldMapSubsystem;

/**
 * UCoMWorldGenerator
 *
 * Stateless UObject that runs the 5-stage deterministic world-generation pipeline and
 * writes the result directly into a UCoMWorldMapSubsystem (all 24 layers):
 *
 *   Stage 1 — Landmass generation:    seeded noise heightmap → ocean/land split
 *   Stage 2 — Terrain distribution:   plane-specific biome palette from height + latitude
 *   Stage 3 — River placement:        hill→ocean flow paths; respects MAP_WRAP_X
 *   Stage 4 — Resource node seeding:  plane-specific ECoMResource at configured density
 *   Stage 5 — Feature placement:      portals, ley-line chains, site markers (min 4-tile spacing)
 *
 * All arithmetic is integer-only — no float, no FFixed64 (generation, not simulation).
 * Same Seed always produces bit-identical output on any platform.
 *
 * Typical usage:
 *   UCoMWorldMapSubsystem* Map = GetGameInstance()->GetSubsystem<UCoMWorldMapSubsystem>();
 *   Map->InitializeLayers();
 *   UCoMWorldGenerator* Gen = NewObject<UCoMWorldGenerator>();
 *   FCoMWorldData Meta = Gen->GenerateWorld(Map, MySeed);
 *   // Map is now fully populated; Meta.Seed can be persisted for save/load
 */
UCLASS(BlueprintType, Blueprintable)
class COMCORE_API UCoMWorldGenerator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Generate a complete world into Map (must have InitializeLayers() called first).
	 * Populates all 24 layers: 8 planes × (Surface + Underdark + Underwater).
	 * Thread-safe: reads no shared mutable state outside Map.
	 *
	 * @param Map   Target subsystem; must not be null. InitializeLayers() must be called first.
	 * @param Seed  Master generation seed. Same seed → identical world.
	 * @return FCoMWorldData metadata (seed, counters, bIsValid).
	 */
	UFUNCTION(BlueprintCallable, Category="WorldGen")
	FCoMWorldData GenerateWorld(UCoMWorldMapSubsystem* Map, int32 Seed) const;

private:
	// ─── Pipeline stages — each writes directly into Map ────────────────────

	/** Stage 1: heightmap per plane → ocean/land split on Surface tiles; populates Underwater. */
	void GenerateLandmass(UCoMWorldMapSubsystem* Map, int32 Seed,
	                      TArray<TArray<int32>>& OutHeightMaps) const;

	/** Stage 2: assign ECoMTerrain to every tile using height + plane palette. */
	void DistributeTerrain(UCoMWorldMapSubsystem* Map,
	                       const TArray<TArray<int32>>& HeightMaps) const;

	/** Stage 3: carve rivers from highland sources to ocean sinks. */
	void PlaceRivers(UCoMWorldMapSubsystem* Map, int32 Seed, FCoMWorldData& OutData) const;

	/** Stage 4: scatter ECoMResource on land surface + underdark tiles. */
	void SeedResources(UCoMWorldMapSubsystem* Map, int32 Seed, FCoMWorldData& OutData) const;

	/** Stage 5: place portals, ley-line node chains, and site markers (4-tile min spacing). */
	void PlaceFeatures(UCoMWorldMapSubsystem* Map, int32 Seed, FCoMWorldData& OutData) const;

	// ─── Per-plane terrain helpers ────────────────────────────────────────────

	static void BuildHeightMap(TArray<int32>& OutMap, ECoMPlane Plane, uint32 PlaneSeed);

	/** Returns the "ocean" terrain for a plane (impassable water tiles). */
	static ECoMTerrain GetOceanTerrain(ECoMPlane Plane);

	/** Maps (plane, height 0..999, latitude 0..100) to a surface land terrain. */
	static ECoMTerrain GetSurfaceLandTerrain(ECoMPlane Plane, int32 Height, int32 Latitude);

	/** Returns the primary underdark terrain for a plane. */
	static ECoMTerrain GetUnderdarkTerrain(ECoMPlane Plane);

	/** Surface resource types valid for a given terrain. */
	static TArray<ECoMResource> GetSurfaceResources(ECoMTerrain Terrain);

	/** Underdark resource types for a given plane. */
	static TArray<ECoMResource> GetUnderdarkResources(ECoMPlane Plane);

	// ─── Deterministic LCG — Knuth multiplier; period 2^32 ───────────────────

	struct FWorldGenRNG
	{
		uint32 State;
		explicit FWorldGenRNG(uint32 Seed) : State(Seed == 0u ? 2654435761u : Seed) {}

		FORCEINLINE uint32 Next()
		{
			State = State * 1664525u + 1013904223u;
			return State;
		}
		FORCEINLINE int32 RandRange(int32 Min, int32 Max)
		{
			if (Min >= Max) return Min;
			return Min + static_cast<int32>(Next() % static_cast<uint32>(Max - Min + 1));
		}
		FORCEINLINE bool Chance(int32 Pct)   // true with Pct/100 probability
		{
			return static_cast<int32>(Next() % 100u) < Pct;
		}
	};

	// ─── Integer noise — X-tileable (MAP_WRAP_X = true) ──────────────────────

	/** Wang-hash 2D → [0, 65535]. X is wrapped to MAP_WIDTH for east-west tileability. */
	static uint32 HashNoise(uint32 Seed, int32 X, int32 Y);

	/** Multi-octave bilinear value noise → [0, 999]. Scale = base cell size in tiles. */
	static int32 OctaveNoise(uint32 Seed, int32 X, int32 Y, int32 Octaves, int32 Scale);

	/** Chebyshev distance between two points, accounting for X-wrap. */
	static int32 WrapDist(int32 AX, int32 AY, int32 BX, int32 BY);
};
