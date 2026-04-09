// Copyright Mythforge Studios. All Rights Reserved.
// CoMTileChunkActor.h -- A single 10x10 tile chunk rendered as a textured plane.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMTileChunkActor.generated.h"

class UCoMWorldMapSubsystem;
class UCoMFogOfWarSubsystem;
class UTexture2D;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * ACoMTileChunkActor
 *
 * Represents one 10x10 section of the overworld tile map as a single flat
 * mesh with a dynamically composited terrain texture. The texture is built
 * by sampling terrain types from the WorldMapSubsystem and compositing
 * 128x128 tile art into a 1280x1280 chunk texture.
 *
 * Fog of war is applied as a per-pixel colour modulation:
 *   - Unexplored tiles: solid black
 *   - Explored but not currently visible: 40% brightness (60% darkened)
 *   - Currently visible: full brightness
 *
 * Chunk lifecycle is managed by UCoMOverworldRenderer, which spawns/despawns
 * chunks as the camera moves across the map.
 */
UCLASS(NotBlueprintable)
class COMRENDERING_API ACoMTileChunkActor : public AActor
{
	GENERATED_BODY()

public:
	ACoMTileChunkActor();

	// ---- Configuration (set by UCoMOverworldRenderer before building) --------

	/** Top-left tile coordinate of this chunk in the 160x100 grid. */
	FIntPoint ChunkOrigin = FIntPoint(0, 0);

	/** Number of tiles per side in this chunk. */
	int32 ChunkSize = 10;

	/** The plane this chunk belongs to. */
	ECoMPlane Plane = ECoMPlane::Aurelith;

	/** The map layer this chunk renders. */
	ECoMMapLayer Layer = ECoMMapLayer::Surface;

	// ---- Build / Rebuild ---------------------------------------------------

	/**
	 * Build (or rebuild) the chunk texture by reading terrain data from the
	 * WorldMapSubsystem and fog state from the FogOfWarSubsystem.
	 *
	 * @param MapSub       World map subsystem (terrain queries).
	 * @param FogSub       Fog of war subsystem (visibility queries).
	 * @param WizardIndex  The local player's wizard index (for fog queries).
	 */
	void BuildChunkTexture(UCoMWorldMapSubsystem* MapSub, UCoMFogOfWarSubsystem* FogSub,
	                       int32 WizardIndex);

	/** Returns true if BuildChunkTexture has been called at least once. */
	bool IsBuilt() const { return bIsBuilt; }

	/** Mark the chunk as needing a texture rebuild on next frame. */
	void MarkDirty() { bDirty = true; }

	/** Returns true if the chunk needs a texture rebuild. */
	bool IsDirty() const { return bDirty; }

	// ---- Components --------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chunk")
	TObjectPtr<UStaticMeshComponent> ChunkMesh;

private:
	/** Dynamically created material instance applied to ChunkMesh. */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ChunkMID;

	/** The composited chunk texture (1280x1280 for 10x10 tiles at 128px). */
	UPROPERTY()
	TObjectPtr<UTexture2D> ChunkTexture;

	bool bIsBuilt = false;
	bool bDirty = false;

	// ---- Internal helpers --------------------------------------------------

	/** Create or retrieve the 1280x1280 transient texture. */
	UTexture2D* GetOrCreateChunkTexture();

	/**
	 * Look up the terrain colour for a given terrain type.
	 * Until full terrain art assets are available, this returns a
	 * representative colour per terrain category.
	 */
	static FColor GetTerrainColor(ECoMTerrain TerrainType);

	/**
	 * Apply fog of war modulation to a tile's colour.
	 * @param BaseColor   Original terrain colour.
	 * @param bExplored   True if the wizard has ever seen this tile.
	 * @param bVisible    True if the wizard currently sees this tile.
	 * @return            Modulated colour.
	 */
	static FColor ApplyFogOfWar(FColor BaseColor, bool bExplored, bool bVisible);

	/** Pixel size of each tile in the chunk texture. */
	static constexpr int32 TILE_PIXEL_SIZE = 128;
};
