// Copyright Mythforge Studios. All Rights Reserved.
// CoMCameraPawn.cpp — COM-029

#include "Camera/CoMCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"

ACoMCameraPawn::ACoMCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Invisible scene root; this moves across the overworld map
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Spring arm: tilted 40° down (range 30–45° felt right in MoM-style 4X)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Root);
	SpringArm->TargetArmLength = TargetArmLength;
	SpringArm->SetRelativeRotation(FRotator(-40.f, 0.f, 0.f));
	SpringArm->bDoCollisionTest         = false;  // Camera floats above terrain
	SpringArm->bEnableCameraLag         = false;  // Crisp strategy-game feel
	SpringArm->bEnableCameraRotationLag = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

// ─── Input ───────────────────────────────────────────────────────────────────

void ACoMCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("CameraMoveForward", this, &ACoMCameraPawn::MoveForward);
	PlayerInputComponent->BindAxis("CameraMoveRight",   this, &ACoMCameraPawn::MoveRight);
	PlayerInputComponent->BindAction("CameraZoomIn",  IE_Pressed, this, &ACoMCameraPawn::ZoomIn);
	PlayerInputComponent->BindAction("CameraZoomOut", IE_Pressed, this, &ACoMCameraPawn::ZoomOut);
}

void ACoMCameraPawn::MoveForward(float Value) { KeyboardInput.Y = Value; }
void ACoMCameraPawn::MoveRight(float Value)   { KeyboardInput.X = Value; }

void ACoMCameraPawn::ZoomIn()
{
	TargetArmLength = FMath::Clamp(TargetArmLength - ZoomStep, MinArmLength, MaxArmLength);
}

void ACoMCameraPawn::ZoomOut()
{
	TargetArmLength = FMath::Clamp(TargetArmLength + ZoomStep, MinArmLength, MaxArmLength);
}

// ─── Tick ────────────────────────────────────────────────────────────────────

void ACoMCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Speed scales with zoom so far-out panning covers more ground
	const float ZoomRatio   = SpringArm->TargetArmLength / MaxArmLength;
	const float SpeedFactor = FMath::Lerp(0.25f, 1.f, ZoomRatio);

	FVector Delta = FVector::ZeroVector;

	// Keyboard pan
	Delta.X += KeyboardInput.Y * KeyboardPanSpeed * SpeedFactor * DeltaTime;
	Delta.Y += KeyboardInput.X * KeyboardPanSpeed * SpeedFactor * DeltaTime;

	// Edge scroll
	if (bEdgeScrollEnabled)
	{
		const FVector2D EdgeDir = ComputeEdgeScrollDir();
		Delta.X += EdgeDir.Y * EdgeScrollSpeed * SpeedFactor * DeltaTime;
		Delta.Y += EdgeDir.X * EdgeScrollSpeed * SpeedFactor * DeltaTime;
	}

	if (!Delta.IsNearlyZero())
	{
		AddActorWorldOffset(Delta);
	}

	// Smooth zoom interpolation
	SpringArm->TargetArmLength = FMath::FInterpTo(
		SpringArm->TargetArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);

	// Reset per-frame axis input
	KeyboardInput = FVector2D::ZeroVector;
}

// ─── Edge scroll ─────────────────────────────────────────────────────────────

FVector2D ACoMCameraPawn::ComputeEdgeScrollDir() const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) { return FVector2D::ZeroVector; }

	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY)) { return FVector2D::ZeroVector; }

	int32 SizeX = 0, SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0) { return FVector2D::ZeroVector; }

	const float NX = MouseX / static_cast<float>(SizeX); // 0=left  1=right
	const float NY = MouseY / static_cast<float>(SizeY); // 0=top   1=bottom

	FVector2D Dir = FVector2D::ZeroVector;
	if      (NX < EdgeScrollMargin)        Dir.X = -1.f;  // scroll left
	else if (NX > 1.f - EdgeScrollMargin)  Dir.X =  1.f;  // scroll right
	if      (NY < EdgeScrollMargin)        Dir.Y = -1.f;  // scroll up (north)
	else if (NY > 1.f - EdgeScrollMargin)  Dir.Y =  1.f;  // scroll down (south)

	return Dir;
}
