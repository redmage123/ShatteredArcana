// Copyright Mythforge Studios. All Rights Reserved.
// CoMOverworldRenderer.cpp -- Overworld tile rendering coordinator.

#include "Overworld/CoMOverworldRenderer.h"
#include "Overworld/CoMTileChunkActor.h"
#include "Overworld/CoMCityIconActor.h"
#include "Overworld/CoMArmyIconActor.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/World/CoMFogOfWarSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMOverworld, Log, All);

// ─── Subsystem lifecycle ────────────────────────────────────────────────────

void UCoMOverworldRenderer::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Cache sibling subsystems
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		WorldMapSub = GI->GetSubsystem<UCoMWorldMapSubsystem>();
		FogSub      = GI->GetSubsystem<UCoMFogOfWarSubsystem>();
		CitySub     = GI->GetSubsystem<UCoMCitySubsystem>();
		UnitSub     = GI->GetSubsystem<UCoMUnitSubsystem>();
	}

	UE_LOG(LogCoMOverworld, Log, TEXT("UCoMOverworldRenderer initialised. "
	       "WorldMap=%s Fog=%s City=%s Unit=%s"),
	       WorldMapSub ? TEXT("OK") : TEXT("null"),
	       FogSub      ? TEXT("OK") : TEXT("null"),
	       CitySub     ? TEXT("OK") : TEXT("null"),
	       UnitSub     ? TEXT("OK") : TEXT("null"));
}

void UCoMOverworldRenderer::Deinitialize()
{
	// Destroy all managed actors
	for (auto& Pair : ActiveChunkMap)
	{
		if (Pair.Value.IsValid() && !Pair.Value->IsActorBeingDestroyed())
		{
			Pair.Value->Destroy();
		}
	}
	ActiveChunkMap.Empty();

	ClearOverlays();

	Super::Deinitialize();
}

bool UCoMOverworldRenderer::ShouldCreateSubsystem(UObject* Outer) const
{
	// Always create — rendering is needed in any game session
	return true;
}

// ─── Plane rendering ────────────────────────────────────────────────────────

void UCoMOverworldRenderer::RenderPlane(ECoMPlane Plane, ECoMMapLayer Layer, int32 WizardIndex)
{
	if (!WorldMapSub || !WorldMapSub->IsInitialized())
	{
		UE_LOG(LogCoMOverworld, Warning,
		       TEXT("RenderPlane: WorldMapSubsystem not ready, deferring"));
		return;
	}

	// If switching planes, tear down the old view
	if (bInitialRenderDone && (Plane != CurrentPlane || Layer != CurrentLayer))
	{
		for (auto& Pair : ActiveChunkMap)
		{
			if (Pair.Value.IsValid())
			{
				Pair.Value->Destroy();
			}
		}
		ActiveChunkMap.Empty();
		ClearOverlays();
	}

	CurrentPlane = Plane;
	CurrentLayer = Layer;
	CurrentWizardId = WizardIndex;

	// Spawn all chunks for the entire map on initial render.
	// For a 160x100 map with 10x10 chunks, that is 16x10 = 160 chunks.
	// This is cheap enough at 160 flat planes. Later, UpdateVisibleChunks
	// can cull far-away chunks if perf requires it.
	for (int32 CY = 0; CY < CHUNKS_Y; ++CY)
	{
		for (int32 CX = 0; CX < CHUNKS_X; ++CX)
		{
			const FIntPoint ChunkCoord(CX, CY);
			const int64 Key = ChunkKey(ChunkCoord);

			if (!ActiveChunkMap.Contains(Key))
			{
				ACoMTileChunkActor* Chunk = SpawnChunk(ChunkCoord);
				if (Chunk)
				{
					Chunk->BuildChunkTexture(WorldMapSub, FogSub, CurrentWizardId);
					ActiveChunkMap.Add(Key, Chunk);
				}
			}
		}
	}

	// Spawn overlays
	SpawnCityIcons();
	SpawnArmySprites();

	bInitialRenderDone = true;

	UE_LOG(LogCoMOverworld, Log,
	       TEXT("RenderPlane complete: Plane=%d Layer=%d Wizard=%d Chunks=%d"),
	       static_cast<int32>(Plane), static_cast<int32>(Layer),
	       WizardIndex, ActiveChunkMap.Num());
}

// ─── Visible chunk updates ──────────────────────────────────────────────────

void UCoMOverworldRenderer::UpdateVisibleChunks(FVector CameraPosition, float ZoomLevel)
{
	if (!bInitialRenderDone)
	{
		return;
	}

	// Rebuild any dirty chunks (marked by localised changes)
	for (auto& Pair : ActiveChunkMap)
	{
		if (Pair.Value.IsValid() && Pair.Value->IsDirty())
		{
			Pair.Value->BuildChunkTexture(WorldMapSub, FogSub, CurrentWizardId);
		}
	}

	// Future optimisation: cull chunks outside camera frustum based on
	// CameraPosition and ZoomLevel. For now, all 160 chunks remain spawned
	// since 160 flat-plane actors with 1280x1280 textures is well within
	// UE5's capacity on modern hardware.
}

TArray<FIntPoint> UCoMOverworldRenderer::CalcVisibleChunkCoords(
	FVector CameraPos, float ZoomLevel) const
{
	// Calculate the tile coordinate at camera center
	const FIntPoint CenterTile = WorldToTile(CameraPos);

	// Estimate visible tile range based on zoom.
	// At max zoom (4000 arm length), roughly 40 tiles are visible horizontally.
	// At min zoom (600 arm length), roughly 6 tiles.
	const float TilesVisible = (ZoomLevel / TILE_WORLD_SIZE) * 1.5f;
	const int32 HalfTilesX = FMath::CeilToInt32(TilesVisible) + CHUNK_SIZE; // +1 chunk buffer
	const int32 HalfTilesY = FMath::CeilToInt32(TilesVisible * 0.75f) + CHUNK_SIZE;

	const int32 MinTileX = CenterTile.X - HalfTilesX;
	const int32 MaxTileX = CenterTile.X + HalfTilesX;
	const int32 MinTileY = FMath::Max(0, CenterTile.Y - HalfTilesY);
	const int32 MaxTileY = FMath::Min(CoM::MAP_HEIGHT - 1, CenterTile.Y + HalfTilesY);

	TArray<FIntPoint> Result;
	for (int32 TY = MinTileY; TY <= MaxTileY; TY += CHUNK_SIZE)
	{
		for (int32 TX = MinTileX; TX <= MaxTileX; TX += CHUNK_SIZE)
		{
			FIntPoint ChunkCoord = TileToChunkCoord(
				UCoMWorldMapSubsystem::NormalizePosition(TX, TY));
			Result.AddUnique(ChunkCoord);
		}
	}
	return Result;
}

// ─── Chunk refresh ──────────────────────────────────────────────────────────

void UCoMOverworldRenderer::RefreshAllChunks()
{
	for (auto& Pair : ActiveChunkMap)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->BuildChunkTexture(WorldMapSub, FogSub, CurrentWizardId);
		}
	}

	// Refresh overlays too
	ClearOverlays();
	SpawnCityIcons();
	SpawnArmySprites();
}

void UCoMOverworldRenderer::RefreshChunkAt(FIntPoint TilePos)
{
	const FIntPoint ChunkCoord = TileToChunkCoord(TilePos);
	const int64 Key = ChunkKey(ChunkCoord);

	if (auto* Found = ActiveChunkMap.Find(Key))
	{
		if (Found->IsValid())
		{
			Found->Get()->BuildChunkTexture(WorldMapSub, FogSub, CurrentWizardId);
		}
	}
}

// ─── Overlay management ─────────────────────────────────────────────────────

void UCoMOverworldRenderer::SpawnCityIcons()
{
	if (!CitySub)
	{
		return;
	}

	UWorld* World = WorldContext.Get();
	if (!World)
	{
		return;
	}

	TArray<const FCoMCityData*> Cities = CitySub->GetCitiesOnPlane(CurrentPlane);

	for (const FCoMCityData* City : Cities)
	{
		if (!City || City->Layer != CurrentLayer)
		{
			continue;
		}

		// Check fog — only show cities the player has explored
		if (FogSub && !FogSub->IsTileExplored(CurrentWizardId, CurrentPlane,
		                                        City->Position, CurrentLayer))
		{
			continue;
		}

		FVector SpawnPos = TileToWorld(City->Position);
		SpawnPos.Z += 10.0f; // Slightly above ground plane

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FTransform CityTransform(FRotator::ZeroRotator, SpawnPos);
		ACoMCityIconActor* Icon = World->SpawnActor<ACoMCityIconActor>(
			ACoMCityIconActor::StaticClass(), CityTransform, Params);
		if (Icon)
		{
			Icon->InitFromCityData(*City);
			CityIconActors.Add(Icon);
		}
	}

	UE_LOG(LogCoMOverworld, Log, TEXT("Spawned %d city icons on plane %d"),
	       CityIconActors.Num(), static_cast<int32>(CurrentPlane));
}

void UCoMOverworldRenderer::SpawnArmySprites()
{
	if (!UnitSub)
	{
		return;
	}

	UWorld* World = WorldContext.Get();
	if (!World)
	{
		return;
	}

	// Iterate all wizards and collect armies on the current plane
	for (int32 WizIdx = 0; WizIdx < CoM::MAX_WIZARDS; ++WizIdx)
	{
		TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesForWizard(WizIdx);
		for (const FCoMArmyGroup* Army : Armies)
		{
			if (!Army || Army->Plane != CurrentPlane || Army->Layer != CurrentLayer)
			{
				continue;
			}

			// Only show armies the player can see (current vision, not just explored)
			if (FogSub && WizIdx != CurrentWizardId)
			{
				if (!FogSub->IsTileVisible(CurrentWizardId, CurrentPlane,
				                            Army->Position, CurrentLayer))
				{
					continue;
				}
			}

			FVector SpawnPos = TileToWorld(Army->Position);
			SpawnPos.Z += 15.0f; // Above city icons

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			const FTransform ArmyTransform(FRotator::ZeroRotator, SpawnPos);
			ACoMArmyIconActor* Icon = World->SpawnActor<ACoMArmyIconActor>(
				ACoMArmyIconActor::StaticClass(), ArmyTransform, Params);
			if (Icon)
			{
				Icon->InitFromArmyData(*Army);
				ArmyIconActors.Add(Icon);
			}
		}
	}

	UE_LOG(LogCoMOverworld, Log, TEXT("Spawned %d army icons on plane %d"),
	       ArmyIconActors.Num(), static_cast<int32>(CurrentPlane));
}

void UCoMOverworldRenderer::UpdateArmyPositions()
{
	// Rather than tracking army->actor mappings, tear down and respawn.
	// This is called once per turn, not per frame, so simplicity wins.
	for (TObjectPtr<AActor>& Actor : ArmyIconActors)
	{
		if (Actor && !Actor->IsActorBeingDestroyed())
		{
			Actor->Destroy();
		}
	}
	ArmyIconActors.Empty();
	SpawnArmySprites();
}

void UCoMOverworldRenderer::ClearOverlays()
{
	for (TObjectPtr<AActor>& Actor : CityIconActors)
	{
		if (Actor && !Actor->IsActorBeingDestroyed())
		{
			Actor->Destroy();
		}
	}
	CityIconActors.Empty();

	for (TObjectPtr<AActor>& Actor : ArmyIconActors)
	{
		if (Actor && !Actor->IsActorBeingDestroyed())
		{
			Actor->Destroy();
		}
	}
	ArmyIconActors.Empty();
}

// ─── Coordinate conversion ──────────────────────────────────────────────────

FIntPoint UCoMOverworldRenderer::WorldToTile(FVector WorldPos) const
{
	int32 TileX = FMath::FloorToInt32(WorldPos.X / TILE_WORLD_SIZE);
	int32 TileY = FMath::FloorToInt32(WorldPos.Y / TILE_WORLD_SIZE);

	// Apply WrapX
	TileX = ((TileX % CoM::MAP_WIDTH) + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
	TileY = FMath::Clamp(TileY, 0, CoM::MAP_HEIGHT - 1);

	return FIntPoint(TileX, TileY);
}

FVector UCoMOverworldRenderer::TileToWorld(FIntPoint TilePos) const
{
	// Return center of tile
	const float WorldX = (static_cast<float>(TilePos.X) + 0.5f) * TILE_WORLD_SIZE;
	const float WorldY = (static_cast<float>(TilePos.Y) + 0.5f) * TILE_WORLD_SIZE;
	return FVector(WorldX, WorldY, 0.0f);
}

// ─── Click handling ─────────────────────────────────────────────────────────

FCoMOverworldClickResult UCoMOverworldRenderer::HandleClick(FVector WorldHitPosition) const
{
	FCoMOverworldClickResult Result;
	Result.TilePosition = WorldToTile(WorldHitPosition);

	// Priority 1: Check for army at tile
	if (UnitSub)
	{
		TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesAtPosition(
			CurrentPlane, CurrentLayer, Result.TilePosition);
		if (Armies.Num() > 0 && Armies[0])
		{
			// Prefer the player's own army if present
			for (const FCoMArmyGroup* Army : Armies)
			{
				if (Army->OwnerWizardIndex == CurrentWizardId)
				{
					Result.Target = ECoMOverworldClickTarget::Army;
					Result.ArmyID = Army->ArmyGroupID;
					return Result;
				}
			}
			// Otherwise return the first army
			Result.Target = ECoMOverworldClickTarget::Army;
			Result.ArmyID = Armies[0]->ArmyGroupID;
			return Result;
		}
	}

	// Priority 2: Check for city at tile
	if (WorldMapSub && WorldMapSub->IsInitialized())
	{
		const FCoMTileData* Tile = WorldMapSub->GetTile(
			CurrentPlane, CurrentLayer, Result.TilePosition.X, Result.TilePosition.Y);
		if (Tile && Tile->CityID >= 0)
		{
			Result.Target = ECoMOverworldClickTarget::City;
			Result.CityID = Tile->CityID;
			return Result;
		}
	}

	// Priority 3: Plain tile click
	Result.Target = ECoMOverworldClickTarget::Tile;
	return Result;
}

// ─── Chunk spawning ─────────────────────────────────────────────────────────

ACoMTileChunkActor* UCoMOverworldRenderer::SpawnChunk(FIntPoint ChunkCoord)
{
	UWorld* World = WorldContext.Get();
	if (!World)
	{
		UE_LOG(LogCoMOverworld, Warning, TEXT("SpawnChunk: No world available"));
		return nullptr;
	}

	const FIntPoint TileOrigin = ChunkCoordToTileOrigin(ChunkCoord);
	const FVector WorldPos(
		static_cast<float>(TileOrigin.X) * TILE_WORLD_SIZE,
		static_cast<float>(TileOrigin.Y) * TILE_WORLD_SIZE,
		0.0f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform ChunkTransform(FRotator::ZeroRotator, WorldPos);
	ACoMTileChunkActor* Chunk = World->SpawnActor<ACoMTileChunkActor>(
		ACoMTileChunkActor::StaticClass(), ChunkTransform, Params);

	if (Chunk)
	{
		Chunk->ChunkOrigin = TileOrigin;
		Chunk->ChunkSize = CHUNK_SIZE;
		Chunk->Plane = CurrentPlane;
		Chunk->Layer = CurrentLayer;

		// Position the mesh so its corner aligns with the tile origin.
		// The engine plane mesh is centered at origin, so offset by half the chunk extent.
		const float HalfExtent = (CHUNK_SIZE * TILE_WORLD_SIZE) * 0.5f;
		Chunk->SetActorLocation(FVector(
			WorldPos.X + HalfExtent,
			WorldPos.Y + HalfExtent,
			0.0f));
	}

	return Chunk;
}

void UCoMOverworldRenderer::DespawnChunk(ACoMTileChunkActor* Chunk)
{
	if (Chunk && !Chunk->IsActorBeingDestroyed())
	{
		Chunk->Destroy();
	}
}

FIntPoint UCoMOverworldRenderer::TileToChunkCoord(FIntPoint TilePos) const
{
	// Normalise tile position first
	int32 TX = ((TilePos.X % CoM::MAP_WIDTH) + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
	int32 TY = FMath::Clamp(TilePos.Y, 0, CoM::MAP_HEIGHT - 1);
	return FIntPoint(TX / CHUNK_SIZE, TY / CHUNK_SIZE);
}

FIntPoint UCoMOverworldRenderer::ChunkCoordToTileOrigin(FIntPoint ChunkCoord) const
{
	return FIntPoint(ChunkCoord.X * CHUNK_SIZE, ChunkCoord.Y * CHUNK_SIZE);
}
