// Copyright Mythforge Studios. All Rights Reserved.
// CoMResourceOverviewWidget.cpp -- Treasury and resource overview implementation.

#include "CoMResourceOverviewWidget.h"

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

namespace ResColors
{
	static const FLinearColor BgDark     = FLinearColor(0.015f, 0.010f, 0.040f, 0.95f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.92f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMResourceOverviewWidget::RebuildWidget()
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

void UCoMResourceOverviewWidget::BuildLayout()
{
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	Background->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = Background;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->AddChild(ScreenOverlay);

	// Gold border panel
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(ResColors::Gold);
	PanelBorder->SetPadding(FMargin(2.0f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(ResColors::PanelBg);
	PanelInner->SetPadding(FMargin(16.0f, 12.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(600.0f);
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

	// ── Header ───────────────────────────────────────────────────────────────
	{
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("Treasury & Resources")));
		HeaderText->SetColorAndOpacity(FSlateColor(ResColors::Gold));
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 22;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);

		UVerticalBoxSlot* HeaderSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HeaderSlotRef) { HeaderSlotRef->SetPadding(FMargin(0, 0, 0, 10)); HeaderSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}

	// ── Summary row ──────────────────────────────────────────────────────────
	{
		SummaryText = WidgetTree->ConstructWidget<UTextBlock>();
		SummaryText->SetText(FText::FromString(TEXT("Total Gold: --  |  Total Mana: --  |  Research: --")));
		SummaryText->SetColorAndOpacity(FSlateColor(ResColors::Gold));
		FSlateFontInfo SumFont = SummaryText->GetFont();
		SumFont.Size = 14;
		SummaryText->SetFont(SumFont);

		UBorder* SumBg = WidgetTree->ConstructWidget<UBorder>();
		SumBg->SetBrushColor(FLinearColor(0.02f, 0.015f, 0.05f, 0.8f));
		SumBg->SetPadding(FMargin(10.0f, 6.0f));
		SumBg->AddChild(SummaryText);

		UVerticalBoxSlot* SumSlotRef = ContentBox->AddChildToVerticalBox(SumBg);
		if (SumSlotRef) { SumSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── City breakdown table (scrollable) ────────────────────────────────────
	{
		CityScrollBox = WidgetTree->ConstructWidget<UScrollBox>();

		USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
		ListSize->SetHeightOverride(240.0f);
		ListSize->AddChild(CityScrollBox);

		UVerticalBoxSlot* ListSlotRef = ContentBox->AddChildToVerticalBox(ListSize);
		if (ListSlotRef) { ListSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Upkeep section ───────────────────────────────────────────────────────
	{
		UpkeepText = WidgetTree->ConstructWidget<UTextBlock>();
		UpkeepText->SetText(FText::FromString(TEXT("Army Upkeep: --  |  Building Upkeep: --  |  Spell Maintenance: --")));
		UpkeepText->SetColorAndOpacity(FSlateColor(ResColors::Grey));
		FSlateFontInfo UpFont = UpkeepText->GetFont();
		UpFont.Size = 12;
		UpkeepText->SetFont(UpFont);

		UVerticalBoxSlot* UpSlotRef = ContentBox->AddChildToVerticalBox(UpkeepText);
		if (UpSlotRef) { UpSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }
	}

	// ── Net income ───────────────────────────────────────────────────────────
	{
		NetIncomeText = WidgetTree->ConstructWidget<UTextBlock>();
		NetIncomeText->SetText(FText::FromString(TEXT("Net Gold: --/turn")));
		NetIncomeText->SetColorAndOpacity(FSlateColor(ResColors::Gold));
		FSlateFontInfo NetFont = NetIncomeText->GetFont();
		NetFont.Size = 14;
		NetFont.TypefaceFontName = FName(TEXT("Bold"));
		NetIncomeText->SetFont(NetFont);

		UVerticalBoxSlot* NetSlotRef = ContentBox->AddChildToVerticalBox(NetIncomeText);
		if (NetSlotRef) { NetSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// ── Close button ─────────────────────────────────────────────────────────
	{
		CloseButton = CreateActionButton(TEXT("Close"), 140.f);

		USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>();
		BtnSize->SetWidthOverride(140.0f);
		BtnSize->SetHeightOverride(36.0f);
		BtnSize->AddChild(CloseButton);

		UVerticalBoxSlot* BtnSlotRef = ContentBox->AddChildToVerticalBox(BtnSize);
		if (BtnSlotRef) { BtnSlotRef->SetHorizontalAlignment(HAlign_Right); }
	}
}

UButton* UCoMResourceOverviewWidget::CreateActionButton(const FString& Label, float Width)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(ResColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(ResColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(ResColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(ResColors::Silver));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 14;
	Text->SetFont(Font);

	Btn->AddChild(Text);
	return Btn;
}

void UCoMResourceOverviewWidget::AddColumnHeaders()
{
	if (!CityScrollBox) { return; }

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();

	auto AddCol = [&](const FString& Label, float Width)
	{
		UTextBlock* Col = WidgetTree->ConstructWidget<UTextBlock>();
		Col->SetText(FText::FromString(Label));
		Col->SetColorAndOpacity(FSlateColor(ResColors::Gold));
		FSlateFontInfo CFont = Col->GetFont();
		CFont.Size = 11;
		CFont.TypefaceFontName = FName(TEXT("Bold"));
		Col->SetFont(CFont);

		USizeBox* ColSize = WidgetTree->ConstructWidget<USizeBox>();
		ColSize->SetWidthOverride(Width);
		ColSize->AddChild(Col);

		UHorizontalBoxSlot* ColSlotRef = HeaderRow->AddChildToHorizontalBox(ColSize);
		if (ColSlotRef) { ColSlotRef->SetPadding(FMargin(2.0f, 0.0f)); }
	};

	AddCol(TEXT("City"), 120.f);
	AddCol(TEXT("Gold"), 55.f);
	AddCol(TEXT("Food"), 55.f);
	AddCol(TEXT("Prod"), 55.f);
	AddCol(TEXT("Mana"), 55.f);
	AddCol(TEXT("Res"), 55.f);
	AddCol(TEXT("Pop"), 50.f);

	CityScrollBox->AddChild(HeaderRow);
}

void UCoMResourceOverviewWidget::AddCityRow(const FString& CityName, int32 GoldVal, int32 Food,
	int32 Production, int32 Mana, int32 Research, int32 Pop, bool bIsTotalRow)
{
	if (!CityScrollBox) { return; }

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	const FLinearColor NameColor = ResColors::Gold;
	const FLinearColor ValColor = bIsTotalRow ? ResColors::Gold : ResColors::Silver;
	const int32 FontSize = bIsTotalRow ? 12 : 11;

	auto AddCell = [&](const FString& Val, float Width, const FLinearColor& Color)
	{
		UTextBlock* Cell = WidgetTree->ConstructWidget<UTextBlock>();
		Cell->SetText(FText::FromString(Val));
		Cell->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo CFont = Cell->GetFont();
		CFont.Size = FontSize;
		if (bIsTotalRow) { CFont.TypefaceFontName = FName(TEXT("Bold")); }
		Cell->SetFont(CFont);

		USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>();
		CellSize->SetWidthOverride(Width);
		CellSize->AddChild(Cell);

		UHorizontalBoxSlot* CellSlotRef = Row->AddChildToHorizontalBox(CellSize);
		if (CellSlotRef) { CellSlotRef->SetPadding(FMargin(2.0f, 1.0f)); }
	};

	AddCell(CityName, 120.f, NameColor);
	AddCell(FString::Printf(TEXT("%d"), GoldVal), 55.f, ValColor);
	AddCell(FString::Printf(TEXT("%d"), Food), 55.f, ValColor);
	AddCell(FString::Printf(TEXT("%d"), Production), 55.f, ValColor);
	AddCell(FString::Printf(TEXT("%d"), Mana), 55.f, ValColor);
	AddCell(FString::Printf(TEXT("%d"), Research), 55.f, ValColor);
	AddCell(FString::Printf(TEXT("%d"), Pop), 50.f, ValColor);

	CityScrollBox->AddChild(Row);
}

// =============================================================================
// NativeConstruct
// =============================================================================

void UCoMResourceOverviewWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UCoMResourceOverviewWidget::OnCloseClicked);
	}
}

// =============================================================================
// Public API
// =============================================================================

void UCoMResourceOverviewWidget::RefreshOverview(int32 WizardId)
{
	CurrentWizardId = WizardId;

	// Placeholder data — in production read from economy subsystem
	if (SummaryText)
	{
		SummaryText->SetText(FText::FromString(
			TEXT("Total Gold: 1250 (+45/turn)  |  Total Mana: 380 (+22/turn)  |  Research: +18/turn")));
	}

	if (CityScrollBox)
	{
		CityScrollBox->ClearChildren();
		AddColumnHeaders();
		AddCityRow(TEXT("Arcanopolis"), 25, 12, 18, 10, 8, 15);
		AddCityRow(TEXT("Shadowgate"),  18, 8, 12, 14, 6, 10);
		AddCityRow(TEXT("Verdania"),    12, 15, 8, 4, 4, 8);

		// Separator
		UBorder* Sep = WidgetTree->ConstructWidget<UBorder>();
		Sep->SetBrushColor(ResColors::GoldDim);
		USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>();
		SepSize->SetHeightOverride(1.0f);
		SepSize->AddChild(Sep);
		CityScrollBox->AddChild(SepSize);

		AddCityRow(TEXT("TOTAL"), 55, 35, 38, 28, 18, 33, true);
	}

	if (UpkeepText)
	{
		UpkeepText->SetText(FText::FromString(
			TEXT("Army Upkeep: -8  |  Building Upkeep: -4  |  Spell Maintenance: -3")));
	}

	if (NetIncomeText)
	{
		NetIncomeText->SetText(FText::FromString(TEXT("Net Gold: +30/turn")));
	}
}

void UCoMResourceOverviewWidget::OnCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
