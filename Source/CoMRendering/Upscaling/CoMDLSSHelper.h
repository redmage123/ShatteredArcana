// Copyright Mythforge Studios. All Rights Reserved.
// CoMDLSSHelper.h — DLSS upscaler init/teardown helper. COM-015-LE
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CoMDLSSHelper.generated.h"

/**
 * UCoMDLSSHelper
 *
 * Thin wrapper around the NVIDIA DLSS UE5 plugin API.
 * Requires: DLSS plugin enabled in .uproject (com.nvidia.dlss / StreamlineCore).
 *
 * Lifecycle:
 *   - Call TryEnable() once after GEngine is ready (called by UCoMRHIAbstraction).
 *   - Call Disable() on app exit or when switching to another upscaler.
 *
 * Plugin availability is detected at runtime via CVar probe so this file
 * compiles without the DLSS plugin headers — no hard link-time coupling.
 *
 * TODO (Sprint 4+): Wire into UCoMRHIAbstraction::SetPreferredUpscaler() call path.
 */
UCLASS()
class COMRENDERING_API UCoMDLSSHelper : public UObject
{
    GENERATED_BODY()

public:
    /** Returns true if DLSS plugin is loaded and DLSS 3+ is available on the GPU. */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static bool IsDLSSAvailable();

    /**
     * Enable DLSS at the requested quality preset.
     * Quality: 0=UltraPerformance, 1=Performance, 2=Balanced, 3=Quality, 4=UltraQuality.
     * Returns false if DLSS unavailable or preset index out of range.
     */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static bool TryEnable(int32 QualityPreset = 3);

    /** Disable DLSS and fall back to TSR. */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static void Disable();

    /** Returns current DLSS quality preset index, or -1 if not active. */
    UFUNCTION(BlueprintPure, Category="Rendering|Upscaling")
    static int32 GetActiveQualityPreset();

private:
    /** CVar probe: r.NGX.DLSS.Enable — exists only when DLSS plugin is loaded. */
    static bool ProbePlugin();
};
