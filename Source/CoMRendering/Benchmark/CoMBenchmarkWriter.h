// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMBenchmarkWriter.h — Serialise FBenchmarkResult to JSON.
// Handles null VRAM on UMA/OpenGL: writes vram_peak_mb as JSON null when VramPeakMb == -1.

#pragma once

#include "CoreMinimal.h"
#include "CoMBenchmarkTypes.h"

class COMRENDERING_API FCoMBenchmarkWriter
{
public:
    /**
     * Write FBenchmarkResult to the path in Result config.
     * Uses atomic write (temp file + rename) — CI never reads a partial file.
     * Returns true on success. On failure, logs error and returns false.
     */
    static bool WriteJSON(const FBenchmarkResult& Result, const FString& OutputPath);

private:
    /** Build the full JSON object tree */
    static TSharedRef<class FJsonObject> BuildJsonObject(const FBenchmarkResult& Result);

    /** Write int64 or JSON null — used for VRAM fields */
    static TSharedRef<class FJsonValue> MakeNullableInt64(int64 Value);
};
