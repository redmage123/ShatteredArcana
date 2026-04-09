// Copyright Mythforge Studios. All Rights Reserved.
// CoMMinimapSubsystem.h -- Data layer for the overworld minimap.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMMinimapSubsystem.generated.h"

class UCoMWorldMapSubsystem;
class UCoMFogOfWarSubsystem;
class UCoMCitySubsystem;
class UCoMUnitSubsystem;

/**
 * UCoMMinimapSubsystem
 *
 * Lightweight GameInstance subsystem that provides the minimap data layer.
 * Owns a pixel buffer (160x100 FColor array) representing the current plane.
 * The UI widget reads this buffer to update a UTexture2D -- keeping rendering
 * concerns out of CoMCore (no RHI/Renderer dependency).
 *
 * Workflow:
 *   1. InitializeForPlane() -- sets plane and wizard context, builds full terrain buffer
 *   2. RefreshTerrain()     -- rebuild terrain colors from world map
 *   3. UpdateFog()          -- overlay fog of war (darken/black out tiles)
 *   4. UpdateMarkers()      -- stamp city and army dots onto the buffer
 *   5. UpdateCameraRect()   -- record camera viewport for the widget to draw
 *   6. GetPixelBuffer()     -- widget reads the final composited buffer
 */
UCLASS()
class COMCORE_API UCoMMinimapSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// -- USubsystem interface -------------------------------------------------
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -- Lifecycle ------------------------------------------------------------

	/** Set up for a specific plane and wizard. Rebuilds the full pixel buffer. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void InitializeForPlane(ECoMPlane Plane, int32 WizardId);

	/** Switch the displayed plane and rebuild. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void SetPlane(ECoMPlane Plane);

	// -- Refresh Methods ------------------------------------------------------

	/** Full terrain rebuild from world map data. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void RefreshTerrain();

	/** Apply fog of war overlay to the terrain buffer. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void UpdateFog();

	/**
	 * Stamp city and army markers onto the composited buffer.
	 * Arrays contain world-space tile positions.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void UpdateMarkers(const TArray<FIntPoint>& PlayerCities, const TArray<FIntPoint>& EnemyCities,
	                   const TArray<FIntPoint>& NeutralCities,
	                   const TArray<FIntPoint>& PlayerArmies, const TArray<FIntPoint>& EnemyArmies);

	/** Record the camera viewport rectangle (in tile coordinates). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void UpdateCameraRect(FIntPoint TopLeft, FIntPoint BottomRight);

	// -- Queries --------------------------------------------------------------

	/** Access the composited pixel buffer (MAP_WIDTH x MAP_HEIGHT). */
	const TArray<FColor>& GetPixelBuffer() const { return CompositedBuffer; }

	/** Access the base terrain buffer (before fog/markers). */
	const TArray<FColor>& GetTerrainBuffer() const { return TerrainBuffer; }

	/** Camera viewport top-left in tile coords. */
	FIntPoint GetCameraTopLeft() const { return CameraTopLeft; }

	/** Camera viewport bottom-right in tile coords. */
	FIntPoint GetCameraBottomRight() const { return CameraBottomRight; }

	/** Convert a UV coordinate [0,1] on the minimap to a world tile position. */
	UFUNCTION(BlueprintPure, Category = "CoM|Minimap")
	FIntPoint ScreenToWorldPosition(FVector2D MinimapUV) const;

	/** Get the currently displayed plane. */
	UFUNCTION(BlueprintPure, Category = "CoM|Minimap")
	ECoMPlane GetCurrentPlane() const { return CurrentPlane; }

	/** Get the current wizard index. */
	UFUNCTION(BlueprintPure, Category = "CoM|Minimap")
	int32 GetCurrentWizardId() const { return CurrentWizardId; }

	// -- Terrain Color Lookup -------------------------------------------------

	/** Map a terrain enum to its minimap display color. */
	UFUNCTION(BlueprintPure, Category = "CoM|Minimap")
	static FColor GetTerrainColor(ECoMTerrain Terrain);

	/** Get the display name for a plane. */
	UFUNCTION(BlueprintPure, Category = "CoM|Minimap")
	static FString GetPlaneDisplayName(ECoMPlane Plane);

private:
	/** Stamp a single pixel into CompositedBuffer (bounds-checked). */
	void SetPixel(int32 X, int32 Y, FColor Color);

	/** Stamp a 2x2 block of pixels into CompositedBuffer (bounds-checked). */
	void SetPixel2x2(int32 X, int32 Y, FColor Color);

	// -- State ----------------------------------------------------------------

	UPROPERTY()
	TObjectPtr<UCoMWorldMapSubsystem> WorldMapSub;

	UPROPERTY()
	TObjectPtr<UCoMFogOfWarSubsystem> FogSub;

	UPROPERTY()
	TObjectPtr<UCoMCitySubsystem> CitySub;

	UPROPERTY()
	TObjectPtr<UCoMUnitSubsystem> UnitSub;

	/** Base terrain colors (no fog, no markers). */
	TArray<FColor> TerrainBuffer;

	/** Final composited buffer: terrain + fog + markers. */
	TArray<FColor> CompositedBuffer;

	ECoMPlane CurrentPlane = ECoMPlane::Aurelith;
	int32 CurrentWizardId = 0;

	FIntPoint CameraTopLeft = FIntPoint::ZeroValue;
	FIntPoint CameraBottomRight = FIntPoint(20, 15);
};
