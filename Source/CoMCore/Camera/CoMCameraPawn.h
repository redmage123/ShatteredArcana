// Copyright Mythforge Studios. All Rights Reserved.
// CoMCameraPawn.h — Top-down overworld camera pawn. COM-029
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CoMCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UCoMUnitSubsystem;
class UCoMCitySubsystem;

/**
 * ACoMCameraPawn
 *
 * Free-floating top-down camera for the Shattered Arcana overworld.
 * Supports three input modes that compose additively each tick:
 *   1. Keyboard pan  — WASD / arrow keys (configurable axis bindings)
 *   2. Edge scroll   — cursor within EdgeScrollMargin of viewport edge
 *   3. Scroll wheel  — zooms spring arm length; smoothly interpolated
 *
 * Pan speed scales with zoom depth so panning feels consistent at all zoom levels.
 * Disable edge scroll (bEdgeScrollEnabled=false) whenever a UI modal is open.
 *
 * The pawn is platform-agnostic (no OS-specific headers, no render module deps).
 * Input bindings assume UE5 legacy axis/action system; migrate to EnhancedInput
 * when CoMUI is integrated.
 */
UCLASS()
class COMCORE_API ACoMCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ACoMCameraPawn();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

	// ─── Components ──────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// ─── Zoom ────────────────────────────────────────────────────────────────

	/** Spring arm length when fully zoomed in (tiles fill screen). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MinArmLength = 600.f;

	/** Spring arm length when fully zoomed out (strategic overview). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float MaxArmLength = 4000.f;

	/** Arm length delta per scroll-wheel tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float ZoomStep = 200.f;

	/** Interpolation speed for smooth zoom transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float ZoomInterpSpeed = 10.f;

	// ─── Pan ─────────────────────────────────────────────────────────────────

	/** World units/s for keyboard pan at maximum zoom (scales linearly with zoom). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan")
	float KeyboardPanSpeed = 1200.f;

	/** World units/s for edge-scroll at maximum zoom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan")
	float EdgeScrollSpeed = 1000.f;

	/**
	 * Normalised viewport margin [0,1] that activates edge-scroll.
	 * 0.02 = 2% of viewport width/height on each side.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan", meta = (ClampMin = "0.0", ClampMax = "0.2"))
	float EdgeScrollMargin = 0.02f;

	/** Set false to suppress edge-scroll (e.g. while a UI modal has focus). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan")
	bool bEdgeScrollEnabled = true;

	// ─── Jump-to-Next ────────────────────────────────────────────────────────

	/** Cycle to the next idle army (with remaining movement) and center camera. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Navigation")
	void JumpToNextIdleArmy();

	/** Cycle to the next army owned by the player and center camera. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Navigation")
	void JumpToNextArmy();

	/** Cycle to the next city owned by the player and center camera. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Navigation")
	void JumpToNextCity();

	/** The wizard index of the local human player (set externally). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Navigation")
	int32 LocalWizardIndex = 0;

private:
	// ─── Input handlers ──────────────────────────────────────────────────────

	void MoveForward(float Value);
	void MoveRight(float Value);
	void ZoomIn();
	void ZoomOut();

	/**
	 * Sample cursor position against viewport edges each tick.
	 * Returns a unit-ish direction vector in XY (X=right, Y=forward).
	 */
	FVector2D ComputeEdgeScrollDir() const;

	// ─── State ───────────────────────────────────────────────────────────────

	/** Desired arm length; spring arm interpolates toward this each tick. */
	float TargetArmLength = 2000.f;

	/** Keyboard pan input accumulated this frame from axis bindings. */
	FVector2D KeyboardInput = FVector2D::ZeroVector;

	/** Cycle indices for jump-to-next navigation. */
	int32 CityJumpIndex = 0;
	int32 ArmyJumpIndex = 0;
	int32 IdleArmyJumpIndex = 0;

	/** Center the camera on a world grid position. */
	void CenterOnPosition(FIntPoint GridPos);
};
