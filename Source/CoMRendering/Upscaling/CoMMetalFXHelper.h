// Copyright Mythforge Studios. All Rights Reserved.
// CoMMetalFXHelper.h — Apple MetalFX upscaler helper. COM-015-LE
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CoMMetalFXHelper.generated.h"

/**
 * UCoMMetalFXHelper
 *
 * Wrapper for Apple MetalFX spatial/temporal upscaling on macOS/iOS.
 * MetalFX is built into UE5's Metal platform layer — no separate plugin download.
 * Only available at runtime on Apple platforms; all methods no-op + return false elsewhere.
 *
 * Mode:
 *   - Temporal: high quality, requires motion vectors (preferred for gameplay).
 *   - Spatial:  lower quality, no motion vectors (fallback for static screens/cutscenes).
 *
 * UCoMRHIAbstraction selects MetalFX as the first-choice upscaler on Metal backends.
 *
 * TODO (Sprint 4+): Wire into UCoMRHIAbstraction::SetPreferredUpscaler().
 * TODO: Confirm r.MetalFX.Enabled CVar name against UE5.4 Metal source.
 */
UCLASS()
class COMRENDERING_API UCoMMetalFXHelper : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Returns true if running on an Apple platform with MetalFX support.
     * False on Windows/Linux (compile-time guard via PLATFORM_MAC).
     */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static bool IsMetalFXAvailable();

    /**
     * Enable MetalFX in temporal mode (preferred).
     * Falls back to spatial mode if temporal unsupported.
     * No-op on non-Apple platforms.
     */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static bool TryEnable();

    /** Enable MetalFX in spatial mode explicitly. */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static bool TryEnableSpatial();

    /** Disable MetalFX and revert to TSR. */
    UFUNCTION(BlueprintCallable, Category="Rendering|Upscaling")
    static void Disable();

    /** Returns true if MetalFX temporal mode is currently active. */
    UFUNCTION(BlueprintPure, Category="Rendering|Upscaling")
    static bool IsTemporalActive();

private:
    static bool ProbeMetalPlatform();
};
