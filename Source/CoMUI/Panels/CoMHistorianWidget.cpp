// Copyright Mythforge Studios. All Rights Reserved.
// CoMHistorianWidget.cpp -- Events log / game history implementation.

#include "CoMHistorianWidget.h"

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

namespace HistColours
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

TSharedRef<SWidget> UCoMHistorianWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMHistorianWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton)              { CloseButton->OnClicked.AddDynamic(this, &UCoMHistorianWidget::OnCloseClicked); }
	if (FilterAllButton)          { FilterAllButton->OnClicked.AddDynamic(this, &UCoMHistorianWidget::OnFilterAllClicked); }
	if (FilterWarsButton)         { FilterWarsButton->OnClicked.AddDynamic(this, &UCoMHistorianWidget::OnFilterWarsClicked); }
	if (FilterDiscoveriesButton)  { FilterDiscoveriesButton->OnClicked.AddDynamic(this, &UCoMHistorianWidget::OnFilterDiscoveriesClicked); }
	if (FilterEventsButton)       { FilterEventsButton->OnClicked.AddDynamic(this, &UCoMHistorianWidget::OnFilterEventsClicked); }
	if (FilterDiplomacyButton)    { FilterDiplomacyButton->OnClicked.AddDynamic(this, &UCoMHistorianWidget::OnFilterDiplomacyClicked); }

	RefreshLog();
}

// =============================================================================
// Public API
// =============================================================================

void UCoMHistorianWidget::RefreshLog()
{
	if (!EventLogScrollBox) { return; }
	EventLogScrollBox->ClearChildren();

	// Placeholder event entries
	struct FLogEntry { int32 Turn; FString Category; FString Description; FString Plane; };
	TArray<FLogEntry> Entries = {
		{ 1,  TEXT("Events"),      TEXT("The Age of Shattered Arcana begins."),               TEXT("Aurelith") },
		{ 3,  TEXT("Discoveries"), TEXT("Ancient ruins discovered in the Verdant Wilds."),    TEXT("Verdantis") },
		{ 5,  TEXT("Wars"),        TEXT("Wizard 2 declares war on Wizard 4."),                TEXT("Noctharion") },
		{ 7,  TEXT("Diplomacy"),   TEXT("Trade agreement signed with Wizard 3."),             TEXT("Aurelith") },
		{ 10, TEXT("Wars"),        TEXT("Battle of the Iron Gates — Wizard 2 victorious."),   TEXT("Infernyx") },
		{ 12, TEXT("Events"),      TEXT("Planar rift opens near the Abyssal frontier."),      TEXT("Abyssal") },
		{ 15, TEXT("Discoveries"), TEXT("Lost grimoire recovered from Ethereal wastes."),     TEXT("Ethereal") },
		{ 18, TEXT("Diplomacy"),   TEXT("Alliance formed between You and Wizard 5."),         TEXT("Aethermist") },
	};

	for (const auto& Entry : Entries)
	{
		if (CurrentFilter != TEXT("All") && Entry.Category != CurrentFilter) { continue; }

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Turn number
		UTextBlock* TurnText = WidgetTree->ConstructWidget<UTextBlock>();
		TurnText->SetText(FText::FromString(FString::Printf(TEXT("T%d"), Entry.Turn)));
		TurnText->SetColorAndOpacity(FSlateColor(HistColours::Gold));
		FSlateFontInfo TFont = TurnText->GetFont();
		TFont.Size = 12;
		TurnText->SetFont(TFont);
		USizeBox* TurnSize = WidgetTree->ConstructWidget<USizeBox>();
		TurnSize->SetWidthOverride(40.0f);
		TurnSize->AddChild(TurnText);
		UHorizontalBoxSlot* TSlotRef = Row->AddChildToHorizontalBox(TurnSize);
		if (TSlotRef) { TSlotRef->SetVerticalAlignment(VAlign_Center); }

		// Description
		UTextBlock* DescText = WidgetTree->ConstructWidget<UTextBlock>();
		DescText->SetText(FText::FromString(Entry.Description));
		DescText->SetColorAndOpacity(FSlateColor(HistColours::Silver));
		DescText->SetAutoWrapText(true);
		FSlateFontInfo DFont = DescText->GetFont();
		DFont.Size = 12;
		DescText->SetFont(DFont);
		UHorizontalBoxSlot* DSlotRef = Row->AddChildToHorizontalBox(DescText);
		if (DSlotRef) { DSlotRef->SetVerticalAlignment(VAlign_Center); DSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); DSlotRef->SetPadding(FMargin(8, 0)); }

		// Plane
		UTextBlock* PlaneText = WidgetTree->ConstructWidget<UTextBlock>();
		PlaneText->SetText(FText::FromString(Entry.Plane));
		PlaneText->SetColorAndOpacity(FSlateColor(HistColours::Grey));
		FSlateFontInfo PFont = PlaneText->GetFont();
		PFont.Size = 11;
		PlaneText->SetFont(PFont);
		UHorizontalBoxSlot* PSlotRef = Row->AddChildToHorizontalBox(PlaneText);
		if (PSlotRef) { PSlotRef->SetVerticalAlignment(VAlign_Center); }

		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>();
		RowBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		RowBorder->SetPadding(FMargin(4.0f, 3.0f));
		RowBorder->AddChild(Row);

		EventLogScrollBox->AddChild(RowBorder);
	}
}

// =============================================================================
// Button callbacks
// =============================================================================

void UCoMHistorianWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

void UCoMHistorianWidget::OnFilterAllClicked()          { CurrentFilter = TEXT("All");         RefreshLog(); }
void UCoMHistorianWidget::OnFilterWarsClicked()         { CurrentFilter = TEXT("Wars");        RefreshLog(); }
void UCoMHistorianWidget::OnFilterDiscoveriesClicked()  { CurrentFilter = TEXT("Discoveries"); RefreshLog(); }
void UCoMHistorianWidget::OnFilterEventsClicked()       { CurrentFilter = TEXT("Events");      RefreshLog(); }
void UCoMHistorianWidget::OnFilterDiplomacyClicked()    { CurrentFilter = TEXT("Diplomacy");   RefreshLog(); }

// =============================================================================
// Helpers
// =============================================================================

UButton* UCoMHistorianWidget::CreateFilterButton(UHorizontalBox* Parent, const FString& Label)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(100.0f);
	SizeBox->SetHeightOverride(30.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(HistColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(HistColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(HistColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(HistColours::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(HistColours::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 11;
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UHorizontalBoxSlot* SlotRef = Parent->AddChildToHorizontalBox(SizeBox);
	if (SlotRef) { SlotRef->SetVerticalAlignment(VAlign_Center); SlotRef->SetPadding(FMargin(3, 0)); }

	return Button;
}

UButton* UCoMHistorianWidget::CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(Width);
	SizeBox->SetHeightOverride(36.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(HistColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(HistColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(HistColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(HistColours::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(HistColours::Silver));
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

void UCoMHistorianWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(HistColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(ScreenOverlay);

	// Center panel
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(HistColours::GoldDim);
	PanelBorder->SetPadding(FMargin(1.5f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(HistColours::PanelBg);
	PanelInner->SetPadding(FMargin(24.0f, 16.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(750.0f);
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
		HeaderText->SetText(FText::FromString(TEXT("Chronicle of Ages")));
		HeaderText->SetColorAndOpacity(FSlateColor(HistColours::Gold));
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
		Div->SetBrushColor(HistColours::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(1.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* DivSlotRef = ContentBox->AddChildToVerticalBox(DivSize);
		if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); DivSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// Filter buttons row
	{
		FilterRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		FilterAllButton         = CreateFilterButton(FilterRow, TEXT("All"));
		FilterWarsButton        = CreateFilterButton(FilterRow, TEXT("Wars"));
		FilterDiscoveriesButton = CreateFilterButton(FilterRow, TEXT("Discoveries"));
		FilterEventsButton      = CreateFilterButton(FilterRow, TEXT("Events"));
		FilterDiplomacyButton   = CreateFilterButton(FilterRow, TEXT("Diplomacy"));

		UVerticalBoxSlot* FilterSlotRef = ContentBox->AddChildToVerticalBox(FilterRow);
		if (FilterSlotRef) { FilterSlotRef->SetHorizontalAlignment(HAlign_Center); FilterSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// Scrollable event list
	EventLogScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	UVerticalBoxSlot* ScrollSlotRef = ContentBox->AddChildToVerticalBox(EventLogScrollBox);
	if (ScrollSlotRef) { ScrollSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

	// Close button
	CloseButton = CreateStyledButton(ContentBox, TEXT("Close"), 140.0f);
}
