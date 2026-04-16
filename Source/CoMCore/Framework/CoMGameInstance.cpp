// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameInstance.cpp — COM-032
#include "Framework/CoMGameInstance.h"
#include "GameFramework/GameUserSettings.h"

UCoMGameInstance::UCoMGameInstance()
{
}

void UCoMGameInstance::Init()
{
	Super::Init();

	// Force windowed fullscreen on startup
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
		Settings->SetScreenResolution(FIntPoint(1920, 1080));
		Settings->ConfirmVideoMode();
		Settings->ApplySettings(false);
	}
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
