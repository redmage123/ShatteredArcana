// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMFogOfWarSubsystem.h"
#include "CoMWorldMapSubsystem.h"
#include "CoMCore/CoreTypes/CoMConstants.h"

// =====================================================================
// Constants
// =====================================================================

namespace CoMFogConstants
{
	static constexpr int32 DefaultSightRadius = 2;
	static constexpr int32 FlyingSightRadius  = 3;
	static constexpr int32 ScoutSightRadius   = 4;
}

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMFogOfWarSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldMapSub = GetGameInstance()->GetSubsystem<UCoMWorldMapSubsystem>();
}

void UCoMFogOfWarSubsystem::Deinitialize()
{
	WorldMapSub = nullptr;
	Super::Deinitialize();
}

// =====================================================================
// Full-turn update
// =====================================================================

void UCoMFogOfWarSubsystem::UpdateAllVision()
{
	ClearCurrentVision();

	// Placeholder: update vision for wizards 0..MAX_WIZARDS-1.
	// UpdateVisionForWizard is a no-op when there are no armies.
	for (int32 i = 0; i < CoM::MAX_WIZARDS; ++i)
	{
		UpdateVisionForWizard(i);
	}

	UE_LOG(LogTemp, Log, TEXT("CoMFogOfWar: Vision updated for all wizards."));
}

// =====================================================================
// Per-wizard update
// =====================================================================

void UCoMFogOfWarSubsystem::ClearCurrentVision()
{
	if (!WorldMapSub)
	{
		return;
	}

	for (int32 WizIdx = 0; WizIdx < CoM::MAX_WIZARDS; ++WizIdx)
	{
		WorldMapSub->ClearAllCurrentVision(WizIdx);
	}
}

void UCoMFogOfWarSubsystem::UpdateVisionForWizard(int32 WizardIndex)
{
	// TODO: Query the army subsystem for all armies owned by WizardIndex.
	// For each army:
	//   1. Get the army's position (plane + tile).
	//   2. Get the army's sight radius via GetSightRadius(ArmyID).
	//   3. Call RevealArea(WizardIndex, Plane, Position, Radius).
	//
	// Placeholder: no army subsystem wired yet, so this is a no-op.
	// Individual calls to RevealTile / RevealArea can still be made
	// directly by other subsystems (e.g. console commands for testing).
}

// =====================================================================
// Queries
// =====================================================================

bool UCoMFogOfWarSubsystem::IsTileVisible(int32 WizardIndex, ECoMPlane Plane, FIntPoint Tile, ECoMMapLayer Layer) const
{
	if (!WorldMapSub)
	{
		return false;
	}
	return WorldMapSub->IsTileVisible(Plane, Layer, Tile.X, Tile.Y, WizardIndex);
}

bool UCoMFogOfWarSubsystem::IsTileExplored(int32 WizardIndex, ECoMPlane Plane, FIntPoint Tile, ECoMMapLayer Layer) const
{
	if (!WorldMapSub)
	{
		return false;
	}
	return WorldMapSub->IsTileRevealed(Plane, Layer, Tile.X, Tile.Y, WizardIndex);
}

// =====================================================================
// Direct manipulation
// =====================================================================

void UCoMFogOfWarSubsystem::RevealTile(int32 WizardIndex, ECoMPlane Plane, FIntPoint Tile, ECoMMapLayer Layer)
{
	if (!WorldMapSub)
	{
		return;
	}
	WorldMapSub->RevealTile(Plane, Layer, Tile.X, Tile.Y, WizardIndex);
	WorldMapSub->SetCurrentVision(Plane, Layer, Tile.X, Tile.Y, WizardIndex, true);
}

void UCoMFogOfWarSubsystem::RevealArea(int32 WizardIndex, ECoMPlane Plane, FIntPoint Center, int32 Radius, ECoMMapLayer Layer)
{
	if (!WorldMapSub)
	{
		return;
	}

	// Reveal all tiles within Manhattan distance <= Radius.
	for (int32 DX = -Radius; DX <= Radius; ++DX)
	{
		const int32 RemainingY = Radius - FMath::Abs(DX);
		for (int32 DY = -RemainingY; DY <= RemainingY; ++DY)
		{
			const int32 WrappedX = ((Center.X + DX) % CoM::MAP_WIDTH + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
			const int32 ClampedY = FMath::Clamp(Center.Y + DY, 0, CoM::MAP_HEIGHT - 1);
			RevealTile(WizardIndex, Plane, FIntPoint(WrappedX, ClampedY), Layer);
		}
	}
}

// =====================================================================
// Sight radius
// =====================================================================

int32 UCoMFogOfWarSubsystem::GetSightRadius(int32 ArmyID) const
{
	// TODO: Query the army/unit subsystem to determine the maximum sight
	// radius among all units in the army.  Categories:
	//   - Scout units:   4 tiles
	//   - Flying units:  3 tiles
	//   - All other:     2 tiles
	//
	// Placeholder: return the default until unit data is wired.
	return CoMFogConstants::DefaultSightRadius;
}
