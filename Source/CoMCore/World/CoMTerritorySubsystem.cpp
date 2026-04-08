// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMTerritorySubsystem.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "Containers/Queue.h"

// ============================================================================
// Subsystem lifecycle
// ============================================================================

void UCoMTerritorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCoMTerritorySubsystem::Deinitialize()
{
	OwnershipMaps.Empty();
	ContestedMaps.Empty();
	DistanceMaps.Empty();
	CitySeeds.Empty();
	Super::Deinitialize();
}

// ============================================================================
// Static helpers
// ============================================================================

const TArray<ECoMMapLayer>& UCoMTerritorySubsystem::AllLayers()
{
	static const TArray<ECoMMapLayer> Layers = {
		ECoMMapLayer::Surface,
		ECoMMapLayer::Underdark,
		ECoMMapLayer::Underwater
	};
	return Layers;
}

const TArray<ECoMPlane>& UCoMTerritorySubsystem::AllPlanes()
{
	static const TArray<ECoMPlane> Planes = {
		ECoMPlane::Aurelith,
		ECoMPlane::Noctharion,
		ECoMPlane::Verdantis,
		ECoMPlane::Infernyx,
		ECoMPlane::Aethermist,
		ECoMPlane::Abyssal,
		ECoMPlane::Ethereal,
		ECoMPlane::Feywild
	};
	return Planes;
}

int32 UCoMTerritorySubsystem::ManhattanWrapX(int32 X1, int32 Y1, int32 X2, int32 Y2)
{
	int32 DX = FMath::Abs(X1 - X2);
	// WrapX: the shorter path around the cylinder.
	DX = FMath::Min(DX, MAP_WIDTH - DX);
	int32 DY = FMath::Abs(Y1 - Y2);
	return DX + DY;
}

bool UCoMTerritorySubsystem::IsTileOcean(const UCoMWorldMapSubsystem* Map, ECoMPlane Plane,
                                          ECoMMapLayer Layer, int32 X, int32 Y)
{
	if (!Map)
	{
		return false;
	}
	const FCoMTileData* Tile = Map->GetTile(Plane, Layer, X, Y);
	if (!Tile)
	{
		return false;
	}
	// Ocean terrain type blocks ownership.
	return Tile->Terrain == ECoMTerrain::Ocean;
}

bool UCoMTerritorySubsystem::IsTileMountain(const UCoMWorldMapSubsystem* Map, ECoMPlane Plane,
                                             ECoMMapLayer Layer, int32 X, int32 Y)
{
	if (!Map)
	{
		return false;
	}
	const FCoMTileData* Tile = Map->GetTile(Plane, Layer, X, Y);
	if (!Tile)
	{
		return false;
	}
	return Tile->Terrain == ECoMTerrain::Mountains;
}

// ============================================================================
// Storage management
// ============================================================================

void UCoMTerritorySubsystem::EnsureLayerStorage(const FCoMLayerKey& Key)
{
	if (!OwnershipMaps.Contains(Key))
	{
		TArray<int32>& Ownership = OwnershipMaps.Add(Key);
		Ownership.SetNumUninitialized(MAP_SIZE);
		FMemory::Memset(Ownership.GetData(), 0xFF, MAP_SIZE * sizeof(int32)); // -1

		TArray<bool>& Contested = ContestedMaps.Add(Key);
		Contested.SetNumZeroed(MAP_SIZE);

		TArray<int32>& Distance = DistanceMaps.Add(Key);
		Distance.SetNumUninitialized(MAP_SIZE);
		FMemory::Memset(Distance.GetData(), 0x7F, MAP_SIZE * sizeof(int32)); // large sentinel
	}
}

void UCoMTerritorySubsystem::ClearLayer(const FCoMLayerKey& Key)
{
	if (TArray<int32>* Ownership = OwnershipMaps.Find(Key))
	{
		FMemory::Memset(Ownership->GetData(), 0xFF, MAP_SIZE * sizeof(int32));
	}
	if (TArray<bool>* Contested = ContestedMaps.Find(Key))
	{
		FMemory::Memset(Contested->GetData(), 0, MAP_SIZE * sizeof(bool));
	}
	if (TArray<int32>* Distance = DistanceMaps.Find(Key))
	{
		FMemory::Memset(Distance->GetData(), 0x7F, MAP_SIZE * sizeof(int32));
	}
}

TArray<const FCoMCitySeed*> UCoMTerritorySubsystem::GetSeedsForLayer(ECoMPlane Plane,
                                                                       ECoMMapLayer Layer) const
{
	TArray<const FCoMCitySeed*> Result;
	for (const FCoMCitySeed& Seed : CitySeeds)
	{
		if (Seed.Plane == Plane && Seed.Layer == Layer)
		{
			Result.Add(&Seed);
		}
	}
	return Result;
}

// ============================================================================
// Seed management
// ============================================================================

void UCoMTerritorySubsystem::SetCitySeed(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint Position,
                                          int32 WizardIndex, int32 InfluenceRadius)
{
	// Replace if a seed already exists at this location on this layer.
	for (FCoMCitySeed& Existing : CitySeeds)
	{
		if (Existing.Plane == Plane && Existing.Layer == Layer && Existing.Position == Position)
		{
			Existing.WizardIndex = WizardIndex;
			Existing.InfluenceRadius = InfluenceRadius;
			return;
		}
	}

	FCoMCitySeed NewSeed;
	NewSeed.Plane = Plane;
	NewSeed.Layer = Layer;
	NewSeed.Position = Position;
	NewSeed.WizardIndex = WizardIndex;
	NewSeed.InfluenceRadius = InfluenceRadius;
	CitySeeds.Add(MoveTemp(NewSeed));
}

void UCoMTerritorySubsystem::RemoveCitySeed(ECoMPlane Plane, ECoMMapLayer Layer,
                                             FIntPoint Position)
{
	CitySeeds.RemoveAll([&](const FCoMCitySeed& Seed)
	{
		return Seed.Plane == Plane && Seed.Layer == Layer && Seed.Position == Position;
	});
}

// ============================================================================
// Recalculation — BFS flood fill
// ============================================================================

/**
 * BFS node: tile position + accumulated cost (influence distance consumed).
 */
struct FBFSNode
{
	int32 X;
	int32 Y;
	int32 Cost; // accumulated traversal cost
};

void UCoMTerritorySubsystem::RecalculateTerritory(UCoMWorldMapSubsystem* Map)
{
	if (!Map)
	{
		return;
	}

	for (ECoMPlane Plane : AllPlanes())
	{
		for (ECoMMapLayer Layer : AllLayers())
		{
			RecalculatePlane(Map, Plane, Layer);
		}
	}
}

void UCoMTerritorySubsystem::RecalculatePlane(UCoMWorldMapSubsystem* Map, ECoMPlane Plane,
                                               ECoMMapLayer Layer)
{
	if (!Map)
	{
		return;
	}

	const FCoMLayerKey Key(Plane, Layer);
	EnsureLayerStorage(Key);
	ClearLayer(Key);

	TArray<int32>& Ownership = OwnershipMaps[Key];
	TArray<bool>& Contested  = ContestedMaps[Key];
	TArray<int32>& Distance  = DistanceMaps[Key];

	TArray<const FCoMCitySeed*> Seeds = GetSeedsForLayer(Plane, Layer);
	if (Seeds.Num() == 0)
	{
		return;
	}

	// 4-connected neighbors (N, S, E, W).
	static constexpr int32 DX[] = { 0,  0, 1, -1 };
	static constexpr int32 DY[] = { -1, 1, 0,  0 };

	// Run one BFS per seed. Each seed writes to the shared ownership/distance
	// maps; a tile is assigned to the seed that reaches it at lowest cost.
	// Equal-cost arrivals from different wizards mark the tile as contested.
	for (const FCoMCitySeed* Seed : Seeds)
	{
		if (Seed->WizardIndex < 0 || Seed->WizardIndex >= CoM::MAX_WIZARDS)
		{
			continue;
		}

		const int32 SeedX = Seed->Position.X;
		const int32 SeedY = Seed->Position.Y;

		// Skip seeds on ocean tiles.
		if (IsTileOcean(Map, Plane, Layer, SeedX, SeedY))
		{
			continue;
		}

		TQueue<FBFSNode> Queue;
		// Local visited set to avoid re-enqueuing within this seed's BFS.
		TArray<int32> LocalBest;
		LocalBest.SetNumUninitialized(MAP_SIZE);
		FMemory::Memset(LocalBest.GetData(), 0x7F, MAP_SIZE * sizeof(int32));

		const int32 StartIdx = TileIndex(SeedX, SeedY);
		LocalBest[StartIdx] = 0;

		// Process the origin tile.
		if (Distance[StartIdx] > 0)
		{
			// First wizard to reach here at cost 0.
			Distance[StartIdx]  = 0;
			Ownership[StartIdx] = Seed->WizardIndex;
			Contested[StartIdx] = false;
		}
		else if (Distance[StartIdx] == 0 && Ownership[StartIdx] != Seed->WizardIndex)
		{
			// Another wizard already has cost 0 here — contested.
			Contested[StartIdx]  = true;
			Ownership[StartIdx]  = CoM::WIZARD_INDEX_NONE;
		}

		FBFSNode StartNode;
		StartNode.X    = SeedX;
		StartNode.Y    = SeedY;
		StartNode.Cost = 0;
		Queue.Enqueue(StartNode);

		while (!Queue.IsEmpty())
		{
			FBFSNode Current;
			Queue.Dequeue(Current);

			// Skip if we already found a cheaper local path to this tile.
			const int32 CurIdx = TileIndex(Current.X, Current.Y);
			if (Current.Cost > LocalBest[CurIdx])
			{
				continue;
			}

			for (int32 Dir = 0; Dir < 4; ++Dir)
			{
				int32 NX = WrapX(Current.X + DX[Dir]);
				int32 NY = Current.Y + DY[Dir];

				// Y does not wrap.
				if (NY < 0 || NY >= MAP_HEIGHT)
				{
					continue;
				}

				// Ocean tiles block influence.
				if (IsTileOcean(Map, Plane, Layer, NX, NY))
				{
					continue;
				}

				// Mountain costs MOUNTAIN_COST instead of 1.
				const int32 StepCost = IsTileMountain(Map, Plane, Layer, NX, NY)
				                           ? MOUNTAIN_COST
				                           : 1;
				const int32 NewCost = Current.Cost + StepCost;

				if (NewCost > Seed->InfluenceRadius)
				{
					continue;
				}

				const int32 NIdx = TileIndex(NX, NY);

				// Only enqueue if this is a better or equal local path.
				if (NewCost > LocalBest[NIdx])
				{
					continue;
				}
				LocalBest[NIdx] = NewCost;

				// Update global ownership/distance.
				if (NewCost < Distance[NIdx])
				{
					// This wizard is strictly closer.
					Distance[NIdx]  = NewCost;
					Ownership[NIdx] = Seed->WizardIndex;
					Contested[NIdx] = false;
				}
				else if (NewCost == Distance[NIdx] && Ownership[NIdx] != Seed->WizardIndex)
				{
					// Equidistant from a different wizard — contested.
					Contested[NIdx]  = true;
					Ownership[NIdx]  = CoM::WIZARD_INDEX_NONE;
				}

				FBFSNode Next;
				Next.X    = NX;
				Next.Y    = NY;
				Next.Cost = NewCost;
				Queue.Enqueue(Next);
			}
		}
	}
}

// ============================================================================
// Queries
// ============================================================================

int32 UCoMTerritorySubsystem::GetTileOwner(ECoMPlane Plane, ECoMMapLayer Layer, int32 X,
                                            int32 Y) const
{
	// WrapX applied in caller % CoM::MAP_WIDTH) + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
	if (X < 0 || X >= MAP_WIDTH || Y < 0 || Y >= MAP_HEIGHT)
	{
		return CoM::WIZARD_INDEX_NONE;
	}

	const FCoMLayerKey Key(Plane, Layer);
	const TArray<int32>* Ownership = OwnershipMaps.Find(Key);
	if (!Ownership)
	{
		return CoM::WIZARD_INDEX_NONE;
	}

	return (*Ownership)[TileIndex(X, Y)];
}

bool UCoMTerritorySubsystem::IsTileContested(ECoMPlane Plane, ECoMMapLayer Layer, int32 X,
                                              int32 Y) const
{
	if (X < 0 || X >= MAP_WIDTH || Y < 0 || Y >= MAP_HEIGHT)
	{
		return false;
	}

	const FCoMLayerKey Key(Plane, Layer);
	const TArray<bool>* Contested = ContestedMaps.Find(Key);
	if (!Contested)
	{
		return false;
	}

	return (*Contested)[TileIndex(X, Y)];
}

int32 UCoMTerritorySubsystem::CountOwnedTiles(int32 WizardIndex, ECoMPlane Plane,
                                               ECoMMapLayer Layer) const
{
	const FCoMLayerKey Key(Plane, Layer);
	const TArray<int32>* Ownership = OwnershipMaps.Find(Key);
	if (!Ownership)
	{
		return 0;
	}

	int32 Count = 0;
	for (int32 Idx = 0; Idx < MAP_SIZE; ++Idx)
	{
		if ((*Ownership)[Idx] == WizardIndex)
		{
			++Count;
		}
	}
	return Count;
}

TArray<FIntPoint> UCoMTerritorySubsystem::GetBorderTiles(int32 WizardIndex, ECoMPlane Plane,
                                                          ECoMMapLayer Layer) const
{
	TArray<FIntPoint> Result;

	const FCoMLayerKey Key(Plane, Layer);
	const TArray<int32>* Ownership = OwnershipMaps.Find(Key);
	if (!Ownership)
	{
		return Result;
	}

	static constexpr int32 DX[] = { 0,  0, 1, -1 };
	static constexpr int32 DY[] = { -1, 1, 0,  0 };

	for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
	{
		for (int32 X = 0; X < MAP_WIDTH; ++X)
		{
			if ((*Ownership)[TileIndex(X, Y)] != WizardIndex)
			{
				continue;
			}

			// Check if any 4-connected neighbor is owned by a different wizard.
			for (int32 Dir = 0; Dir < 4; ++Dir)
			{
				int32 NX = WrapX(X + DX[Dir]);
				int32 NY = Y + DY[Dir];

				if (NY < 0 || NY >= MAP_HEIGHT)
				{
					continue;
				}

				const int32 NeighborOwner = (*Ownership)[TileIndex(NX, NY)];
				if (NeighborOwner != WizardIndex && NeighborOwner != CoM::WIZARD_INDEX_NONE)
				{
					Result.Add(FIntPoint(X, Y));
					break; // only add this tile once
				}
			}
		}
	}

	return Result;
}
