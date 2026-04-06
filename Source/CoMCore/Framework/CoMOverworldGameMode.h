// Copyright Mythforge Studios. All Rights Reserved.
// CoMOverworldGameMode.h — GameMode for the overworld map screen.
// COM-029 / COM-032
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CoMOverworldGameMode.generated.h"

class ACoMCameraPawn;
class ACoMGameState;
class ACoMHumanPlayerController;
class UCoMTurnSubsystem;

/**
 * ACoMOverworldGameMode
 *
 * Governs the overworld session: registers the correct Pawn/Controller/State
 * classes, logs subsystem readiness on BeginPlay, and exposes the single
 * entry point to begin a new game or resume from save.
 *
 * On BeginPlay, if this is a new session (no LoadedSaveSlotName), consumes
 * wizard-creation settings from UCoMGameInstance::NewGameSettings.
 *
 * Set as the GameMode Override on L_Overworld.
 */
UCLASS()
class COMCORE_API ACoMOverworldGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACoMOverworldGameMode();

	/** Begin a fresh game with the supplied wizard configurations. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartNewGame(int32 NumWizards);

	/** Resume from a previously saved game slot. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	bool LoadGame(const FString& SlotName);

protected:
	virtual void BeginPlay() override;
};
