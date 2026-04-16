// Copyright Mythforge Studios. All Rights Reserved.
// CoMNotificationCenterWidget.cpp -- Notification center implementation.

#include "CoMNotificationCenterWidget.h"

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

namespace NotifColors
{
	static const FLinearColor BgDark     = FLinearColor(0.015f, 0.010f, 0.040f, 0.95f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);

	// Category colors
	static const FLinearColor Combat     = FLinearColor(0.9f, 0.15f, 0.1f, 1.0f);
	static const FLinearColor City       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Research   = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);
	static const FLinearColor Diplomacy  = FLinearColor(0.1f, 0.75f, 0.2f, 1.0f);
	static const FLinearColor Event      = FLinearColor(0.6f, 0.2f, 0.8f, 1.0f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMNotificationCenterWidget::RebuildWidget()
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

void UCoMNotificationCenterWidget::BuildLayout()
{
	// Panel anchored to right side (300x600)
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	Background->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = Background;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->AddChild(ScreenOverlay);

	// Panel with gold border, positioned right
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(NotifColors::Gold);
	PanelBorder->SetPadding(FMargin(2.0f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(NotifColors::PanelBg);
	PanelInner->SetPadding(FMargin(12.0f, 10.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(300.0f);
	PanelSize->SetHeightOverride(600.0f);
	PanelSize->AddChild(PanelBorder);

	UOverlaySlot* PanelSlotRef = ScreenOverlay->AddChildToOverlay(PanelSize);
	if (PanelSlotRef)
	{
		PanelSlotRef->SetHorizontalAlignment(HAlign_Right);
		PanelSlotRef->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelInner->AddChild(ContentBox);

	// ── Header ───────────────────────────────────────────────────────────────
	{
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("Notifications")));
		HeaderText->SetColorAndOpacity(FSlateColor(NotifColors::Gold));
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 20;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);

		UVerticalBoxSlot* HeaderSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HeaderSlotRef) { HeaderSlotRef->SetPadding(FMargin(0, 0, 0, 8)); HeaderSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}

	// ── Filter buttons ───────────────────────────────────────────────────────
	{
		UHorizontalBox* FilterRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		FilterAllBtn       = CreateFilterButton(TEXT("All"), FilterRow);
		FilterCombatBtn    = CreateFilterButton(TEXT("Combat"), FilterRow);
		FilterCityBtn      = CreateFilterButton(TEXT("City"), FilterRow);
		FilterResearchBtn  = CreateFilterButton(TEXT("Res"), FilterRow);
		FilterDiplomacyBtn = CreateFilterButton(TEXT("Dip"), FilterRow);

		UVerticalBoxSlot* FilterSlotRef = ContentBox->AddChildToVerticalBox(FilterRow);
		if (FilterSlotRef) { FilterSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Scrollable notification list ─────────────────────────────────────────
	{
		NotifScrollBox = WidgetTree->ConstructWidget<UScrollBox>();

		USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
		ListSize->SetHeightOverride(430.0f);
		ListSize->AddChild(NotifScrollBox);

		UVerticalBoxSlot* ListSlotRef = ContentBox->AddChildToVerticalBox(ListSize);
		if (ListSlotRef) { ListSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Bottom buttons: Clear All + Close ────────────────────────────────────
	{
		UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		ClearAllButton = CreateActionButton(TEXT("Clear All"), 110.f);
		USizeBox* ClearSize = WidgetTree->ConstructWidget<USizeBox>();
		ClearSize->SetWidthOverride(110.0f);
		ClearSize->SetHeightOverride(34.0f);
		ClearSize->AddChild(ClearAllButton);

		UHorizontalBoxSlot* ClearSlotRef = ButtonRow->AddChildToHorizontalBox(ClearSize);
		if (ClearSlotRef) { ClearSlotRef->SetPadding(FMargin(0, 0, 8, 0)); }

		CloseButton = CreateActionButton(TEXT("Close"), 110.f);
		USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>();
		CloseSize->SetWidthOverride(110.0f);
		CloseSize->SetHeightOverride(34.0f);
		CloseSize->AddChild(CloseButton);

		UHorizontalBoxSlot* CloseSlotRef = ButtonRow->AddChildToHorizontalBox(CloseSize);
		if (CloseSlotRef) { CloseSlotRef->SetHorizontalAlignment(HAlign_Right); }

		UVerticalBoxSlot* BtnRowSlotRef = ContentBox->AddChildToVerticalBox(ButtonRow);
		if (BtnRowSlotRef) { BtnRowSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}
}

UButton* UCoMNotificationCenterWidget::CreateFilterButton(const FString& Label, UHorizontalBox* Parent)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(NotifColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(NotifColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(NotifColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(NotifColors::Silver));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 10;
	Text->SetFont(Font);

	Btn->AddChild(Text);

	UHorizontalBoxSlot* BtnSlotRef = Parent->AddChildToHorizontalBox(Btn);
	if (BtnSlotRef) { BtnSlotRef->SetPadding(FMargin(0, 0, 4, 0)); }

	return Btn;
}

UButton* UCoMNotificationCenterWidget::CreateActionButton(const FString& Label, float Width)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(NotifColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(NotifColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(NotifColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(NotifColors::Silver));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 12;
	Text->SetFont(Font);

	Btn->AddChild(Text);
	return Btn;
}

// =============================================================================
// NativeConstruct
// =============================================================================

void UCoMNotificationCenterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)         { CloseButton->OnClicked.AddDynamic(this, &UCoMNotificationCenterWidget::OnCloseClicked); }
	if (ClearAllButton)      { ClearAllButton->OnClicked.AddDynamic(this, &UCoMNotificationCenterWidget::OnClearAllClicked); }
	if (FilterAllBtn)        { FilterAllBtn->OnClicked.AddDynamic(this, &UCoMNotificationCenterWidget::OnFilterAllClicked); }
	if (FilterCombatBtn)     { FilterCombatBtn->OnClicked.AddDynamic(this, &UCoMNotificationCenterWidget::OnFilterCombatClicked); }
	if (FilterCityBtn)       { FilterCityBtn->OnClicked.AddDynamic(this, &UCoMNotificationCenterWidget::OnFilterCityClicked); }
	if (FilterResearchBtn)   { FilterResearchBtn->OnClicked.AddDynamic(this, &UCoMNotificationCenterWidget::OnFilterResearchClicked); }
	if (FilterDiplomacyBtn)  { FilterDiplomacyBtn->OnClicked.AddDynamic(this, &UCoMNotificationCenterWidget::OnFilterDiplomacyClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMNotificationCenterWidget::AddNotification(int32 Turn, const FString& Category, const FString& Message)
{
	FNotifEntry Entry;
	Entry.Turn = Turn;
	Entry.Category = Category;
	Entry.Message = Message;

	// Insert at front (newest first)
	AllNotifications.Insert(Entry, 0);
	RefreshList();
}

void UCoMNotificationCenterWidget::FilterByCategory(const FString& Category)
{
	ActiveFilter = Category;
	RefreshList();
}

void UCoMNotificationCenterWidget::RefreshList()
{
	if (!NotifScrollBox) { return; }
	NotifScrollBox->ClearChildren();

	for (const FNotifEntry& Entry : AllNotifications)
	{
		if (ActiveFilter != TEXT("All") && Entry.Category != ActiveFilter)
		{
			continue;
		}

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Category color dot
		FLinearColor CatColor = GetNotifCategoryColor(Entry.Category);
		UBorder* Dot = WidgetTree->ConstructWidget<UBorder>();
		Dot->SetBrushColor(CatColor);
		USizeBox* DotSize = WidgetTree->ConstructWidget<USizeBox>();
		DotSize->SetWidthOverride(8.0f);
		DotSize->SetHeightOverride(8.0f);
		DotSize->AddChild(Dot);

		UHorizontalBoxSlot* DotSlotRef = Row->AddChildToHorizontalBox(DotSize);
		if (DotSlotRef) { DotSlotRef->SetVerticalAlignment(VAlign_Center); DotSlotRef->SetPadding(FMargin(0, 2, 6, 2)); }

		// Turn number
		UTextBlock* TurnText = WidgetTree->ConstructWidget<UTextBlock>();
		TurnText->SetText(FText::FromString(FString::Printf(TEXT("T%d"), Entry.Turn)));
		TurnText->SetColorAndOpacity(FSlateColor(NotifColors::Grey));
		FSlateFontInfo TurnFont = TurnText->GetFont();
		TurnFont.Size = 10;
		TurnText->SetFont(TurnFont);

		USizeBox* TurnSize = WidgetTree->ConstructWidget<USizeBox>();
		TurnSize->SetWidthOverride(30.0f);
		TurnSize->AddChild(TurnText);

		UHorizontalBoxSlot* TurnSlotRef = Row->AddChildToHorizontalBox(TurnSize);
		if (TurnSlotRef) { TurnSlotRef->SetVerticalAlignment(VAlign_Center); TurnSlotRef->SetPadding(FMargin(0, 0, 4, 0)); }

		// Message text
		UTextBlock* MsgText = WidgetTree->ConstructWidget<UTextBlock>();
		MsgText->SetText(FText::FromString(Entry.Message));
		MsgText->SetColorAndOpacity(FSlateColor(NotifColors::Silver));
		MsgText->SetAutoWrapText(true);
		FSlateFontInfo MsgFont = MsgText->GetFont();
		MsgFont.Size = 11;
		MsgText->SetFont(MsgFont);

		UHorizontalBoxSlot* MsgSlotRef = Row->AddChildToHorizontalBox(MsgText);
		if (MsgSlotRef) { MsgSlotRef->SetVerticalAlignment(VAlign_Center); MsgSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

		NotifScrollBox->AddChild(Row);

		// Thin separator
		USpacer* Sp = WidgetTree->ConstructWidget<USpacer>();
		Sp->SetSize(FVector2D(0.0f, 3.0f));
		NotifScrollBox->AddChild(Sp);
	}
}

FLinearColor UCoMNotificationCenterWidget::GetNotifCategoryColor(const FString& Category)
{
	if (Category == TEXT("Combat"))    return NotifColors::Combat;
	if (Category == TEXT("City"))      return NotifColors::City;
	if (Category == TEXT("Research"))  return NotifColors::Research;
	if (Category == TEXT("Diplomacy")) return NotifColors::Diplomacy;
	if (Category == TEXT("Event"))     return NotifColors::Event;
	return NotifColors::Silver;
}

void UCoMNotificationCenterWidget::OnCloseClicked()          { SetVisibility(ESlateVisibility::Collapsed); }
void UCoMNotificationCenterWidget::OnClearAllClicked()       { AllNotifications.Empty(); RefreshList(); }
void UCoMNotificationCenterWidget::OnFilterAllClicked()      { FilterByCategory(TEXT("All")); }
void UCoMNotificationCenterWidget::OnFilterCombatClicked()   { FilterByCategory(TEXT("Combat")); }
void UCoMNotificationCenterWidget::OnFilterCityClicked()     { FilterByCategory(TEXT("City")); }
void UCoMNotificationCenterWidget::OnFilterResearchClicked() { FilterByCategory(TEXT("Research")); }
void UCoMNotificationCenterWidget::OnFilterDiplomacyClicked(){ FilterByCategory(TEXT("Diplomacy")); }
