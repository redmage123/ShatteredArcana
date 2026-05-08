// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMWorldMapSubsystem.cpp — World map storage and query subsystem.
// COM-026: 24-layer map (8 planes x 3 layers), WrapX, fog-of-war bitmasks.

#include "World/CoMWorldMapSubsystem.h"

// ─────────────────────────────────────────────────────────────────────────────
// USubsystem interface
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldMapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Layers are not populated here — world generation calls InitializeLayers()
	// explicitly once terrain gen is ready to run.
}

void UCoMWorldMapSubsystem::Deinitialize()
{
	Layers.Empty();
	Sites.Empty();
	NextSiteID = 1;
	bLayersReady = false;
	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Sites
// ─────────────────────────────────────────────────────────────────────────────

int32 UCoMWorldMapSubsystem::RegisterSite(const FCoMSite& Site)
{
	FCoMSite Copy = Site;
	if (Copy.SiteID <= 0)
	{
		Copy.SiteID = NextSiteID++;
	}
	else
	{
		NextSiteID = FMath::Max(NextSiteID, Copy.SiteID + 1);
	}
	Sites.Add(Copy.SiteID, Copy);
	return Copy.SiteID;
}

const FCoMSite* UCoMWorldMapSubsystem::GetSite(int32 SiteID) const
{
	return Sites.Find(SiteID);
}

FCoMSite* UCoMWorldMapSubsystem::GetSiteMutable(int32 SiteID)
{
	return Sites.Find(SiteID);
}

TArray<FCoMSite> UCoMWorldMapSubsystem::GetAllSites() const
{
	TArray<FCoMSite> Out;
	Out.Reserve(Sites.Num());
	for (const auto& Pair : Sites) { Out.Add(Pair.Value); }
	return Out;
}

TArray<FCoMSite> UCoMWorldMapSubsystem::GetSitesOnPlane(ECoMPlane Plane) const
{
	TArray<FCoMSite> Out;
	for (const auto& Pair : Sites)
	{
		if (Pair.Value.Plane == Plane) { Out.Add(Pair.Value); }
	}
	return Out;
}

void UCoMWorldMapSubsystem::ExportAllSites(TArray<FCoMSite>& OutSites) const
{
	OutSites.Reset();
	for (const auto& Pair : Sites) { OutSites.Add(Pair.Value); }
}

void UCoMWorldMapSubsystem::ImportAllSites(const TArray<FCoMSite>& InSites)
{
	Sites.Empty();
	NextSiteID = 1;
	for (const FCoMSite& S : InSites)
	{
		Sites.Add(S.SiteID, S);
		NextSiteID = FMath::Max(NextSiteID, S.SiteID + 1);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Map Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldMapSubsystem::InitializeLayers()
{
	Layers.Empty();
	Layers.SetNum(CoM::TOTAL_MAP_LAYERS);

	const TArray<ECoMPlane> AllPlanes =
	{
		ECoMPlane::Aurelith,
		ECoMPlane::Noctharion,
		ECoMPlane::Verdantis,
		ECoMPlane::Infernyx,
		ECoMPlane::Aethermist,
		ECoMPlane::Abyssal,
		ECoMPlane::Ethereal,
		ECoMPlane::Feywild
	};

	const TArray<ECoMMapLayer> AllLayers =
	{
		ECoMMapLayer::Surface,
		ECoMMapLayer::Underdark,
		ECoMMapLayer::Underwater
	};

	for (const ECoMPlane Plane : AllPlanes)
	{
		for (const ECoMMapLayer LayerType : AllLayers)
		{
			const int32 Idx = CoMMap::LayerIndex(Plane, LayerType);
			FCoMMapLayerData& LayerData = Layers[Idx];
			LayerData.Plane = Plane;
			LayerData.Layer = LayerType;

			// Allocate tiles — all 16,000 per layer regardless of type.
			// Underwater tiles start impassable; world gen un-marks valid ocean sub-zones.
			LayerData.Tiles.SetNum(CoM::MAP_TILES_PER_LAYER);

			for (int32 Y = 0; Y < CoM::MAP_HEIGHT; ++Y)
			{
				for (int32 X = 0; X < CoM::MAP_WIDTH; ++X)
				{
					FCoMTileData& Tile = LayerData.Tiles[Y * CoM::MAP_WIDTH + X];
					Tile.Position = FIntPoint(X, Y);
					Tile.FogRevealed    = 0u;
					Tile.CurrentVision  = 0u;
					Tile.OwnerWizardIndex = CoM::WIZARD_INDEX_NONE;
					Tile.CityID         = -1;
					Tile.PortalID       = -1;
					Tile.MineID         = -1;
					Tile.SiteID         = -1;
					Tile.RoadLevel      = 0;
					Tile.MoveCostModifier = FFixed64(1);

					// Underwater layers are impassable by default;
					// terrain for Surface/Underdark defaults to Grassland (world gen overwrites).
					Tile.bImpassable = (LayerType == ECoMMapLayer::Underwater);
				}
			}
		}
	}

	bLayersReady = true;

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMWorldMapSubsystem: Initialised %d layers (%d tiles each, %d total)."),
	       CoM::TOTAL_MAP_LAYERS, CoM::MAP_TILES_PER_LAYER, TotalTiles());
}

// ─────────────────────────────────────────────────────────────────────────────
// Layer Accessors
// ─────────────────────────────────────────────────────────────────────────────

const FCoMMapLayerData& UCoMWorldMapSubsystem::GetLayer(ECoMPlane Plane, ECoMMapLayer Layer) const
{
	const int32 Idx = CoMMap::LayerIndex(Plane, Layer);
	check(Layers.IsValidIndex(Idx));
	return Layers[Idx];
}

FCoMMapLayerData& UCoMWorldMapSubsystem::GetLayerMutable(ECoMPlane Plane, ECoMMapLayer Layer)
{
	const int32 Idx = CoMMap::LayerIndex(Plane, Layer);
	check(Layers.IsValidIndex(Idx));
	return Layers[Idx];
}

// ─────────────────────────────────────────────────────────────────────────────
// Tile Accessors
// ─────────────────────────────────────────────────────────────────────────────

const FCoMTileData* UCoMWorldMapSubsystem::GetTile(
    ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y) const
{
	if (!bLayersReady) { return nullptr; }
	return GetLayer(Plane, Layer).GetTile(X, Y);
}

FCoMTileData* UCoMWorldMapSubsystem::GetTileMutable(
    ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y)
{
	if (!bLayersReady) { return nullptr; }
	return GetLayerMutable(Plane, Layer).GetTile(X, Y);
}

const FCoMTileData* UCoMWorldMapSubsystem::GetTileAtPos(
    ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position) const
{
	return GetTile(Plane, Layer, Position.X, Position.Y);
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinate Utilities
// ─────────────────────────────────────────────────────────────────────────────

FIntPoint UCoMWorldMapSubsystem::NormalizePosition(int32 X, int32 Y)
{
	// WrapX: modulo with positive-remainder guarantee
	const int32 WrappedX = ((X % CoM::MAP_WIDTH) + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
	// ClampY: no vertical wrap
	const int32 ClampedY = FMath::Clamp(Y, 0, CoM::MAP_HEIGHT - 1);
	return FIntPoint(WrappedX, ClampedY);
}

int32 UCoMWorldMapSubsystem::TileIndex(int32 X, int32 Y)
{
	const FIntPoint Pos = NormalizePosition(X, Y);
	return Pos.Y * CoM::MAP_WIDTH + Pos.X;
}

bool UCoMWorldMapSubsystem::IsValidPosition(int32 X, int32 Y)
{
	// X always valid (wraps). Y invalid only if outside [0, MAP_HEIGHT-1].
	return Y >= 0 && Y < CoM::MAP_HEIGHT;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fog of War
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldMapSubsystem::RevealTile(
    ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y, int32 WizardIndex)
{
	if (!IsValidWizardIndex(WizardIndex)) { return; }

	FCoMTileData* Tile = GetTileMutable(Plane, Layer, X, Y);
	if (!Tile) { return; }

	const uint32 Bit = 1u << static_cast<uint32>(WizardIndex);
	Tile->FogRevealed   |= Bit;
	Tile->CurrentVision |= Bit; // Revealing implies currently visible
}

void UCoMWorldMapSubsystem::SetCurrentVision(
    ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y, int32 WizardIndex, bool bVisible)
{
	if (!IsValidWizardIndex(WizardIndex)) { return; }

	FCoMTileData* Tile = GetTileMutable(Plane, Layer, X, Y);
	if (!Tile) { return; }

	const uint32 Bit = 1u << static_cast<uint32>(WizardIndex);
	if (bVisible)
	{
		Tile->CurrentVision |= Bit;
		Tile->FogRevealed   |= Bit; // Seeing it now means it's revealed
	}
	else
	{
		Tile->CurrentVision &= ~Bit;
	}
}

bool UCoMWorldMapSubsystem::IsTileRevealed(
    ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y, int32 WizardIndex) const
{
	if (!IsValidWizardIndex(WizardIndex)) { return false; }
	const FCoMTileData* Tile = GetTile(Plane, Layer, X, Y);
	if (!Tile) { return false; }
	return (Tile->FogRevealed & (1u << static_cast<uint32>(WizardIndex))) != 0u;
}

bool UCoMWorldMapSubsystem::IsTileVisible(
    ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y, int32 WizardIndex) const
{
	if (!IsValidWizardIndex(WizardIndex)) { return false; }
	const FCoMTileData* Tile = GetTile(Plane, Layer, X, Y);
	if (!Tile) { return false; }
	return (Tile->CurrentVision & (1u << static_cast<uint32>(WizardIndex))) != 0u;
}

void UCoMWorldMapSubsystem::ClearCurrentVision(
    ECoMPlane Plane, ECoMMapLayer Layer, int32 WizardIndex)
{
	if (!IsValidWizardIndex(WizardIndex)) { return; }
	if (!bLayersReady) { return; }

	FCoMMapLayerData& LayerData = GetLayerMutable(Plane, Layer);
	const uint32 Mask = ~(1u << static_cast<uint32>(WizardIndex));
	for (FCoMTileData& Tile : LayerData.Tiles)
	{
		Tile.CurrentVision &= Mask;
	}
}

void UCoMWorldMapSubsystem::ClearAllCurrentVision(int32 WizardIndex)
{
	if (!IsValidWizardIndex(WizardIndex)) { return; }

	const TArray<ECoMPlane> AllPlanes =
	{
		ECoMPlane::Aurelith, ECoMPlane::Noctharion, ECoMPlane::Verdantis,
		ECoMPlane::Infernyx, ECoMPlane::Aethermist,
		ECoMPlane::Abyssal,
		ECoMPlane::Ethereal,
		ECoMPlane::Feywild
	};
	const TArray<ECoMMapLayer> AllLayers =
	{
		ECoMMapLayer::Surface, ECoMMapLayer::Underdark, ECoMMapLayer::Underwater
	};

	for (const ECoMPlane Plane : AllPlanes)
	{
		for (const ECoMMapLayer Layer : AllLayers)
		{
			ClearCurrentVision(Plane, Layer, WizardIndex);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Neighbor Queries
// ─────────────────────────────────────────────────────────────────────────────

TArray<FIntPoint> UCoMWorldMapSubsystem::GetAdjacentPositions(int32 X, int32 Y) const
{
	TArray<FIntPoint> Result;
	Result.Reserve(8);

	const int32 Deltas[8][2] = {
		{-1,-1},{0,-1},{1,-1},
		{-1, 0},       {1, 0},
		{-1, 1},{0, 1},{1, 1}
	};

	for (const auto& D : Deltas)
	{
		const int32 NX = X + D[0];
		const int32 NY = Y + D[1];
		if (IsValidPosition(NX, NY))
		{
			Result.Add(NormalizePosition(NX, NY));
		}
	}
	return Result;
}

TArray<FIntPoint> UCoMWorldMapSubsystem::GetPositionsInRange(
    int32 CenterX, int32 CenterY, int32 Range) const
{
	TArray<FIntPoint> Result;
	// Reserve a conservative upper bound
	Result.Reserve((2 * Range + 1) * (2 * Range + 1));

	for (int32 DY = -Range; DY <= Range; ++DY)
	{
		for (int32 DX = -Range; DX <= Range; ++DX)
		{
			if (FMath::Abs(DX) + FMath::Abs(DY) > Range) { continue; } // Manhattan
			const int32 NY = CenterY + DY;
			if (!IsValidPosition(CenterX + DX, NY)) { continue; }
			Result.Add(NormalizePosition(CenterX + DX, NY));
		}
	}
	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

int32 UCoMWorldMapSubsystem::CountRevealedTiles(
    ECoMPlane Plane, ECoMMapLayer Layer, int32 WizardIndex) const
{
	if (!IsValidWizardIndex(WizardIndex) || !bLayersReady) { return 0; }

	const FCoMMapLayerData& LayerData = GetLayer(Plane, Layer);
	const uint32 Bit = 1u << static_cast<uint32>(WizardIndex);
	int32 Count = 0;
	for (const FCoMTileData& Tile : LayerData.Tiles)
	{
		if (Tile.FogRevealed & Bit) { ++Count; }
	}
	return Count;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private Helpers
// ─────────────────────────────────────────────────────────────────────────────

bool UCoMWorldMapSubsystem::IsValidWizardIndex(int32 WizardIndex) const
{
	if (WizardIndex < 0 || WizardIndex >= CoM::MAX_WIZARDS)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("UCoMWorldMapSubsystem: WizardIndex %d out of range [0, %d)."),
		       WizardIndex, CoM::MAX_WIZARDS);
		return false;
	}
	return true;
}


// ── Debug Fog Helpers ─────────────────────────────────────────────────────

void UCoMWorldMapSubsystem::DebugRevealAll(int32 WizardIndex)
{
	if (!IsValidWizardIndex(WizardIndex)) return;
	const uint32 Bit = 1u << WizardIndex;
	for (FCoMMapLayerData& LayerData : Layers)
	{
		for (FCoMTileData& Tile : LayerData.Tiles)
		{
			Tile.FogRevealed |= Bit;
			Tile.CurrentVision |= Bit;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[WorldMap] Debug: revealed all for wizard %d"), WizardIndex);
}

void UCoMWorldMapSubsystem::DebugRevealEverything()
{
	for (FCoMMapLayerData& LayerData : Layers)
	{
		for (FCoMTileData& Tile : LayerData.Tiles)
		{
			Tile.FogRevealed = static_cast<uint32>(~0u);
			Tile.CurrentVision = static_cast<uint32>(~0u);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[WorldMap] Debug: revealed EVERYTHING"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load Export/Import
// ─────────────────────────────────────────────────────────────────────────────

TArray<FCoMMapLayerData> UCoMWorldMapSubsystem::ExportAllLayers() const
{
	return Layers;
}

void UCoMWorldMapSubsystem::ImportAllLayers(const TArray<FCoMMapLayerData>& InLayers)
{
	Layers = InLayers;
	bLayersReady = Layers.Num() > 0;
	UE_LOG(LogTemp, Log, TEXT("[WorldMap] ImportAllLayers: %d layers imported."), Layers.Num());
}
