// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMFogOfWarSubsystem.generated.h"

class UCoMWorldMapSubsystem;

/**
 * Fog of War subsystem.
 *
 * Tracks per-wizard, per-plane vision using a bitmask approach:
 *   - Bit N of the uint32 stored at a tile = wizard N has explored / can see.
 *   - Supports up to 32 wizards per bitmask (more than enough for 4-player games).
 *
 * Two layers are maintained:
 *   ExploredMask — once set, never cleared (permanent map knowledge).
 *   VisibleMask  — cleared each turn, then rebuilt from army positions.
 *
 * Sight radius is determined by army composition:
 *   - Default:  2 tiles (Manhattan distance)
 *   - Flying:   3 tiles
 *   - Scout:    4 tiles
 *   The maximum sight radius among all units in the army is used.
 *
 * Vision is updated once per turn by UpdateAllVision(), called from
 * the Turn Manager during the FogOfWar phase.
 */
UCLASS()
class COMCORE_API UCoMFogOfWarSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -----------------------------------------------------------------
	// Full-turn update
	// -----------------------------------------------------------------

	/** Clear all VisibleMasks and rebuild vision for every wizard. */
	UFUNCTION(BlueprintCallable, Category = "CoM|FogOfWar")
	void UpdateAllVision();

	// -----------------------------------------------------------------
	// Per-wizard update
	// -----------------------------------------------------------------

	/** Clear the visible mask for all planes (does NOT touch explored). */
	UFUNCTION(BlueprintCallable, Category = "CoM|FogOfWar")
	void ClearCurrentVision();

	/**
	 * Iterate all armies owned by WizardIndex and reveal tiles within
	 * each army's sight radius.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|FogOfWar")
	void UpdateVisionForWizard(int32 WizardIndex);

	// -----------------------------------------------------------------
	// Queries
	// -----------------------------------------------------------------

	/** True if the wizard can currently see the tile (this turn). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|FogOfWar")
	bool IsTileVisible(int32 WizardIndex, ECoMPlane Plane, FIntPoint Tile, ECoMMapLayer Layer = ECoMMapLayer::Surface) const;

	/** True if the wizard has ever seen the tile. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|FogOfWar")
	bool IsTileExplored(int32 WizardIndex, ECoMPlane Plane, FIntPoint Tile, ECoMMapLayer Layer = ECoMMapLayer::Surface) const;

	// -----------------------------------------------------------------
	// Direct manipulation
	// -----------------------------------------------------------------

	/**
	 * Mark a tile as both explored and visible for the given wizard.
	 * Used by armies, scouts, spells, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|FogOfWar")
	void RevealTile(int32 WizardIndex, ECoMPlane Plane, FIntPoint Tile, ECoMMapLayer Layer = ECoMMapLayer::Surface);

	/**
	 * Reveal all tiles within Manhattan distance <= Radius around Center
	 * for the given wizard on the given plane and layer.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|FogOfWar")
	void RevealArea(int32 WizardIndex, ECoMPlane Plane, FIntPoint Center, int32 Radius, ECoMMapLayer Layer = ECoMMapLayer::Surface);

	// -----------------------------------------------------------------
	// Sight radius
	// -----------------------------------------------------------------

	/**
	 * Return the sight radius for a given army.
	 * Base 2 for most units, 4 for scouts, 3 for flying.
	 * Uses the maximum across all units in the army (placeholder logic
	 * until the army/unit subsystem is wired).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|FogOfWar")
	int32 GetSightRadius(int32 ArmyID) const;

private:
	// -----------------------------------------------------------------
	// Delegation target — all fog state lives on WorldMapSubsystem tiles
	// -----------------------------------------------------------------

	UPROPERTY()
	TObjectPtr<UCoMWorldMapSubsystem> WorldMapSub;
};
