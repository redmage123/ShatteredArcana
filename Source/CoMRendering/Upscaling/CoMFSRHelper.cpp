// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMFSRHelper.cpp — FSR 2/3 upscaler helper (COM-015)

#include "CoMRendering/Upscaling/CoMFSRHelper.h"

bool UCoMFSRHelper::IsFSRAvailable()
{
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR2.Enabled"));
	return CVar != nullptr;
}

bool UCoMFSRHelper::TryEnable(int32 QualityPreset)
{
	if (!IsFSRAvailable()) return false;
	static const auto CVarEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR2.Enabled"));
	if (CVarEnable) CVarEnable->Set(1);
	static const auto CVarQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR2.QualityMode"));
	if (CVarQuality) CVarQuality->Set(QualityPreset);
	return true;
}

void UCoMFSRHelper::Disable()
{
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR2.Enabled"));
	if (CVar) CVar->Set(0);
}

int32 UCoMFSRHelper::GetActiveQualityPreset()
{
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR2.QualityMode"));
	return CVar ? CVar->GetInt() : -1;
}

int32 UCoMFSRHelper::GetFSRVersion()
{
	// Returns 3, 2, or 0. Check for FSR3 first (newer API).
	if (IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR3.Enabled"))) return 3;
	if (IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR2.Enabled"))) return 2;
	return 0;
}

bool UCoMFSRHelper::ProbePlugin()
{
	return IsFSRAvailable();
}

int32 UCoMFSRHelper::DetectVersion()
{
	return GetFSRVersion();
}
