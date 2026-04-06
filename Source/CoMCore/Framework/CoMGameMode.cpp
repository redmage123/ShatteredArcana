// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameMode.cpp — COM-032
#include "Framework/CoMGameMode.h"
#include "Framework/CoMPlayerController.h"
#include "Framework/CoMGameInstance.h"

// ─── ACoMCombatGameMode ───────────────────────────────────────────────────────

ACoMCombatGameMode::ACoMCombatGameMode()
{
	PlayerControllerClass = ACoMHumanPlayerController::StaticClass();
	// No free-roaming camera in combat; the combat subsystem assigns pawns directly.
	DefaultPawnClass = nullptr;
}

void ACoMCombatGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (const UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance()))
	{
		if (!CoMGI->CombatContext.IsValid())
		{
			UE_LOG(LogTemp, Error,
			       TEXT("ACoMCombatGameMode: CombatContext is invalid — level entered "
			            "without a valid context. This is a bug; return to the overworld."));
		}
		// TODO (Phase 5): Pass CombatContext to UCoMCombatSubsystem and start the battle.
	}
}

// ─── ACoMExplorationGameMode ──────────────────────────────────────────────────

ACoMExplorationGameMode::ACoMExplorationGameMode()
{
	PlayerControllerClass = ACoMHumanPlayerController::StaticClass();
	DefaultPawnClass      = nullptr;
}

void ACoMExplorationGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (const UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance()))
	{
		if (!CoMGI->ExplorationContext.IsValid())
		{
			UE_LOG(LogTemp, Error,
			       TEXT("ACoMExplorationGameMode: ExplorationContext is invalid — level "
			            "entered without a valid context."));
		}
		// TODO (Phase 7): Pass ExplorationContext to UCoMExplorationSubsystem.
	}
}
