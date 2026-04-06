// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMBenchmarkSubsystem.cpp

#include "CoMBenchmarkSubsystem.h"
#include "CoMBenchmarkCommandLine.h"
#include "Engine/Engine.h"
#include "Misc/App.h"
#include "HAL/PlatformMisc.h"
#include "Scalability.h"

void UCoMBenchmarkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bBenchmarkMode = FCoMBenchmarkCommandLine::ParseFromCommandLine(Config);
    if (!bBenchmarkMode)
    {
        return;
    }

    // Pre-allocate arrays to reduce hitching during capture
    // Estimate: duration * 120 fps (generous upper bound)
    const int32 EstFrames = FMath::CeilToInt(Config.DurationSeconds * 120.0f);
    FrametimesMs.Reserve(EstFrames);
    GameThreadTimesMs.Reserve(EstFrames);
    RenderThreadTimesMs.Reserve(EstFrames); // Sprint 2: populated
    GpuFrameTimesMs.Reserve(EstFrames);     // Sprint 2: populated
    DrawCallsPerFrame.Reserve(EstFrames);   // Sprint 2: populated
    VramSamplesKb.Reserve(
        FMath::CeilToInt(Config.DurationSeconds / VramSampleIntervalSeconds) + 4);

    ApplyTierPreset();

    UE_LOG(LogTemp, Log,
        TEXT("[BenchmarkHarness] Initialized. Scene=%s Tier=%s Duration=%.0fs Warmup=%.0fs"),
        *Config.SceneId,
        *UEnum::GetValueAsString(Config.Tier),
        Config.DurationSeconds,
        Config.WarmupSeconds);
}

void UCoMBenchmarkSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UCoMBenchmarkSubsystem::BeginWarmup()
{
    WarmupElapsed = 0.0f;
    State = EHarnessState::Warmup;
    UE_LOG(LogTemp, Log, TEXT("[BenchmarkHarness] Warmup started (%.0fs)"), Config.WarmupSeconds);
}

void UCoMBenchmarkSubsystem::BeginCapture()
{
    // Streaming flush sequence (Sprint 1: partial — full GPU drain in Sprint 2)
    // IStreamingManager::Get() is always valid (not a pointer)
    IStreamingManager::Get().StreamAllResources(0.0f);
    FlushAsyncLoading();
    FlushRenderingCommands();
    if (GEngine)
    {
        GEngine->ForceGarbageCollection(/*bForcePurge=*/true);
    }
    FlushRenderingCommands();

    CaptureElapsed = 0.0f;
    State = EHarnessState::Capturing;
    UE_LOG(LogTemp, Log, TEXT("[BenchmarkHarness] Capture started (%.0fs)"), Config.DurationSeconds);
}

void UCoMBenchmarkSubsystem::Tick(float DeltaTime)
{
    switch (State)
    {
        case EHarnessState::Warmup:
        {
            WarmupElapsed += DeltaTime;
            if (WarmupElapsed >= Config.WarmupSeconds)
            {
                BeginCapture();
            }
            break;
        }
        case EHarnessState::Capturing:
        {
            CaptureFrame(DeltaTime);
            break;
        }
        default:
            break;
    }
}

void UCoMBenchmarkSubsystem::CaptureFrame(float DeltaSeconds)
{
    // Wall-clock frametime from DeltaTime
    FrametimesMs.Add(DeltaSeconds * 1000.0f);

    // Game thread time — UE5 global updated each frame (ms)
    GameThreadTimesMs.Add(GGameThreadTime);

    // Sprint 1 stubs — will be wired to real sources in Sprint 2
    RenderThreadTimesMs.Add(0.0f);
    GpuFrameTimesMs.Add(0.0f);
    DrawCallsPerFrame.Add(0);

    // VRAM sampled at reduced frequency to limit query overhead
    VramSampleTimer += DeltaSeconds;
    if (VramSampleTimer >= VramSampleIntervalSeconds)
    {
        VramSampleTimer = 0.0f;
        SampleVram();
    }

    CaptureElapsed += DeltaSeconds;
    if (CaptureElapsed >= Config.DurationSeconds)
    {
        State = EHarnessState::Done;
        UE_LOG(LogTemp, Log,
            TEXT("[BenchmarkHarness] Capture complete. %d frames in %.2fs."),
            FrametimesMs.Num(), CaptureElapsed);
    }
}

void UCoMBenchmarkSubsystem::SampleVram()
{
    // Sprint 1: Sprint 2 will use RHIGetTextureMemoryStats() with DedicatedVideoMemory check
    // For now mark as unavailable (stub) — matches Sprint 1 JSON contract
    bVramUnavailable = true;
}

bool UCoMBenchmarkSubsystem::FinaliseResults(FBenchmarkResult& OutResult) const
{
    if (FrametimesMs.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[BenchmarkHarness] FinaliseResults: no frames captured"));
        return false;
    }

    // --- meta ---
    OutResult.SceneId              = Config.SceneId;
    OutResult.Tier                 = UEnum::GetValueAsString(Config.Tier);
    OutResult.Resolution           = Config.Resolution.IsEmpty() ? TEXT("native") : Config.Resolution;
    OutResult.Upscaling            = UEnum::GetValueAsString(Config.Upscaling);
    OutResult.Api                  = GDynamicRHI ? GDynamicRHI->GetName() : TEXT("unknown");
    OutResult.BuildVersion         = FApp::GetBuildVersion();
#ifdef COM_COMMIT_SHA
    OutResult.CommitSha            = TEXT(COM_COMMIT_SHA);
#else
    OutResult.CommitSha            = TEXT("dev-build");   // no -DCOM_COMMIT_SHA injected
#endif
    OutResult.RunTimestampUtc      = FDateTime::UtcNow().ToString(TEXT("%Y-%m-%dT%H:%M:%SZ"));
    OutResult.BenchmarkSuiteVersion= TEXT("1.0");
    OutResult.HostGpu              = FPlatformMisc::GetPrimaryGPUBrand();
    OutResult.HostCpu              = FPlatformMisc::GetCPUBrand();
    OutResult.HostRamGb            = static_cast<int32>(
                                         FPlatformMemory::GetPhysicalGBRam());
    OutResult.HostVramMb           = bVramUnavailable ? -1 :
                                         (VramSamplesKb.IsEmpty() ? -1 :
                                          [&]() -> int64 { int64 m = (VramSamplesKb)[0]; for (auto v : (VramSamplesKb)) if (v > m) m = v; return m; }() / 1024LL);
    OutResult.HostOs               = FPlatformMisc::GetOSVersion();

    // --- frametimes_ms ---
    OutResult.FrametimesMs = FrametimesMs;

    // --- summary ---
    OutResult.DurationSeconds = CaptureElapsed;
    OutResult.FrameCount      = FrametimesMs.Num();
    OutResult.AvgFrametimeMs  = FrametimesMs.Num() > 0
        ? Algo::Accumulate(FrametimesMs, 0.0f) / FrametimesMs.Num()
        : 0.0f;
    OutResult.AvgFps          = OutResult.AvgFrametimeMs > 0.0f
        ? 1000.0f / OutResult.AvgFrametimeMs
        : 0.0f;

    // Sorted copy for percentile calculations
    TArray<float> Sorted = FrametimesMs;
    Sorted.Sort();

    OutResult.MinFrametimeMs = Sorted[0];
    OutResult.MaxFrametimeMs = Sorted.Last();
    OutResult.P50FrametimeMs = CalculatePercentile(Sorted, 50.0f);
    OutResult.P95FrametimeMs = CalculatePercentile(Sorted, 95.0f);
    OutResult.P99FrametimeMs = CalculatePercentile(Sorted, 99.0f);

    // Hitches: frametime > 3× P50 AND > 50ms
    const float P50 = OutResult.P50FrametimeMs;
    float TotalStallMs = 0.0f;
    for (int32 i = 0; i < FrametimesMs.Num(); ++i)
    {
        const float Ft = FrametimesMs[i];
        if (Ft > 3.0f * P50 && Ft > 50.0f)
        {
            FBenchmarkHitch Hitch;
            Hitch.FrameIndex    = i;
            Hitch.FrametimeMs   = Ft;
            Hitch.ElapsedSeconds= static_cast<float>(i) * OutResult.AvgFrametimeMs / 1000.0f;
            OutResult.Hitches.Add(Hitch);
            TotalStallMs += Ft;
        }
        if (Ft > 100.0f) OutResult.HitchesOver100Ms++;
        if (Ft > 200.0f) OutResult.HitchesOver200Ms++;
    }
    OutResult.TotalStreamingStallMs = TotalStallMs;

    // --- gpu (Sprint 1 stubs) ---
    OutResult.GpuFrameTimeAvgMs  = 0.0f;
    OutResult.GpuFrameTimeP95Ms  = 0.0f;
    OutResult.VramPeakMb         = bVramUnavailable ? -1 :
                                       (VramSamplesKb.IsEmpty() ? -1 :
                                        [&]() -> int64 { int64 m = (VramSamplesKb)[0]; for (auto v : (VramSamplesKb)) if (v > m) m = v; return m; }() / 1024LL);
    OutResult.VramAvgMb          = -1;
    OutResult.DrawCallsAvg       = 0.0f;
    OutResult.DrawCallsPeak      = 0;
    OutResult.TriangleCountAvgM  = 0.0f;

    // --- cpu (game thread real, render thread stub) ---
    if (!GameThreadTimesMs.IsEmpty())
    {
        OutResult.GameThreadAvgMs = Algo::Accumulate(GameThreadTimesMs, 0.0f)
                                    / GameThreadTimesMs.Num();
        TArray<float> GtSorted = GameThreadTimesMs;
        GtSorted.Sort();
        OutResult.GameThreadP95Ms = CalculatePercentile(GtSorted, 95.0f);
    }
    OutResult.RenderThreadAvgMs  = 0.0f; // Sprint 2
    OutResult.RenderThreadP95Ms  = 0.0f; // Sprint 2

    // --- pass_fail (frametime thresholds only in Sprint 1) ---
    // Embedded thresholds are conservative — authoritative baselines live in
    // engineering/testing/baselines/. CI must always compare against baselines.
    OutResult.Overall = TEXT("PASS");
    // Sprint 1: no embedded threshold enforcement (Sprint 2 adds exit code 2 logic)

    return true;
}

TStatId UCoMBenchmarkSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCoMBenchmarkSubsystem, STATGROUP_Tickables);
}

float UCoMBenchmarkSubsystem::CalculatePercentile(TArray<float> Sorted, float Pct)
{
    // Sorted must be passed pre-sorted ascending
    if (Sorted.IsEmpty()) return 0.0f;
    const int32 Idx = FMath::Clamp(
        FMath::FloorToInt((Pct / 100.0f) * static_cast<float>(Sorted.Num())),
        0, Sorted.Num() - 1);
    return Sorted[Idx];
}

void UCoMBenchmarkSubsystem::ApplyTierPreset()
{
    Scalability::FQualityLevels Levels;
    switch (Config.Tier)
    {
        case EBenchmarkTier::Minimum:
            Levels.SetFromSingleQualityLevel(0); break;
        case EBenchmarkTier::Recommended:
            Levels.SetFromSingleQualityLevel(2); break;
        case EBenchmarkTier::High:
            Levels.SetFromSingleQualityLevel(3); break;
        case EBenchmarkTier::Cinematic:
            Levels.SetFromSingleQualityLevel(4); break;
        default:
            Levels.SetFromSingleQualityLevel(2); break;
    }
    Scalability::SetQualityLevels(Levels);
    Scalability::SaveState(GGameUserSettingsIni);

    UE_LOG(LogTemp, Log, TEXT("[BenchmarkHarness] Applied tier preset: %s"),
        *UEnum::GetValueAsString(Config.Tier));
}
