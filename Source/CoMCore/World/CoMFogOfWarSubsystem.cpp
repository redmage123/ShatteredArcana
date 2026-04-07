// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMFogOfWarSubsystem.h"

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
	ExploredMask.Empty();
	VisibleMask.Empty();
}

void UCoMFogOfWarSubsystem::Deinitialize()
{
	ExploredMask.Empty();
	VisibleMask.Empty();
	Super::Deinitialize();
}

// =====================================================================
// Full-turn update
// =====================================================================

void UCoMFogOfWarSubsystem::UpdateAllVision()
{
	ClearCurrentVision();

	// TODO: Query the TurnManager for the list of active wizard indices.
	// For now, iterate wizards 0..31 and rely on the fact that
	// UpdateVisionForWizard is a no-op when there are no armies.

	// Placeholder: update vision for wizards 0..7 (typical max game size).
	for (int32 i = 0; i < 8; ++i)
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
	VisibleMask.Empty();
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

bool UCoMFogOfWarSubsystem::IsTileVisible(int32 WizardIndex, ECoMPlane Plane, FIntPoint Tile) const
{
	const TMap<FIntPoint, uint32>& Map = GetVisibleMapConst(Plane);
	if (const uint32* Mask = Map.Find(Tile))
	{
		return (*Mask & WizardBit(WizardIndex)) != 0;
	}
	return false;
}

bool UCoMFogOfWarSubsystem::IsTileExplored(int32 WizardIndex, ECoMPlane Plane, FIntPoint Tile) const
{
	const TMap<FIntPoint, uint32>& Map = GetExploredMapConst(Plane);
	if (const uint32* Mask = Map.Find(Tile))
	{
		return (*Mask & WizardBit(WizardIndex)) != 0;
	}
	return false;
}

// =====================================================================
// Direct manipulation
// =====================================================================

void UCoMFogOfWarSubsystem::RevealTile(int32 WizardIndex, ECoMPlane Plane, FIntPoint Tile)
{
	const uint32 Bit = WizardBit(WizardIndex);

	// Set explored (permanent).
	uint32& ExploredBits = GetExploredMap(Plane).FindOrAdd(Tile, 0);
	ExploredBits |= Bit;

	// Set visible (cleared each turn).
	uint32& VisibleBits = GetVisibleMap(Plane).FindOrAdd(Tile, 0);
	VisibleBits |= Bit;
}

void UCoMFogOfWarSubsystem::RevealArea(int32 WizardIndex, ECoMPlane Plane, FIntPoint Center, int32 Radius)
{
	// Reveal all tiles within Manhattan distance <= Radius.
	for (int32 DX = -Radius; DX <= Radius; ++DX)
	{
		const int32 RemainingY = Radius - FMath::Abs(DX);
		for (int32 DY = -RemainingY; DY <= RemainingY; ++DY)
		{
			const FIntPoint Tile(Center.X + DX, Center.Y + DY);
			RevealTile(WizardIndex, Plane, Tile);
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

// =====================================================================
// Internal helpers
// =====================================================================

TMap<FIntPoint, uint32>& UCoMFogOfWarSubsystem::GetExploredMap(ECoMPlane Plane)
{
	return ExploredMask.FindOrAdd(static_cast<uint8>(Plane));
}

const TMap<FIntPoint, uint32>& UCoMFogOfWarSubsystem::GetExploredMapConst(ECoMPlane Plane) const
{
	static const TMap<FIntPoint, uint32> EmptyMap;
	if (const TMap<FIntPoint, uint32>* Found = ExploredMask.Find(static_cast<uint8>(Plane)))
	{
		return *Found;
	}
	return EmptyMap;
}

TMap<FIntPoint, uint32>& UCoMFogOfWarSubsystem::GetVisibleMap(ECoMPlane Plane)
{
	return VisibleMask.FindOrAdd(static_cast<uint8>(Plane));
}

const TMap<FIntPoint, uint32>& UCoMFogOfWarSubsystem::GetVisibleMapConst(ECoMPlane Plane) const
{
	static const TMap<FIntPoint, uint32> EmptyMap;
	if (const TMap<FIntPoint, uint32>* Found = VisibleMask.Find(static_cast<uint8>(Plane)))
	{
		return *Found;
	}
	return EmptyMap;
}

uint32 UCoMFogOfWarSubsystem::WizardBit(int32 WizardIndex)
{
	check(WizardIndex >= 0 && WizardIndex < 32);
	return 1u << static_cast<uint32>(WizardIndex);
}
