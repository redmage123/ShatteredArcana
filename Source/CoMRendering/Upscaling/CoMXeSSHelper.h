// Copyright Mythforge Studios. All Rights Reserved.
// CoMXeSSHelper.h — Intel XeSS upscaler init/teardown helper. COM-015-LE
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CoMXeSSHelper.generated.h"

/**
 * UCoMXeSSHelper
 *
 * Thin wrapper around the Intel XeSS UE5 plugin API (GitHub: GameTechDev/XeSS-Unreal).
 *
 * XeSS runs on all GPU vendors but performs best on Intel Arc with XMX matrix units.
 * On non-Intel GPUs it automatically uses a DP4a generic path.
 *
 * Detection via CVar probe (r.XeSS.Enabled) — no hard plugin dep at compile time.
 *
 * TODO (Sprint 4+): Wire into UCoMRHIAbstraction::SetPreferredUpscaler().
 */
UCLASS()
class COMRENDERING_API UCoMXeSSHelper : public UObject
{
    GENERATED_BODY()

public:
    /** Returns true if XeSS plugin is loaded and enabled. */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static bool IsXeSSAvailable();

    /**
     * Enable XeSS at the requested quality preset.
     * Quality: 0=UltraPerformance, 1=Performance, 2=Balanced, 3=Quality, 4=UltraQuality.
     * Returns false if XeSS unavailable.
     */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static bool TryEnable(int32 QualityPreset = 3);

    /** Disable XeSS and revert to TSR. */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static void Disable();

    /** Returns active XeSS quality preset, or -1 if not active. */
    UFUNCTION(BlueprintPure, Category="Rendering|Upscaling")
    static int32 GetActiveQualityPreset();

    /**
     * Returns true if running on Intel Arc with native XMX acceleration.
     * False = generic DP4a path (still works, slightly lower quality/perf).
     */
    UFUNCTION(BlueprintPure, Category="Rendering|Upscaling")
    static bool IsNativeXMXPath();

private:
    static bool ProbePlugin();
};
