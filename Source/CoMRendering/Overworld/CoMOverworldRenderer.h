// Copyright Mythforge Studios. All Rights Reserved.
// CoMOverworldRenderer.h -- Main rendering coordinator for the overworld tile map.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMOverworldRenderer.generated.h"

class ACoMTileChunkActor;
class ACoMCityIconActor;
class ACoMArmyIconActor;
class UCoMWorldMapSubsystem;
class UCoMFogOfWarSubsystem;
class UCoMCitySubsystem;
class UCoMUnitSubsystem;

// Click result — what the player clicked on the overworld map
UENUM(BlueprintType)
enum class ECoMOverworldClickTarget : uint8
{
	None,
	Tile,
	City,
	Army
};

USTRUCT(BlueprintType)
struct COMRENDERING_API FCoMOverworldClickResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ECoMOverworldClickTarget Target = ECoMOverworldClickTarget::None;

	UPROPERTY(BlueprintReadOnly)
	FIntPoint TilePosition = FIntPoint(-1, -1);

	UPROPERTY(BlueprintReadOnly)
	int32 CityID = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 ArmyID = -1;
};

// Delegates for click events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTileClicked, FIntPoint, TilePos);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityClicked, int32, CityID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArmyClicked, int32, ArmyID);

/**
 * UCoMOverworldRenderer
 *
 * GameInstance subsystem that manages the visual representation of the overworld.
 * It does not do direct GPU rendering; instead it manages ACoMTileChunkActor
 * instances that UE5 renders through the standard pipeline.
 *
 * The 160x100 tile map is divided into 10x10 chunks (16 columns x 10 rows).
 * Only chunks visible to the camera (plus a 1-chunk buffer) are spawned.
 * As the camera pans, chunks entering the viewport are created and chunks
 * leaving are despawned or pooled.
 *
 * City and army icons are separate billboard actors placed above the tile plane.
 *
 * Coordinate system:
 *   Tile (X, Y) -> World (X * TILE_WORLD_SIZE_CM, Y * TILE_WORLD_SIZE_CM, 0)
 *   WrapX: tile X wraps at MAP_WIDTH (160)
 *
 * Usage:
 *   After the world map is initialised, call RenderPlane() to build the initial
 *   view. Call UpdateVisibleChunks() each frame from CameraPawn::Tick().
 */
UCLASS()
class COMRENDERING_API UCoMOverworldRenderer : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── USubsystem interface ────────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// ── Plane rendering ─────────────────────────────────────────────────────

	/**
	 * Set the active plane and layer to render, and build all visible chunks.
	 * This is the main entry point — call after world map is initialised.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Overworld")
	void RenderPlane(ECoMPlane Plane, ECoMMapLayer Layer, int32 WizardIndex);

	/**
	 * Update which chunks are visible based on current camera position/zoom.
	 * Call this every frame from ACoMCameraPawn::Tick().
	 *
	 * @param CameraPosition  The camera pawn's world-space location.
	 * @param ZoomLevel       Current spring arm length (controls viewport extents).
	 */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Overworld")
	void UpdateVisibleChunks(FVector CameraPosition, float ZoomLevel);

	/**
	 * Force a full redraw of all active chunks.
	 * Use on plane change, turn end, or after world-gen.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Overworld")
	void RefreshAllChunks();

	/**
	 * Refresh the single chunk containing the given tile position.
	 * Use for localised changes (tile corruption, road placement, etc.).
	 */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Overworld")
	void RefreshChunkAt(FIntPoint TilePos);

	// ── Overlay management ──────────────────────────────────────────────────

	/** Spawn city icon actors for all cities on the current plane. */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Overworld")
	void SpawnCityIcons();

	/** Spawn army icon actors for all armies on the current plane. */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Overworld")
	void SpawnArmySprites();

	/** Update positions of existing army icons (after movement). */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Overworld")
	void UpdateArmyPositions();

	/** Remove all overlay actors (city icons, army sprites). */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Overworld")
	void ClearOverlays();

	// ── Coordinate conversion ───────────────────────────────────────────────

	/** Convert a world-space position to a tile coordinate. */
	UFUNCTION(BlueprintPure, Category = "Rendering|Overworld")
	FIntPoint WorldToTile(FVector WorldPos) const;

	/** Convert a tile coordinate to a world-space position (center of tile). */
	UFUNCTION(BlueprintPure, Category = "Rendering|Overworld")
	FVector TileToWorld(FIntPoint TilePos) const;

	// ── Click handling ──────────────────────────────────────────────────────

	/**
	 * Perform a hit test at the given world position and return what was clicked.
	 * Checks army icons, city icons, and tiles in priority order.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Overworld")
	FCoMOverworldClickResult HandleClick(FVector WorldHitPosition) const;

	// ── Delegates ───────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Rendering|Overworld")
	FOnTileClicked OnTileClicked;

	UPROPERTY(BlueprintAssignable, Category = "Rendering|Overworld")
	FOnCityClicked OnCityClicked;

	UPROPERTY(BlueprintAssignable, Category = "Rendering|Overworld")
	FOnArmyClicked OnArmyClicked;

	// ── World context ───────────────────────────────────────────────────────

	/**
	 * Set the UWorld pointer used for spawning actors.
	 * Must be called before RenderPlane(). The rendering bridge actor
	 * (ACoMOverworldRenderingActor) does this automatically on BeginPlay.
	 */
	void SetWorldContext(UWorld* InWorld) { WorldContext = TWeakObjectPtr<UWorld>(InWorld); }

	// ── State queries ───────────────────────────────────────────────────────

	ECoMPlane GetCurrentPlane() const { return CurrentPlane; }
	ECoMMapLayer GetCurrentLayer() const { return CurrentLayer; }
	int32 GetCurrentWizardId() const { return CurrentWizardId; }

private:
	// ── Chunk management ────────────────────────────────────────────────────

	/** Spawn a chunk actor for the given chunk grid coordinate. */
	ACoMTileChunkActor* SpawnChunk(FIntPoint ChunkCoord);

	/** Despawn and destroy a chunk actor. */
	void DespawnChunk(ACoMTileChunkActor* Chunk);

	/** Calculate which chunk grid coordinates should be visible. */
	TArray<FIntPoint> CalcVisibleChunkCoords(FVector CameraPos, float ZoomLevel) const;

	/** Convert a tile position to the chunk grid coordinate that contains it. */
	FIntPoint TileToChunkCoord(FIntPoint TilePos) const;

	/** Convert a chunk grid coordinate to the top-left tile coordinate. */
	FIntPoint ChunkCoordToTileOrigin(FIntPoint ChunkCoord) const;

	// ── Subsystem caches ────────────────────────────────────────────────────

	UPROPERTY()
	TObjectPtr<UCoMWorldMapSubsystem> WorldMapSub;

	UPROPERTY()
	TObjectPtr<UCoMFogOfWarSubsystem> FogSub;

	UPROPERTY()
	TObjectPtr<UCoMCitySubsystem> CitySub;

	UPROPERTY()
	TObjectPtr<UCoMUnitSubsystem> UnitSub;

	// ── Active state ────────────────────────────────────────────────────────

	/**
	 * Map from chunk grid coord (encoded as int64) to spawned chunk actor.
	 * Not a UPROPERTY — chunk actors are world-owned and tracked by GC via
	 * the world's actor list. We just maintain lookup references here.
	 */
	TMap<int64, TWeakObjectPtr<ACoMTileChunkActor>> ActiveChunkMap;

	/** City overlay actors on the current plane. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> CityIconActors;

	/** Army overlay actors on the current plane. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> ArmyIconActors;

	/** Cached world pointer for actor spawning. Set via SetWorldContext().
	 *  Not a UPROPERTY — the world's lifetime exceeds the subsystem's. */
	TWeakObjectPtr<UWorld> WorldContext;

	ECoMPlane CurrentPlane = ECoMPlane::Aurelith;
	ECoMMapLayer CurrentLayer = ECoMMapLayer::Surface;
	int32 CurrentWizardId = 0;
	bool bInitialRenderDone = false;

	// ── Constants ───────────────────────────────────────────────────────────

	static constexpr int32 CHUNK_SIZE = 10;
	static constexpr int32 CHUNKS_X = CoM::MAP_WIDTH / CHUNK_SIZE;   // 16
	static constexpr int32 CHUNKS_Y = CoM::MAP_HEIGHT / CHUNK_SIZE;  // 10
	static constexpr float TILE_WORLD_SIZE = CoM::TILE_WORLD_SIZE_CM;

	/** Encode a 2D chunk coordinate into a single int64 key for TMap. */
	static int64 ChunkKey(FIntPoint Coord)
	{
		return static_cast<int64>(Coord.X) << 32 | (static_cast<int64>(Coord.Y) & 0xFFFFFFFF);
	}
};
