// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMDLSSHelper.cpp — DLSS upscaler helper (COM-015)

#include "CoMRendering/Upscaling/CoMDLSSHelper.h"

bool UCoMDLSSHelper::IsDLSSAvailable()
{
	// DLSS requires the NVIDIA DLSS plugin to be loaded and an RTX-class GPU.
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
	return CVar != nullptr && CVar->GetBool();
}

bool UCoMDLSSHelper::TryEnable(int32 QualityPreset)
{
	if (!IsDLSSAvailable()) return false;
	static const auto CVarQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Quality"));
	if (CVarQuality) CVarQuality->Set(QualityPreset);
	static const auto CVarEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
	if (CVarEnable) CVarEnable->Set(1);
	return true;
}

void UCoMDLSSHelper::Disable()
{
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
	if (CVar) CVar->Set(0);
}

int32 UCoMDLSSHelper::GetActiveQualityPreset()
{
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Quality"));
	return CVar ? CVar->GetInt() : -1;
}
