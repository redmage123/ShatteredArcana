// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellVFXWidget.h -- Sprite-sheet spell VFX overlay widget.
// Phase 7 -- Shattered Arcana

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMSpellVFXWidget.generated.h"

class UCanvasPanel;
class UImage;
class UTexture2D;

// ─────────────────────────────────────────────────────────────────────────────
// PER-EFFECT ANIMATION STATE (one per simultaneously playing effect)
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT()
struct FCoMVFXAnimInstance
{
	GENERATED_BODY()

	/** Subsystem playback handle. */
	UPROPERTY()
	int32 PlaybackID = 0;

	/** The UImage widget displaying this effect. */
	UPROPERTY()
	TObjectPtr<UImage> ImageWidget = nullptr;

	/** Loaded sprite sheet texture. */
	UPROPERTY()
	TObjectPtr<UTexture2D> SpriteSheet = nullptr;

	/** Total frames in the sheet. */
	int32 FrameCount = 8;

	/** Seconds per frame. */
	float FrameDuration = 0.1f;

	/** Display scale multiplier. */
	float Scale = 1.0f;

	/** Width of a single frame in pixels. */
	int32 FrameWidth = 0;

	/** Height of a single frame in pixels. */
	int32 FrameHeight = 0;

	/** Total sheet width in pixels. */
	int32 SheetWidth = 0;

	/** Current frame index (0-based). */
	int32 CurrentFrame = 0;

	/** Time accumulated on the current frame. */
	float TimeAccumulator = 0.0f;

	/** Screen position for this effect (pixel coords). */
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	/** Whether this animation is still playing. */
	bool bIsPlaying = true;

	/** Timer handle for frame advancement. */
	FTimerHandle FrameTimerHandle;
};

// ─────────────────────────────────────────────────────────────────────────────
// SPELL VFX WIDGET
// ─────────────────────────────────────────────────────────────────────────────

/**
 * UCoMSpellVFXWidget
 *
 * Full-viewport overlay widget that renders 2D sprite-sheet spell effects.
 * Supports multiple simultaneous effects positioned anywhere on screen.
 *
 * Each effect loads a horizontal sprite sheet, displays one frame at a time
 * using UV region manipulation on a FSlateBrush, and auto-removes when the
 * animation completes.
 *
 * Managed by CoMUISubsystem. Listens to CoMSpellVFXSubsystem::OnEffectRequested.
 */
UCLASS()
class COMUI_API UCoMSpellVFXWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// -- UUserWidget overrides ────────────────────────────────────────────────

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// -- Animation Control ───────────────────────────────────────────────────

	/**
	 * Begin a new sprite-sheet animation at the given screen position.
	 * @param PlaybackID     Unique ID from the VFX subsystem.
	 * @param SpriteSheet    The horizontal sprite sheet texture.
	 * @param FrameCount     Number of frames in the sheet.
	 * @param FrameDuration  Seconds per frame.
	 * @param ScreenPosition Pixel position on screen (top-left of the effect).
	 * @param Scale          Display scale multiplier.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellVFX")
	void PlaySpellAnimation(int32 PlaybackID, UTexture2D* SpriteSheet,
	                   int32 FrameCount, float FrameDuration,
	                   FVector2D ScreenPosition, float Scale = 1.0f);

	/** Stop a specific animation by playback ID. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellVFX")
	void StopSpellAnimation(int32 PlaybackID);

	/** Stop all currently playing animations. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellVFX")
	void StopAllSpellAnimations();

	/** Returns true if any animations are currently playing. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellVFX")
	bool HasActiveAnimations() const { return ActiveAnimations.Num() > 0; }

protected:
	// -- Widget hierarchy (built in C++) ──────────────────────────────────────

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas;

private:
	// -- Internal ─────────────────────────────────────────────────────────────

	/** Build the widget tree layout. */
	void BuildLayout();

	/** Advance a single animation instance to the next frame. */
	void AdvanceFrame(int32 PlaybackID);

	/** Update the UImage brush to show the current frame. */
	void UpdateFrameBrush(FCoMVFXAnimInstance& Anim);

	/** Clean up a finished animation (remove image, clear timer). */
	void CleanupAnimation(int32 Index);

	/** All active animation instances. */
	UPROPERTY()
	TArray<FCoMVFXAnimInstance> ActiveAnimations;
};
