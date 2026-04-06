// Copyright Mythforge Studios. All Rights Reserved.
// CoMFSRHelper.h — AMD FSR 2/3 upscaler init/teardown helper. COM-015-LE
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CoMFSRHelper.generated.h"

/**
 * UCoMFSRHelper
 *
 * Thin wrapper around the AMD FSR UE5 plugin API (GPUOpen).
 * Plugin: AMD FidelityFX Super Resolution (FSR2GamePlugin or FSR3GamePlugin).
 *
 * FSR is hardware-agnostic and available on all GPU vendors.
 * It is the primary upscaler fallback when DLSS/XeSS are unavailable.
 *
 * Detection via CVar r.FidelityFX.FSR2.Enabled (FSR2) or
 * r.FidelityFX.FSR3.Enabled (FSR3) — no hard plugin dep at compile time.
 *
 * TODO (Sprint 4+): Wire into UCoMRHIAbstraction::SetPreferredUpscaler().
 */
UCLASS()
class COMRENDERING_API UCoMFSRHelper : public UObject
{
    GENERATED_BODY()

public:
    /** Returns true if FSR plugin (2 or 3) is loaded and enabled. */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static bool IsFSRAvailable();

    /**
     * Enable FSR at the requested quality preset.
     * Quality: 0=UltraPerformance, 1=Performance, 2=Balanced, 3=Quality, 4=NativeAA.
     * Prefers FSR3 if available; falls back to FSR2.
     * Returns false if plugin unavailable.
     */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static bool TryEnable(int32 QualityPreset = 3);

    /** Disable FSR and revert to TSR. */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static void Disable();

    /** Returns active FSR quality preset, or -1 if not active. */
    UFUNCTION(BlueprintPure, Category="Rendering|Upscaling")
    static int32 GetActiveQualityPreset();

    /**
     * Returns 3 if FSR3 plugin is active, 2 if FSR2, 0 if unavailable.
     * Use this to log/display the FSR version in the options menu.
     */
    UFUNCTION(BlueprintPure, Category="Rendering|Upscaling")
    static int32 GetFSRVersion();

private:
    static bool ProbePlugin();
    static int32 DetectVersion(); // returns 3, 2, or 0
};
