// Copyright Mythforge Studios 2026. All Rights Reserved.
// BenchmarkScenarioActor.h — Abstract base class for all benchmark scenario actors.
// Sprint 1: skeleton with stub SetupScene(). Concrete subclasses ship Sprint 2.
// QA-owned UBenchmarkScenarioData DataAsset is defined here; instances authored in UE5 editor.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoMRendering/Benchmark/CoMBenchmarkTypes.h"
#include "Engine/DataAsset.h"
#include "BenchmarkScenarioActor.generated.h"

// Forward declarations
class UCoMSaveSubsystem;

// ---------------------------------------------------------------------------
// Spawn config types — used inside UBenchmarkScenarioData
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct COMRENDERING_API FBenchmarkUnitSpawn
{
    GENERATED_BODY()

    /** Race tag, e.g. "Race.HighMen" */
    UPROPERTY(EditDefaultsOnly) FName RaceTag;

    /** Wizard owner slot (0–7) */
    UPROPERTY(EditDefaultsOnly) int32 OwnerSlot = 0;

    /** Spawn tile position */
    UPROPERTY(EditDefaultsOnly) FIntPoint TilePosition = FIntPoint(0, 0);

    /** Stack size */
    UPROPERTY(EditDefaultsOnly) int32 StackSize = 1;
};

USTRUCT(BlueprintType)
struct COMRENDERING_API FBenchmarkPortalConfig
{
    GENERATED_BODY()

    /** Source plane index (0–7) */
    UPROPERTY(EditDefaultsOnly) int32 SourcePlane = 0;

    /** Destination plane index */
    UPROPERTY(EditDefaultsOnly) int32 DestPlane = 1;

    /** Portal tile position */
    UPROPERTY(EditDefaultsOnly) FIntPoint TilePosition = FIntPoint(0, 0);
};

USTRUCT(BlueprintType)
struct COMRENDERING_API FBenchmarkCameraConfig
{
    GENERATED_BODY()

    /** Starting tile the camera focuses on */
    UPROPERTY(EditDefaultsOnly) FIntPoint FocusTile = FIntPoint(64, 64);

    /** Zoom level (0 = min zoom, 1 = max zoom) */
    UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ZoomNormalized = 0.5f;
};

// ---------------------------------------------------------------------------
// UBenchmarkScenarioData — QA-owned DataAsset
// Engineering authors the class; QA authors instances in UE5 editor.
// ---------------------------------------------------------------------------

UCLASS(BlueprintType)
class COMRENDERING_API UBenchmarkScenarioData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** Must match the -benchmark-scene scene_id value */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FString SceneId;

    /**
     * Fixed deterministic seed for FCoMDeterministicRandom.
     * Changing this value invalidates all baselines for this scene.
     * Requires QA Lead sign-off before any seed change.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) int64 Seed = 0;

    /** Units to spawn — order is deterministic (sorted by unit entry order in array) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TArray<FBenchmarkUnitSpawn> Units;

    /** Weather preset applied at scene start */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) uint8 WeatherPreset = 0;

    /** Portal connections to create */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TArray<FBenchmarkPortalConfig> Portals;

    /** Camera starting state */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FBenchmarkCameraConfig Camera;
};

// ---------------------------------------------------------------------------
// ABenchmarkScenarioActor — abstract base
// ---------------------------------------------------------------------------

UCLASS(Abstract)
class COMRENDERING_API ABenchmarkScenarioActor : public AActor
{
    GENERATED_BODY()

public:
    ABenchmarkScenarioActor();

    /**
     * Called synchronously by ACoMBenchmarkGameMode::BeginPlay() before warmup.
     * Must complete before returning — no latent actions, no async.
     * Subclasses must call Super::SetupScene() first to run common assertions.
     * Sprint 1: base implementation is a no-op stub.
     * Sprint 2: base implementation runs AssertNoSaveState() + seeds RNG.
     */
    virtual void SetupScene(const FBenchmarkConfig& Config);

    /** DataAsset reference — set by level placement or factory in GameMode */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<UBenchmarkScenarioData> ScenarioData;

protected:
    /**
     * Assert no save game is loaded — crashes loudly (check(false)) if
     * UCoMSaveSubsystem has an active save. Prevents contaminated baseline results.
     * Sprint 2: wires up to UCoMSaveSubsystem.
     */
    void AssertNoSaveState() const;

    /** Spawn all units from ScenarioData->Units in deterministic order */
    void SpawnUnitsFromConfig();

    /** Apply ScenarioData->WeatherPreset */
    void ApplyWeatherPreset();

    /** Create portals from ScenarioData->Portals */
    void SpawnPortals();
};
