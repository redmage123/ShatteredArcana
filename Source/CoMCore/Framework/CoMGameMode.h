// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameMode.h — Combat and Exploration game modes. COM-032
//
// Note: ACoMOverworldGameMode lives in CoMOverworldGameMode.h (COM-029);
//       this file adds the two subordinate level modes.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CoMGameMode.generated.h"

class ACoMHumanPlayerController;

/**
 * ACoMCombatGameMode
 *
 * Tactical combat map game mode.  Validates that UCoMGameInstance::CombatContext
 * is populated before play begins; logs an error and returns to the overworld if not.
 * Full UCoMCombatSubsystem wiring is deferred to Phase 5 (Sprint 3–4).
 *
 * Callers must populate UCoMGameInstance::CombatContext before the level transition.
 */
UCLASS()
class COMCORE_API ACoMCombatGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACoMCombatGameMode();

protected:
	virtual void BeginPlay() override;

	// TODO (Phase 5): Bind UCoMCombatSubsystem and resolve CombatContext.
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * ACoMExplorationGameMode
 *
 * Dungeon / encounter map game mode.  Validates that
 * UCoMGameInstance::ExplorationContext is populated before play begins.
 * Full UCoMExplorationSubsystem wiring is deferred to Phase 7 (Sprint 4–5).
 *
 * Callers must populate UCoMGameInstance::ExplorationContext before the level transition.
 */
UCLASS()
class COMCORE_API ACoMExplorationGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACoMExplorationGameMode();

protected:
	virtual void BeginPlay() override;

	// TODO (Phase 7): Bind UCoMExplorationSubsystem and resolve ExplorationContext.
};
