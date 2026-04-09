#include "Framework/CoMOverworldGameMode.h"
#include "Framework/CoMPlayerController.h"
#include "Framework/CoMGameInstance.h"
#include "Camera/CoMCameraPawn.h"
#include "Framework/CoMGameState.h"
#include "Turn/CoMTurnSubsystem.h"
#include "World/CoMWorldMapSubsystem.h"
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
	if (W)
	{
		// WorldMap and Turn subsystems auto-initialise via UWorldSubsystem::Initialize().
		// Log readiness so QA can confirm in the PIE output log.
		const UCoMWorldMapSubsystem* MapSys  = GetGameInstance()->GetSubsystem<UCoMWorldMapSubsystem>();
		const UCoMTurnSubsystem*     TurnSys = GetGameInstance()->GetSubsystem<UCoMTurnSubsystem>();

		UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] WorldMapSubsystem ready: %s"),
		       MapSys ? TEXT("YES") : TEXT("NO"));
		UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] TurnSubsystem ready: %s"),
		       TurnSys ? TEXT("YES") : TEXT("NO"));
	}

	// On a new game session, consume the wizard-creation settings that the UI
	// flow deposited in GameInstance before the level transition (COM-032).
	if (UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance()))
	{
		if (!CoMGI->IsLoadedGame() && CoMGI->NewGameSettings.WizardClass != ECoMWizardClass::None)
		{
			const FCoMNewGameSettings Settings = CoMGI->ConsumeNewGameSettings();
			// TODO (Sprint 2): Forward Settings to local PlayerState via PlayerController.
			UE_LOG(LogTemp, Log,
			       TEXT("[CoMOverworldGameMode] Consumed NewGameSettings for wizard '%s'."),
			       *Settings.WizardName.ToString());
		}
	}
}

void ACoMOverworldGameMode::StartNewGame(int32 NumWizards)
{
	UWorld* W = GetWorld();
	if (!W) { return; }

	// Initialise the 24-layer world map (8 planes × 3 layers).
	UCoMWorldMapSubsystem* MapSys = GetGameInstance()->GetSubsystem<UCoMWorldMapSubsystem>();
	if (MapSys)
	{
		MapSys->InitializeLayers();
		UE_LOG(LogTemp, Log, TEXT("[CoMOverworldGameMode] World map initialised, NumLayers=%d"),
		       CoM::TOTAL_MAP_LAYERS);
	}

	// Boot the turn engine.
	UCoMTurnSubsystem* TurnSys = GetGameInstance()->GetSubsystem<UCoMTurnSubsystem>();
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

