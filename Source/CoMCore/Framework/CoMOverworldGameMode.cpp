#include "Framework/CoMOverworldGameMode.h"
#include "Framework/CoMPlayerController.h"
#include "Framework/CoMGameInstance.h"
#include "Camera/CoMCameraPawn.h"
#include "Framework/CoMGameState.h"
#include "Turn/CoMTurnSubsystem.h"
#include "World/CoMWorldMapSubsystem.h"
#include "Economy/CoMCitySubsystem.h"
#include "Units/CoMUnitSubsystem.h"
#include "CoreTypes/CoMGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"


// Copyright Mythforge Studios. All Rights Reserved.
// CoMOverworldGameMode.cpp — COM-029 / COM-032


ACoMOverworldGameMode::ACoMOverworldGameMode()
{
	PlayerControllerClass = ACoMHumanPlayerController::StaticClass();  // COM-032
	DefaultPawnClass      = ACoMCameraPawn::StaticClass();
	GameStateClass        = ACoMGameState::StaticClass();
}

void ACoMOverworldGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* W = GetWorld();
	if (!W) { return; }

	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	// Log subsystem readiness.
	const UCoMWorldMapSubsystem* MapSys  = GI->GetSubsystem<UCoMWorldMapSubsystem>();
	const UCoMTurnSubsystem*     TurnSys = GI->GetSubsystem<UCoMTurnSubsystem>();

	UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] WorldMapSubsystem ready: %s"),
	       MapSys ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] TurnSubsystem ready: %s"),
	       TurnSys ? TEXT("YES") : TEXT("NO"));

	UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GI);

	// Check if a game is already in progress (e.g. loaded save).
	if (TurnSys && TurnSys->HasGameStarted())
	{
		// Spawn the rendering bridge actor for the resumed game.
		UClass* RenderActorClass = LoadClass<AActor>(nullptr,
			TEXT("/Script/CoMRendering.CoMOverworldRenderingActor"));
		if (RenderActorClass)
		{
			W->SpawnActor<AActor>(RenderActorClass, FTransform::Identity);
			UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] Spawned OverworldRenderingActor (resume)"));
		}

		// Resume — just show the HUD.
		if (CoMGI) { CoMGI->OnHUDRequested.Broadcast(); }
		return;
	}

	// Check if the UI flow deposited wizard-creation settings.
	if (CoMGI && !CoMGI->IsLoadedGame() && CoMGI->NewGameSettings.IsValid())
	{
		const FCoMNewGameSettings Settings = CoMGI->ConsumeNewGameSettings();
		UE_LOG(LogTemp, Log,
		       TEXT("[CoMOverworldGameMode] Consumed NewGameSettings for wizard '%s'."),
		       *Settings.WizardName.ToString());
		// Start the game with these settings.
		StartNewGame(1);
		return;
	}

	// No settings, no loaded game — show the main menu.
	if (CoMGI) { CoMGI->OnMainMenuRequested.Broadcast(); }
}

void ACoMOverworldGameMode::StartNewGame(int32 NumWizards)
{
	UWorld* W = GetWorld();
	if (!W) { return; }

	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GI);

	// Hide any menus before starting.
	if (CoMGI) { CoMGI->OnHideMenusRequested.Broadcast(); }

	// Initialise the 24-layer world map (8 planes × 3 layers).
	UCoMWorldMapSubsystem* MapSys = GI->GetSubsystem<UCoMWorldMapSubsystem>();
	if (MapSys)
	{
		MapSys->InitializeLayers();
		UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] World map initialised, NumLayers=%d"),
		       CoM::TOTAL_MAP_LAYERS);

		// Generate basic terrain on Aurelith Surface layer.
		for (int32 Y = 0; Y < CoM::MAP_HEIGHT; ++Y)
		{
			for (int32 X = 0; X < CoM::MAP_WIDTH; ++X)
			{
				FCoMTileData* Tile = MapSys->GetTileMutable(ECoMPlane::Aurelith, ECoMMapLayer::Surface, X, Y);
				if (!Tile) { continue; }

				// Simple deterministic terrain generation.
				const int32 Hash = (X * 7 + Y * 13 + X * Y) % 100;
				if (Hash < 40)       { Tile->Terrain = ECoMTerrain::Grassland; }
				else if (Hash < 55)  { Tile->Terrain = ECoMTerrain::Forest; }
				else if (Hash < 70)  { Tile->Terrain = ECoMTerrain::Hills; }
				else if (Hash < 80)  { Tile->Terrain = ECoMTerrain::Plains; }
				else if (Hash < 88)  { Tile->Terrain = ECoMTerrain::Mountains; }
				else if (Hash < 93)  { Tile->Terrain = ECoMTerrain::Swamp; }
				else                 { Tile->Terrain = ECoMTerrain::Ocean; }

				Tile->bImpassable = (Tile->Terrain == ECoMTerrain::Ocean || Tile->Terrain == ECoMTerrain::Mountains);

				// Reveal for player (wizard 0).
				Tile->FogRevealed |= (1u << 0);
				Tile->CurrentVision |= (1u << 0);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] Terrain generated for Aurelith Surface."));
	}

	// Found a starting city for the player at (80, 50).
	UCoMCitySubsystem* CitySub = GI->GetSubsystem<UCoMCitySubsystem>();
	if (CitySub)
	{
		const int32 CityId = CitySub->FoundCity(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface,
		                                        FIntPoint(80, 50), CoMTags::Race::HighMen,
		                                        FText::FromString(TEXT("Arcanum")));
		UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] Founded starting city, CityID=%d"), CityId);
	}

	// Spawn a starting army with swordsmen and a settler at (82, 50).
	UCoMUnitSubsystem* UnitSub = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (UnitSub)
	{
		const int32 ArmyId = UnitSub->CreateArmy(0, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(82, 50));
		// SpawnUnit takes SpecID (int32); use 1 for swordsmen, 2 for settler as placeholder IDs.
		const int32 SwordsmenId = UnitSub->SpawnUnit(1, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(82, 50), 0);
		UnitSub->AddUnitToArmy(SwordsmenId, ArmyId);
		const int32 SettlerId = UnitSub->SpawnUnit(2, ECoMPlane::Aurelith, ECoMMapLayer::Surface, FIntPoint(82, 50), 0);
		UnitSub->AddUnitToArmy(SettlerId, ArmyId);
		UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] Spawned starting army, ArmyID=%d (Swordsmen=%d, Settler=%d)"),
		       ArmyId, SwordsmenId, SettlerId);
	}

	// Spawn the overworld rendering bridge actor.
	// Use LoadClass to avoid a CoMCore -> CoMRendering module dependency.
	UClass* RenderActorClass = LoadClass<AActor>(nullptr,
		TEXT("/Script/CoMRendering.CoMOverworldRenderingActor"));
	if (RenderActorClass)
	{
		W->SpawnActor<AActor>(RenderActorClass, FTransform::Identity);
		UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] Spawned OverworldRenderingActor"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CoMOverworldGameMode] Could not load CoMOverworldRenderingActor class"));
	}

	// Show the in-game HUD.
	if (CoMGI) { CoMGI->OnHUDRequested.Broadcast(); }

	// Boot the turn engine.
	UCoMTurnSubsystem* TurnSys = GI->GetSubsystem<UCoMTurnSubsystem>();
	if (TurnSys)
	{
		TurnSys->StartGame();
		UE_LOG(LogTemp, Log,
		       TEXT("[CoMOverworldGameMode] TurnSubsystem started, NumWizards=%d"), NumWizards);
	}
}

bool ACoMOverworldGameMode::LoadGame(const FString& SlotName)
{
	// Save/load integration is a Phase 2 deliverable (COM-110).
	// Stubbed here so Blueprint callers compile; returns false until implemented.
	UE_LOG(LogTemp, Warning,
	       TEXT("[CoMOverworldGameMode] LoadGame('%s') not yet implemented (COM-110)."),
	       *SlotName);
	return false;
}

