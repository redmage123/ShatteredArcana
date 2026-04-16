// Copyright Mythforge Studios. All Rights Reserved.
// CoMAccessibilitySubsystem.cpp -- Accessibility features implementation.

#include "Framework/CoMAccessibilitySubsystem.h"
#include "Engine/UserInterfaceSettings.h"

void UCoMAccessibilitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("[Accessibility] Subsystem initialized. UI Scale: %.1f"), UIScaleFactor);
}

void UCoMAccessibilitySubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// ── UI Scaling ──────────────────────────────────────────────────────────────

void UCoMAccessibilitySubsystem::SetUIScale(float Scale)
{
	UIScaleFactor = FMath::Clamp(Scale, UIScaleMin, UIScaleMax);
	ApplyUIScale();
	OnUIScaleChanged.Broadcast(UIScaleFactor);
	UE_LOG(LogTemp, Log, TEXT("[Accessibility] UI Scale set to %.2f"), UIScaleFactor);
}

void UCoMAccessibilitySubsystem::ZoomIn()
{
	SetUIScale(UIScaleFactor + UIScaleStep);
}

void UCoMAccessibilitySubsystem::ZoomOut()
{
	SetUIScale(UIScaleFactor - UIScaleStep);
}

void UCoMAccessibilitySubsystem::ApplyUIScale()
{
	// Apply DPI scale override via the application scale
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetApplicationScale(UIScaleFactor);
	}
}

// ── High Contrast Mode ──────────────────────────────────────────────────────

void UCoMAccessibilitySubsystem::SetHighContrastMode(bool bEnabled)
{
	bHighContrastMode = bEnabled;
	BroadcastAccessibilityChanged();
	UE_LOG(LogTemp, Log, TEXT("[Accessibility] High contrast: %s"), bEnabled ? TEXT("ON") : TEXT("OFF"));
}

// ── Colorblind Mode ─────────────────────────────────────────────────────────

void UCoMAccessibilitySubsystem::SetColorblindMode(ECoMColorblindMode Mode)
{
	ColorblindMode = Mode;
	BroadcastAccessibilityChanged();
	UE_LOG(LogTemp, Log, TEXT("[Accessibility] Colorblind mode: %d"), static_cast<int32>(Mode));
}

FLinearColor UCoMAccessibilitySubsystem::AdjustColorForColorblind(const FLinearColor& Color) const
{
	switch (ColorblindMode)
	{
	case ECoMColorblindMode::Deuteranopia:
	case ECoMColorblindMode::Protanopia:
	{
		// Shift red-green to blue-yellow spectrum
		float Luminance = Color.R * 0.299f + Color.G * 0.587f + Color.B * 0.114f;
		return FLinearColor(
			Luminance * 0.8f + Color.B * 0.2f,
			Luminance * 0.7f + Color.B * 0.3f,
			Color.B,
			Color.A);
	}
	case ECoMColorblindMode::Tritanopia:
	{
		// Shift blue-yellow to red-green spectrum
		float Luminance = Color.R * 0.299f + Color.G * 0.587f + Color.B * 0.114f;
		return FLinearColor(
			Color.R,
			Luminance * 0.7f + Color.R * 0.3f,
			Luminance * 0.8f + Color.R * 0.2f,
			Color.A);
	}
	case ECoMColorblindMode::Achromatopsia:
	{
		float Grey = Color.R * 0.299f + Color.G * 0.587f + Color.B * 0.114f;
		return FLinearColor(Grey, Grey, Grey, Color.A);
	}
	default:
		return Color;
	}
}

// ── Screen Reader ───────────────────────────────────────────────────────────

void UCoMAccessibilitySubsystem::SetScreenReaderEnabled(bool bEnabled)
{
	bScreenReaderEnabled = bEnabled;
	BroadcastAccessibilityChanged();
	UE_LOG(LogTemp, Log, TEXT("[Accessibility] Screen reader: %s"), bEnabled ? TEXT("ON") : TEXT("OFF"));
}

void UCoMAccessibilitySubsystem::AnnounceText(const FString& Text)
{
	if (!bScreenReaderEnabled) return;

	// Log for TTS integration — a real implementation would call
	// platform-specific TTS APIs (SAPI on Windows, AVSpeechSynthesizer on Mac)
	UE_LOG(LogTemp, Log, TEXT("[ScreenReader] %s"), *Text);

	// Also fire an on-screen notification for visual confirmation
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("[SR] %s"), *Text));
	}
}

// ── Subtitles ───────────────────────────────────────────────────────────────

void UCoMAccessibilitySubsystem::SetSubtitlesEnabled(bool bEnabled)
{
	bSubtitlesEnabled = bEnabled;
	BroadcastAccessibilityChanged();
}

void UCoMAccessibilitySubsystem::SetSubtitleScale(float Scale)
{
	SubtitleScale = FMath::Clamp(Scale, 0.5f, 3.0f);
}

// ── Large Font ──────────────────────────────────────────────────────────────

void UCoMAccessibilitySubsystem::SetLargeFontMode(bool bEnabled)
{
	bLargeFontMode = bEnabled;
	BroadcastAccessibilityChanged();
	UE_LOG(LogTemp, Log, TEXT("[Accessibility] Large font: %s"), bEnabled ? TEXT("ON") : TEXT("OFF"));
}

// ── Internal ────────────────────────────────────────────────────────────────

void UCoMAccessibilitySubsystem::BroadcastAccessibilityChanged()
{
	bool bAny = bHighContrastMode || (ColorblindMode != ECoMColorblindMode::None) ||
	            bScreenReaderEnabled || bLargeFontMode;
	OnAccessibilityChanged.Broadcast(bAny);
}
