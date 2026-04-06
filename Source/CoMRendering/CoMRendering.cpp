// Copyright Mythforge Studios. All Rights Reserved.
// CoMRendering.cpp — Module entry point for the CoMRendering runtime module.
//
// This module owns:
//   RHI/CoMRHIAbstraction — backend detection, feature queries, upscaler selection
//   PSO/CoMPSOCache       — PSO bundle loading and fallback
//   Upscaling/            — per-vendor upscaler init helpers (future)
//
// Rule: NEVER call DX12/Vulkan/Metal/OpenGL APIs directly. Always go through UE5 RHI.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMRendering, Log, All);

class FCoMRenderingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogCoMRendering, Log, TEXT("CoMRendering module loaded"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogCoMRendering, Log, TEXT("CoMRendering module unloaded"));
	}
};

IMPLEMENT_MODULE(FCoMRenderingModule, CoMRendering);
