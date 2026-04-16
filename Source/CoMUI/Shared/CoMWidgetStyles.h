// Copyright Mythforge Studios. All Rights Reserved.
// CoMWidgetStyles.h -- Shared dark fantasy UI styling for all Shattered Arcana widgets.
// Provides styled button creation, color constants, and layout helpers.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"

/**
 * Shared UI styling for Shattered Arcana's dark fantasy aesthetic.
 * All colors, button styles, and widget construction helpers in one place.
 * Use these instead of raw Box-styled buttons for a consistent, polished look.
 */
namespace CoMStyle
{
	// ── Color Palette ────────────────────────────────────────────────────────

	// Backgrounds
	inline const FLinearColor BgDarkest    = FLinearColor(0.010f, 0.008f, 0.025f, 1.0f);
	inline const FLinearColor BgDark       = FLinearColor(0.025f, 0.020f, 0.055f, 1.0f);
	inline const FLinearColor BgMid        = FLinearColor(0.045f, 0.035f, 0.090f, 1.0f);
	inline const FLinearColor BgPanel      = FLinearColor(0.035f, 0.028f, 0.075f, 0.95f);

	// Gold accent family
	inline const FLinearColor GoldBright   = FLinearColor(1.000f, 0.843f, 0.000f, 1.0f);
	inline const FLinearColor Gold         = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	inline const FLinearColor GoldDim      = FLinearColor(0.500f, 0.380f, 0.080f, 0.6f);
	inline const FLinearColor GoldDark     = FLinearColor(0.300f, 0.220f, 0.050f, 0.8f);

	// Text
	inline const FLinearColor TextWhite    = FLinearColor(0.950f, 0.940f, 0.920f, 1.0f);
	inline const FLinearColor TextSilver   = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	inline const FLinearColor TextGrey     = FLinearColor(0.500f, 0.500f, 0.550f, 1.0f);
	inline const FLinearColor TextGold     = Gold;

	// Button colors (layered for 3D effect)
	inline const FLinearColor BtnOuterHL   = FLinearColor(0.450f, 0.350f, 0.120f, 0.9f);  // Top/left highlight
	inline const FLinearColor BtnOuterSH   = FLinearColor(0.150f, 0.100f, 0.030f, 0.9f);  // Bottom/right shadow
	inline const FLinearColor BtnFaceN     = FLinearColor(0.080f, 0.055f, 0.160f, 1.0f);   // Normal face
	inline const FLinearColor BtnFaceH     = FLinearColor(0.120f, 0.080f, 0.220f, 1.0f);   // Hovered face
	inline const FLinearColor BtnFaceP     = FLinearColor(0.050f, 0.035f, 0.110f, 1.0f);   // Pressed face
	inline const FLinearColor BtnBorderN   = FLinearColor(0.400f, 0.300f, 0.080f, 0.7f);   // Normal border
	inline const FLinearColor BtnBorderH   = FLinearColor(0.700f, 0.530f, 0.100f, 1.0f);   // Hover border (bright gold)
	inline const FLinearColor BtnBorderP   = FLinearColor(0.300f, 0.220f, 0.060f, 0.8f);   // Pressed border

	// ── Button Factory ───────────────────────────────────────────────────────

	/**
	 * Create a styled fantasy button with beveled border effect.
	 * Returns the UButton. The OutContainer receives the outermost widget to add to layouts.
	 *
	 * Structure: SizeBox → OuterBorder(gold) → HighlightBorder(top-left bright) →
	 *            ShadowBorder(bottom-right dark) → Button(styled) → ContentOverlay
	 *
	 * @param WidgetTree    The widget tree to construct widgets from
	 * @param Label         Button text
	 * @param Width         Desired width
	 * @param Height        Desired height
	 * @param FontSize      Label font size
	 * @param OutContainer  Receives the outermost USizeBox for adding to layouts
	 */
	inline UButton* CreateStyledButton(
		UWidgetTree* WidgetTree,
		const FString& Label,
		float Width = 160.f,
		float Height = 44.f,
		int32 FontSize = 14,
		USizeBox** OutContainer = nullptr)
	{
		// Outer size constraint
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
		SizeBox->SetWidthOverride(Width);
		SizeBox->SetHeightOverride(Height);

		// Gold outer border (1.5px)
		UBorder* OuterBorder = WidgetTree->ConstructWidget<UBorder>();
		OuterBorder->SetBrushColor(BtnBorderN);
		OuterBorder->SetPadding(FMargin(1.5f));

		// Top-left highlight border (simulates bevel light)
		UBorder* HighlightBorder = WidgetTree->ConstructWidget<UBorder>();
		HighlightBorder->SetBrushColor(BtnOuterHL);
		HighlightBorder->SetPadding(FMargin(1.0f, 1.0f, 0.0f, 0.0f));

		// Bottom-right shadow border (simulates bevel shadow)
		UBorder* ShadowBorder = WidgetTree->ConstructWidget<UBorder>();
		ShadowBorder->SetBrushColor(BtnOuterSH);
		ShadowBorder->SetPadding(FMargin(0.0f, 0.0f, 1.0f, 1.0f));

		// The actual button
		UButton* Button = WidgetTree->ConstructWidget<UButton>();

		FButtonStyle Style = Button->GetStyle();
		Style.Normal.DrawAs = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(BtnFaceN);
		Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(BtnFaceH);
		Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(BtnFaceP);
		Button->SetStyle(Style);

		// Label with shadow for depth
		UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
		BtnLabel->SetText(FText::FromString(Label));
		BtnLabel->SetColorAndOpacity(FSlateColor(TextSilver));
		BtnLabel->SetJustification(ETextJustify::Center);

		FSlateFontInfo Font = BtnLabel->GetFont();
		Font.Size = FontSize;
		BtnLabel->SetFont(Font);
		BtnLabel->SetShadowOffset(FVector2D(1.0f, 1.0f));
		BtnLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));

		Button->AddChild(BtnLabel);

		// Assemble the bevel stack
		ShadowBorder->AddChild(Button);
		HighlightBorder->AddChild(ShadowBorder);
		OuterBorder->AddChild(HighlightBorder);
		SizeBox->AddChild(OuterBorder);

		if (OutContainer) { *OutContainer = SizeBox; }
		return Button;
	}

	/**
	 * Create a styled gold-accented action button (more prominent than standard).
	 * Used for primary actions: Start Game, End Turn, Cast Spell, etc.
	 */
	inline UButton* CreatePrimaryButton(
		UWidgetTree* WidgetTree,
		const FString& Label,
		float Width = 180.f,
		float Height = 48.f,
		int32 FontSize = 16,
		USizeBox** OutContainer = nullptr)
	{
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
		SizeBox->SetWidthOverride(Width);
		SizeBox->SetHeightOverride(Height);

		// Bright gold outer border
		UBorder* OuterBorder = WidgetTree->ConstructWidget<UBorder>();
		OuterBorder->SetBrushColor(Gold);
		OuterBorder->SetPadding(FMargin(2.0f));

		// Inner gold highlight
		UBorder* InnerBorder = WidgetTree->ConstructWidget<UBorder>();
		InnerBorder->SetBrushColor(GoldDark);
		InnerBorder->SetPadding(FMargin(1.0f));

		UButton* Button = WidgetTree->ConstructWidget<UButton>();

		// Richer, warmer colors for primary buttons
		FButtonStyle Style = Button->GetStyle();
		Style.Normal.DrawAs = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(FLinearColor(0.12f, 0.06f, 0.02f, 1.0f)); // Deep warm brown
		Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(FLinearColor(0.18f, 0.10f, 0.04f, 1.0f));
		Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.08f, 0.04f, 0.01f, 1.0f));
		Button->SetStyle(Style);

		UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
		BtnLabel->SetText(FText::FromString(Label));
		BtnLabel->SetColorAndOpacity(FSlateColor(GoldBright));
		BtnLabel->SetJustification(ETextJustify::Center);

		FSlateFontInfo Font = BtnLabel->GetFont();
		Font.Size = FontSize;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		BtnLabel->SetFont(Font);
		BtnLabel->SetShadowOffset(FVector2D(1.5f, 1.5f));
		BtnLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));

		Button->AddChild(BtnLabel);
		InnerBorder->AddChild(Button);
		OuterBorder->AddChild(InnerBorder);
		SizeBox->AddChild(OuterBorder);

		if (OutContainer) { *OutContainer = SizeBox; }
		return Button;
	}

	/**
	 * Create a small icon-style button (for [+], [-], arrows, etc.)
	 */
	inline UButton* CreateSmallButton(
		UWidgetTree* WidgetTree,
		const FString& Symbol,
		float Size = 32.f,
		USizeBox** OutContainer = nullptr)
	{
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
		SizeBox->SetWidthOverride(Size);
		SizeBox->SetHeightOverride(Size);

		UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
		Border->SetBrushColor(BtnBorderN);
		Border->SetPadding(FMargin(1.0f));

		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style = Button->GetStyle();
		Style.Normal.DrawAs = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(BtnFaceN);
		Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(BtnFaceH);
		Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(BtnFaceP);
		Button->SetStyle(Style);

		UTextBlock* SymLabel = WidgetTree->ConstructWidget<UTextBlock>();
		SymLabel->SetText(FText::FromString(Symbol));
		SymLabel->SetColorAndOpacity(FSlateColor(Gold));
		SymLabel->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = SymLabel->GetFont();
		Font.Size = static_cast<int32>(Size * 0.5f);
		SymLabel->SetFont(Font);

		Button->AddChild(SymLabel);
		Border->AddChild(Button);
		SizeBox->AddChild(Border);

		if (OutContainer) { *OutContainer = SizeBox; }
		return Button;
	}

	/**
	 * Create a styled panel with gold border and dark background.
	 * Returns the inner UBorder to add content to.
	 */
	inline UBorder* CreatePanel(
		UWidgetTree* WidgetTree,
		float GoldBorderWidth = 1.5f,
		const FLinearColor& InnerColor = BgPanel,
		const FMargin& InnerPadding = FMargin(20.f, 15.f),
		USizeBox** OutContainer = nullptr,
		float Width = 0.f,
		float Height = 0.f)
	{
		UBorder* OuterBorder = WidgetTree->ConstructWidget<UBorder>();
		OuterBorder->SetBrushColor(GoldDim);
		OuterBorder->SetPadding(FMargin(GoldBorderWidth));

		UBorder* InnerBorder = WidgetTree->ConstructWidget<UBorder>();
		InnerBorder->SetBrushColor(InnerColor);
		InnerBorder->SetPadding(InnerPadding);
		OuterBorder->AddChild(InnerBorder);

		if (Width > 0.f || Height > 0.f)
		{
			USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
			if (Width > 0.f) SizeBox->SetWidthOverride(Width);
			if (Height > 0.f) SizeBox->SetHeightOverride(Height);
			SizeBox->AddChild(OuterBorder);
			if (OutContainer) { *OutContainer = SizeBox; }
		}
		else
		{
			if (OutContainer) { *OutContainer = nullptr; }
		}

		return InnerBorder;
	}
}
