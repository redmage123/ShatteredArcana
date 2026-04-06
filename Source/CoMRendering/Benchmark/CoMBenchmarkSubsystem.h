// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMBenchmarkSubsystem.h — UGameInstanceSubsystem that accumulates per-frame stats.
// Sprint 1: game thread frametimes real; GPU/render thread stats zero-stubbed.
// Sprint 2: wire up FRHIGPUTiming, GRenderThreadTime, RHIGetTextureMemoryStats.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CoMBenchmarkTypes.h"
#include "CoMBenchmarkSubsystem.generated.h"

UCLASS()
class COMRENDERING_API UCoMBenchmarkSubsystem
    : public UGameInstanceSubsystem
    , public FTickableGameObject
{
    GENERATED_BODY()

public:
    // UGameInstanceSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // FTickableGameObject
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return bBenchmarkMode; }

    // ------------------------------------------------------------------
    // State machine — called by ABenchmarkHarnessActor
    // ------------------------------------------------------------------

    /** Begin warmup countdown. Metrics are discarded during warmup. */
    void BeginWarmup();

    /**
     * Flush streaming and begin metric capture.
     * Called by ABenchmarkHarnessActor after warmup expires.
     */
    void BeginCapture();

    /**
     * End capture and compute final result struct.
     * Returns false if no frames were captured.
     */
    bool FinaliseResults(FBenchmarkResult& OutResult) const;

    /** True after ParseFromCommandLine succeeds with -benchmark flag */
    bool IsBenchmarkMode() const { return bBenchmarkMode; }

    /** Parsed config — available after Initialize() */
    const FBenchmarkConfig& GetConfig() const { return Config; }

private:
    // ------------------------------------------------------------------
    // Internal
    // ------------------------------------------------------------------

    /** Called each frame during capture window */
    void CaptureFrame(float DeltaSeconds);

    /** VRAM sample — called every VramSampleIntervalSeconds during capture */
    void SampleVram();

    /** Apply the scalability preset from Config.Tier */
    void ApplyTierPreset();

    static float CalculatePercentile(TArray<float> Sorted, float Pct);

    // ------------------------------------------------------------------
    // Config
    // ------------------------------------------------------------------
    FBenchmarkConfig Config;
    bool bBenchmarkMode = false;

    // ------------------------------------------------------------------
    // Capture state
    // ------------------------------------------------------------------
    enum class EHarnessState { Idle, Warmup, Capturing, Done };
    EHarnessState State = EHarnessState::Idle;

    float WarmupElapsed   = 0.0f;
    float CaptureElapsed  = 0.0f;

    // Raw per-frame arrays — pre-allocated to expected frame count
    TArray<float> FrametimesMs;
    TArray<float> GameThreadTimesMs;
    TArray<float> RenderThreadTimesMs; // Sprint 2: filled from GRenderThreadTime
    TArray<float> GpuFrameTimesMs;     // Sprint 2: filled from FRHIGPUTiming
    TArray<int32> DrawCallsPerFrame;   // Sprint 2: filled from GNumDrawCallsRHI
    TArray<int64> VramSamplesKb;       // sampled every VramSampleIntervalSeconds

    // VRAM
    bool  bVramUnavailable = false;
    float VramSampleTimer  = 0.0f;
    static constexpr float VramSampleIntervalSeconds = 2.0f;
};
