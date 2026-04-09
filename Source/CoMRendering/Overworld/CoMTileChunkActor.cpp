// Copyright Mythforge Studios. All Rights Reserved.
// CoMTileChunkActor.cpp -- Chunk actor: composites terrain tiles into a single textured plane.

#include "Overworld/CoMTileChunkActor.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/World/CoMFogOfWarSubsystem.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMChunk, Log, All);

// ─── Constructor ────────────────────────────────────────────────────────────

ACoMTileChunkActor::ACoMTileChunkActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ChunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChunkMesh"));
	SetRootComponent(ChunkMesh);

	// Use the built-in engine plane mesh (1x1 unit, flat on XY plane)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		ChunkMesh->SetStaticMesh(PlaneMeshFinder.Object);
	}

	ChunkMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ChunkMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ChunkMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ChunkMesh->CastShadow = false;
}

// ─── Texture management ─────────────────────────────────────────────────────

UTexture2D* ACoMTileChunkActor::GetOrCreateChunkTexture()
{
	if (ChunkTexture)
	{
		return ChunkTexture;
	}

	const int32 TexSize = ChunkSize * TILE_PIXEL_SIZE; // 10 * 128 = 1280

	ChunkTexture = UTexture2D::CreateTransient(TexSize, TexSize, PF_B8G8R8A8);
	if (!ChunkTexture)
	{
		UE_LOG(LogCoMChunk, Error, TEXT("Failed to create transient chunk texture %dx%d"),
		       TexSize, TexSize);
		return nullptr;
	}

	ChunkTexture->Filter = TF_Nearest; // Pixel-art look — no bilinear blur
	ChunkTexture->SRGB = true;
	ChunkTexture->CompressionSettings = TC_VectorDisplacementmap; // Uncompressed
	ChunkTexture->AddressX = TA_Clamp;
	ChunkTexture->AddressY = TA_Clamp;
	ChunkTexture->UpdateResource();

	return ChunkTexture;
}

// ─── Build chunk texture ────────────────────────────────────────────────────

void ACoMTileChunkActor::BuildChunkTexture(UCoMWorldMapSubsystem* MapSub,
                                           UCoMFogOfWarSubsystem* FogSub,
                                           int32 WizardIndex)
{
	if (!MapSub)
	{
		UE_LOG(LogCoMChunk, Warning, TEXT("BuildChunkTexture: MapSub is null"));
		return;
	}
	if (!MapSub->IsInitialized())
	{
		UE_LOG(LogCoMChunk, Warning, TEXT("BuildChunkTexture: World map not initialised"));
		return;
	}

	UTexture2D* Tex = GetOrCreateChunkTexture();
	if (!Tex)
	{
		return;
	}

	const int32 TexSize = ChunkSize * TILE_PIXEL_SIZE;

	// Lock the mip 0 bulk data for writing
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (!RawData)
	{
		UE_LOG(LogCoMChunk, Error, TEXT("BuildChunkTexture: Failed to lock texture bulk data"));
		return;
	}

	FColor* Pixels = static_cast<FColor*>(RawData);

	// Iterate each tile in the chunk
	for (int32 LocalY = 0; LocalY < ChunkSize; ++LocalY)
	{
		for (int32 LocalX = 0; LocalX < ChunkSize; ++LocalX)
		{
			const int32 WorldX = ChunkOrigin.X + LocalX;
			const int32 WorldY = ChunkOrigin.Y + LocalY;

			// Get terrain type from world map (handles WrapX internally)
			const FCoMTileData* TileData = MapSub->GetTile(Plane, Layer, WorldX, WorldY);
			ECoMTerrain TerrainType = TileData ? TileData->Terrain : ECoMTerrain::Ocean;

			// Determine terrain base colour
			FColor TileColor = GetTerrainColor(TerrainType);

			// Apply fog of war
			if (FogSub)
			{
				const FIntPoint TilePos(WorldX, WorldY);
				const bool bExplored = FogSub->IsTileExplored(WizardIndex, Plane, TilePos, Layer);
				const bool bVisible  = FogSub->IsTileVisible(WizardIndex, Plane, TilePos, Layer);
				TileColor = ApplyFogOfWar(TileColor, bExplored, bVisible);
			}

			// Fill the 128x128 pixel block for this tile
			const int32 PixelStartX = LocalX * TILE_PIXEL_SIZE;
			const int32 PixelStartY = LocalY * TILE_PIXEL_SIZE;

			for (int32 PY = 0; PY < TILE_PIXEL_SIZE; ++PY)
			{
				const int32 RowOffset = (PixelStartY + PY) * TexSize + PixelStartX;
				for (int32 PX = 0; PX < TILE_PIXEL_SIZE; ++PX)
				{
					// Add subtle grid lines at tile edges (1px dark border)
					if (PX == 0 || PY == 0)
					{
						FColor GridColor;
						GridColor.R = static_cast<uint8>(TileColor.R * 0.7f);
						GridColor.G = static_cast<uint8>(TileColor.G * 0.7f);
						GridColor.B = static_cast<uint8>(TileColor.B * 0.7f);
						GridColor.A = 255;
						Pixels[RowOffset + PX] = GridColor;
					}
					else
					{
						Pixels[RowOffset + PX] = TileColor;
					}
				}
			}
		}
	}

	Mip.BulkData.Unlock();
	Tex->UpdateResource();

	// Create or update the material instance
	if (!ChunkMID)
	{
		// Use a simple unlit material so the map is always fully readable
		UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
		if (BaseMat)
		{
			ChunkMID = UMaterialInstanceDynamic::Create(BaseMat, this);
		}
	}

	if (ChunkMID)
	{
		ChunkMID->SetTextureParameterValue(FName(TEXT("BaseColor")), Tex);
		ChunkMesh->SetMaterial(0, ChunkMID);
	}

	// Scale the mesh to cover ChunkSize * TILE_WORLD_SIZE in world units.
	// The engine Plane mesh is 100x100 UU by default, so we scale accordingly.
	const float WorldExtent = ChunkSize * CoM::TILE_WORLD_SIZE_CM;
	const float MeshBaseSize = 100.0f; // Engine plane is 100x100 UU
	const float ScaleFactor = WorldExtent / MeshBaseSize;
	ChunkMesh->SetWorldScale3D(FVector(ScaleFactor, ScaleFactor, 1.0f));

	bIsBuilt = true;
	bDirty = false;

	UE_LOG(LogCoMChunk, Verbose, TEXT("Built chunk at (%d,%d) plane=%d layer=%d"),
	       ChunkOrigin.X, ChunkOrigin.Y, static_cast<int32>(Plane), static_cast<int32>(Layer));
}

// ─── Terrain colour table ───────────────────────────────────────────────────
// Provides a representative colour per terrain type for the procedural chunk
// texture. When proper 128x128 tile art is loaded from content, these colours
// will be replaced by texture sampling from a terrain atlas.

FColor ACoMTileChunkActor::GetTerrainColor(ECoMTerrain TerrainType)
{
	switch (TerrainType)
	{
	// ── Aurelith Surface ─────────────────────────────────────────────────
	case ECoMTerrain::Grassland:        return FColor(  86, 176,  64);
	case ECoMTerrain::Forest:           return FColor(  34, 120,  28);
	case ECoMTerrain::Hills:            return FColor( 140, 160,  80);
	case ECoMTerrain::Mountains:        return FColor( 128, 112,  96);
	case ECoMTerrain::Desert:           return FColor( 220, 200, 140);
	case ECoMTerrain::Tundra:           return FColor( 200, 210, 220);
	case ECoMTerrain::Swamp:            return FColor(  70, 110,  60);
	case ECoMTerrain::Ocean:            return FColor(  30,  60, 160);
	case ECoMTerrain::River:            return FColor(  60, 120, 200);
	case ECoMTerrain::Shore:            return FColor( 180, 200, 140);
	case ECoMTerrain::Volcano:          return FColor( 180,  40,  20);
	case ECoMTerrain::Plains:           return FColor( 160, 190,  80);
	case ECoMTerrain::Savanna:          return FColor( 190, 180, 100);
	case ECoMTerrain::Badlands:         return FColor( 160, 100,  60);
	case ECoMTerrain::Jungle:           return FColor(  20, 100,  30);
	case ECoMTerrain::EnchantedForest:  return FColor(  60, 160, 120);
	case ECoMTerrain::CrystalLake:      return FColor( 100, 180, 220);
	case ECoMTerrain::AncientHighway:   return FColor( 180, 170, 150);

	// ── Noctharion Surface ──────────────────────────────────────────────
	case ECoMTerrain::ShadowForest:     return FColor(  30,  30,  60);
	case ECoMTerrain::ObsidianPlains:   return FColor(  40,  35,  50);
	case ECoMTerrain::CrystalDesert:    return FColor( 160, 140, 180);
	case ECoMTerrain::DarkMountains:    return FColor(  60,  50,  70);
	case ECoMTerrain::VoidOcean:        return FColor(  10,  10,  40);
	case ECoMTerrain::CorruptedSwamp:   return FColor(  50,  60,  30);
	case ECoMTerrain::AshWastes:        return FColor( 100,  90,  80);
	case ECoMTerrain::TwilightHills:    return FColor(  80,  70, 100);
	case ECoMTerrain::GloomTundra:      return FColor( 120, 120, 140);
	case ECoMTerrain::ShadowRift:       return FColor(  20,  10,  30);
	case ECoMTerrain::FloatingObsidian: return FColor(  50,  40,  60);
	case ECoMTerrain::ManaGeyser:       return FColor( 100,  60, 180);

	// ── Verdantis Surface ───────────────────────────────────────────────
	case ECoMTerrain::MegaJungle:       return FColor(  10,  80,  20);
	case ECoMTerrain::FungalForest:     return FColor( 120,  90, 140);
	case ECoMTerrain::LivingSwamp:      return FColor(  50, 100,  40);
	case ECoMTerrain::RootMountains:    return FColor( 100, 80,   50);
	case ECoMTerrain::PollenPlains:     return FColor( 200, 210, 100);
	case ECoMTerrain::SporeDesert:      return FColor( 180, 160, 120);
	case ECoMTerrain::AmberCoast:       return FColor( 200, 160,  60);
	case ECoMTerrain::VineHills:        return FColor(  80, 140,  60);
	case ECoMTerrain::BioluminescentDeep: return FColor( 40, 180, 160);
	case ECoMTerrain::WorldTree:        return FColor(  60, 200,  80);
	case ECoMTerrain::SentientTerrain:  return FColor( 100, 160, 100);
	case ECoMTerrain::MyceliumNetwork:  return FColor( 140, 120, 160);

	// ── Infernyx Surface ────────────────────────────────────────────────
	case ECoMTerrain::LavaFields:       return FColor( 200,  60,  10);
	case ECoMTerrain::ObsidianSpires:   return FColor(  40,  30,  30);
	case ECoMTerrain::AshDesert:        return FColor( 130, 110, 100);
	case ECoMTerrain::SulfurSeas:       return FColor( 180, 180,  40);
	case ECoMTerrain::MagmaRivers:      return FColor( 240, 100,  20);
	case ECoMTerrain::CinderHills:      return FColor( 100,  70,  50);
	case ECoMTerrain::ScorchedForest:   return FColor(  60,  40,  20);
	case ECoMTerrain::EmberPlains:      return FColor( 180, 120,  60);
	case ECoMTerrain::BasaltMountains:  return FColor(  50,  45,  40);
	case ECoMTerrain::FireGeyser:       return FColor( 255, 140,  20);
	case ECoMTerrain::LavaTube_Surface: return FColor( 160,  50,  10);
	case ECoMTerrain::VolcanicChain:    return FColor( 120,  40,  20);

	// ── Aethermist Surface ──────────────────────────────────────────────
	case ECoMTerrain::CloudPlains:      return FColor( 200, 210, 240);
	case ECoMTerrain::CrystalSpires:    return FColor( 180, 200, 255);
	case ECoMTerrain::VoidRifts:        return FColor(  20,  10,  50);
	case ECoMTerrain::FloatingIslands:  return FColor( 140, 180, 200);
	case ECoMTerrain::MistOcean:        return FColor( 160, 180, 210);
	case ECoMTerrain::DreamMeadows:     return FColor( 180, 200, 180);
	case ECoMTerrain::ResonancePeaks:   return FColor( 160, 160, 200);
	case ECoMTerrain::EtherMarshes:     return FColor( 120, 150, 170);
	case ECoMTerrain::StarlightTundra:  return FColor( 200, 200, 230);
	case ECoMTerrain::GravityAnomaly:   return FColor( 100,  80, 140);
	case ECoMTerrain::PhaseShiftingTerrain: return FColor(140, 120, 180);
	case ECoMTerrain::AuroraField:      return FColor( 100, 200, 160);

	// ── Abyssal Surface ─────────────────────────────────────────────────
	case ECoMTerrain::BonePlains:       return FColor( 200, 190, 170);
	case ECoMTerrain::BloodRivers:      return FColor( 140,  20,  20);
	case ECoMTerrain::ScreamingChasms:  return FColor(  60,  20,  30);
	case ECoMTerrain::LivingWalls:      return FColor( 150,  80,  80);
	case ECoMTerrain::DemonPillars:     return FColor( 100,  40,  50);
	case ECoMTerrain::FleshCitadels:    return FColor( 160, 100, 100);
	case ECoMTerrain::VoidFissures:     return FColor(  20,  10,  30);
	case ECoMTerrain::ChaosStormfield:  return FColor( 140,  60, 120);
	case ECoMTerrain::AbyssalMires:     return FColor(  80,  50,  40);
	case ECoMTerrain::ChasmEdge:        return FColor(  70,  40,  40);
	case ECoMTerrain::CarrionDesert:    return FColor( 180, 150, 120);
	case ECoMTerrain::GoreMarshes:      return FColor( 120,  50,  40);

	// ── Infernyx Iron Quarter ───────────────────────────────────────────
	case ECoMTerrain::IronFortresses:   return FColor( 100, 100, 110);
	case ECoMTerrain::SoulForges:       return FColor( 160,  80,  40);
	case ECoMTerrain::FireLakes:        return FColor( 220, 100,  30);
	case ECoMTerrain::BrimstoneHighways: return FColor(140, 120,  60);
	case ECoMTerrain::ContractSpires:   return FColor( 120, 100, 100);
	case ECoMTerrain::HellgateMarkets:  return FColor( 160, 120,  80);
	case ECoMTerrain::TormentFields:    return FColor( 140,  60,  60);

	// ── Ethereal Surface ────────────────────────────────────────────────
	case ECoMTerrain::MistSeas:             return FColor( 180, 190, 210);
	case ECoMTerrain::CrystallizedMemories: return FColor( 200, 210, 230);
	case ECoMTerrain::EchoRuins:            return FColor( 150, 140, 160);
	case ECoMTerrain::ThoughtStorms:        return FColor( 130, 120, 170);
	case ECoMTerrain::PhaseShores:          return FColor( 160, 180, 200);
	case ECoMTerrain::DreamDeserts:         return FColor( 200, 190, 170);
	case ECoMTerrain::SpiritArchipelagos:   return FColor( 140, 170, 200);
	case ECoMTerrain::ReflectionMirrors:    return FColor( 210, 220, 240);
	case ECoMTerrain::VoidWhisperFields:    return FColor(  80,  70, 100);
	case ECoMTerrain::MemoryLandscape:      return FColor( 170, 160, 190);
	case ECoMTerrain::FadeZones:            return FColor( 160, 160, 160);

	// ── Feywild Surface ─────────────────────────────────────────────────
	case ECoMTerrain::EternalForest:        return FColor(  40, 160,  60);
	case ECoMTerrain::ShiftingGlades:       return FColor( 100, 200, 100);
	case ECoMTerrain::MushroomRings:        return FColor( 180, 130, 180);
	case ECoMTerrain::MirrorLakes:          return FColor( 120, 180, 220);
	case ECoMTerrain::FeyMarketCrossroads:  return FColor( 200, 180, 120);
	case ECoMTerrain::ThorncraftHollow:     return FColor(  80, 100,  50);
	case ECoMTerrain::AncientStandingStones: return FColor(150, 150, 140);
	case ECoMTerrain::BlossomPeaks:         return FColor( 220, 160, 180);
	case ECoMTerrain::SilverMistPlains:     return FColor( 190, 200, 210);
	case ECoMTerrain::TwilightMeadows:      return FColor( 140, 160, 180);
	case ECoMTerrain::WildHuntGrounds:      return FColor(  60, 100,  40);
	case ECoMTerrain::FeyWastes:            return FColor( 160, 140, 120);
	case ECoMTerrain::SeelieCourt:          return FColor( 220, 220, 180);
	case ECoMTerrain::UnseelieHollow:       return FColor(  40,  30,  50);
	case ECoMTerrain::ErlingPath:           return FColor( 180, 200, 160);

	// ── Underdark terrain (all planes) — general dark palette ────────────
	case ECoMTerrain::CavernFloor:      return FColor(  80,  70,  60);
	case ECoMTerrain::FungalGrove:      return FColor(  90,  80, 110);
	case ECoMTerrain::UndergroundLake:  return FColor(  40,  60, 120);
	case ECoMTerrain::CrystalCavern_Arc: return FColor(120, 140, 180);
	case ECoMTerrain::DwarvenRuins:     return FColor( 110, 100,  80);
	case ECoMTerrain::CollapsedTunnel:  return FColor(  70,  60,  50);
	case ECoMTerrain::LavaTube_Arc:     return FColor( 140,  50,  20);
	case ECoMTerrain::DeepChasm:        return FColor(  20,  15,  20);
	case ECoMTerrain::MineralVein:      return FColor( 140, 130, 100);
	case ECoMTerrain::UndergroundRiver: return FColor(  50,  80, 140);

	// Noctharion Underdark
	case ECoMTerrain::ShadowCavern:     return FColor(  25,  20,  40);
	case ECoMTerrain::VoidPocket:       return FColor(  10,   5,  20);
	case ECoMTerrain::BoneOssuary:      return FColor( 160, 150, 130);
	case ECoMTerrain::ObsidianLabyrinth: return FColor( 35,  30,  40);
	case ECoMTerrain::SoulWell:         return FColor(  80,  40, 120);
	case ECoMTerrain::CorruptedCrystal: return FColor( 100,  60, 100);
	case ECoMTerrain::WebbedTunnels:    return FColor(  90,  80,  70);
	case ECoMTerrain::EchoChamber:      return FColor(  70,  60,  80);
	case ECoMTerrain::DarkRiver:        return FColor(  20,  30,  60);
	case ECoMTerrain::NightmareRealm:   return FColor(  40,  10,  30);

	// Verdantis Underdark
	case ECoMTerrain::RootTunnel:       return FColor(  80,  60,  40);
	case ECoMTerrain::MyceliumNet_Verd: return FColor( 110,  90, 120);
	case ECoMTerrain::UndergroundGarden: return FColor( 60, 120,  50);
	case ECoMTerrain::AmberPool:        return FColor( 180, 140,  40);
	case ECoMTerrain::InsectHive:       return FColor( 140, 120,  60);
	case ECoMTerrain::PetrifiedForest:  return FColor( 100,  90,  70);
	case ECoMTerrain::MudCavern:        return FColor(  90,  70,  50);
	case ECoMTerrain::CocoonChamber:    return FColor( 160, 140, 100);
	case ECoMTerrain::SapRiver:         return FColor( 140, 160,  40);
	case ECoMTerrain::SeedVault:        return FColor( 100, 120,  60);

	// Infernyx Underdark
	case ECoMTerrain::MagmaChamber:     return FColor( 200,  60,  10);
	case ECoMTerrain::ObsidianCavern:   return FColor(  35,  30,  30);
	case ECoMTerrain::SulfurPit:        return FColor( 160, 160,  30);
	case ECoMTerrain::ForgeCavern:      return FColor( 140, 100,  50);
	case ECoMTerrain::CinderTunnel:     return FColor(  80,  60,  40);
	case ECoMTerrain::LavaRiver_Inf:    return FColor( 220,  80,  10);
	case ECoMTerrain::DemonPit:         return FColor( 100,  30,  30);
	case ECoMTerrain::CrystalForge:     return FColor( 160, 140, 180);
	case ECoMTerrain::EmberShelf:       return FColor( 160, 100,  40);
	case ECoMTerrain::SmokeVent:        return FColor( 100,  90,  80);

	// Aethermist Underdark
	case ECoMTerrain::GravityWell:      return FColor(  60,  40, 100);
	case ECoMTerrain::PhaseCavern:      return FColor( 100,  80, 140);
	case ECoMTerrain::CrystalResonance: return FColor( 160, 180, 220);
	case ECoMTerrain::VoidBubble:       return FColor(  20,  10,  40);
	case ECoMTerrain::DreamGrotto:      return FColor( 140, 160, 180);
	case ECoMTerrain::TemporalEddy:     return FColor( 120, 100, 160);
	case ECoMTerrain::MirrorCavern:     return FColor( 180, 190, 210);
	case ECoMTerrain::StarlightPool:    return FColor( 160, 180, 220);
	case ECoMTerrain::ThoughtCorridor:  return FColor( 100,  90, 120);
	case ECoMTerrain::ParadoxChamber:   return FColor( 140, 100, 160);

	// Abyssal Underdark
	case ECoMTerrain::ChasmFloor:       return FColor(  40,  20,  20);
	case ECoMTerrain::BoneLabyrinth:    return FColor( 160, 140, 120);
	case ECoMTerrain::DemonNest:        return FColor( 100,  30,  30);
	case ECoMTerrain::FleshCorridor:    return FColor( 140,  80,  80);
	case ECoMTerrain::BloodPool_Aby:    return FColor( 120,  10,  10);
	case ECoMTerrain::ChaosCore:        return FColor( 160,  40, 140);
	case ECoMTerrain::VoidPocket_Aby:   return FColor(  15,   5,  25);
	case ECoMTerrain::SoulMeatCavern:   return FColor( 140, 100,  90);

	// Infernyx Iron Depths
	case ECoMTerrain::SoulForgeCavern:  return FColor( 140,  80,  40);
	case ECoMTerrain::ContractVault:    return FColor( 110, 100, 100);
	case ECoMTerrain::FireLakeCavern:   return FColor( 200,  80,  20);
	case ECoMTerrain::HellmarketCave:   return FColor( 140, 110,  70);

	// Ethereal Underdark
	case ECoMTerrain::MemoryPool:       return FColor( 140, 150, 180);
	case ECoMTerrain::EchoTunnel:       return FColor( 120, 110, 140);
	case ECoMTerrain::PhaseLabyrinth:   return FColor( 100,  90, 130);
	case ECoMTerrain::DreamChamber:     return FColor( 160, 170, 200);
	case ECoMTerrain::ThoughtCorridor_Eth: return FColor(110, 100, 130);
	case ECoMTerrain::SpiritWell:       return FColor( 120, 160, 200);
	case ECoMTerrain::MistCavern:       return FColor( 150, 160, 170);
	case ECoMTerrain::ForgottenRealm:   return FColor( 130, 120, 140);

	// Feywild Underdark
	case ECoMTerrain::FeyRootLair:      return FColor(  70, 100,  50);
	case ECoMTerrain::FeyUndermarket:   return FColor( 160, 140, 100);
	case ECoMTerrain::MushroomCave_Fey: return FColor( 140, 110, 150);
	case ECoMTerrain::CrystalSpring_Fey: return FColor(120, 180, 200);
	case ECoMTerrain::AncientBurrow:    return FColor(  90,  80,  60);
	case ECoMTerrain::CobwebHalls:      return FColor( 100,  90,  80);
	case ECoMTerrain::GlowwormDen:      return FColor(  80, 160, 120);
	case ECoMTerrain::FeyVault:         return FColor( 140, 160, 120);

	// ── Underwater ──────────────────────────────────────────────────────
	case ECoMTerrain::OceanFloor:       return FColor(  20,  40, 100);
	case ECoMTerrain::CoralReef:        return FColor( 200, 120, 100);
	case ECoMTerrain::KelpForest:       return FColor(  30, 100,  40);
	case ECoMTerrain::DeepTrench:       return FColor(  10,  15,  60);
	case ECoMTerrain::UnderwaterVolcano: return FColor(160,  50,  20);
	case ECoMTerrain::SunkenRuins:      return FColor( 100, 110, 120);
	case ECoMTerrain::CrystalGrotto_UW: return FColor( 120, 160, 200);
	case ECoMTerrain::AbyssalPlain:     return FColor(  15,  20,  60);
	case ECoMTerrain::Seamount:         return FColor(  60,  70, 100);
	case ECoMTerrain::ThermalVent:      return FColor( 180,  80,  20);

	default:
		return FColor(255, 0, 255); // Magenta = unmapped terrain (debug)
	}
}

// ─── Fog of war colour modulation ───────────────────────────────────────────

FColor ACoMTileChunkActor::ApplyFogOfWar(FColor BaseColor, bool bExplored, bool bVisible)
{
	if (bVisible)
	{
		// Full brightness — tile is currently seen
		return BaseColor;
	}

	if (bExplored)
	{
		// Explored but not visible: darken to 40% brightness
		FColor Dimmed;
		Dimmed.R = static_cast<uint8>(BaseColor.R * 0.4f);
		Dimmed.G = static_cast<uint8>(BaseColor.G * 0.4f);
		Dimmed.B = static_cast<uint8>(BaseColor.B * 0.4f);
		Dimmed.A = 255;
		return Dimmed;
	}

	// Unexplored: solid black
	return FColor(0, 0, 0, 255);
}
