// Copyright Mythforge Studios. All Rights Reserved.
// CoMMinimapSubsystem.cpp -- Data layer for the overworld minimap.

#include "CoMMinimapSubsystem.h"
#include "CoMWorldMapSubsystem.h"
#include "CoMFogOfWarSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/CoreTypes/CoMStructs.h"

// ─────────────────────────────────────────────────────────────────────────────
// USubsystem Interface
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		WorldMapSub = GI->GetSubsystem<UCoMWorldMapSubsystem>();
		FogSub      = GI->GetSubsystem<UCoMFogOfWarSubsystem>();
		CitySub     = GI->GetSubsystem<UCoMCitySubsystem>();
		UnitSub     = GI->GetSubsystem<UCoMUnitSubsystem>();
	}

	const int32 PixelCount = CoM::MAP_WIDTH * CoM::MAP_HEIGHT;
	TerrainBuffer.SetNumZeroed(PixelCount);
	CompositedBuffer.SetNumZeroed(PixelCount);
}

void UCoMMinimapSubsystem::Deinitialize()
{
	TerrainBuffer.Empty();
	CompositedBuffer.Empty();
	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapSubsystem::InitializeForPlane(ECoMPlane Plane, int32 WizardId)
{
	CurrentPlane    = Plane;
	CurrentWizardId = WizardId;
	RefreshTerrain();
	UpdateFog();
}

void UCoMMinimapSubsystem::SetPlane(ECoMPlane Plane)
{
	if (CurrentPlane != Plane)
	{
		CurrentPlane = Plane;
		RefreshTerrain();
		UpdateFog();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Refresh Methods
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapSubsystem::RefreshTerrain()
{
	if (!WorldMapSub || !WorldMapSub->IsInitialized())
	{
		// Fill with black if the world map is not ready yet.
		const int32 PixelCount = CoM::MAP_WIDTH * CoM::MAP_HEIGHT;
		for (int32 i = 0; i < PixelCount; ++i)
		{
			TerrainBuffer[i] = FColor::Black;
		}
		CompositedBuffer = TerrainBuffer;
		return;
	}

	const FCoMMapLayerData& Layer = WorldMapSub->GetLayer(CurrentPlane, ECoMMapLayer::Surface);

	for (int32 Y = 0; Y < CoM::MAP_HEIGHT; ++Y)
	{
		for (int32 X = 0; X < CoM::MAP_WIDTH; ++X)
		{
			const int32 Idx = Y * CoM::MAP_WIDTH + X;
			const FCoMTileData* Tile = Layer.GetTile(X, Y);
			if (Tile)
			{
				TerrainBuffer[Idx] = GetTerrainColor(Tile->Terrain);
			}
			else
			{
				TerrainBuffer[Idx] = FColor::Black;
			}
		}
	}

	// Start composited from terrain base.
	CompositedBuffer = TerrainBuffer;
}

void UCoMMinimapSubsystem::UpdateFog()
{
	// Start from terrain base.
	CompositedBuffer = TerrainBuffer;

	if (!FogSub)
	{
		return;
	}

	for (int32 Y = 0; Y < CoM::MAP_HEIGHT; ++Y)
	{
		for (int32 X = 0; X < CoM::MAP_WIDTH; ++X)
		{
			const int32 Idx = Y * CoM::MAP_WIDTH + X;
			const FIntPoint TilePos(X, Y);

			const bool bVisible  = FogSub->IsTileVisible(CurrentWizardId, CurrentPlane, TilePos);
			const bool bExplored = FogSub->IsTileExplored(CurrentWizardId, CurrentPlane, TilePos);

			if (!bExplored)
			{
				// Unexplored: black
				CompositedBuffer[Idx] = FColor::Black;
			}
			else if (!bVisible)
			{
				// Explored but not currently visible: darken by multiply 0.4
				FColor& C = CompositedBuffer[Idx];
				C.R = static_cast<uint8>(C.R * 0.4f);
				C.G = static_cast<uint8>(C.G * 0.4f);
				C.B = static_cast<uint8>(C.B * 0.4f);
			}
			// Else: currently visible — full color, leave as-is.
		}
	}
}

void UCoMMinimapSubsystem::UpdateMarkers(
	const TArray<FIntPoint>& PlayerCities, const TArray<FIntPoint>& EnemyCities,
	const TArray<FIntPoint>& NeutralCities,
	const TArray<FIntPoint>& PlayerArmies, const TArray<FIntPoint>& EnemyArmies)
{
	// Re-apply fog to reset markers from previous frame.
	UpdateFog();

	// City markers: 2x2 pixels
	static const FColor PlayerCityColor(0xDA, 0xA5, 0x20, 0xFF);  // Gold #daa520
	static const FColor EnemyCityColor(0xCC, 0x22, 0x22, 0xFF);   // Red #cc2222
	static const FColor NeutralCityColor(0xCC, 0xCC, 0xCC, 0xFF); // White-ish #cccccc

	for (const FIntPoint& Pos : PlayerCities)
	{
		SetPixel2x2(Pos.X, Pos.Y, PlayerCityColor);
	}
	for (const FIntPoint& Pos : EnemyCities)
	{
		SetPixel2x2(Pos.X, Pos.Y, EnemyCityColor);
	}
	for (const FIntPoint& Pos : NeutralCities)
	{
		SetPixel2x2(Pos.X, Pos.Y, NeutralCityColor);
	}

	// Army markers: 1x1 pixel
	static const FColor PlayerArmyColor(0x00, 0xFF, 0x00, 0xFF);  // Bright green #00ff00
	static const FColor EnemyArmyColor(0xFF, 0x00, 0x00, 0xFF);   // Red #ff0000

	for (const FIntPoint& Pos : PlayerArmies)
	{
		SetPixel(Pos.X, Pos.Y, PlayerArmyColor);
	}
	for (const FIntPoint& Pos : EnemyArmies)
	{
		SetPixel(Pos.X, Pos.Y, EnemyArmyColor);
	}
}

void UCoMMinimapSubsystem::UpdateCameraRect(FIntPoint TopLeft, FIntPoint BottomRight)
{
	CameraTopLeft     = TopLeft;
	CameraBottomRight = BottomRight;
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

FIntPoint UCoMMinimapSubsystem::ScreenToWorldPosition(FVector2D MinimapUV) const
{
	const int32 X = FMath::Clamp(FMath::FloorToInt32(MinimapUV.X * CoM::MAP_WIDTH), 0, CoM::MAP_WIDTH - 1);
	const int32 Y = FMath::Clamp(FMath::FloorToInt32(MinimapUV.Y * CoM::MAP_HEIGHT), 0, CoM::MAP_HEIGHT - 1);
	return FIntPoint(X, Y);
}

// ─────────────────────────────────────────────────────────────────────────────
// Pixel Helpers
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapSubsystem::SetPixel(int32 X, int32 Y, FColor Color)
{
	if (X >= 0 && X < CoM::MAP_WIDTH && Y >= 0 && Y < CoM::MAP_HEIGHT)
	{
		CompositedBuffer[Y * CoM::MAP_WIDTH + X] = Color;
	}
}

void UCoMMinimapSubsystem::SetPixel2x2(int32 X, int32 Y, FColor Color)
{
	SetPixel(X,     Y,     Color);
	SetPixel(X + 1, Y,     Color);
	SetPixel(X,     Y + 1, Color);
	SetPixel(X + 1, Y + 1, Color);
}

// ─────────────────────────────────────────────────────────────────────────────
// Terrain Color Lookup
// ─────────────────────────────────────────────────────────────────────────────

FColor UCoMMinimapSubsystem::GetTerrainColor(ECoMTerrain Terrain)
{
	// Group terrain types by visual category for minimap display.
	// Each plane has unique terrain enums but they map to familiar visual types.
	switch (Terrain)
	{
	// ── Ocean / Water ─────────────────────────────────────────────────────
	case ECoMTerrain::Ocean:
	case ECoMTerrain::VoidOcean:
	case ECoMTerrain::SulfurSeas:
	case ECoMTerrain::MistOcean:
	case ECoMTerrain::MistSeas:
		return FColor(0x00, 0x1A, 0x33, 0xFF); // Deep blue #001a33

	case ECoMTerrain::River:
	case ECoMTerrain::Shore:
	case ECoMTerrain::CrystalLake:
	case ECoMTerrain::AmberCoast:
	case ECoMTerrain::MagmaRivers:
	case ECoMTerrain::BioluminescentDeep:
	case ECoMTerrain::MirrorLakes:
	case ECoMTerrain::PhaseShores:
	case ECoMTerrain::BloodRivers:
		return FColor(0x00, 0x33, 0x66, 0xFF); // Medium blue

	// ── Grassland / Plains ────────────────────────────────────────────────
	case ECoMTerrain::Grassland:
	case ECoMTerrain::Plains:
	case ECoMTerrain::Savanna:
	case ECoMTerrain::PollenPlains:
	case ECoMTerrain::CloudPlains:
	case ECoMTerrain::DreamMeadows:
	case ECoMTerrain::SilverMistPlains:
	case ECoMTerrain::TwilightMeadows:
	case ECoMTerrain::EmberPlains:
	case ECoMTerrain::AncientHighway:
		return FColor(0x2A, 0x7A, 0x2A, 0xFF); // Green #2a7a2a

	// ── Forest / Jungle ───────────────────────────────────────────────────
	case ECoMTerrain::Forest:
	case ECoMTerrain::Jungle:
	case ECoMTerrain::EnchantedForest:
	case ECoMTerrain::MegaJungle:
	case ECoMTerrain::FungalForest:
	case ECoMTerrain::ShadowForest:
	case ECoMTerrain::ScorchedForest:
	case ECoMTerrain::EternalForest:
	case ECoMTerrain::ShiftingGlades:
	case ECoMTerrain::ThorncraftHollow:
	case ECoMTerrain::WildHuntGrounds:
		return FColor(0x1A, 0x5A, 0x1A, 0xFF); // Dark green #1a5a1a

	// ── Mountains ─────────────────────────────────────────────────────────
	case ECoMTerrain::Mountains:
	case ECoMTerrain::DarkMountains:
	case ECoMTerrain::RootMountains:
	case ECoMTerrain::BasaltMountains:
	case ECoMTerrain::ResonancePeaks:
	case ECoMTerrain::BlossomPeaks:
	case ECoMTerrain::ObsidianSpires:
		return FColor(0x80, 0x80, 0x80, 0xFF); // Grey #808080

	// ── Desert / Sand ─────────────────────────────────────────────────────
	case ECoMTerrain::Desert:
	case ECoMTerrain::CrystalDesert:
	case ECoMTerrain::SporeDesert:
	case ECoMTerrain::AshDesert:
	case ECoMTerrain::DreamDeserts:
	case ECoMTerrain::CarrionDesert:
	case ECoMTerrain::Badlands:
		return FColor(0xC2, 0xA6, 0x4A, 0xFF); // Tan #c2a64a

	// ── Tundra / Snow ─────────────────────────────────────────────────────
	case ECoMTerrain::Tundra:
	case ECoMTerrain::GloomTundra:
	case ECoMTerrain::StarlightTundra:
	case ECoMTerrain::FeyWastes:
		return FColor(0xD0, 0xD0, 0xE0, 0xFF); // White-ish #d0d0e0

	// ── Swamp ─────────────────────────────────────────────────────────────
	case ECoMTerrain::Swamp:
	case ECoMTerrain::LivingSwamp:
	case ECoMTerrain::CorruptedSwamp:
	case ECoMTerrain::EtherMarshes:
	case ECoMTerrain::AbyssalMires:
	case ECoMTerrain::GoreMarshes:
	case ECoMTerrain::MushroomRings:
		return FColor(0x3A, 0x5A, 0x2A, 0xFF); // Dark green-brown #3a5a2a

	// ── Hills ─────────────────────────────────────────────────────────────
	case ECoMTerrain::Hills:
	case ECoMTerrain::TwilightHills:
	case ECoMTerrain::VineHills:
	case ECoMTerrain::CinderHills:
		return FColor(0x8A, 0x7A, 0x4A, 0xFF); // Light brown #8a7a4a

	// ── Volcanic / Lava ───────────────────────────────────────────────────
	case ECoMTerrain::Volcano:
	case ECoMTerrain::LavaFields:
	case ECoMTerrain::VolcanicChain:
	case ECoMTerrain::FireGeyser:
	case ECoMTerrain::LavaTube_Surface:
	case ECoMTerrain::ManaGeyser:
	case ECoMTerrain::FireLakes:
	case ECoMTerrain::TormentFields:
		return FColor(0xCC, 0x44, 0x00, 0xFF); // Orange-red #cc4400

	// ── Shadow / Void ─────────────────────────────────────────────────────
	case ECoMTerrain::ShadowRift:
	case ECoMTerrain::ObsidianPlains:
	case ECoMTerrain::AshWastes:
	case ECoMTerrain::FloatingObsidian:
	case ECoMTerrain::VoidRifts:
	case ECoMTerrain::VoidFissures:
	case ECoMTerrain::VoidWhisperFields:
	case ECoMTerrain::FadeZones:
	case ECoMTerrain::ScreamingChasms:
	case ECoMTerrain::ChasmEdge:
		return FColor(0x2A, 0x0A, 0x3A, 0xFF); // Dark purple #2a0a3a

	// ── Crystal / Ethereal ────────────────────────────────────────────────
	case ECoMTerrain::CrystalSpires:
	case ECoMTerrain::FloatingIslands:
	case ECoMTerrain::GravityAnomaly:
	case ECoMTerrain::PhaseShiftingTerrain:
	case ECoMTerrain::AuroraField:
	case ECoMTerrain::CrystallizedMemories:
	case ECoMTerrain::ReflectionMirrors:
	case ECoMTerrain::SpiritArchipelagos:
	case ECoMTerrain::AncientStandingStones:
	case ECoMTerrain::SeelieCourt:
	case ECoMTerrain::UnseelieHollow:
	case ECoMTerrain::ErlingPath:
		return FColor(0x80, 0xC0, 0xFF, 0xFF); // Light blue #80c0ff

	// ── Corrupted / Abyssal ───────────────────────────────────────────────
	case ECoMTerrain::BonePlains:
	case ECoMTerrain::LivingWalls:
	case ECoMTerrain::DemonPillars:
	case ECoMTerrain::FleshCitadels:
	case ECoMTerrain::ChaosStormfield:
	case ECoMTerrain::IronFortresses:
	case ECoMTerrain::SoulForges:
	case ECoMTerrain::BrimstoneHighways:
	case ECoMTerrain::ContractSpires:
	case ECoMTerrain::HellgateMarkets:
		return FColor(0x4A, 0x0A, 0x0A, 0xFF); // Dark red #4a0a0a

	// ── Ethereal / Dream ──────────────────────────────────────────────────
	case ECoMTerrain::EchoRuins:
	case ECoMTerrain::ThoughtStorms:
	case ECoMTerrain::MemoryLandscape:
	case ECoMTerrain::FeyMarketCrossroads:
		return FColor(0x60, 0x80, 0xA0, 0xFF); // Muted blue-grey

	// ── Underdark (cavern default) ────────────────────────────────────────
	case ECoMTerrain::CavernFloor:
	case ECoMTerrain::FungalGrove:
	case ECoMTerrain::UndergroundLake:
	case ECoMTerrain::CrystalCavern_Arc:
	case ECoMTerrain::DwarvenRuins:
	case ECoMTerrain::CollapsedTunnel:
	case ECoMTerrain::LavaTube_Arc:
	case ECoMTerrain::DeepChasm:
	case ECoMTerrain::MineralVein:
	case ECoMTerrain::UndergroundRiver:
	case ECoMTerrain::ShadowCavern:
	case ECoMTerrain::VoidPocket:
	case ECoMTerrain::BoneOssuary:
	case ECoMTerrain::ObsidianLabyrinth:
	case ECoMTerrain::SoulWell:
	case ECoMTerrain::CorruptedCrystal:
	case ECoMTerrain::WebbedTunnels:
	case ECoMTerrain::EchoChamber:
	case ECoMTerrain::DarkRiver:
	case ECoMTerrain::NightmareRealm:
	case ECoMTerrain::RootTunnel:
	case ECoMTerrain::MyceliumNet_Verd:
	case ECoMTerrain::UndergroundGarden:
	case ECoMTerrain::AmberPool:
	case ECoMTerrain::InsectHive:
	case ECoMTerrain::PetrifiedForest:
	case ECoMTerrain::MudCavern:
	case ECoMTerrain::CocoonChamber:
	case ECoMTerrain::SapRiver:
	case ECoMTerrain::SeedVault:
	case ECoMTerrain::MagmaChamber:
	case ECoMTerrain::ObsidianCavern:
	case ECoMTerrain::SulfurPit:
	case ECoMTerrain::ForgeCavern:
	case ECoMTerrain::CinderTunnel:
	case ECoMTerrain::LavaRiver_Inf:
	case ECoMTerrain::DemonPit:
	case ECoMTerrain::CrystalForge:
	case ECoMTerrain::EmberShelf:
	case ECoMTerrain::SmokeVent:
	case ECoMTerrain::GravityWell:
	case ECoMTerrain::PhaseCavern:
	case ECoMTerrain::CrystalResonance:
	case ECoMTerrain::VoidBubble:
	case ECoMTerrain::DreamGrotto:
	case ECoMTerrain::TemporalEddy:
	case ECoMTerrain::MirrorCavern:
	case ECoMTerrain::StarlightPool:
	case ECoMTerrain::ThoughtCorridor:
	case ECoMTerrain::ParadoxChamber:
	case ECoMTerrain::ChasmFloor:
	case ECoMTerrain::BoneLabyrinth:
	case ECoMTerrain::DemonNest:
	case ECoMTerrain::FleshCorridor:
	case ECoMTerrain::BloodPool_Aby:
	case ECoMTerrain::ChaosCore:
	case ECoMTerrain::VoidPocket_Aby:
	case ECoMTerrain::SoulMeatCavern:
	case ECoMTerrain::SoulForgeCavern:
	case ECoMTerrain::ContractVault:
	case ECoMTerrain::FireLakeCavern:
	case ECoMTerrain::HellmarketCave:
	case ECoMTerrain::MemoryPool:
	case ECoMTerrain::EchoTunnel:
	case ECoMTerrain::PhaseLabyrinth:
	case ECoMTerrain::DreamChamber:
	case ECoMTerrain::ThoughtCorridor_Eth:
	case ECoMTerrain::SpiritWell:
	case ECoMTerrain::MistCavern:
	case ECoMTerrain::ForgottenRealm:
	case ECoMTerrain::FeyRootLair:
	case ECoMTerrain::FeyUndermarket:
	case ECoMTerrain::MushroomCave_Fey:
	case ECoMTerrain::CrystalSpring_Fey:
	case ECoMTerrain::AncientBurrow:
	case ECoMTerrain::CobwebHalls:
	case ECoMTerrain::GlowwormDen:
	case ECoMTerrain::FeyVault:
		return FColor(0x44, 0x33, 0x22, 0xFF); // Dark brown (underground)

	// ── Underwater ────────────────────────────────────────────────────────
	case ECoMTerrain::OceanFloor:
	case ECoMTerrain::CoralReef:
	case ECoMTerrain::KelpForest:
	case ECoMTerrain::DeepTrench:
	case ECoMTerrain::UnderwaterVolcano:
	case ECoMTerrain::SunkenRuins:
	case ECoMTerrain::CrystalGrotto_UW:
	case ECoMTerrain::AbyssalPlain:
	case ECoMTerrain::Seamount:
	case ECoMTerrain::ThermalVent:
		return FColor(0x00, 0x0D, 0x1A, 0xFF); // Very deep blue

	// ── Other / Special Surface ───────────────────────────────────────────
	case ECoMTerrain::WorldTree:
	case ECoMTerrain::SentientTerrain:
	case ECoMTerrain::MyceliumNetwork:
		return FColor(0x0A, 0x4A, 0x0A, 0xFF); // Very dark green (special)

	default:
		return FColor(0x40, 0x40, 0x40, 0xFF); // Grey fallback
	}
}

FString UCoMMinimapSubsystem::GetPlaneDisplayName(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return TEXT("Aurelith");
	case ECoMPlane::Noctharion: return TEXT("Noctharion");
	case ECoMPlane::Verdantis:  return TEXT("Verdantis");
	case ECoMPlane::Infernyx:   return TEXT("Infernyx");
	case ECoMPlane::Aethermist: return TEXT("Aethermist");
	case ECoMPlane::Abyssal:    return TEXT("Abyssal");
	case ECoMPlane::Ethereal:   return TEXT("Ethereal");
	case ECoMPlane::Feywild:    return TEXT("Feywild");
	default:                    return TEXT("Unknown");
	}
}
