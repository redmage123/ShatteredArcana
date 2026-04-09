// Copyright Mythforge Studios. All Rights Reserved.
// CoMMainMenuGameMode.cpp -- Title screen game mode implementation.

#include "Framework/CoMMainMenuGameMode.h"
#include "Framework/CoMGameInstance.h"

// =============================================================================
// Constructor
// =============================================================================

ACoMMainMenuGameMode::ACoMMainMenuGameMode()
{
	// No pawn on the title screen.
	DefaultPawnClass = nullptr;
}

// =============================================================================
// BeginPlay
// =============================================================================

void ACoMMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Fire the delegate so that CoMUISubsystem (which listens from the CoMUI
	// module) can display the main menu widget. This avoids a compile-time
	// dependency from CoMCore -> CoMUI.
	if (UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance()))
	{
		CoMGI->OnMainMenuRequested.Broadcast();
	}
	else
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("ACoMMainMenuGameMode: Could not get UCoMGameInstance. "
		            "Main menu will not be shown automatically."));
	}
}
