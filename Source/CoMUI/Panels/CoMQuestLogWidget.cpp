// Copyright Mythforge Studios. All Rights Reserved.
// CoMQuestLogWidget.cpp -- Quest journal implementation.

#include "CoMQuestLogWidget.h"

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
#include "Components/ProgressBar.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"

namespace QuestColors
{
	static const FLinearColor BgDark     = FLinearColor(0.015f, 0.010f, 0.040f, 0.95f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.92f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor ProgressFill = FLinearColor(0.2f, 0.6f, 0.3f, 1.0f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMQuestLogWidget::RebuildWidget()
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

void UCoMQuestLogWidget::BuildLayout()
{
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	Background->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = Background;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->AddChild(ScreenOverlay);

	// Gold border panel
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(QuestColors::Gold);
	PanelBorder->SetPadding(FMargin(2.0f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(QuestColors::PanelBg);
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
		HeaderText->SetText(FText::FromString(TEXT("Quest Journal")));
		HeaderText->SetColorAndOpacity(FSlateColor(QuestColors::Gold));
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 22;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);

		UVerticalBoxSlot* HeaderSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HeaderSlotRef) { HeaderSlotRef->SetPadding(FMargin(0, 0, 0, 8)); HeaderSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}

	// ── Tabs: Active / Completed / Failed ────────────────────────────────────
	{
		UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		ActiveTab    = CreateTabButton(TEXT("Active"));
		CompletedTab = CreateTabButton(TEXT("Completed"));
		FailedTab    = CreateTabButton(TEXT("Failed"));

		UHorizontalBoxSlot* A = TabRow->AddChildToHorizontalBox(ActiveTab);
		if (A) { A->SetPadding(FMargin(0, 0, 6, 0)); }
		UHorizontalBoxSlot* C = TabRow->AddChildToHorizontalBox(CompletedTab);
		if (C) { C->SetPadding(FMargin(0, 0, 6, 0)); }
		TabRow->AddChildToHorizontalBox(FailedTab);

		UVerticalBoxSlot* TabSlotRef = ContentBox->AddChildToVerticalBox(TabRow);
		if (TabSlotRef) { TabSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Quest list (scrollable) ──────────────────────────────────────────────
	{
		QuestScrollBox = WidgetTree->ConstructWidget<UScrollBox>();

		USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
		ListSize->SetHeightOverride(260.0f);
		ListSize->AddChild(QuestScrollBox);

		UVerticalBoxSlot* ListSlotRef = ContentBox->AddChildToVerticalBox(ListSize);
		if (ListSlotRef) { ListSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Selected quest detail ────────────────────────────────────────────────
	{
		UBorder* DetailBg = WidgetTree->ConstructWidget<UBorder>();
		DetailBg->SetBrushColor(FLinearColor(0.02f, 0.015f, 0.05f, 0.8f));
		DetailBg->SetPadding(FMargin(10.0f, 6.0f));

		UVerticalBox* DetailBox = WidgetTree->ConstructWidget<UVerticalBox>();
		DetailBg->AddChild(DetailBox);

		DetailNameText = WidgetTree->ConstructWidget<UTextBlock>();
		DetailNameText->SetText(FText::FromString(TEXT("Select a quest")));
		DetailNameText->SetColorAndOpacity(FSlateColor(QuestColors::Gold));
		FSlateFontInfo NameFont = DetailNameText->GetFont();
		NameFont.Size = 14;
		DetailNameText->SetFont(NameFont);
		UVerticalBoxSlot* NameSlotRef = DetailBox->AddChildToVerticalBox(DetailNameText);
		if (NameSlotRef) { NameSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

		DetailDescText = WidgetTree->ConstructWidget<UTextBlock>();
		DetailDescText->SetText(FText::GetEmpty());
		DetailDescText->SetColorAndOpacity(FSlateColor(QuestColors::Silver));
		DetailDescText->SetAutoWrapText(true);
		FSlateFontInfo DescFont = DetailDescText->GetFont();
		DescFont.Size = 12;
		DetailDescText->SetFont(DescFont);
		UVerticalBoxSlot* DescSlotRef = DetailBox->AddChildToVerticalBox(DetailDescText);
		if (DescSlotRef) { DescSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

		DetailRewardText = WidgetTree->ConstructWidget<UTextBlock>();
		DetailRewardText->SetText(FText::GetEmpty());
		DetailRewardText->SetColorAndOpacity(FSlateColor(QuestColors::Gold));
		FSlateFontInfo RewardFont = DetailRewardText->GetFont();
		RewardFont.Size = 11;
		DetailRewardText->SetFont(RewardFont);
		DetailBox->AddChildToVerticalBox(DetailRewardText);

		UVerticalBoxSlot* DetailSlotRef = ContentBox->AddChildToVerticalBox(DetailBg);
		if (DetailSlotRef) { DetailSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
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

UButton* UCoMQuestLogWidget::CreateTabButton(const FString& Label)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(QuestColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(QuestColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(QuestColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(QuestColors::Gold));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 12;
	Text->SetFont(Font);

	Btn->AddChild(Text);
	return Btn;
}

UButton* UCoMQuestLogWidget::CreateActionButton(const FString& Label, float Width)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(QuestColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(QuestColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(QuestColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(QuestColors::Silver));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 14;
	Text->SetFont(Font);

	Btn->AddChild(Text);
	return Btn;
}

void UCoMQuestLogWidget::AddQuestEntry(const FString& Name, const FString& Description,
	const FString& Objective, float Progress, const FString& Reward)
{
	if (!QuestScrollBox) { return; }

	UBorder* EntryBg = WidgetTree->ConstructWidget<UBorder>();
	EntryBg->SetBrushColor(FLinearColor(0.02f, 0.015f, 0.05f, 0.5f));
	EntryBg->SetPadding(FMargin(8.0f, 6.0f));

	UVerticalBox* EntryBox = WidgetTree->ConstructWidget<UVerticalBox>();
	EntryBg->AddChild(EntryBox);

	// Quest name
	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>();
	NameText->SetText(FText::FromString(Name));
	NameText->SetColorAndOpacity(FSlateColor(QuestColors::Gold));
	FSlateFontInfo NameFont = NameText->GetFont();
	NameFont.Size = 14;
	NameText->SetFont(NameFont);
	UVerticalBoxSlot* NameSlotRef = EntryBox->AddChildToVerticalBox(NameText);
	if (NameSlotRef) { NameSlotRef->SetPadding(FMargin(0, 0, 0, 2)); }

	// Description
	UTextBlock* DescText = WidgetTree->ConstructWidget<UTextBlock>();
	DescText->SetText(FText::FromString(Description));
	DescText->SetColorAndOpacity(FSlateColor(QuestColors::Silver));
	DescText->SetAutoWrapText(true);
	FSlateFontInfo DescFont = DescText->GetFont();
	DescFont.Size = 11;
	DescText->SetFont(DescFont);
	UVerticalBoxSlot* DescSlotRef = EntryBox->AddChildToVerticalBox(DescText);
	if (DescSlotRef) { DescSlotRef->SetPadding(FMargin(0, 0, 0, 2)); }

	// Objective
	UTextBlock* ObjText = WidgetTree->ConstructWidget<UTextBlock>();
	ObjText->SetText(FText::FromString(Objective));
	ObjText->SetColorAndOpacity(FSlateColor(QuestColors::Grey));
	FSlateFontInfo ObjFont = ObjText->GetFont();
	ObjFont.Size = 10;
	ObjText->SetFont(ObjFont);
	EntryBox->AddChildToVerticalBox(ObjText);

	// Progress bar
	if (Progress >= 0.0f)
	{
		UProgressBar* PBar = WidgetTree->ConstructWidget<UProgressBar>();
		PBar->SetPercent(Progress);
		PBar->SetFillColorAndOpacity(QuestColors::ProgressFill);

		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>();
		BarSize->SetHeightOverride(10.0f);
		BarSize->AddChild(PBar);

		UVerticalBoxSlot* BarSlotRef = EntryBox->AddChildToVerticalBox(BarSize);
		if (BarSlotRef) { BarSlotRef->SetPadding(FMargin(0, 4, 0, 2)); }
	}

	// Reward
	UTextBlock* RewardText = WidgetTree->ConstructWidget<UTextBlock>();
	RewardText->SetText(FText::FromString(FString::Printf(TEXT("Reward: %s"), *Reward)));
	RewardText->SetColorAndOpacity(FSlateColor(QuestColors::Gold));
	FSlateFontInfo RewardFont = RewardText->GetFont();
	RewardFont.Size = 10;
	RewardText->SetFont(RewardFont);
	EntryBox->AddChildToVerticalBox(RewardText);

	QuestScrollBox->AddChild(EntryBg);

	// Spacer
	USpacer* Sp = WidgetTree->ConstructWidget<USpacer>();
	Sp->SetSize(FVector2D(0.0f, 4.0f));
	QuestScrollBox->AddChild(Sp);
}

// =============================================================================
// NativeConstruct
// =============================================================================

void UCoMQuestLogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)    { CloseButton->OnClicked.AddDynamic(this, &UCoMQuestLogWidget::OnCloseClicked); }
	if (ActiveTab)      { ActiveTab->OnClicked.AddDynamic(this, &UCoMQuestLogWidget::OnActiveTabClicked); }
	if (CompletedTab)   { CompletedTab->OnClicked.AddDynamic(this, &UCoMQuestLogWidget::OnCompletedTabClicked); }
	if (FailedTab)      { FailedTab->OnClicked.AddDynamic(this, &UCoMQuestLogWidget::OnFailedTabClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMQuestLogWidget::RefreshQuests(int32 WizardId)
{
	CurrentWizardId = WizardId;

	if (!QuestScrollBox) { return; }
	QuestScrollBox->ClearChildren();

	// Placeholder quests — in production these come from a quest subsystem
	if (CurrentTab == TEXT("Active"))
	{
		AddQuestEntry(TEXT("The Lost Tome"), TEXT("Find the ancient spellbook hidden in the ruins of Noctharion."),
			TEXT("Explore the Shadow Ruins"), 0.35f, TEXT("Rare Spell: Shadow Gate"));
		AddQuestEntry(TEXT("Dragon's Hoard"), TEXT("Slay the red dragon terrorizing the Infernyx border."),
			TEXT("Defeat Pyraxis the Scorcher"), 0.0f, TEXT("500 Gold, Dragon Scale"));
		AddQuestEntry(TEXT("Alliance of Light"), TEXT("Forge a treaty with the Aurelith kingdom."),
			TEXT("Raise reputation to Friendly"), 0.6f, TEXT("Life Spellbook +1"));
	}
	else if (CurrentTab == TEXT("Completed"))
	{
		AddQuestEntry(TEXT("First Steps"), TEXT("Found your first city on the material plane."),
			TEXT("Completed"), 1.0f, TEXT("100 Gold"));
	}
	else
	{
		AddQuestEntry(TEXT("The Spy Network"), TEXT("Your espionage network was discovered."),
			TEXT("Failed"), -1.0f, TEXT("N/A"));
	}
}

void UCoMQuestLogWidget::OnCloseClicked()     { SetVisibility(ESlateVisibility::Collapsed); }
void UCoMQuestLogWidget::OnActiveTabClicked()  { CurrentTab = TEXT("Active"); RefreshQuests(CurrentWizardId); }
void UCoMQuestLogWidget::OnCompletedTabClicked() { CurrentTab = TEXT("Completed"); RefreshQuests(CurrentWizardId); }
void UCoMQuestLogWidget::OnFailedTabClicked()  { CurrentTab = TEXT("Failed"); RefreshQuests(CurrentWizardId); }
