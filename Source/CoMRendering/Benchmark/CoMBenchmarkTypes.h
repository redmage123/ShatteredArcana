// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMBenchmarkTypes.h — FBenchmarkConfig, FBenchmarkResult, FBenchmarkHitch
// Sprint 1: full struct definitions. GPU stat fields are zero-stubbed at runtime.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoMBenchmarkTypes.generated.h"

/** Graphics API selection override */
UENUM(BlueprintType)
enum class EBenchmarkApi : uint8
{
    Auto     UMETA(DisplayName = "Auto"),
    DX12     UMETA(DisplayName = "DX12"),
    OpenGL   UMETA(DisplayName = "OpenGL"),
    Vulkan   UMETA(DisplayName = "Vulkan"),
    Metal    UMETA(DisplayName = "Metal"),
    NullRHI  UMETA(DisplayName = "NullRHI"),  // CI smoke only
};

/** Quality tier preset — maps to Scalability::FQualityLevels */
UENUM(BlueprintType)
enum class EBenchmarkTier : uint8
{
    Minimum     UMETA(DisplayName = "min"),
    Recommended UMETA(DisplayName = "rec"),
    High        UMETA(DisplayName = "high"),
    Cinematic   UMETA(DisplayName = "cin"),
};

/** Upscaling technology selection */
UENUM(BlueprintType)
enum class EBenchmarkUpscaling : uint8
{
    None UMETA(DisplayName = "none"),
    TSR  UMETA(DisplayName = "tsr"),
    DLSS UMETA(DisplayName = "dlss"),
    FSR  UMETA(DisplayName = "fsr"),
    XeSS UMETA(DisplayName = "xess"),
};

// ---------------------------------------------------------------------------
// Parsed + validated command-line config
// ---------------------------------------------------------------------------

USTRUCT()
struct COMRENDERING_API FBenchmarkConfig
{
    GENERATED_BODY()

    /** -benchmark-scene value, e.g. "bench_aurelith_overworld" */
    UPROPERTY() FString SceneId;

    /** -benchmark-tier */
    UPROPERTY() EBenchmarkTier Tier = EBenchmarkTier::Recommended;

    /** -benchmark-duration (seconds). Default 120. */
    UPROPERTY() float DurationSeconds = 120.0f;

    /** -benchmark-warmup (seconds). Default 10. */
    UPROPERTY() float WarmupSeconds = 10.0f;

    /** -benchmark-out path. Required. */
    UPROPERTY() FString OutputPath;

    /** -benchmark-resolution, e.g. "1920x1080". Empty = native. */
    UPROPERTY() FString Resolution;

    /** -benchmark-upscaling */
    UPROPERTY() EBenchmarkUpscaling Upscaling = EBenchmarkUpscaling::None;

    /** -benchmark-api */
    UPROPERTY() EBenchmarkApi Api = EBenchmarkApi::Auto;

    /** True if -benchmark flag was present */
    UPROPERTY() bool bBenchmarkMode = false;
};

// ---------------------------------------------------------------------------
// Per-frame hitch record
// ---------------------------------------------------------------------------

USTRUCT()
struct COMRENDERING_API FBenchmarkHitch
{
    GENERATED_BODY()

    /** Frame index within the capture window */
    UPROPERTY() int32 FrameIndex = 0;

    /** Frame time in ms */
    UPROPERTY() float FrametimeMs = 0.0f;

    /** Elapsed seconds since capture start */
    UPROPERTY() float ElapsedSeconds = 0.0f;
};

// ---------------------------------------------------------------------------
// Pass/fail threshold violation
// ---------------------------------------------------------------------------

USTRUCT()
struct COMRENDERING_API FBenchmarkViolation
{
    GENERATED_BODY()

    /** Human-readable description, e.g. "p95_frametime_ms: 28.4 > 25.0" */
    UPROPERTY() FString Description;
};

// ---------------------------------------------------------------------------
// Full result — computed by FinaliseResults()
// ---------------------------------------------------------------------------

USTRUCT()
struct COMRENDERING_API FBenchmarkResult
{
    GENERATED_BODY()

    // ------------------------------------------------------------------
    // meta — populated from FBenchmarkConfig + FPlatformMisc at runtime
    // ------------------------------------------------------------------
    UPROPERTY() FString SceneId;
    UPROPERTY() FString Tier;               // "min" | "rec" | "high" | "cin"
    UPROPERTY() FString Resolution;         // "1920x1080"
    UPROPERTY() FString Upscaling;          // "none" | "tsr" | ...
    UPROPERTY() FString Api;                // detected from active RHI name
    UPROPERTY() FString BuildVersion;
    UPROPERTY() FString CommitSha;
    UPROPERTY() FString RunTimestampUtc;
    UPROPERTY() FString BenchmarkSuiteVersion = TEXT("1.0");
    UPROPERTY() FString HostGpu;
    UPROPERTY() FString HostCpu;
    UPROPERTY() int32   HostRamGb   = 0;
    UPROPERTY() int64   HostVramMb  = -1;   // -1 = unavailable (UMA/OpenGL)
    UPROPERTY() FString HostDriver;
    UPROPERTY() FString HostOs;

    // ------------------------------------------------------------------
    // frametimes_ms — raw capture array (~7200 elements @ 60fps / 120s)
    // ------------------------------------------------------------------
    UPROPERTY() TArray<float> FrametimesMs;

    // ------------------------------------------------------------------
    // summary — computed from FrametimesMs
    // ------------------------------------------------------------------
    UPROPERTY() float DurationSeconds  = 0.0f;
    UPROPERTY() int32 FrameCount       = 0;
    UPROPERTY() float AvgFps           = 0.0f;
    UPROPERTY() float AvgFrametimeMs   = 0.0f;
    UPROPERTY() float P50FrametimeMs   = 0.0f;
    UPROPERTY() float P95FrametimeMs   = 0.0f;
    UPROPERTY() float P99FrametimeMs   = 0.0f;
    UPROPERTY() float MinFrametimeMs   = 0.0f;
    UPROPERTY() float MaxFrametimeMs   = 0.0f;
    UPROPERTY() TArray<FBenchmarkHitch> Hitches;

    // ------------------------------------------------------------------
    // gpu — Sprint 1: all zero-stubbed except VramPeakMb
    // Sprint 2: wired to FRHIGPUTiming + RHIGetTextureMemoryStats
    // ------------------------------------------------------------------
    UPROPERTY() float GpuFrameTimeAvgMs   = 0.0f;  // stub Sprint 1
    UPROPERTY() float GpuFrameTimeP95Ms   = 0.0f;  // stub Sprint 1
    UPROPERTY() int64 VramPeakMb          = -1;     // -1 = unavailable
    UPROPERTY() int64 VramAvgMb           = -1;
    UPROPERTY() float DrawCallsAvg        = 0.0f;   // stub Sprint 1
    UPROPERTY() int32 DrawCallsPeak       = 0;      // stub Sprint 1
    UPROPERTY() float TriangleCountAvgM   = 0.0f;   // stub Sprint 1

    // ------------------------------------------------------------------
    // cpu — game thread real in Sprint 1, render thread stub
    // ------------------------------------------------------------------
    UPROPERTY() float GameThreadAvgMs    = 0.0f;
    UPROPERTY() float GameThreadP95Ms    = 0.0f;
    UPROPERTY() float RenderThreadAvgMs  = 0.0f;   // stub Sprint 1
    UPROPERTY() float RenderThreadP95Ms  = 0.0f;   // stub Sprint 1

    // ------------------------------------------------------------------
    // streaming — hitch-based in Sprint 1; streaming API Sprint 2
    // ------------------------------------------------------------------
    UPROPERTY() int32 HitchesOver100Ms       = 0;
    UPROPERTY() int32 HitchesOver200Ms       = 0;
    UPROPERTY() float TotalStreamingStallMs  = 0.0f;

    // ------------------------------------------------------------------
    // pass_fail
    // ------------------------------------------------------------------
    UPROPERTY() FString Overall = TEXT("PASS");     // "PASS" | "FAIL"
    UPROPERTY() TArray<FBenchmarkViolation> Violations;
};
