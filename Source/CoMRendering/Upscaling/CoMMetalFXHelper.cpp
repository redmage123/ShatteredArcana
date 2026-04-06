// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMMetalFXHelper.cpp — Apple MetalFX upscaler helper (COM-015)
// Note: MetalFX temporal upscaling is macOS-only. On non-Mac platforms all
// methods return false/no-op — they will never be called at runtime since
// UCoMRHIAbstraction::SelectBestUpscaler() gates on PLATFORM_MAC.

#include "CoMRendering/Upscaling/CoMMetalFXHelper.h"

bool UCoMMetalFXHelper::IsMetalFXAvailable()
{
#if PLATFORM_MAC
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalFX.Upscale"));
	return CVar != nullptr && CVar->GetBool();
#else
	return false;
#endif
}

bool UCoMMetalFXHelper::TryEnable()
{
#if PLATFORM_MAC
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalFX.Upscale"));
	if (CVar) { CVar->Set(1); return true; }
#endif
	return false;
}

bool UCoMMetalFXHelper::TryEnableSpatial()
{
#if PLATFORM_MAC
	// Spatial upscaling (single-frame, lower quality than temporal)
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalFX.Spatial"));
	if (CVar) { CVar->Set(1); return true; }
#endif
	return false;
}

void UCoMMetalFXHelper::Disable()
{
#if PLATFORM_MAC
	static const auto CVarT = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalFX.Upscale"));
	if (CVarT) CVarT->Set(0);
	static const auto CVarS = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalFX.Spatial"));
	if (CVarS) CVarS->Set(0);
#endif
}

bool UCoMMetalFXHelper::IsTemporalActive()
{
#if PLATFORM_MAC
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalFX.Upscale"));
	return CVar ? CVar->GetBool() : false;
#else
	return false;
#endif
}

bool UCoMMetalFXHelper::ProbeMetalPlatform()
{
#if PLATFORM_MAC
	return true;
#else
	return false;
#endif
}
