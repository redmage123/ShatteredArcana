// Copyright Mythforge Studios. All Rights Reserved.
// CoMTurnSummaryWidget.cpp -- Turn end summary report implementation.

#include "CoMTurnSummaryWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"

// =============================================================================
// Colour palette
// =============================================================================

namespace TurnSumColors
{
	static const FLinearColor BgDark     = FLinearColor(0.015f, 0.010f, 0.040f, 0.95f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.92f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);

	// Category colors
	static const FLinearColor CityGold   = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor ResBlue    = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);
	static const FLinearColor MilRed     = FLinearColor(0.9f, 0.15f, 0.1f, 1.0f);
	static const FLinearColor DipGreen   = FLinearColor(0.1f, 0.75f, 0.2f, 1.0f);
	static const FLinearColor EvtPurple  = FLinearColor(0.6f, 0.2f, 0.8f, 1.0f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMTurnSummaryWidget::RebuildWidget()
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

void UCoMTurnSummaryWidget::BuildLayout()
{
	// ── Full-screen dark background ──────────────────────────────────────────
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(TurnSumColors::BgDark);
	Background->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = Background;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->AddChild(ScreenOverlay);

	// ── Center panel with gold border ────────────────────────────────────────
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(TurnSumColors::GoldDim);
	PanelBorder->SetPadding(FMargin(2.0f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(TurnSumColors::PanelBg);
	PanelInner->SetPadding(FMargin(20.0f, 15.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(700.0f);
	PanelSize->SetHeightOverride(600.0f);
	PanelSize->AddChild(PanelBorder);

	UOverlaySlot* PanelSlotRef = ScreenOverlay->AddChildToOverlay(PanelSize);
	if (PanelSlotRef)
	{
		PanelSlotRef->SetHorizontalAlignment(HAlign_Center);
		PanelSlotRef->SetVerticalAlignment(VAlign_Center);
	}

	// ── Content column ───────────────────────────────────────────────────────
	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelInner->AddChild(ContentBox);

	// ── Header ───────────────────────────────────────────────────────────────
	{
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("Turn Summary")));
		HeaderText->SetColorAndOpacity(FSlateColor(TurnSumColors::Gold));
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 26;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);

		UVerticalBoxSlot* HeaderSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HeaderSlotRef) { HeaderSlotRef->SetPadding(FMargin(0, 0, 0, 12)); HeaderSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}

	// ── Scrollable event list ────────────────────────────────────────────────
	{
		EventScrollBox = WidgetTree->ConstructWidget<UScrollBox>();

		USizeBox* ScrollSize = WidgetTree->ConstructWidget<USizeBox>();
		ScrollSize->SetHeightOverride(420.0f);
		ScrollSize->AddChild(EventScrollBox);

		UVerticalBoxSlot* ScrollSlotRef = ContentBox->AddChildToVerticalBox(ScrollSize);
		if (ScrollSlotRef) { ScrollSlotRef->SetPadding(FMargin(0, 0, 0, 12)); }
	}

	// ── Continue button ──────────────────────────────────────────────────────
	{
		ContinueButton = CreateActionButton(TEXT("Continue"), 180.f);

		USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>();
		BtnSize->SetWidthOverride(180.0f);
		BtnSize->SetHeightOverride(42.0f);
		BtnSize->AddChild(ContinueButton);

		UVerticalBoxSlot* BtnSlotRef = ContentBox->AddChildToVerticalBox(BtnSize);
		if (BtnSlotRef) { BtnSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}
}

UButton* UCoMTurnSummaryWidget::CreateActionButton(const FString& Label, float Width)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(TurnSumColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(TurnSumColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(TurnSumColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(TurnSumColors::Gold));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 16;
	Text->SetFont(Font);

	Btn->AddChild(Text);
	return Btn;
}

// =============================================================================
// NativeConstruct
// =============================================================================

void UCoMTurnSummaryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &UCoMTurnSummaryWidget::OnContinueClicked);
	}
}

// =============================================================================
// Public API
// =============================================================================

void UCoMTurnSummaryWidget::AddEvent(const FString& Category, const FString& Description)
{
	PendingEvents.FindOrAdd(Category).Add(Description);
}

void UCoMTurnSummaryWidget::ClearEvents()
{
	PendingEvents.Empty();
	if (EventScrollBox)
	{
		EventScrollBox->ClearChildren();
	}
}

void UCoMTurnSummaryWidget::ShowSummary(int32 TurnNumber)
{
	CurrentTurnNumber = TurnNumber;

	if (HeaderText)
	{
		HeaderText->SetText(FText::FromString(
			FString::Printf(TEXT("Turn %d Summary"), TurnNumber)));
	}

	if (!EventScrollBox)
	{
		return;
	}

	EventScrollBox->ClearChildren();

	// Ordered categories
	static const TArray<FString> CategoryOrder = {
		TEXT("Cities"), TEXT("Research"), TEXT("Military"), TEXT("Diplomacy"), TEXT("Events")
	};

	for (const FString& Cat : CategoryOrder)
	{
		if (const TArray<FString>* Events = PendingEvents.Find(Cat))
		{
			if (Events->Num() > 0)
			{
				AppendCategorySection(Cat, GetCategoryColor(Cat), *Events);
			}
		}
	}

	// Any remaining categories not in the standard list
	for (const auto& Pair : PendingEvents)
	{
		if (!CategoryOrder.Contains(Pair.Key) && Pair.Value.Num() > 0)
		{
			AppendCategorySection(Pair.Key, TurnSumColors::Silver, Pair.Value);
		}
	}
}

void UCoMTurnSummaryWidget::AppendCategorySection(const FString& CatName, const FLinearColor& Color, const TArray<FString>& Events)
{
	if (!EventScrollBox) { return; }

	// Category header
	UTextBlock* CatHeader = WidgetTree->ConstructWidget<UTextBlock>();
	CatHeader->SetText(FText::FromString(CatName.ToUpper()));
	CatHeader->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo CatFont = CatHeader->GetFont();
	CatFont.Size = 16;
	CatFont.TypefaceFontName = FName(TEXT("Bold"));
	CatHeader->SetFont(CatFont);

	UBorder* CatBorder = WidgetTree->ConstructWidget<UBorder>();
	CatBorder->SetBrushColor(FLinearColor(Color.R, Color.G, Color.B, 0.15f));
	CatBorder->SetPadding(FMargin(8.0f, 4.0f));
	CatBorder->AddChild(CatHeader);

	EventScrollBox->AddChild(CatBorder);

	// Individual events
	for (const FString& Evt : Events)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Color indicator dot
		UBorder* Dot = WidgetTree->ConstructWidget<UBorder>();
		Dot->SetBrushColor(Color);
		USizeBox* DotSize = WidgetTree->ConstructWidget<USizeBox>();
		DotSize->SetWidthOverride(8.0f);
		DotSize->SetHeightOverride(8.0f);
		DotSize->AddChild(Dot);

		UHorizontalBoxSlot* DotSlotRef = Row->AddChildToHorizontalBox(DotSize);
		if (DotSlotRef) { DotSlotRef->SetVerticalAlignment(VAlign_Center); DotSlotRef->SetPadding(FMargin(12.0f, 2.0f, 8.0f, 2.0f)); }

		// Event text
		UTextBlock* EvtText = WidgetTree->ConstructWidget<UTextBlock>();
		EvtText->SetText(FText::FromString(Evt));
		EvtText->SetColorAndOpacity(FSlateColor(TurnSumColors::Silver));
		FSlateFontInfo EvtFont = EvtText->GetFont();
		EvtFont.Size = 13;
		EvtText->SetFont(EvtFont);

		UHorizontalBoxSlot* TextSlotRef = Row->AddChildToHorizontalBox(EvtText);
		if (TextSlotRef) { TextSlotRef->SetVerticalAlignment(VAlign_Center); }

		EventScrollBox->AddChild(Row);
	}

	// Spacer between categories
	USpacer* Sp = WidgetTree->ConstructWidget<USpacer>();
	Sp->SetSize(FVector2D(0.0f, 8.0f));
	EventScrollBox->AddChild(Sp);
}

FLinearColor UCoMTurnSummaryWidget::GetCategoryColor(const FString& Category)
{
	if (Category == TEXT("Cities"))     return TurnSumColors::CityGold;
	if (Category == TEXT("Research"))   return TurnSumColors::ResBlue;
	if (Category == TEXT("Military"))   return TurnSumColors::MilRed;
	if (Category == TEXT("Diplomacy"))  return TurnSumColors::DipGreen;
	if (Category == TEXT("Events"))     return TurnSumColors::EvtPurple;
	return TurnSumColors::Silver;
}

void UCoMTurnSummaryWidget::OnContinueClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
