// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMBenchmarkCommandLine.h — Parse and validate -benchmark-* command-line args.
// On validation failure: writes to stderr + FPlatformMisc::LocalPrint, exits 1.

#pragma once

#include "CoreMinimal.h"
#include "CoMBenchmarkTypes.h"

/** Valid scene IDs — checked at parse time so invalid scenes fail fast */
static const TArray<FString> GValidBenchmarkSceneIds =
{
    TEXT("bench_aurelith_overworld"),
    TEXT("bench_portal_combat"),
    TEXT("bench_world_strategic"),
    TEXT("bench_city_view"),
    TEXT("bench_combat_16v16"),
    TEXT("bench_noctharion_night"),
    TEXT("bench_billboard_stress"),
};

class COMRENDERING_API FCoMBenchmarkCommandLine
{
public:
    /**
     * Parse -benchmark-* flags from the process command line.
     * Returns true if -benchmark was present and all required args are valid.
     * On any validation error: prints to stderr and calls FPlatformMisc::RequestExit(1).
     */
    static bool ParseFromCommandLine(FBenchmarkConfig& OutConfig);

private:
    static EBenchmarkTier      ParseTier(const FString& Value);
    static EBenchmarkUpscaling ParseUpscaling(const FString& Value);
    static EBenchmarkApi       ParseApi(const FString& Value);

    /** Print to stderr AND UE log AND FPlatformMisc::LocalPrint — visible in headless CI */
    static void ReportErrorAndExit(const FString& Message);

    /** Resolve output directory and verify it exists */
    static bool ValidateOutputPath(const FString& Path);

    /** Validate WxH resolution string, e.g. "1920x1080" */
    static bool ValidateResolution(const FString& Value);
};
