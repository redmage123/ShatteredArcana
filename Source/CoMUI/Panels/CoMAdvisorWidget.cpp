// Copyright Mythforge Studios. All Rights Reserved.
// CoMAdvisorWidget.cpp -- Strategic advisor recommendations implementation.

#include "CoMAdvisorWidget.h"

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

namespace AdvColours
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

	// Section accent colours
	static const FLinearColor Military   = FLinearColor(0.8f, 0.2f, 0.2f, 1.0f);
	static const FLinearColor Economic   = FLinearColor(0.85f, 0.75f, 0.2f, 1.0f);
	static const FLinearColor Magic      = FLinearColor(0.4f, 0.3f, 0.9f, 1.0f);
	static const FLinearColor Diplomatic = FLinearColor(0.2f, 0.7f, 0.4f, 1.0f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMAdvisorWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMAdvisorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton) { CloseButton->OnClicked.AddDynamic(this, &UCoMAdvisorWidget::OnCloseClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMAdvisorWidget::SetWizardId(int32 WizardId)
{
	CurrentWizardId = WizardId;

	if (!AdviceScrollBox) { return; }
	AdviceScrollBox->ClearChildren();

	UVerticalBox* ContentVBox = WidgetTree->ConstructWidget<UVerticalBox>();
	AdviceScrollBox->AddChild(ContentVBox);

	// Placeholder advice per domain
	AddAdviceSection(ContentVBox, TEXT("Military"),
		AdvColours::Military,
		TEXT("Your northern border is vulnerable. Consider garrisoning Ironhold with at least 4 units. Wizard 2's army near Noctharion is growing — prepare defenses."));

	AddAdviceSection(ContentVBox, TEXT("Economic"),
		AdvColours::Economic,
		TEXT("Gold income is strong but mana production lags behind rivals. Build a Mana Forge in your capital. Consider founding a new city near the crystal deposits on Verdantis."));

	AddAdviceSection(ContentVBox, TEXT("Magic"),
		AdvColours::Magic,
		TEXT("Research priority: unlock Arcane tier 3 spells before turn 25. Your spell power is below average — allocate more mana to research. Consider learning Planar Shift for multi-plane expansion."));

	AddAdviceSection(ContentVBox, TEXT("Diplomatic"),
		AdvColours::Diplomatic,
		TEXT("Wizard 3 has positive relations — a trade agreement would benefit both. Wizard 5 is a growing threat; consider a defensive pact with Wizard 4. Avoid provoking Wizard 2 until your military is stronger."));
}

void UCoMAdvisorWidget::OnCloseClicked()
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

void UCoMAdvisorWidget::AddAdviceSection(UVerticalBox* Parent, const FString& Title, const FLinearColor& AccentColor, const FString& Advice)
{
	// Section header with colored accent bar
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();

	// Accent bar
	UBorder* AccentBar = WidgetTree->ConstructWidget<UBorder>();
	AccentBar->SetBrushColor(AccentColor);
	USizeBox* AccentSize = WidgetTree->ConstructWidget<USizeBox>();
	AccentSize->SetWidthOverride(4.0f);
	AccentSize->SetHeightOverride(20.0f);
	AccentSize->AddChild(AccentBar);
	UHorizontalBoxSlot* AccSlotRef = HeaderRow->AddChildToHorizontalBox(AccentSize);
	if (AccSlotRef) { AccSlotRef->SetVerticalAlignment(VAlign_Center); AccSlotRef->SetPadding(FMargin(0, 0, 8, 0)); }

	// Section title
	UTextBlock* SectionTitle = WidgetTree->ConstructWidget<UTextBlock>();
	SectionTitle->SetText(FText::FromString(Title));
	SectionTitle->SetColorAndOpacity(FSlateColor(AccentColor));
	FSlateFontInfo TFont = SectionTitle->GetFont();
	TFont.Size = 16;
	TFont.TypefaceFontName = FName(TEXT("Bold"));
	SectionTitle->SetFont(TFont);
	UHorizontalBoxSlot* TSlotRef = HeaderRow->AddChildToHorizontalBox(SectionTitle);
	if (TSlotRef) { TSlotRef->SetVerticalAlignment(VAlign_Center); }

	UVerticalBoxSlot* HdrSlotRef = Parent->AddChildToVerticalBox(HeaderRow);
	if (HdrSlotRef) { HdrSlotRef->SetPadding(FMargin(0, 10, 0, 4)); }

	// Advice text
	UTextBlock* AdviceText = WidgetTree->ConstructWidget<UTextBlock>();
	AdviceText->SetText(FText::FromString(Advice));
	AdviceText->SetColorAndOpacity(FSlateColor(AdvColours::Silver));
	AdviceText->SetAutoWrapText(true);
	FSlateFontInfo AFont = AdviceText->GetFont();
	AFont.Size = 13;
	AdviceText->SetFont(AFont);
	UVerticalBoxSlot* ASlotRef = Parent->AddChildToVerticalBox(AdviceText);
	if (ASlotRef) { ASlotRef->SetPadding(FMargin(12, 0, 0, 6)); }

	// Thin divider
	UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
	Div->SetBrushColor(AdvColours::GoldDim);
	USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
	DivSize->SetHeightOverride(1.0f);
	DivSize->AddChild(Div);
	UVerticalBoxSlot* DivSlotRef = Parent->AddChildToVerticalBox(DivSize);
	if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); DivSlotRef->SetPadding(FMargin(0, 4, 0, 0)); }
}

UButton* UCoMAdvisorWidget::CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(Width);
	SizeBox->SetHeightOverride(36.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(AdvColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(AdvColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(AdvColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(AdvColours::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(AdvColours::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 13;
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UVerticalBoxSlot* SlotRef = Parent->AddChildToVerticalBox(SizeBox);
	if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 6, 0, 0)); }

	return Button;
}

// =============================================================================
// Layout
// =============================================================================

void UCoMAdvisorWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(AdvColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(ScreenOverlay);

	// Center panel
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(AdvColours::GoldDim);
	PanelBorder->SetPadding(FMargin(1.5f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(AdvColours::PanelBg);
	PanelInner->SetPadding(FMargin(24.0f, 16.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(650.0f);
	PanelSize->SetHeightOverride(550.0f);
	PanelSize->AddChild(PanelBorder);

	UOverlaySlot* PanelSlotRef = ScreenOverlay->AddChildToOverlay(PanelSize);
	if (PanelSlotRef)
	{
		PanelSlotRef->SetHorizontalAlignment(HAlign_Center);
		PanelSlotRef->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelInner->AddChild(ContentBox);

	// Header
	{
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("Royal Advisor")));
		HeaderText->SetColorAndOpacity(FSlateColor(AdvColours::Gold));
		HeaderText->SetJustification(ETextJustify::Center);
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 24;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);
		UVerticalBoxSlot* HdrSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HdrSlotRef) { HdrSlotRef->SetHorizontalAlignment(HAlign_Center); HdrSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// Divider
	{
		UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
		Div->SetBrushColor(AdvColours::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(1.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* DivSlotRef = ContentBox->AddChildToVerticalBox(DivSize);
		if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); DivSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }
	}

	// Scrollable advice content
	AdviceScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	UVerticalBoxSlot* ScrollSlotRef = ContentBox->AddChildToVerticalBox(AdviceScrollBox);
	if (ScrollSlotRef) { ScrollSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

	// Close button
	CloseButton = CreateStyledButton(ContentBox, TEXT("Close"), 140.0f);
}
