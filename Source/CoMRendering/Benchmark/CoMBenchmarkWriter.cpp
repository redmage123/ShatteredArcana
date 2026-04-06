// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMBenchmarkWriter.cpp

#include "CoMBenchmarkWriter.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

bool FCoMBenchmarkWriter::WriteJSON(const FBenchmarkResult& Result, const FString& OutputPath)
{
    // Build JSON
    TSharedRef<FJsonObject> Root = BuildJsonObject(Result);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString, 0 /*Indent*/);
    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        UE_LOG(LogTemp, Error, TEXT("[BenchmarkWriter] JSON serialisation failed"));
        return false;
    }

    // Atomic write: write to temp file then rename — prevents partial reads by CI
    const FString TempPath = OutputPath + TEXT(".tmp");
    if (!FFileHelper::SaveStringToFile(JsonString, *TempPath,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[BenchmarkWriter] Failed to write temp file: %s"), *TempPath);
        return false;
    }

    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    if (!PF.MoveFile(*OutputPath, *TempPath))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[BenchmarkWriter] Failed to rename '%s' -> '%s'"), *TempPath, *OutputPath);
        PF.DeleteFile(*TempPath);
        return false;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[BenchmarkWriter] JSON written (%d chars) -> %s"),
        JsonString.Len(), *OutputPath);
    return true;
}

TSharedRef<FJsonObject> FCoMBenchmarkWriter::BuildJsonObject(const FBenchmarkResult& Result)
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

    // ------------------------------------------------------------------
    // meta
    // ------------------------------------------------------------------
    TSharedRef<FJsonObject> Meta = MakeShared<FJsonObject>();
    Meta->SetStringField(TEXT("scene_id"),               Result.SceneId);
    Meta->SetStringField(TEXT("tier"),                   Result.Tier);
    Meta->SetStringField(TEXT("resolution"),             Result.Resolution);
    Meta->SetStringField(TEXT("upscaling"),              Result.Upscaling);
    Meta->SetStringField(TEXT("api"),                    Result.Api);
    Meta->SetStringField(TEXT("build_version"),          Result.BuildVersion);
    Meta->SetStringField(TEXT("commit_sha"),             Result.CommitSha);
    Meta->SetStringField(TEXT("run_timestamp_utc"),      Result.RunTimestampUtc);
    Meta->SetStringField(TEXT("benchmark_suite_version"),Result.BenchmarkSuiteVersion);
    Meta->SetStringField(TEXT("host_gpu"),               Result.HostGpu);
    Meta->SetStringField(TEXT("host_cpu"),               Result.HostCpu);
    Meta->SetNumberField(TEXT("host_ram_gb"),            Result.HostRamGb);
    Meta->SetStringField(TEXT("host_driver"),            Result.HostDriver);
    Meta->SetStringField(TEXT("host_os"),                Result.HostOs);
    Root->SetObjectField(TEXT("meta"), Meta);

    // ------------------------------------------------------------------
    // frametimes_ms — raw array
    // ------------------------------------------------------------------
    TArray<TSharedPtr<FJsonValue>> FtArray;
    FtArray.Reserve(Result.FrametimesMs.Num());
    for (float Ft : Result.FrametimesMs)
    {
        FtArray.Add(MakeShared<FJsonValueNumber>(Ft));
    }
    Root->SetArrayField(TEXT("frametimes_ms"), FtArray);

    // ------------------------------------------------------------------
    // summary
    // ------------------------------------------------------------------
    TSharedRef<FJsonObject> Summary = MakeShared<FJsonObject>();
    Summary->SetNumberField(TEXT("duration_seconds"),  Result.DurationSeconds);
    Summary->SetNumberField(TEXT("frame_count"),       Result.FrameCount);
    Summary->SetNumberField(TEXT("avg_fps"),           Result.AvgFps);
    Summary->SetNumberField(TEXT("avg_frametime_ms"),  Result.AvgFrametimeMs);
    Summary->SetNumberField(TEXT("p50_frametime_ms"),  Result.P50FrametimeMs);
    Summary->SetNumberField(TEXT("p95_frametime_ms"),  Result.P95FrametimeMs);
    Summary->SetNumberField(TEXT("p99_frametime_ms"),  Result.P99FrametimeMs);
    Summary->SetNumberField(TEXT("min_frametime_ms"),  Result.MinFrametimeMs);
    Summary->SetNumberField(TEXT("max_frametime_ms"),  Result.MaxFrametimeMs);

    TArray<TSharedPtr<FJsonValue>> HitchArray;
    for (const FBenchmarkHitch& H : Result.Hitches)
    {
        TSharedRef<FJsonObject> HObj = MakeShared<FJsonObject>();
        HObj->SetNumberField(TEXT("frame_index"),     H.FrameIndex);
        HObj->SetNumberField(TEXT("frametime_ms"),    H.FrametimeMs);
        HObj->SetNumberField(TEXT("elapsed_seconds"), H.ElapsedSeconds);
        HitchArray.Add(MakeShared<FJsonValueObject>(HObj));
    }
    Summary->SetArrayField(TEXT("hitches"), HitchArray);
    Root->SetObjectField(TEXT("summary"), Summary);

    // ------------------------------------------------------------------
    // gpu — Sprint 1: stubs noted in JSON for CI awareness
    // ------------------------------------------------------------------
    TSharedRef<FJsonObject> Gpu = MakeShared<FJsonObject>();
    Gpu->SetNumberField(TEXT("frame_time_avg_ms"), Result.GpuFrameTimeAvgMs);
    Gpu->SetNumberField(TEXT("frame_time_p95_ms"), Result.GpuFrameTimeP95Ms);
    // VRAM: write null when unavailable (UMA/OpenGL/stub)
    Gpu->SetField(TEXT("vram_peak_mb"), MakeNullableInt64(Result.VramPeakMb));
    Gpu->SetField(TEXT("vram_avg_mb"),  MakeNullableInt64(Result.VramAvgMb));
    Gpu->SetNumberField(TEXT("draw_calls_avg"),          Result.DrawCallsAvg);
    Gpu->SetNumberField(TEXT("draw_calls_peak"),         Result.DrawCallsPeak);
    Gpu->SetNumberField(TEXT("triangle_count_avg_millions"), Result.TriangleCountAvgM);
    Gpu->SetStringField(TEXT("note"), TEXT("stub_sprint1")); // removed in Sprint 2
    Root->SetObjectField(TEXT("gpu"), Gpu);

    // ------------------------------------------------------------------
    // cpu
    // ------------------------------------------------------------------
    TSharedRef<FJsonObject> Cpu = MakeShared<FJsonObject>();
    Cpu->SetNumberField(TEXT("game_thread_avg_ms"),    Result.GameThreadAvgMs);
    Cpu->SetNumberField(TEXT("game_thread_p95_ms"),    Result.GameThreadP95Ms);
    Cpu->SetNumberField(TEXT("render_thread_avg_ms"),  Result.RenderThreadAvgMs);
    Cpu->SetNumberField(TEXT("render_thread_p95_ms"),  Result.RenderThreadP95Ms);
    Root->SetObjectField(TEXT("cpu"), Cpu);

    // ------------------------------------------------------------------
    // streaming
    // ------------------------------------------------------------------
    TSharedRef<FJsonObject> Streaming = MakeShared<FJsonObject>();
    Streaming->SetNumberField(TEXT("hitches_over_100ms"),      Result.HitchesOver100Ms);
    Streaming->SetNumberField(TEXT("hitches_over_200ms"),      Result.HitchesOver200Ms);
    Streaming->SetNumberField(TEXT("total_streaming_stall_ms"),Result.TotalStreamingStallMs);
    Root->SetObjectField(TEXT("streaming"), Streaming);

    // ------------------------------------------------------------------
    // pass_fail
    // ------------------------------------------------------------------
    TSharedRef<FJsonObject> PassFail = MakeShared<FJsonObject>();
    PassFail->SetStringField(TEXT("overall"), Result.Overall);
    TArray<TSharedPtr<FJsonValue>> ViolationArray;
    for (const FBenchmarkViolation& V : Result.Violations)
    {
        ViolationArray.Add(MakeShared<FJsonValueString>(V.Description));
    }
    PassFail->SetArrayField(TEXT("violations"), ViolationArray);
    Root->SetObjectField(TEXT("pass_fail"), PassFail);

    return Root;
}

TSharedRef<FJsonValue> FCoMBenchmarkWriter::MakeNullableInt64(int64 Value)
{
    if (Value < 0)
    {
        return MakeShared<FJsonValueNull>();
    }
    return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
}
