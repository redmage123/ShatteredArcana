// Copyright Mythforge Studios 2026. All Rights Reserved.
// BenchmarkScenarioActor.cpp — Sprint 1 stub implementation.
// Sprint 2: flesh out with real unit/portal/weather spawn logic.

#include "BenchmarkScenarioActor.h"

ABenchmarkScenarioActor::ABenchmarkScenarioActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ABenchmarkScenarioActor::SetupScene(const FBenchmarkConfig& Config)
{
    // Sprint 1 stub — scene is empty.
    // Sprint 2 base implementation will:
    //   1. AssertNoSaveState()
    //   2. Seed FCoMDeterministicRandom from ScenarioData->Seed
    //   3. Call SpawnUnitsFromConfig(), ApplyWeatherPreset(), SpawnPortals()
    UE_LOG(LogTemp, Log,
        TEXT("[BenchmarkScenario] SetupScene stub — scene '%s' (Sprint 1, no actors spawned)"),
        *Config.SceneId);
}

void ABenchmarkScenarioActor::AssertNoSaveState() const
{
    // Sprint 2: GetGameInstance()->GetSubsystem<UCoMSaveSubsystem>()->AssertNoActiveSave()
    // Intentional check(false) crash if save state is detected — contaminated results
    // are worse than a crash.
}

void ABenchmarkScenarioActor::SpawnUnitsFromConfig()
{
    // Sprint 2: iterate ScenarioData->Units (already in deterministic insertion order)
    // For each entry: resolve race actor class from tag, spawn at TilePosition, assign OwnerSlot
}

void ABenchmarkScenarioActor::ApplyWeatherPreset()
{
    // Sprint 2: call UCoMWorldMapSubsystem->SetWeatherPreset(ScenarioData->WeatherPreset)
}

void ABenchmarkScenarioActor::SpawnPortals()
{
    // Sprint 2: iterate ScenarioData->Portals, call portal creation API per entry
}
