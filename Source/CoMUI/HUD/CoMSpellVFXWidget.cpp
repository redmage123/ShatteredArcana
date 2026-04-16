// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellVFXWidget.cpp -- Sprite-sheet spell VFX overlay widget implementation.

#include "CoMSpellVFXWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "TimerManager.h"

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMSpellVFXWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

// =============================================================================
// BuildLayout
// =============================================================================

void UCoMSpellVFXWidget::BuildLayout()
{
	// Root canvas panel -- full-screen, hit-test invisible so clicks pass through.
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = RootCanvas;
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

// =============================================================================
// Construction / Destruction
// =============================================================================

void UCoMSpellVFXWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Ensure the widget itself is hit-test invisible (overlay only).
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCoMSpellVFXWidget::NativeDestruct()
{
	StopAllAnimations();
	Super::NativeDestruct();
}

// =============================================================================
// PlayAnimation
// =============================================================================

void UCoMSpellVFXWidget::PlaySpellAnimation(int32 PlaybackID, UTexture2D* SpriteSheet,
                                        int32 FrameCount, float FrameDuration,
                                        FVector2D ScreenPosition, float Scale)
{
	if (!SpriteSheet || FrameCount <= 0 || !RootCanvas)
	{
		return;
	}

	// Build the animation instance.
	FCoMVFXAnimInstance Anim;
	Anim.PlaybackID = PlaybackID;
	Anim.SpriteSheet = SpriteSheet;
	Anim.FrameCount = FrameCount;
	Anim.FrameDuration = FMath::Max(FrameDuration, 0.01f);
	Anim.Scale = Scale;
	Anim.CurrentFrame = 0;
	Anim.TimeAccumulator = 0.0f;
	Anim.bIsPlaying = true;
	Anim.ScreenPosition = ScreenPosition;

	// Calculate frame dimensions from the sprite sheet.
	// Sprite sheets are horizontal strips: total width / frame count = frame width.
	Anim.SheetWidth = SpriteSheet->GetSizeX();
	Anim.FrameHeight = SpriteSheet->GetSizeY();
	Anim.FrameWidth = (FrameCount > 0) ? (Anim.SheetWidth / FrameCount) : Anim.SheetWidth;

	// Create a UImage widget for this effect.
	if (WidgetTree)
	{
		Anim.ImageWidget = WidgetTree->ConstructWidget<UImage>();
	}

	if (!Anim.ImageWidget)
	{
		return;
	}

	// Add the image to the canvas at the requested position.
	UCanvasPanelSlot* CanvasSlotRef = RootCanvas->AddChildToCanvas(Anim.ImageWidget);
	if (CanvasSlotRef)
	{
		const float DisplayWidth  = static_cast<float>(Anim.FrameWidth) * Scale;
		const float DisplayHeight = static_cast<float>(Anim.FrameHeight) * Scale;

		CanvasSlotRef->SetPosition(ScreenPosition);
		CanvasSlotRef->SetSize(FVector2D(DisplayWidth, DisplayHeight));
		CanvasSlotRef->SetAutoSize(false);
	}

	// Set the initial frame brush.
	UpdateFrameBrush(Anim);

	// Set up a repeating timer to advance frames.
	if (UWorld* World = GetWorld())
	{
		const int32 PID = PlaybackID;
		World->GetTimerManager().SetTimer(
			Anim.FrameTimerHandle,
			FTimerDelegate::CreateUObject(this, &UCoMSpellVFXWidget::AdvanceFrame, PID),
			Anim.FrameDuration,
			true);
	}

	ActiveAnimations.Add(MoveTemp(Anim));
}

// =============================================================================
// Frame Advancement
// =============================================================================

void UCoMSpellVFXWidget::AdvanceFrame(int32 PlaybackID)
{
	for (int32 Idx = 0; Idx < ActiveAnimations.Num(); ++Idx)
	{
		FCoMVFXAnimInstance& Anim = ActiveAnimations[Idx];
		if (Anim.PlaybackID != PlaybackID)
		{
			continue;
		}

		Anim.CurrentFrame++;

		if (Anim.CurrentFrame >= Anim.FrameCount)
		{
			// Animation finished -- clean up.
			CleanupAnimation(Idx);
			return;
		}

		UpdateFrameBrush(Anim);
		return;
	}
}

// =============================================================================
// UpdateFrameBrush
// =============================================================================

void UCoMSpellVFXWidget::UpdateFrameBrush(FCoMVFXAnimInstance& Anim)
{
	if (!Anim.ImageWidget || !Anim.SpriteSheet || Anim.SheetWidth <= 0)
	{
		return;
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Anim.SpriteSheet);
	Brush.ImageSize = FVector2D(
		static_cast<float>(Anim.FrameWidth),
		static_cast<float>(Anim.FrameHeight));
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.Tiling = ESlateBrushTileType::NoTile;

	// UV region: horizontal strip, one frame at a time.
	const float U0 = static_cast<float>(Anim.CurrentFrame * Anim.FrameWidth) / static_cast<float>(Anim.SheetWidth);
	const float U1 = static_cast<float>((Anim.CurrentFrame + 1) * Anim.FrameWidth) / static_cast<float>(Anim.SheetWidth);

	Brush.SetUVRegion(FBox2D(
		FVector2D(U0, 0.0f),
		FVector2D(U1, 1.0f)));

	Anim.ImageWidget->SetBrush(Brush);
}

// =============================================================================
// Stop / Cleanup
// =============================================================================

void UCoMSpellVFXWidget::StopSpellAnimation(int32 PlaybackID)
{
	for (int32 Idx = 0; Idx < ActiveAnimations.Num(); ++Idx)
	{
		if (ActiveAnimations[Idx].PlaybackID == PlaybackID)
		{
			CleanupAnimation(Idx);
			return;
		}
	}
}

void UCoMSpellVFXWidget::StopAllSpellAnimations()
{
	for (int32 Idx = ActiveAnimations.Num() - 1; Idx >= 0; --Idx)
	{
		CleanupAnimation(Idx);
	}
}

void UCoMSpellVFXWidget::CleanupAnimation(int32 Index)
{
	if (!ActiveAnimations.IsValidIndex(Index))
	{
		return;
	}

	FCoMVFXAnimInstance& Anim = ActiveAnimations[Index];

	// Clear the frame timer.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Anim.FrameTimerHandle);
	}

	// Remove the image widget from the canvas.
	if (Anim.ImageWidget)
	{
		if (RootCanvas)
		{
			RootCanvas->RemoveChild(Anim.ImageWidget);
		}
		Anim.ImageWidget = nullptr;
	}

	ActiveAnimations.RemoveAt(Index);
}
