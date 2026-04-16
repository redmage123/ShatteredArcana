// Copyright Mythforge Studios. All Rights Reserved.
// CoMAccessibilitySubsystem.h -- Accessibility features: UI scaling, high contrast,
// colorblind modes, screen reader support, subtitle system.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMAccessibilitySubsystem.generated.h"

UENUM(BlueprintType)
enum class ECoMColorblindMode : uint8
{
	None         UMETA(DisplayName = "None"),
	Deuteranopia UMETA(DisplayName = "Deuteranopia (Red-Green)"),
	Protanopia   UMETA(DisplayName = "Protanopia (Red-Green)"),
	Tritanopia   UMETA(DisplayName = "Tritanopia (Blue-Yellow)"),
	Achromatopsia UMETA(DisplayName = "Achromatopsia (Monochrome)"),
};

/**
 * UCoMAccessibilitySubsystem
 *
 * Manages accessibility settings: UI scaling (scroll zoom), high contrast mode,
 * colorblind support, screen reader text, and subtitle/caption system.
 * Settings persist across sessions.
 */
UCLASS()
class COMCORE_API UCoMAccessibilitySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── UI Scaling (scroll to zoom) ──────────────────────────────────────

	/** Get current UI scale factor (1.0 = default, 0.5 = half, 2.0 = double). */
	UFUNCTION(BlueprintPure, Category = "Accessibility")
	float GetUIScale() const { return UIScaleFactor; }

	/** Set UI scale factor. Clamp between 0.5 and 3.0. */
	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void SetUIScale(float Scale);

	/** Increase UI scale by step (for scroll wheel). */
	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void ZoomIn();

	/** Decrease UI scale by step. */
	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void ZoomOut();

	// ── High Contrast Mode ───────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Accessibility")
	bool IsHighContrastEnabled() const { return bHighContrastMode; }

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void SetHighContrastMode(bool bEnabled);

	// ── Colorblind Mode ──────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Accessibility")
	ECoMColorblindMode GetColorblindMode() const { return ColorblindMode; }

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void SetColorblindMode(ECoMColorblindMode Mode);

	/** Adjust a color for the current colorblind mode. */
	UFUNCTION(BlueprintPure, Category = "Accessibility")
	FLinearColor AdjustColorForColorblind(const FLinearColor& Color) const;

	// ── Screen Reader / Text-to-Speech ───────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Accessibility")
	bool IsScreenReaderEnabled() const { return bScreenReaderEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void SetScreenReaderEnabled(bool bEnabled);

	/** Announce text for screen readers (logs to output for TTS integration). */
	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void AnnounceText(const FString& Text);

	// ── Subtitles / Captions ─────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Accessibility")
	bool AreSubtitlesEnabled() const { return bSubtitlesEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void SetSubtitlesEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Accessibility")
	float GetSubtitleScale() const { return SubtitleScale; }

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void SetSubtitleScale(float Scale);

	// ── Large Font Mode ──────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Accessibility")
	bool IsLargeFontEnabled() const { return bLargeFontMode; }

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	void SetLargeFontMode(bool bEnabled);

	/** Get font size multiplier (1.0 or 1.5 for large font). */
	UFUNCTION(BlueprintPure, Category = "Accessibility")
	float GetFontSizeMultiplier() const { return bLargeFontMode ? 1.5f : 1.0f; }

	// ── Delegates ────────────────────────────────────────────────────────

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIScaleChanged, float, NewScale);
	UPROPERTY(BlueprintAssignable, Category = "Accessibility")
	FOnUIScaleChanged OnUIScaleChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAccessibilityChanged, bool, bAnyAccessibilityActive);
	UPROPERTY(BlueprintAssignable, Category = "Accessibility")
	FOnAccessibilityChanged OnAccessibilityChanged;

private:
	float UIScaleFactor = 1.0f;
	float UIScaleStep = 0.1f;
	float UIScaleMin = 0.5f;
	float UIScaleMax = 3.0f;

	bool bHighContrastMode = false;
	ECoMColorblindMode ColorblindMode = ECoMColorblindMode::None;
	bool bScreenReaderEnabled = false;
	bool bSubtitlesEnabled = true;
	float SubtitleScale = 1.0f;
	bool bLargeFontMode = false;

	void ApplyUIScale();
	void BroadcastAccessibilityChanged();
};
