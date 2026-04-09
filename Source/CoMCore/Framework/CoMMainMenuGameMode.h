// Copyright Mythforge Studios. All Rights Reserved.
// CoMMainMenuGameMode.h -- Game mode for the title screen / main menu level.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CoMMainMenuGameMode.generated.h"

/**
 * ACoMMainMenuGameMode
 *
 * Lightweight game mode used on the title screen level.
 * No pawn is spawned. On BeginPlay the CoMUISubsystem is asked to display
 * the main menu widget via runtime class lookup (avoids CoMCore -> CoMUI
 * compile-time dependency).
 */
UCLASS()
class COMCORE_API ACoMMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACoMMainMenuGameMode();

protected:
	virtual void BeginPlay() override;
};
