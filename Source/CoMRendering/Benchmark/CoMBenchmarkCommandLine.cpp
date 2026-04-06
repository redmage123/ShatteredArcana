// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMBenchmarkCommandLine.cpp

#include "CoMBenchmarkCommandLine.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"

bool FCoMBenchmarkCommandLine::ParseFromCommandLine(FBenchmarkConfig& OutConfig)
{
    // -benchmark must be present to activate harness mode
    if (!FParse::Param(FCommandLine::Get(), TEXT("benchmark")))
    {
        OutConfig.bBenchmarkMode = false;
        return false;
    }
    OutConfig.bBenchmarkMode = true;

    // --- Required: -benchmark-scene ---
    if (!FParse::Value(FCommandLine::Get(), TEXT("-benchmark-scene="), OutConfig.SceneId))
    {
        ReportErrorAndExit(TEXT("Missing required arg: -benchmark-scene=<scene_id>"));
    }
    if (!GValidBenchmarkSceneIds.Contains(OutConfig.SceneId))
    {
        ReportErrorAndExit(FString::Printf(
            TEXT("Invalid scene_id '%s'. Valid values: bench_aurelith_overworld, "
                 "bench_portal_combat, bench_world_strategic, bench_city_view, "
                 "bench_combat_16v16, bench_noctharion_night, bench_billboard_stress"),
            *OutConfig.SceneId));
    }

    // --- Required: -benchmark-out ---
    if (!FParse::Value(FCommandLine::Get(), TEXT("-benchmark-out="), OutConfig.OutputPath))
    {
        ReportErrorAndExit(TEXT("Missing required arg: -benchmark-out=<path>"));
    }
    if (!ValidateOutputPath(OutConfig.OutputPath))
    {
        ReportErrorAndExit(FString::Printf(
            TEXT("Output path directory does not exist: '%s'"),
            *FPaths::GetPath(OutConfig.OutputPath)));
    }

    // --- Required: -benchmark-tier ---
    FString TierStr;
    if (!FParse::Value(FCommandLine::Get(), TEXT("-benchmark-tier="), TierStr))
    {
        ReportErrorAndExit(TEXT("Missing required arg: -benchmark-tier=<min|rec|high|cin>"));
    }
    OutConfig.Tier = ParseTier(TierStr);

    // --- Optional: -benchmark-duration ---
    float Duration = 120.0f;
    if (FParse::Value(FCommandLine::Get(), TEXT("-benchmark-duration="), Duration))
    {
        if (Duration < 10.0f || Duration > 3600.0f)
        {
            ReportErrorAndExit(TEXT("-benchmark-duration must be between 10 and 3600 seconds"));
        }
        OutConfig.DurationSeconds = Duration;
    }

    // --- Optional: -benchmark-warmup ---
    float Warmup = 10.0f;
    FParse::Value(FCommandLine::Get(), TEXT("-benchmark-warmup="), Warmup);
    OutConfig.WarmupSeconds = FMath::Max(0.0f, Warmup);

    // --- Optional: -benchmark-resolution ---
    FParse::Value(FCommandLine::Get(), TEXT("-benchmark-resolution="), OutConfig.Resolution);
    if (!OutConfig.Resolution.IsEmpty() && !ValidateResolution(OutConfig.Resolution))
    {
        ReportErrorAndExit(FString::Printf(
            TEXT("Invalid -benchmark-resolution format '%s'. Expected WxH, e.g. 1920x1080"),
            *OutConfig.Resolution));
    }

    // --- Optional: -benchmark-upscaling ---
    FString UpscalingStr;
    if (FParse::Value(FCommandLine::Get(), TEXT("-benchmark-upscaling="), UpscalingStr))
    {
        OutConfig.Upscaling = ParseUpscaling(UpscalingStr);
    }

    // --- Optional: -benchmark-api ---
    FString ApiStr;
    if (FParse::Value(FCommandLine::Get(), TEXT("-benchmark-api="), ApiStr))
    {
        OutConfig.Api = ParseApi(ApiStr);

        // Platform compatibility check
#if PLATFORM_WINDOWS
        if (OutConfig.Api == EBenchmarkApi::Metal)
        {
            ReportErrorAndExit(TEXT("-benchmark-api=metal is not available on Windows"));
        }
#elif PLATFORM_LINUX
        if (OutConfig.Api == EBenchmarkApi::DX12 || OutConfig.Api == EBenchmarkApi::Metal)
        {
            ReportErrorAndExit(TEXT("-benchmark-api=dx12 and metal are not available on Linux"));
        }
#elif PLATFORM_MAC
        if (OutConfig.Api == EBenchmarkApi::DX12)
        {
            ReportErrorAndExit(TEXT("-benchmark-api=dx12 is not available on macOS"));
        }
#endif
    }

    return true;
}

EBenchmarkTier FCoMBenchmarkCommandLine::ParseTier(const FString& Value)
{
    if (Value == TEXT("min"))  return EBenchmarkTier::Minimum;
    if (Value == TEXT("rec"))  return EBenchmarkTier::Recommended;
    if (Value == TEXT("high")) return EBenchmarkTier::High;
    if (Value == TEXT("cin"))  return EBenchmarkTier::Cinematic;

    ReportErrorAndExit(FString::Printf(
        TEXT("Invalid -benchmark-tier '%s'. Valid values: min, rec, high, cin"), *Value));
    return EBenchmarkTier::Recommended; // unreachable — ReportErrorAndExit exits
}

EBenchmarkUpscaling FCoMBenchmarkCommandLine::ParseUpscaling(const FString& Value)
{
    if (Value == TEXT("none")) return EBenchmarkUpscaling::None;
    if (Value == TEXT("tsr"))  return EBenchmarkUpscaling::TSR;
    if (Value == TEXT("dlss")) return EBenchmarkUpscaling::DLSS;
    if (Value == TEXT("fsr"))  return EBenchmarkUpscaling::FSR;
    if (Value == TEXT("xess")) return EBenchmarkUpscaling::XeSS;

    ReportErrorAndExit(FString::Printf(
        TEXT("Invalid -benchmark-upscaling '%s'. Valid values: none, tsr, dlss, fsr, xess"), *Value));
    return EBenchmarkUpscaling::None;
}

EBenchmarkApi FCoMBenchmarkCommandLine::ParseApi(const FString& Value)
{
    if (Value == TEXT("dx12"))   return EBenchmarkApi::DX12;
    if (Value == TEXT("opengl")) return EBenchmarkApi::OpenGL;
    if (Value == TEXT("vulkan")) return EBenchmarkApi::Vulkan;
    if (Value == TEXT("metal"))  return EBenchmarkApi::Metal;
    if (Value == TEXT("nullrhi"))return EBenchmarkApi::NullRHI;

    ReportErrorAndExit(FString::Printf(
        TEXT("Invalid -benchmark-api '%s'. Valid values: dx12, opengl, vulkan, metal, nullrhi"), *Value));
    return EBenchmarkApi::Auto;
}

void FCoMBenchmarkCommandLine::ReportErrorAndExit(const FString& Message)
{
    const FString Full = FString::Printf(TEXT("[BenchmarkHarness] ERROR: %s\n"), *Message);
    // Write to all output channels so headless CI captures it
    UE_LOG(LogTemp, Error, TEXT("%s"), *Full);
    FPlatformMisc::LocalPrint(*Full);
    fprintf(stderr, "%s\n", TCHAR_TO_UTF8(*Full));
    FPlatformMisc::RequestExit(/*bForce=*/true);
}

bool FCoMBenchmarkCommandLine::ValidateOutputPath(const FString& Path)
{
    const FString Dir = FPaths::GetPath(Path);
    if (Dir.IsEmpty()) return true; // relative path with no directory — current dir is valid
    return FPaths::DirectoryExists(Dir);
}

bool FCoMBenchmarkCommandLine::ValidateResolution(const FString& Value)
{
    // Expect format "WxH" where W and H are positive integers
    int32 XPos;
    if (!Value.FindChar(TEXT('x'), XPos)) return false;
    const FString WidthStr  = Value.Left(XPos);
    const FString HeightStr = Value.Mid(XPos + 1);
    if (WidthStr.IsEmpty() || HeightStr.IsEmpty()) return false;
    const int32 W = FCString::Atoi(*WidthStr);
    const int32 H = FCString::Atoi(*HeightStr);
    return W > 0 && H > 0;
}
