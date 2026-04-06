// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMEnums.h"
#include "CoMStructs.h"
#include "CoMConstants.h"
#include "CoMTerritorySubsystem.generated.h"

/**
 * Seed registered by a city or wizard tower that projects territorial influence.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMCitySeed
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoMPlane Plane = ECoMPlane::Arcanus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoMMapLayer Layer = ECoMMapLayer::Surface;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Position = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WizardIndex = CoM::WIZARD_INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InfluenceRadius = CoM::MAX_INFLUENCE_RADIUS;
};

/**
 * Key for looking up per-layer ownership data.
 */
USTRUCT()
struct FCoMLayerKey
{
	GENERATED_BODY()

	ECoMPlane Plane = ECoMPlane::Arcanus;
	ECoMMapLayer Layer = ECoMMapLayer::Surface;

	FCoMLayerKey() = default;
	FCoMLayerKey(ECoMPlane InPlane, ECoMMapLayer InLayer) : Plane(InPlane), Layer(InLayer) {}

	bool operator==(const FCoMLayerKey& Other) const
	{
		return Plane == Other.Plane && Layer == Other.Layer;
	}

	friend uint32 GetTypeHash(const FCoMLayerKey& Key)
	{
		return HashCombine(GetTypeHash(static_cast<uint8>(Key.Plane)),
		                   GetTypeHash(static_cast<uint8>(Key.Layer)));
	}
};

/**
 * Flood-fill territory subsystem.
 *
 * Computes per-layer ownership maps by BFS-expanding from city seeds.
 * Each tile is owned by the wizard whose seed is closest (Manhattan distance,
 * WrapX-aware). Equidistant tiles from different wizards are contested.
 *
 * Layers are independent: owning the Surface does NOT grant ownership of the
 * Underdark beneath. Read-only in Phase 2; write path wired in Phase 4.
 */
UCLASS()
class COMCORE_API UCoMTerritorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---------- Public API ----------

	/** Full recalc for every plane and layer. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Territory")
	void RecalculateTerritory(class UCoMWorldMapSubsystem* Map);

	/** Recalc a single plane/layer. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Territory")
	void RecalculatePlane(class UCoMWorldMapSubsystem* Map, ECoMPlane Plane, ECoMMapLayer Layer);

	/** Register a city or tower as an influence source. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Territory")
	void SetCitySeed(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position, int32 WizardIndex,
	                 int32 InfluenceRadius = CoM::MAX_INFLUENCE_RADIUS);

	/** Remove a previously registered city seed. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Territory")
	void RemoveCitySeed(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position);

	/** Returns wizard index owning the tile, or WIZARD_INDEX_NONE (-1). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Territory")
	int32 GetTileOwner(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y) const;

	/** True when two or more wizards have equal closest distance to this tile. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Territory")
	bool IsTileContested(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y) const;

	/** Count how many tiles a wizard owns on the given plane/layer. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Territory")
	int32 CountOwnedTiles(int32 WizardIndex, ECoMPlane Plane, ECoMMapLayer Layer) const;

	/** Tiles owned by WizardIndex that are adjacent to another wizard's territory. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Territory")
	TArray<FIntPoint> GetBorderTiles(int32 WizardIndex, ECoMPlane Plane, ECoMMapLayer Layer) const;

private:
	// ---------- Internals ----------

	static constexpr int32 MAP_WIDTH  = 160;
	static constexpr int32 MAP_HEIGHT = 100;
	static constexpr int32 MAP_SIZE   = MAP_WIDTH * MAP_HEIGHT;

	/** Mountain traversal cost (tiles consumed per mountain step). */
	static constexpr int32 MOUNTAIN_COST = 3;

	/** Per-layer flat ownership array: value = wizard index or -1. */
	TMap<FCoMLayerKey, TArray<int32>> OwnershipMaps;

	/** Per-layer contested flags. */
	TMap<FCoMLayerKey, TArray<bool>> ContestedMaps;

	/** Per-layer closest-distance scratch buffer (re-used across recalcs). */
	TMap<FCoMLayerKey, TArray<int32>> DistanceMaps;

	/** All registered city seeds. */
	TArray<FCoMCitySeed> CitySeeds;

	// Helpers

	/** Ensure ownership/contested/distance arrays exist for this key. */
	void EnsureLayerStorage(const FCoMLayerKey& Key);

	/** Clear ownership + contested + distance for one layer. */
	void ClearLayer(const FCoMLayerKey& Key);

	/** WrapX-aware Manhattan distance. */
	static int32 ManhattanWrapX(int32 X1, int32 Y1, int32 X2, int32 Y2);

	/** Flat-array index from tile coordinates (no bounds check). */
	static FORCEINLINE int32 TileIndex(int32 X, int32 Y) { return Y * MAP_WIDTH + X; }

	/** WrapX coordinate normalization. */
	static FORCEINLINE int32 WrapX(int32 X) { return ((X % MAP_WIDTH) + MAP_WIDTH) % MAP_WIDTH; }

	/** True if the tile is ocean and therefore cannot be owned. */
	static bool IsTileOcean(const class UCoMWorldMapSubsystem* Map, ECoMPlane Plane,
	                        ECoMMapLayer Layer, int32 X, int32 Y);

	/** True if the tile is a mountain (costs extra influence to cross). */
	static bool IsTileMountain(const class UCoMWorldMapSubsystem* Map, ECoMPlane Plane,
	                           ECoMMapLayer Layer, int32 X, int32 Y);

	/** Collect seeds that belong to a given plane/layer. */
	TArray<const FCoMCitySeed*> GetSeedsForLayer(ECoMPlane Plane, ECoMMapLayer Layer) const;

	/** All map layers we need to iterate. */
	static const TArray<ECoMMapLayer>& AllLayers();

	/** All planes we need to iterate. */
	static const TArray<ECoMPlane>& AllPlanes();
};
