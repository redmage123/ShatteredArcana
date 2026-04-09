// Copyright Mythforge Studios. All Rights Reserved.
// CoMOverworldRenderingActor.h -- Bridge actor that wires the overworld renderer
// into the game loop (BeginPlay -> initial render, Tick -> chunk updates).
//
// Place one instance of this actor in L_Overworld (or spawn from Blueprint).
// It avoids CoMCore depending on CoMRendering by keeping the wiring in the
// rendering module.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMOverworldRenderingActor.generated.h"

class UCoMOverworldRenderer;

/**
 * ACoMOverworldRenderingActor
 *
 * Lightweight actor placed in the overworld level that bridges the
 * UCoMOverworldRenderer subsystem into the game loop:
 *
 *   BeginPlay:  Calls RenderPlane() to build the initial tile view.
 *   Tick:       Calls UpdateVisibleChunks() with the camera's current position.
 *
 * This actor lives in CoMRendering so it can include the renderer header
 * without creating a circular module dependency (CoMCore does not include
 * CoMRendering).
 */
UCLASS(Blueprintable, BlueprintType)
class COMRENDERING_API ACoMOverworldRenderingActor : public AActor
{
	GENERATED_BODY()

public:
	ACoMOverworldRenderingActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Which plane to render on startup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overworld Rendering")
	ECoMPlane InitialPlane = ECoMPlane::Aurelith;

	/** Which layer to render on startup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overworld Rendering")
	ECoMMapLayer InitialLayer = ECoMMapLayer::Surface;

	/** Wizard index of the local human player (for fog of war). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overworld Rendering")
	int32 LocalWizardIndex = 0;

private:
	UPROPERTY()
	TObjectPtr<UCoMOverworldRenderer> CachedRenderer;
};
