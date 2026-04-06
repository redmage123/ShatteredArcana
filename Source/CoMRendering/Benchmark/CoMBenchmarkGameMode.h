// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMBenchmarkGameMode.h — AGameMode for L_BenchmarkBase.
// Drives the full benchmark startup sequence: scene setup → warmup → capture → write → exit.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "CoMBenchmarkTypes.h"
#include "CoMBenchmarkGameMode.generated.h"

class ABenchmarkScenarioActor;
class UCoMBenchmarkSubsystem;

UCLASS()
class COMRENDERING_API ACoMBenchmarkGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ACoMBenchmarkGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    /** Spawn the correct ABenchmarkScenarioActor for the configured scene_id */
    ABenchmarkScenarioActor* SpawnScenarioActor();

    /** Apply -benchmark-resolution if specified */
    void ApplyResolutionOverride(const FString& Resolution);

    /** Finalize, write JSON, and exit with appropriate code */
    void FinalizeBenchmark();

    UPROPERTY() TObjectPtr<ABenchmarkScenarioActor> ScenarioActor;

    UCoMBenchmarkSubsystem* BenchmarkSubsystem = nullptr;

    bool bFinalized = false;
};
