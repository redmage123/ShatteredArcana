// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellTreeWidget.cpp -- Visual spell research tree implementation.

#include "CoMSpellTreeWidget.h"

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

namespace TreeColors
{
	static const FLinearColor BgDark     = FLinearColor(0.015f, 0.010f, 0.040f, 0.95f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.92f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);

	// Node state colors
	static const FLinearColor Known       = FLinearColor(0.1f, 0.7f, 0.1f, 1.0f);   // Green
	static const FLinearColor Researching = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);   // Blue
	static const FLinearColor Available   = FLinearColor(0.5f, 0.5f, 0.55f, 1.0f);  // Grey
	static const FLinearColor Locked      = FLinearColor(0.1f, 0.1f, 0.19f, 1.0f);  // Dark #1a1a30

	// Realm tab colors (same scheme as SpellBookWidget)
	static const FLinearColor Life    = FLinearColor(1.0f, 1.0f, 0.9f, 1.0f);
	static const FLinearColor Death   = FLinearColor(0.5f, 0.0f, 0.5f, 1.0f);
	static const FLinearColor Chaos   = FLinearColor(1.0f, 0.2f, 0.0f, 1.0f);
	static const FLinearColor Nature  = FLinearColor(0.0f, 0.8f, 0.0f, 1.0f);
	static const FLinearColor Sorcery = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);
	static const FLinearColor Arcane  = FLinearColor(0.8f, 0.6f, 0.0f, 1.0f);
	static const FLinearColor Binding = FLinearColor(0.6f, 0.0f, 0.0f, 1.0f);
	static const FLinearColor Spirit  = FLinearColor(0.6f, 0.4f, 1.0f, 1.0f);
	static const FLinearColor Glamour = FLinearColor(1.0f, 0.4f, 0.8f, 1.0f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMSpellTreeWidget::RebuildWidget()
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

void UCoMSpellTreeWidget::BuildLayout()
{
	// Full-screen dark background
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(TreeColors::BgDark);
	Background->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = Background;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->AddChild(ScreenOverlay);

	// Gold border panel
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(TreeColors::Gold);
	PanelBorder->SetPadding(FMargin(2.0f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(TreeColors::PanelBg);
	PanelInner->SetPadding(FMargin(16.0f, 12.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(950.0f);
	PanelSize->SetHeightOverride(700.0f);
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
		HeaderText->SetText(FText::FromString(TEXT("Arcane Research")));
		HeaderText->SetColorAndOpacity(FSlateColor(TreeColors::Gold));
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 24;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);

		UVerticalBoxSlot* HeaderSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HeaderSlotRef) { HeaderSlotRef->SetPadding(FMargin(0, 0, 0, 8)); HeaderSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}

	// ── Realm tabs ───────────────────────────────────────────────────────────
	{
		RealmTabsBox = WidgetTree->ConstructWidget<UHorizontalBox>();

		LifeTab    = CreateRealmTab(TEXT("Life"),    TreeColors::Life);
		DeathTab   = CreateRealmTab(TEXT("Death"),   TreeColors::Death);
		ChaosTab   = CreateRealmTab(TEXT("Chaos"),   TreeColors::Chaos);
		NatureTab  = CreateRealmTab(TEXT("Nature"),  TreeColors::Nature);
		SorceryTab = CreateRealmTab(TEXT("Sorcery"), TreeColors::Sorcery);
		ArcaneTab  = CreateRealmTab(TEXT("Arcane"),  TreeColors::Arcane);
		BindingTab = CreateRealmTab(TEXT("Binding"), TreeColors::Binding);
		SpiritTab  = CreateRealmTab(TEXT("Spirit"),  TreeColors::Spirit);
		GlamourTab = CreateRealmTab(TEXT("Glamour"), TreeColors::Glamour);

		UVerticalBoxSlot* TabSlotRef = ContentBox->AddChildToVerticalBox(RealmTabsBox);
		if (TabSlotRef) { TabSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Tree area (scrollable) ───────────────────────────────────────────────
	{
		TreeScrollBox = WidgetTree->ConstructWidget<UScrollBox>();

		// Placeholder tree: 5 tiers with placeholder spell nodes
		for (int32 Tier = 1; Tier <= 5; ++Tier)
		{
			// Tier label
			UTextBlock* TierLabel = WidgetTree->ConstructWidget<UTextBlock>();
			TierLabel->SetText(FText::FromString(FString::Printf(TEXT("Tier %d"), Tier)));
			TierLabel->SetColorAndOpacity(FSlateColor(TreeColors::Grey));
			FSlateFontInfo TierFont = TierLabel->GetFont();
			TierFont.Size = 11;
			TierLabel->SetFont(TierFont);
			TreeScrollBox->AddChild(TierLabel);

			// Row of spell nodes
			UHorizontalBox* NodeRow = WidgetTree->ConstructWidget<UHorizontalBox>();
			int32 NodeCount = FMath::Max(5 - Tier + 1, 2);
			for (int32 N = 0; N < NodeCount; ++N)
			{
				FLinearColor NodeColor;
				if (Tier == 1 && N == 0) { NodeColor = TreeColors::Known; }
				else if (Tier == 1) { NodeColor = TreeColors::Researching; }
				else if (Tier <= 2) { NodeColor = TreeColors::Available; }
				else { NodeColor = TreeColors::Locked; }

				FString SpellName = FString::Printf(TEXT("Spell %d-%d"), Tier, N + 1);
				UButton* Node = CreateSpellNode(SpellName, NodeColor);

				USizeBox* NodeSize = WidgetTree->ConstructWidget<USizeBox>();
				NodeSize->SetWidthOverride(110.0f);
				NodeSize->SetHeightOverride(40.0f);
				NodeSize->AddChild(Node);

				UHorizontalBoxSlot* NodeSlotRef = NodeRow->AddChildToHorizontalBox(NodeSize);
				if (NodeSlotRef) { NodeSlotRef->SetPadding(FMargin(4.0f, 2.0f)); }
			}
			TreeScrollBox->AddChild(NodeRow);

			// Prerequisite line between tiers
			if (Tier < 5)
			{
				UVerticalBox* LineBox = WidgetTree->ConstructWidget<UVerticalBox>();
				CreatePrereqLine(LineBox);
				TreeScrollBox->AddChild(LineBox);
			}
		}

		USizeBox* TreeSize = WidgetTree->ConstructWidget<USizeBox>();
		TreeSize->SetHeightOverride(380.0f);
		TreeSize->AddChild(TreeScrollBox);

		UVerticalBoxSlot* TreeSlotRef = ContentBox->AddChildToVerticalBox(TreeSize);
		if (TreeSlotRef) { TreeSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Selected spell details ───────────────────────────────────────────────
	{
		UBorder* DetailBg = WidgetTree->ConstructWidget<UBorder>();
		DetailBg->SetBrushColor(FLinearColor(0.02f, 0.015f, 0.05f, 0.8f));
		DetailBg->SetPadding(FMargin(12.0f, 8.0f));

		UVerticalBox* DetailBox = WidgetTree->ConstructWidget<UVerticalBox>();
		DetailBg->AddChild(DetailBox);

		SelectedSpellNameText = WidgetTree->ConstructWidget<UTextBlock>();
		SelectedSpellNameText->SetText(FText::FromString(TEXT("No spell selected")));
		SelectedSpellNameText->SetColorAndOpacity(FSlateColor(TreeColors::Gold));
		FSlateFontInfo NameFont = SelectedSpellNameText->GetFont();
		NameFont.Size = 16;
		SelectedSpellNameText->SetFont(NameFont);
		UVerticalBoxSlot* NameSlotRef = DetailBox->AddChildToVerticalBox(SelectedSpellNameText);
		if (NameSlotRef) { NameSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

		SelectedSpellDescText = WidgetTree->ConstructWidget<UTextBlock>();
		SelectedSpellDescText->SetText(FText::FromString(TEXT("Click a spell node to view details.")));
		SelectedSpellDescText->SetColorAndOpacity(FSlateColor(TreeColors::Silver));
		SelectedSpellDescText->SetAutoWrapText(true);
		FSlateFontInfo DescFont = SelectedSpellDescText->GetFont();
		DescFont.Size = 12;
		SelectedSpellDescText->SetFont(DescFont);
		UVerticalBoxSlot* DescSlotRef = DetailBox->AddChildToVerticalBox(SelectedSpellDescText);
		if (DescSlotRef) { DescSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

		SelectedSpellCostText = WidgetTree->ConstructWidget<UTextBlock>();
		SelectedSpellCostText->SetText(FText::GetEmpty());
		SelectedSpellCostText->SetColorAndOpacity(FSlateColor(TreeColors::Grey));
		FSlateFontInfo CostFont = SelectedSpellCostText->GetFont();
		CostFont.Size = 11;
		SelectedSpellCostText->SetFont(CostFont);
		DetailBox->AddChildToVerticalBox(SelectedSpellCostText);

		UVerticalBoxSlot* DetailSlotRef = ContentBox->AddChildToVerticalBox(DetailBg);
		if (DetailSlotRef) { DetailSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Bottom buttons ───────────────────────────────────────────────────────
	{
		UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		ResearchThisButton = CreateActionButton(TEXT("Research This"), 160.f);
		USizeBox* ResSize = WidgetTree->ConstructWidget<USizeBox>();
		ResSize->SetWidthOverride(160.0f);
		ResSize->SetHeightOverride(38.0f);
		ResSize->AddChild(ResearchThisButton);

		UHorizontalBoxSlot* ResSlotRef = ButtonRow->AddChildToHorizontalBox(ResSize);
		if (ResSlotRef) { ResSlotRef->SetPadding(FMargin(0, 0, 10, 0)); }

		CloseButton = CreateActionButton(TEXT("Close"), 140.f);
		USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>();
		CloseSize->SetWidthOverride(140.0f);
		CloseSize->SetHeightOverride(38.0f);
		CloseSize->AddChild(CloseButton);

		UHorizontalBoxSlot* CloseSlotRef = ButtonRow->AddChildToHorizontalBox(CloseSize);
		if (CloseSlotRef) { CloseSlotRef->SetHorizontalAlignment(HAlign_Right); }

		UVerticalBoxSlot* BtnRowSlotRef = ContentBox->AddChildToVerticalBox(ButtonRow);
		if (BtnRowSlotRef) { BtnRowSlotRef->SetHorizontalAlignment(HAlign_Right); }
	}
}

UButton* UCoMSpellTreeWidget::CreateRealmTab(const FString& Label, const FLinearColor& Color)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(TreeColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(TreeColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(TreeColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 11;
	Text->SetFont(Font);

	Btn->AddChild(Text);

	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(88.0f);
	SizeBox->SetHeightOverride(32.0f);
	SizeBox->AddChild(Btn);

	UHorizontalBoxSlot* TabSlotRef = RealmTabsBox->AddChildToHorizontalBox(SizeBox);
	if (TabSlotRef) { TabSlotRef->SetPadding(FMargin(2.0f, 0.0f)); }

	return Btn;
}

UButton* UCoMSpellTreeWidget::CreateActionButton(const FString& Label, float Width)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(TreeColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(TreeColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(TreeColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(TreeColors::Silver));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 14;
	Text->SetFont(Font);

	Btn->AddChild(Text);
	return Btn;
}

UButton* UCoMSpellTreeWidget::CreateSpellNode(const FString& SpellName, const FLinearColor& NodeColor)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(NodeColor);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(FLinearColor(NodeColor.R * 1.3f, NodeColor.G * 1.3f, NodeColor.B * 1.3f, 1.0f));
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(NodeColor);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(SpellName));
	Text->SetColorAndOpacity(FSlateColor(TreeColors::Silver));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 10;
	Text->SetFont(Font);

	Btn->AddChild(Text);
	return Btn;
}

void UCoMSpellTreeWidget::CreatePrereqLine(UVerticalBox* Parent)
{
	if (!Parent) { return; }

	UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
	Line->SetBrushColor(TreeColors::GoldDim);

	USizeBox* LineSize = WidgetTree->ConstructWidget<USizeBox>();
	LineSize->SetWidthOverride(2.0f);
	LineSize->SetHeightOverride(16.0f);
	LineSize->AddChild(Line);

	UVerticalBoxSlot* LineSlotRef = Parent->AddChildToVerticalBox(LineSize);
	if (LineSlotRef) { LineSlotRef->SetHorizontalAlignment(HAlign_Center); }
}

// =============================================================================
// NativeConstruct
// =============================================================================

void UCoMSpellTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)        { CloseButton->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnCloseClicked); }
	if (ResearchThisButton) { ResearchThisButton->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnResearchThisClicked); }

	if (LifeTab)    { LifeTab->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnLifeTabClicked); }
	if (DeathTab)   { DeathTab->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnDeathTabClicked); }
	if (ChaosTab)   { ChaosTab->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnChaosTabClicked); }
	if (NatureTab)  { NatureTab->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnNatureTabClicked); }
	if (SorceryTab) { SorceryTab->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnSorceryTabClicked); }
	if (ArcaneTab)  { ArcaneTab->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnArcaneTabClicked); }
	if (BindingTab) { BindingTab->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnBindingTabClicked); }
	if (SpiritTab)  { SpiritTab->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnSpiritTabClicked); }
	if (GlamourTab) { GlamourTab->OnClicked.AddDynamic(this, &UCoMSpellTreeWidget::OnGlamourTabClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMSpellTreeWidget::SetWizardId(int32 WizardId)
{
	CurrentWizardId = WizardId;
	// In production: load wizard's research state and refresh the tree
}

void UCoMSpellTreeWidget::SelectRealm(ECoMSpellRealm Realm)
{
	CurrentRealm = Realm;

	if (HeaderText)
	{
		const UEnum* RealmEnum = StaticEnum<ECoMSpellRealm>();
		FString RealmName = RealmEnum
			? RealmEnum->GetDisplayNameTextByValue(static_cast<int64>(Realm)).ToString()
			: TEXT("Unknown");
		HeaderText->SetText(FText::FromString(FString::Printf(TEXT("Arcane Research \u2014 %s"), *RealmName)));
	}
}

void UCoMSpellTreeWidget::OnCloseClicked()         { SetVisibility(ESlateVisibility::Collapsed); }
void UCoMSpellTreeWidget::OnResearchThisClicked()   { /* Route through magic subsystem */ }

void UCoMSpellTreeWidget::OnLifeTabClicked()    { SelectRealm(ECoMSpellRealm::Life); }
void UCoMSpellTreeWidget::OnDeathTabClicked()   { SelectRealm(ECoMSpellRealm::Death); }
void UCoMSpellTreeWidget::OnChaosTabClicked()   { SelectRealm(ECoMSpellRealm::Chaos); }
void UCoMSpellTreeWidget::OnNatureTabClicked()  { SelectRealm(ECoMSpellRealm::Nature); }
void UCoMSpellTreeWidget::OnSorceryTabClicked() { SelectRealm(ECoMSpellRealm::Sorcery); }
void UCoMSpellTreeWidget::OnArcaneTabClicked()  { SelectRealm(ECoMSpellRealm::Arcane); }
void UCoMSpellTreeWidget::OnBindingTabClicked() { SelectRealm(ECoMSpellRealm::Binding); }
void UCoMSpellTreeWidget::OnSpiritTabClicked()  { SelectRealm(ECoMSpellRealm::Spirit); }
void UCoMSpellTreeWidget::OnGlamourTabClicked() { SelectRealm(ECoMSpellRealm::Glamour); }
