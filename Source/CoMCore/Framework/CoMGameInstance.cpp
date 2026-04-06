// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameInstance.cpp — COM-032
#include "Framework/CoMGameInstance.h"

UCoMGameInstance::UCoMGameInstance()
{
}

void UCoMGameInstance::Init()
{
	Super::Init();
}

void UCoMGameInstance::Shutdown()
{
	// Clear all contexts so stale data cannot bleed into a subsequent session
	// (e.g. Quick Restart without a full process exit).
	CombatContext.Reset();
	ExplorationContext.Reset();
	NewGameSettings.Reset();

	Super::Shutdown();
}

FCoMNewGameSettings UCoMGameInstance::ConsumeNewGameSettings()
{
	const FCoMNewGameSettings Copy = NewGameSettings;
	NewGameSettings.Reset();
	return Copy;
}
