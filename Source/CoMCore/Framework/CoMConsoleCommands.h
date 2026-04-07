// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMConsoleCommands.generated.h"

class UCoMTurnManager;
class UCoMFogOfWarSubsystem;

/**
 * Console command interface for headless play-testing.
 *
 * Registers UE console commands (com.*) that drive the game loop
 * without a UI.  All subsystem access is resolved at call time
 * via GetGameInstance()->GetSubsystem<>().
 *
 * Commands:
 *   com.newgame <numHumans> <numAI>   — initialize a new game
 *   com.endturn                        — end current wizard's input phase
 *   com.status                         — print turn/wizard/army/city summary
 *   com.movearmyto <ArmyID> <X> <Y>   — queue army movement
 *   com.armies                         — list armies for current wizard
 *   com.cities                         — list cities for current wizard
 *   com.mapinfo <X> <Y>               — print tile info at coordinates
 *   com.spawnunit <SpecID> <X> <Y>    — debug: spawn a unit
 *   com.spawnarmyat <X> <Y>           — debug: create army with 3 basic units
 *   com.declarewar <TargetWizardID>   — declare war on another wizard
 */
UCLASS()
class COMCORE_API UCoMConsoleCommands : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	// -----------------------------------------------------------------
	// Command handlers
	// -----------------------------------------------------------------

	void Cmd_NewGame(const TArray<FString>& Args);
	void Cmd_EndTurn(const TArray<FString>& Args);
	void Cmd_Status(const TArray<FString>& Args);
	void Cmd_MoveArmyTo(const TArray<FString>& Args);
	void Cmd_Armies(const TArray<FString>& Args);
	void Cmd_Cities(const TArray<FString>& Args);
	void Cmd_MapInfo(const TArray<FString>& Args);
	void Cmd_SpawnUnit(const TArray<FString>& Args);
	void Cmd_SpawnArmyAt(const TArray<FString>& Args);
	void Cmd_DeclareWar(const TArray<FString>& Args);

	// -----------------------------------------------------------------
	// Registered command handles (stored so we can unregister)
	// -----------------------------------------------------------------

	TArray<IConsoleObject*> RegisteredCommands;
};
