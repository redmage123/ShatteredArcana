// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMWorldMapSubsystem.h — World map storage and query subsystem.
// COM-026: 24 map layers (8 planes × 3 layers), WrapX, fog-of-war bitmasks.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMWorldMapSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constants used only by this subsystem (not globally shared)
// ─────────────────────────────────────────────────────────────────────────────

namespace CoMMap
{
	/** Layers per plane: Surface, Underdark, Underwater = 3. */
	constexpr int32 LAYERS_PER_PLANE = 3;

	/** Total map layers = 8 planes × 3 layers. */
	constexpr int32 TOTAL_LAYERS = CoM::NUM_PLANES * LAYERS_PER_PLANE; // 24

	/**
	 * Computes the flat layer index from a plane + layer enum pair.
	 * Layout: [Aurelith/Surface, Aurelith/Underdark, Aurelith/Underwater,
	 *          Noctharion/Surface, ..., Aethermist/Underwater]
	 */
	FORCEINLINE int32 LayerIndex(ECoMPlane Plane, ECoMMapLayer Layer)
	{
		return static_cast<int32>(Plane) * LAYERS_PER_PLANE
		     + static_cast<int32>(Layer);
	}
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * UCoMWorldMapSubsystem
 *
 * GameInstance subsystem — single owner, lives for the lifetime of the game instance.
 * Stores the full 24-layer tile map (8 planes × Surface/Underdark/Underwater),
 * provides WrapX-aware tile accessors, and manages per-wizard fog-of-war bitmasks.
 *
 * Tile layout per layer:
 *   - Width  = CoM::MAP_WIDTH  (160)
 *   - Height = CoM::MAP_HEIGHT (100)
 *   - Count  = 16,000 per layer × 24 layers = 384,000 total tile slots
 *
 * Note on Underwater layers:
 *   Surface and Underdark layers are fully populated (all 16,000 tiles allocated).
 *   Underwater layers are allocated with the same grid but tiles default to impassable.
 *   The ocean tile network uses sparse SiteID/PortalID references — the full grid
 *   allocation avoids special-case branch paths in the renderer and AI.
 *
 * Fog of war:
 *   FogRevealed and CurrentVision uint32 bitmasks live directly on FCoMTileData
 *   (bit N = wizard index N, max 32 wizards → supports MAX_WIZARDS=14 with margin).
 *   The subsystem exposes convenience helpers to set/query individual wizard fog.
 */
UCLASS()
class COMCORE_API UCoMWorldMapSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── USubsystem interface ──────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── Map Lifecycle ─────────────────────────────────────────────────────

	/**
	 * Allocates and zero-initialises all 15 layers.
	 * Called by world generation before populating terrain/resources.
	 * Safe to call multiple times (resets map state).
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldMap")
	void InitializeLayers();

	/** Returns true if the layers have been allocated. */
	UFUNCTION(BlueprintPure, Category = "WorldMap")
	bool IsInitialized() const { return bLayersReady; }

	// ── Layer Accessors ───────────────────────────────────────────────────

	/**
	 * Returns a const reference to the full FCoMMapLayerData for a plane+layer.
	 * Crashes with check() if index is out of range — use in debug/dev only.
	 */
	const FCoMMapLayerData& GetLayer(ECoMPlane Plane, ECoMMapLayer Layer) const;

	/** Non-const version for world gen and tile mutation. */
	FCoMMapLayerData& GetLayerMutable(ECoMPlane Plane, ECoMMapLayer Layer);

	// ── Tile Accessors ────────────────────────────────────────────────────

	/**
	 * Returns a pointer to the tile at (X, Y) on a given plane+layer.
	 * X wraps east-west (WrapX). Y is clamped to [0, MAP_HEIGHT-1].
	 * Returns nullptr if the layer is not initialised.
	 */
	// C++ only — UHT does not allow raw pointer return types in UFUNCTION.
	// Blueprint: use GetTileCopy() instead (TODO Sprint 2). See BV02-FIX-002.
	const FCoMTileData* GetTile(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y) const;

	/** Mutable tile access — for world gen, enchantments, corruption, etc. */
	FCoMTileData* GetTileMutable(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y);

	/** Returns tile at position struct (convenience overload). */
	// C++ only — UHT does not allow raw pointer return types in UFUNCTION.
	const FCoMTileData* GetTileAtPos(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position) const;

	// ── Coordinate Utilities ──────────────────────────────────────────────

	/**
	 * Wraps X into [0, MAP_WIDTH-1] and clamps Y into [0, MAP_HEIGHT-1].
	 * The map wraps east-west (CoM::MAP_WRAP_X) but not north-south.
	 */
	UFUNCTION(BlueprintPure, Category = "WorldMap")
	static FIntPoint NormalizePosition(int32 X, int32 Y);

	/** Returns flat tile index for (X, Y) with WrapX applied. */
	UFUNCTION(BlueprintPure, Category = "WorldMap")
	static int32 TileIndex(int32 X, int32 Y);

	/** Returns true if (X, Y) is within bounds after clamping/wrapping. Always true for valid Y. */
	UFUNCTION(BlueprintPure, Category = "WorldMap")
	static bool IsValidPosition(int32 X, int32 Y);

	// ── Fog of War ────────────────────────────────────────────────────────

	/**
	 * Marks a tile as revealed (ever seen) by a wizard.
	 * Sets bit WizardIndex in FogRevealed on the tile.
	 * WizardIndex must be in [0, MAX_WIZARDS-1].
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldMap|Fog")
	void RevealTile(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y, int32 WizardIndex);

	/**
	 * Sets whether a wizard currently has vision on a tile (CurrentVision bitmask).
	 * Revealing vision also sets FogRevealed.
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldMap|Fog")
	void SetCurrentVision(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y,
	                      int32 WizardIndex, bool bVisible);

	/** Returns true if wizard WizardIndex has ever seen this tile. */
	UFUNCTION(BlueprintPure, Category = "WorldMap|Fog")
	bool IsTileRevealed(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y,
	                    int32 WizardIndex) const;

	/** Returns true if wizard WizardIndex currently sees this tile. */
	UFUNCTION(BlueprintPure, Category = "WorldMap|Fog")
	bool IsTileVisible(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y,
	                   int32 WizardIndex) const;

	/**
	 * Clears CurrentVision for ALL tiles on a given plane+layer for a specific wizard.
	 * Call each turn before recalculating vision from army/city ranges.
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldMap|Fog")
	void ClearCurrentVision(ECoMPlane Plane, ECoMMapLayer Layer, int32 WizardIndex);

	/**
	 * Clears CurrentVision across ALL 24 layers for a wizard.
	 * Use at start-of-turn before vision recalc sweep.
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldMap|Fog")
	void ClearAllCurrentVision(int32 WizardIndex);

	/** Debug: reveal all tiles on all layers for a wizard. */
	UFUNCTION(BlueprintCallable, Category = "WorldMap|Fog|Debug")
	void DebugRevealAll(int32 WizardIndex);

	/** Debug: reveal all tiles on all layers for ALL wizards. */
	UFUNCTION(BlueprintCallable, Category = "WorldMap|Fog|Debug")
	void DebugRevealEverything();

	// ── Neighbor Queries ──────────────────────────────────────────────────

	/**
	 * Returns the up-to-8 tiles adjacent to (X, Y).
	 * Respects WrapX — west of (0,Y) = (MAP_WIDTH-1, Y).
	 * Excludes tiles that would be out of Y bounds (no WrapY).
	 */
	UFUNCTION(BlueprintPure, Category = "WorldMap")
	TArray<FIntPoint> GetAdjacentPositions(int32 X, int32 Y) const;

	/**
	 * Collects all tile positions within ManhattanRange tiles of (CenterX, CenterY).
	 * WrapX is applied. Useful for fog-of-war range circles.
	 */
	UFUNCTION(BlueprintPure, Category = "WorldMap")
	TArray<FIntPoint> GetPositionsInRange(int32 CenterX, int32 CenterY, int32 Range) const;

	// ── Statistics ────────────────────────────────────────────────────────

	/** Returns number of tiles revealed (ever seen) by a wizard on a specific layer. */
	UFUNCTION(BlueprintPure, Category = "WorldMap|Stats")
	int32 CountRevealedTiles(ECoMPlane Plane, ECoMMapLayer Layer, int32 WizardIndex) const;

	/** Returns total tile count per layer (MAP_WIDTH × MAP_HEIGHT). */
	UFUNCTION(BlueprintPure, Category = "WorldMap|Stats")
	static int32 TilesPerLayer() { return CoM::MAP_TILES_PER_LAYER; }

	/** Returns total allocated tiles across all 24 layers. */
	UFUNCTION(BlueprintPure, Category = "WorldMap|Stats")
	static int32 TotalTiles() { return CoMMap::TOTAL_LAYERS * CoM::MAP_TILES_PER_LAYER; }

private:
	/**
	 * Flat array of all 15 FCoMMapLayerData entries.
	 * Index via CoMMap::LayerIndex(Plane, Layer).
	 * Stored here (not as a TMap) for O(1) cache-friendly access.
	 */
	TArray<FCoMMapLayerData> Layers;

	/** True once InitializeLayers() has successfully populated Layers. */
	bool bLayersReady = false;

	/** Internal helper: validates wizard index and logs error if out of range. */
	bool IsValidWizardIndex(int32 WizardIndex) const;
};
