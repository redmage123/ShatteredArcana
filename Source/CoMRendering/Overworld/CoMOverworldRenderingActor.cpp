// Copyright Mythforge Studios. All Rights Reserved.
// CoMOverworldRenderingActor.cpp -- Wires the renderer into BeginPlay/Tick.

#include "Overworld/CoMOverworldRenderingActor.h"
#include "Overworld/CoMOverworldRenderer.h"
#include "CoMCore/Camera/CoMCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMRenderActor, Log, All);

ACoMOverworldRenderingActor::ACoMOverworldRenderingActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // After camera moves
}

void ACoMOverworldRenderingActor::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogCoMRenderActor, Error, TEXT("No GameInstance — cannot initialise renderer"));
		return;
	}

	CachedRenderer = GI->GetSubsystem<UCoMOverworldRenderer>();
	if (!CachedRenderer)
	{
		UE_LOG(LogCoMRenderActor, Error, TEXT("UCoMOverworldRenderer subsystem not found"));
		return;
	}

	// Provide the world context so the renderer can spawn actors
	CachedRenderer->SetWorldContext(GetWorld());

	// Kick off the initial render.
	// A brief delay allows other subsystems (world gen, turn manager) to finish
	// their BeginPlay work before we query tile data. Using a timer here keeps
	// the approach simple without needing a delegate chain.
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
	{
		if (CachedRenderer)
		{
			CachedRenderer->RenderPlane(InitialPlane, InitialLayer, LocalWizardIndex);
			UE_LOG(LogCoMRenderActor, Log,
			       TEXT("Initial RenderPlane called: Plane=%d Layer=%d Wizard=%d"),
			       static_cast<int32>(InitialPlane), static_cast<int32>(InitialLayer),
			       LocalWizardIndex);
		}
	}, 0.1f, false);
}

void ACoMOverworldRenderingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CachedRenderer)
	{
		return;
	}

	// Find the camera pawn to get current position and zoom level
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	APawn* ControlledPawn = PC->GetPawn();
	ACoMCameraPawn* CamPawn = Cast<ACoMCameraPawn>(ControlledPawn);
	if (!CamPawn)
	{
		return;
	}

	const FVector CameraPos = CamPawn->GetActorLocation();
	const float ZoomLevel = CamPawn->SpringArm
		? CamPawn->SpringArm->TargetArmLength
		: 2000.0f;

	CachedRenderer->UpdateVisibleChunks(CameraPos, ZoomLevel);
}
