// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMXeSSHelper.cpp — Intel XeSS upscaler helper (COM-015)

#include "CoMRendering/Upscaling/CoMXeSSHelper.h"

bool UCoMXeSSHelper::IsXeSSAvailable()
{
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));
	return CVar != nullptr;
}

bool UCoMXeSSHelper::TryEnable(int32 QualityPreset)
{
	if (!IsXeSSAvailable()) return false;
	static const auto CVarEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));
	if (CVarEnable) CVarEnable->Set(1);
	static const auto CVarQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Quality"));
	if (CVarQuality) CVarQuality->Set(QualityPreset);
	return true;
}

void UCoMXeSSHelper::Disable()
{
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));
	if (CVar) CVar->Set(0);
}

int32 UCoMXeSSHelper::GetActiveQualityPreset()
{
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Quality"));
	return CVar ? CVar->GetInt() : -1;
}

bool UCoMXeSSHelper::IsNativeXMXPath()
{
	// Intel XMX (Matrix Extension) path is available on Xe-HPG (Arc) and later.
	// XeSS plugin reports this via the XeSS.NativeXMX CVar when supported.
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.NativeXMX"));
	return CVar ? CVar->GetBool() : false;
}

bool UCoMXeSSHelper::ProbePlugin()
{
	return IsXeSSAvailable();
}
