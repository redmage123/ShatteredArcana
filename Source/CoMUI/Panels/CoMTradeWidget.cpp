// Copyright Mythforge Studios. All Rights Reserved.
// CoMTradeWidget.cpp -- Trade route management implementation.

#include "CoMTradeWidget.h"

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

namespace TradeColours
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
	static const FLinearColor Green      = FLinearColor(0.2f, 0.7f, 0.3f, 1.0f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMTradeWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMTradeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton)        { CloseButton->OnClicked.AddDynamic(this, &UCoMTradeWidget::OnCloseClicked); }
	if (ProposeTradeButton) { ProposeTradeButton->OnClicked.AddDynamic(this, &UCoMTradeWidget::OnProposeTradeClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMTradeWidget::SetWizardId(int32 WizardId)
{
	CurrentWizardId = WizardId;

	if (!TradeListScrollBox) { return; }
	TradeListScrollBox->ClearChildren();

	// Placeholder trade routes
	struct FTradeRoute { FString Partner; FString Goods; int32 Income; };
	TArray<FTradeRoute> Routes = {
		{ TEXT("Wizard 3"),  TEXT("Iron Ore, Gems"),     45 },
		{ TEXT("Wizard 5"),  TEXT("Mana Crystals"),       30 },
		{ TEXT("Wizard 4"),  TEXT("Food, Lumber"),        25 },
	};

	// Column headers
	{
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		auto AddColHeader = [&](const FString& Text, float FillWeight)
		{
			UTextBlock* Col = WidgetTree->ConstructWidget<UTextBlock>();
			Col->SetText(FText::FromString(Text));
			Col->SetColorAndOpacity(FSlateColor(TradeColours::Gold));
			FSlateFontInfo CFont = Col->GetFont();
			CFont.Size = 12;
			CFont.TypefaceFontName = FName(TEXT("Bold"));
			Col->SetFont(CFont);
			UHorizontalBoxSlot* CSlotRef = HeaderRow->AddChildToHorizontalBox(Col);
			if (CSlotRef)
			{
				CSlotRef->SetVerticalAlignment(VAlign_Center);
				if (FillWeight > 0.f) { CSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
				CSlotRef->SetPadding(FMargin(4, 0));
			}
		};

		AddColHeader(TEXT("Partner"), 1.0f);
		AddColHeader(TEXT("Goods"), 1.5f);
		AddColHeader(TEXT("Income/Turn"), 0.0f);

		UBorder* HdrBorder = WidgetTree->ConstructWidget<UBorder>();
		HdrBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		HdrBorder->SetPadding(FMargin(4.0f, 4.0f));
		HdrBorder->AddChild(HeaderRow);
		TradeListScrollBox->AddChild(HdrBorder);
	}

	for (const auto& Route : Routes)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Partner
		UTextBlock* PartnerText = WidgetTree->ConstructWidget<UTextBlock>();
		PartnerText->SetText(FText::FromString(Route.Partner));
		PartnerText->SetColorAndOpacity(FSlateColor(TradeColours::Silver));
		FSlateFontInfo PFont = PartnerText->GetFont();
		PFont.Size = 13;
		PartnerText->SetFont(PFont);
		UHorizontalBoxSlot* PSlotRef = Row->AddChildToHorizontalBox(PartnerText);
		if (PSlotRef) { PSlotRef->SetVerticalAlignment(VAlign_Center); PSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); PSlotRef->SetPadding(FMargin(4, 0)); }

		// Goods
		UTextBlock* GoodsText = WidgetTree->ConstructWidget<UTextBlock>();
		GoodsText->SetText(FText::FromString(Route.Goods));
		GoodsText->SetColorAndOpacity(FSlateColor(TradeColours::Grey));
		FSlateFontInfo GFont = GoodsText->GetFont();
		GFont.Size = 12;
		GoodsText->SetFont(GFont);
		UHorizontalBoxSlot* GSlotRef = Row->AddChildToHorizontalBox(GoodsText);
		if (GSlotRef) { GSlotRef->SetVerticalAlignment(VAlign_Center); GSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); GSlotRef->SetPadding(FMargin(4, 0)); }

		// Income
		UTextBlock* IncomeText = WidgetTree->ConstructWidget<UTextBlock>();
		IncomeText->SetText(FText::FromString(FString::Printf(TEXT("+%d gold"), Route.Income)));
		IncomeText->SetColorAndOpacity(FSlateColor(TradeColours::Green));
		FSlateFontInfo IFont = IncomeText->GetFont();
		IFont.Size = 12;
		IncomeText->SetFont(IFont);
		UHorizontalBoxSlot* ISlotRef = Row->AddChildToHorizontalBox(IncomeText);
		if (ISlotRef) { ISlotRef->SetVerticalAlignment(VAlign_Center); ISlotRef->SetPadding(FMargin(4, 0)); }

		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>();
		RowBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		RowBorder->SetPadding(FMargin(4.0f, 3.0f));
		RowBorder->AddChild(Row);
		TradeListScrollBox->AddChild(RowBorder);
	}
}

void UCoMTradeWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

void UCoMTradeWidget::OnProposeTradeClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Trade: Propose Trade clicked"));
}

// =============================================================================
// Helpers
// =============================================================================

UButton* UCoMTradeWidget::CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(Width);
	SizeBox->SetHeightOverride(36.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(TradeColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(TradeColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(TradeColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(TradeColours::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(TradeColours::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 13;
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UVerticalBoxSlot* SlotRef = Parent->AddChildToVerticalBox(SizeBox);
	if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 4)); }

	return Button;
}

// =============================================================================
// Layout
// =============================================================================

void UCoMTradeWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(TradeColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(ScreenOverlay);

	// Center panel
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(TradeColours::GoldDim);
	PanelBorder->SetPadding(FMargin(1.5f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(TradeColours::PanelBg);
	PanelInner->SetPadding(FMargin(24.0f, 16.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(650.0f);
	PanelSize->SetHeightOverride(450.0f);
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
		HeaderText->SetText(FText::FromString(TEXT("Trade Routes")));
		HeaderText->SetColorAndOpacity(FSlateColor(TradeColours::Gold));
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
		Div->SetBrushColor(TradeColours::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(1.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* DivSlotRef = ContentBox->AddChildToVerticalBox(DivSize);
		if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); DivSlotRef->SetPadding(FMargin(0, 0, 0, 12)); }
	}

	// Scrollable trade route list
	TradeListScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	UVerticalBoxSlot* ScrollSlotRef = ContentBox->AddChildToVerticalBox(TradeListScrollBox);
	if (ScrollSlotRef) { ScrollSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

	// Propose Trade button
	ProposeTradeButton = CreateStyledButton(ContentBox, TEXT("Propose Trade"), 180.0f);

	// Close button
	CloseButton = CreateStyledButton(ContentBox, TEXT("Close"), 140.0f);
}
