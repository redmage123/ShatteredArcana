// Copyright Mythforge Studios. All Rights Reserved.
// CoMWizardRankingsWidget.cpp -- Power graph and wizard comparison implementation.

#include "CoMWizardRankingsWidget.h"

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

namespace RankColors
{
	static const FLinearColor BgDark     = FLinearColor(0.015f, 0.010f, 0.040f, 0.95f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.92f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor GraphBg    = FLinearColor(0.02f, 0.03f, 0.12f, 1.0f);
	static const FLinearColor RowHighlight = FLinearColor(0.855f, 0.647f, 0.125f, 0.15f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMWizardRankingsWidget::RebuildWidget()
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

void UCoMWizardRankingsWidget::BuildLayout()
{
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	Background->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = Background;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->AddChild(ScreenOverlay);

	// Gold border panel
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(RankColors::Gold);
	PanelBorder->SetPadding(FMargin(2.0f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(RankColors::PanelBg);
	PanelInner->SetPadding(FMargin(16.0f, 12.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(800.0f);
	PanelSize->SetHeightOverride(600.0f);
	PanelSize->AddChild(PanelBorder);

	UOverlaySlot* PanelSlotRef = ScreenOverlay->AddChildToOverlay(PanelSize);
	if (PanelSlotRef)
	{
		PanelSlotRef->SetHorizontalAlignment(HAlign_Center);
		PanelSlotRef->SetVerticalAlignment(VAlign_Center);
	}

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelInner->AddChild(ContentBox);

	// ── Header ───────────────────────────────────────────────────────────────
	{
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("Wizard Rankings")));
		HeaderText->SetColorAndOpacity(FSlateColor(RankColors::Gold));
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 24;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);

		UVerticalBoxSlot* HeaderSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HeaderSlotRef) { HeaderSlotRef->SetPadding(FMargin(0, 0, 0, 10)); HeaderSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}

	// ── Power graph placeholder ──────────────────────────────────────────────
	{
		UBorder* GraphBorder = WidgetTree->ConstructWidget<UBorder>();
		GraphBorder->SetBrushColor(RankColors::GraphBg);
		GraphBorder->SetPadding(FMargin(10.0f));

		GraphPlaceholderText = WidgetTree->ConstructWidget<UTextBlock>();
		GraphPlaceholderText->SetText(FText::FromString(TEXT("Power Graph")));
		GraphPlaceholderText->SetColorAndOpacity(FSlateColor(RankColors::Grey));
		GraphPlaceholderText->SetJustification(ETextJustify::Center);
		FSlateFontInfo GFont = GraphPlaceholderText->GetFont();
		GFont.Size = 18;
		GraphPlaceholderText->SetFont(GFont);
		GraphBorder->AddChild(GraphPlaceholderText);

		USizeBox* GraphSize = WidgetTree->ConstructWidget<USizeBox>();
		GraphSize->SetWidthOverride(700.0f);
		GraphSize->SetHeightOverride(120.0f);
		GraphSize->AddChild(GraphBorder);

		UVerticalBoxSlot* GraphSlotRef = ContentBox->AddChildToVerticalBox(GraphSize);
		if (GraphSlotRef) { GraphSlotRef->SetPadding(FMargin(0, 0, 0, 4)); GraphSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}

	// ── Score summary text ───────────────────────────────────────────────────
	{
		ScoreSummaryText = WidgetTree->ConstructWidget<UTextBlock>();
		ScoreSummaryText->SetText(FText::FromString(TEXT("No ranking data available.")));
		ScoreSummaryText->SetColorAndOpacity(FSlateColor(RankColors::Silver));
		FSlateFontInfo SFont = ScoreSummaryText->GetFont();
		SFont.Size = 12;
		ScoreSummaryText->SetFont(SFont);

		UVerticalBoxSlot* SumSlotRef = ContentBox->AddChildToVerticalBox(ScoreSummaryText);
		if (SumSlotRef) { SumSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Column headers ───────────────────────────────────────────────────────
	{
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		auto AddColHeader = [&](const FString& Label, float Width)
		{
			UTextBlock* Col = WidgetTree->ConstructWidget<UTextBlock>();
			Col->SetText(FText::FromString(Label));
			Col->SetColorAndOpacity(FSlateColor(RankColors::Gold));
			FSlateFontInfo CFont = Col->GetFont();
			CFont.Size = 12;
			CFont.TypefaceFontName = FName(TEXT("Bold"));
			Col->SetFont(CFont);

			USizeBox* ColSize = WidgetTree->ConstructWidget<USizeBox>();
			ColSize->SetWidthOverride(Width);
			ColSize->AddChild(Col);

			UHorizontalBoxSlot* ColSlotRef = HeaderRow->AddChildToHorizontalBox(ColSize);
			if (ColSlotRef) { ColSlotRef->SetPadding(FMargin(2.0f, 0.0f)); }
		};

		AddColHeader(TEXT("#"), 30.f);
		AddColHeader(TEXT("Wizard"), 140.f);
		AddColHeader(TEXT("Score"), 65.f);
		AddColHeader(TEXT("Cities"), 60.f);
		AddColHeader(TEXT("Military"), 70.f);
		AddColHeader(TEXT("Mana"), 60.f);
		AddColHeader(TEXT("Spells"), 60.f);
		AddColHeader(TEXT("Territory"), 75.f);

		UVerticalBoxSlot* HdrSlotRef = ContentBox->AddChildToVerticalBox(HeaderRow);
		if (HdrSlotRef) { HdrSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }
	}

	// ── Scrollable ranking rows ──────────────────────────────────────────────
	{
		RankingScrollBox = WidgetTree->ConstructWidget<UScrollBox>();

		USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
		ListSize->SetHeightOverride(220.0f);
		ListSize->AddChild(RankingScrollBox);

		UVerticalBoxSlot* ListSlotRef = ContentBox->AddChildToVerticalBox(ListSize);
		if (ListSlotRef) { ListSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// ── Close button ─────────────────────────────────────────────────────────
	{
		CloseButton = CreateActionButton(TEXT("Close"), 140.f);

		USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>();
		BtnSize->SetWidthOverride(140.0f);
		BtnSize->SetHeightOverride(38.0f);
		BtnSize->AddChild(CloseButton);

		UVerticalBoxSlot* BtnSlotRef = ContentBox->AddChildToVerticalBox(BtnSize);
		if (BtnSlotRef) { BtnSlotRef->SetHorizontalAlignment(HAlign_Right); }
	}
}

UButton* UCoMWizardRankingsWidget::CreateActionButton(const FString& Label, float Width)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(RankColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(RankColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(RankColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(RankColors::Silver));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 14;
	Text->SetFont(Font);

	Btn->AddChild(Text);
	return Btn;
}

// =============================================================================
// NativeConstruct
// =============================================================================

void UCoMWizardRankingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UCoMWizardRankingsWidget::OnCloseClicked);
	}
}

// =============================================================================
// Public API
// =============================================================================

void UCoMWizardRankingsWidget::RefreshRankings()
{
	if (!RankingScrollBox) { return; }
	RankingScrollBox->ClearChildren();

	// Placeholder data — in production this would read from the game subsystem.
	AddRankingRow(1, TEXT("Merlin"),    FLinearColor(0.2f, 0.3f, 1.0f, 1.0f), 1250, 5, 320, 180, 24, 42, true);
	AddRankingRow(2, TEXT("Rjak"),      FLinearColor(0.8f, 0.1f, 0.1f, 1.0f), 1100, 4, 290, 150, 18, 38, false);
	AddRankingRow(3, TEXT("Sssra"),     FLinearColor(0.1f, 0.7f, 0.1f, 1.0f),  950, 3, 200, 120, 15, 30, false);
	AddRankingRow(4, TEXT("Lo Pan"),    FLinearColor(0.9f, 0.8f, 0.0f, 1.0f),  800, 3, 180, 100, 12, 25, false);
	AddRankingRow(5, TEXT("Sharee"),    FLinearColor(0.6f, 0.0f, 0.6f, 1.0f),  650, 2, 140,  80, 10, 20, false);

	if (ScoreSummaryText)
	{
		ScoreSummaryText->SetText(FText::FromString(
			TEXT("Merlin: 1250  |  Rjak: 1100  |  Sssra: 950  |  Lo Pan: 800  |  Sharee: 650")));
	}
}

void UCoMWizardRankingsWidget::AddRankingRow(int32 Rank, const FString& WizardName,
	const FLinearColor& WizardColor, int32 Score, int32 Cities, int32 Military,
	int32 Mana, int32 Spells, int32 Territory, bool bIsPlayer)
{
	if (!RankingScrollBox) { return; }

	UBorder* RowBg = WidgetTree->ConstructWidget<UBorder>();
	RowBg->SetBrushColor(bIsPlayer ? RankColors::RowHighlight : FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	RowBg->SetPadding(FMargin(2.0f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	RowBg->AddChild(Row);

	auto AddCell = [&](const FString& Val, float Width, const FLinearColor& Color)
	{
		UTextBlock* Cell = WidgetTree->ConstructWidget<UTextBlock>();
		Cell->SetText(FText::FromString(Val));
		Cell->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo CFont = Cell->GetFont();
		CFont.Size = 12;
		Cell->SetFont(CFont);

		USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>();
		CellSize->SetWidthOverride(Width);
		CellSize->AddChild(Cell);

		UHorizontalBoxSlot* CellSlotRef = Row->AddChildToHorizontalBox(CellSize);
		if (CellSlotRef) { CellSlotRef->SetPadding(FMargin(2.0f, 1.0f)); CellSlotRef->SetVerticalAlignment(VAlign_Center); }
	};

	// Rank number
	AddCell(FString::Printf(TEXT("%d"), Rank), 30.f, RankColors::Silver);

	// Wizard color indicator + name
	{
		UBorder* ColorDot = WidgetTree->ConstructWidget<UBorder>();
		ColorDot->SetBrushColor(WizardColor);
		USizeBox* DotSize = WidgetTree->ConstructWidget<USizeBox>();
		DotSize->SetWidthOverride(12.0f);
		DotSize->SetHeightOverride(12.0f);
		DotSize->AddChild(ColorDot);

		UHorizontalBoxSlot* DotSlotRef = Row->AddChildToHorizontalBox(DotSize);
		if (DotSlotRef) { DotSlotRef->SetVerticalAlignment(VAlign_Center); DotSlotRef->SetPadding(FMargin(2.0f, 0.0f, 4.0f, 0.0f)); }

		AddCell(WizardName, 124.f, bIsPlayer ? RankColors::Gold : RankColors::Silver);
	}

	AddCell(FString::Printf(TEXT("%d"), Score), 65.f, RankColors::Silver);
	AddCell(FString::Printf(TEXT("%d"), Cities), 60.f, RankColors::Silver);
	AddCell(FString::Printf(TEXT("%d"), Military), 70.f, RankColors::Silver);
	AddCell(FString::Printf(TEXT("%d"), Mana), 60.f, RankColors::Silver);
	AddCell(FString::Printf(TEXT("%d"), Spells), 60.f, RankColors::Silver);
	AddCell(FString::Printf(TEXT("%d"), Territory), 75.f, RankColors::Silver);

	RankingScrollBox->AddChild(RowBg);
}

void UCoMWizardRankingsWidget::OnCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
