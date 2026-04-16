// Copyright Mythforge Studios. All Rights Reserved.
// CoMEnchantmentsWidget.cpp -- Active global enchantments implementation.

#include "CoMEnchantmentsWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

#include "CoMUI/CoMUISubsystem.h"

namespace EnchColours
{
	static const FLinearColor BgDark     = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.816f, 0.816f, 0.863f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPressed = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMEnchantmentsWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMEnchantmentsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton) { CloseButton->OnClicked.AddDynamic(this, &UCoMEnchantmentsWidget::OnCloseClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMEnchantmentsWidget::SetWizardId(int32 WizardId)
{
	CurrentWizardId = WizardId;

	if (!EnchantmentListScrollBox) { return; }
	EnchantmentListScrollBox->ClearChildren();

	// Placeholder enchantment entries
	struct FPlaceholder { FString Name; FString Caster; int32 ManaCost; };
	TArray<FPlaceholder> Entries = {
		{ TEXT("Eternal Night"),     TEXT("Wizard 2"), 8 },
		{ TEXT("Nature's Wrath"),    TEXT("Wizard 3"), 12 },
		{ TEXT("Arcane Supremacy"),  TEXT("You"),      15 },
		{ TEXT("Planar Rift"),       TEXT("Wizard 5"), 10 },
	};

	for (const auto& Entry : Entries)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Name
		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>();
		NameText->SetText(FText::FromString(Entry.Name));
		NameText->SetColorAndOpacity(FSlateColor(EnchColours::Silver));
		FSlateFontInfo NFont = NameText->GetFont();
		NFont.Size = 13;
		NameText->SetFont(NFont);
		UHorizontalBoxSlot* NSlotRef = Row->AddChildToHorizontalBox(NameText);
		if (NSlotRef) { NSlotRef->SetVerticalAlignment(VAlign_Center); NSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

		// Caster
		UTextBlock* CasterText = WidgetTree->ConstructWidget<UTextBlock>();
		CasterText->SetText(FText::FromString(Entry.Caster));
		CasterText->SetColorAndOpacity(FSlateColor(EnchColours::Grey));
		FSlateFontInfo CFont = CasterText->GetFont();
		CFont.Size = 12;
		CasterText->SetFont(CFont);
		UHorizontalBoxSlot* CSlotRef = Row->AddChildToHorizontalBox(CasterText);
		if (CSlotRef) { CSlotRef->SetVerticalAlignment(VAlign_Center); CSlotRef->SetPadding(FMargin(8, 0)); }

		// Mana cost
		UTextBlock* ManaText = WidgetTree->ConstructWidget<UTextBlock>();
		ManaText->SetText(FText::FromString(FString::Printf(TEXT("%d/turn"), Entry.ManaCost)));
		ManaText->SetColorAndOpacity(FSlateColor(EnchColours::Gold));
		FSlateFontInfo MFont = ManaText->GetFont();
		MFont.Size = 12;
		ManaText->SetFont(MFont);
		UHorizontalBoxSlot* MSlotRef = Row->AddChildToHorizontalBox(ManaText);
		if (MSlotRef) { MSlotRef->SetVerticalAlignment(VAlign_Center); MSlotRef->SetPadding(FMargin(8, 0)); }

		// Dispel button (small inline)
		USizeBox* DispelSize = WidgetTree->ConstructWidget<USizeBox>();
		DispelSize->SetWidthOverride(70.0f);
		DispelSize->SetHeightOverride(26.0f);

		UButton* DispelBtn = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style = DispelBtn->GetStyle();
		Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(EnchColours::BtnNormal);
		Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(EnchColours::BtnHover);
		Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(EnchColours::BtnPressed);
		DispelBtn->SetStyle(Style);

		UTextBlock* DispelLabel = WidgetTree->ConstructWidget<UTextBlock>();
		DispelLabel->SetText(FText::FromString(TEXT("Dispel")));
		DispelLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.3f, 0.3f, 1.0f)));
		DispelLabel->SetJustification(ETextJustify::Center);
		FSlateFontInfo DFont = DispelLabel->GetFont();
		DFont.Size = 10;
		DispelLabel->SetFont(DFont);
		DispelBtn->AddChild(DispelLabel);
		DispelSize->AddChild(DispelBtn);

		UHorizontalBoxSlot* DSlotRef = Row->AddChildToHorizontalBox(DispelSize);
		if (DSlotRef) { DSlotRef->SetVerticalAlignment(VAlign_Center); DSlotRef->SetPadding(FMargin(4, 0, 0, 0)); }

		// Wrap row in a border for padding
		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>();
		RowBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		RowBorder->SetPadding(FMargin(4.0f, 4.0f));
		RowBorder->AddChild(Row);

		EnchantmentListScrollBox->AddChild(RowBorder);
	}
}

void UCoMEnchantmentsWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

// =============================================================================
// Helpers
// =============================================================================

UButton* UCoMEnchantmentsWidget::CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(Width);
	SizeBox->SetHeightOverride(36.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(EnchColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(EnchColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(EnchColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(EnchColours::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(EnchColours::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 13;
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UVerticalBoxSlot* SlotRef = Parent->AddChildToVerticalBox(SizeBox);
	if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 3)); }

	return Button;
}

// =============================================================================
// Layout
// =============================================================================

void UCoMEnchantmentsWidget::BuildLayout()
{
	// ── Full-screen dark background ──────────────────────────────────────
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(EnchColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(ScreenOverlay);

	// ── Center panel ─────────────────────────────────────────────────────
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(EnchColours::GoldDim);
	PanelBorder->SetPadding(FMargin(1.5f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(EnchColours::PanelBg);
	PanelInner->SetPadding(FMargin(24.0f, 16.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(700.0f);
	PanelSize->SetHeightOverride(500.0f);
	PanelSize->AddChild(PanelBorder);

	UOverlaySlot* PanelSlotRef = ScreenOverlay->AddChildToOverlay(PanelSize);
	if (PanelSlotRef)
	{
		PanelSlotRef->SetHorizontalAlignment(HAlign_Center);
		PanelSlotRef->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelInner->AddChild(ContentBox);

	// ── Header ───────────────────────────────────────────────────────────
	{
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("Active Enchantments")));
		HeaderText->SetColorAndOpacity(FSlateColor(EnchColours::Gold));
		HeaderText->SetJustification(ETextJustify::Center);
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 24;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);
		UVerticalBoxSlot* HdrSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HdrSlotRef) { HdrSlotRef->SetHorizontalAlignment(HAlign_Center); HdrSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Divider ──────────────────────────────────────────────────────────
	{
		UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
		Div->SetBrushColor(EnchColours::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(1.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* DivSlotRef = ContentBox->AddChildToVerticalBox(DivSize);
		if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); DivSlotRef->SetPadding(FMargin(0, 0, 0, 12)); }
	}

	// ── Scrollable list ──────────────────────────────────────────────────
	EnchantmentListScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	UVerticalBoxSlot* ScrollSlotRef = ContentBox->AddChildToVerticalBox(EnchantmentListScrollBox);
	if (ScrollSlotRef)
	{
		ScrollSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// ── Close button ─────────────────────────────────────────────────────
	CloseButton = CreateStyledButton(ContentBox, TEXT("Close"), 140.0f);
}
