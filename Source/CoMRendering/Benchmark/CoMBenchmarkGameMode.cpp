// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMBenchmarkGameMode.cpp

#include "CoMBenchmarkGameMode.h"
#include "CoMBenchmarkSubsystem.h"
#include "CoMBenchmarkWriter.h"
#include "Benchmark/Scenarios/BenchmarkScenarioActor.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"

// Scenario actor forward declarations — Sprint 1 uses stub base class
// Sprint 2: include concrete headers per scene
#include "Benchmark/Scenarios/BenchmarkScenarioActor.h"

ACoMBenchmarkGameMode::ACoMBenchmarkGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ACoMBenchmarkGameMode::BeginPlay()
{
    Super::BeginPlay();

    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    BenchmarkSubsystem = GI->GetSubsystem<UCoMBenchmarkSubsystem>();
    if (!BenchmarkSubsystem || !BenchmarkSubsystem->IsBenchmarkMode())
    {
        // Not in benchmark mode — this GameMode should only be used on L_BenchmarkBase
        UE_LOG(LogTemp, Warning,
            TEXT("[BenchmarkGameMode] BeginPlay in non-benchmark mode — skipping harness setup"));
        return;
    }

    const FBenchmarkConfig& Config = BenchmarkSubsystem->GetConfig();

    // Suppress cursor + all interactive input
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }

    // Suppress map warning dialogs in benchmark mode
    if (GEngine)
    {
        GEngine->bSuppressMapWarnings = true;
    }

    // Apply resolution override if requested
    if (!Config.Resolution.IsEmpty())
    {
        ApplyResolutionOverride(Config.Resolution);
    }

    // Spawn scenario actor for the configured scene_id
    ScenarioActor = SpawnScenarioActor();
    if (!ScenarioActor)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[BenchmarkGameMode] Failed to spawn scenario actor for scene '%s'"),
            *Config.SceneId);
        FPlatformMisc::RequestExit(/*bForce=*/true);
        return;
    }

    // Run synchronous scene setup before warmup begins
    ScenarioActor->SetupScene(Config);

    // Hand off to subsystem — begins warmup countdown
    BenchmarkSubsystem->BeginWarmup();
}

void ACoMBenchmarkGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!BenchmarkSubsystem || !BenchmarkSubsystem->IsBenchmarkMode() || bFinalized)
    {
        return;
    }

    // Check if capture is complete (subsystem transitions to Done state internally)
    // We detect completion by asking if capture elapsed >= configured duration
    // The subsystem's Tick handles the state machine; we just watch for Done
    const FBenchmarkConfig& Config = BenchmarkSubsystem->GetConfig();
    (void)Config; // used below in FinalizeBenchmark

    // Subsystem will have stopped calling CaptureFrame once Done — we check via
    // a finalise attempt each frame (idempotent — only writes once)
    FinalizeBenchmark();
}

void ACoMBenchmarkGameMode::FinalizeBenchmark()
{
    if (bFinalized) return;

    // Only finalise once capture is done — check via attempting FinaliseResults
    FBenchmarkResult Result;
    if (!BenchmarkSubsystem->FinaliseResults(Result))
    {
        return; // Not done yet or nothing captured
    }

    bFinalized = true;

    const FString OutputPath = BenchmarkSubsystem->GetConfig().OutputPath;
    const bool bWriteOk = FCoMBenchmarkWriter::WriteJSON(Result, OutputPath);

    if (!bWriteOk)
    {
        UE_LOG(LogTemp, Error, TEXT("[BenchmarkGameMode] JSON write failed — exiting 1"));
        FPlatformMisc::RequestExit(/*bForce=*/true);
        return;
    }

    // Exit 0: harness completed cleanly. CI reads pass_fail.overall from JSON.
    // Exit 2 (threshold failure) is reserved for Sprint 2 when embedded thresholds are wired.
    UE_LOG(LogTemp, Log,
        TEXT("[BenchmarkGameMode] Complete. overall=%s — exiting 0"),
        *Result.Overall);

    FPlatformMisc::RequestExit(/*bForce=*/false);
}

ABenchmarkScenarioActor* ACoMBenchmarkGameMode::SpawnScenarioActor()
{
    const FString& SceneId = BenchmarkSubsystem->GetConfig().SceneId;

    // Sprint 1: all scenes use the stub base actor
    // Sprint 2: replace with factory / per-scene class lookup
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ABenchmarkScenarioActor* Actor = GetWorld()->SpawnActor<ABenchmarkScenarioActor>(
        ABenchmarkScenarioActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

    if (Actor)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[BenchmarkGameMode] Spawned scenario actor for scene '%s'"), *SceneId);
    }
    return Actor;
}

void ACoMBenchmarkGameMode::ApplyResolutionOverride(const FString& Resolution)
{
    // Parse "WxH"
    int32 XPos;
    if (!Resolution.FindChar(TEXT('x'), XPos)) return;
    const int32 W = FCString::Atoi(*Resolution.Left(XPos));
    const int32 H = FCString::Atoi(*Resolution.Mid(XPos + 1));
    if (W <= 0 || H <= 0) return;

    if (GEngine && GEngine->GameViewport)
    {
        FSystemResolution::RequestResolutionChange(W, H, EWindowMode::Fullscreen);
        UE_LOG(LogTemp, Log,
            TEXT("[BenchmarkGameMode] Resolution override applied: %dx%d"), W, H);
    }
}
