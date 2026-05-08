// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMTutorialWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

#include "CoMUI/CoMUISubsystem.h"

#define LOCTEXT_NAMESPACE "CoMTutorial"

namespace TutColours
{
	static const FLinearColor BgDim   = FLinearColor(0.0f, 0.0f, 0.0f, 0.75f);
	static const FLinearColor Panel   = FLinearColor(0.04f, 0.03f, 0.10f, 0.97f);
	static const FLinearColor Gold    = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver  = FLinearColor(0.85f, 0.85f, 0.90f, 1.0f);
	static const FLinearColor Grey    = FLinearColor(0.55f, 0.55f, 0.60f, 1.0f);
	static const FLinearColor Btn     = FLinearColor(0.06f, 0.04f, 0.14f, 1.0f);
	static const FLinearColor BtnHov  = FLinearColor(0.10f, 0.07f, 0.22f, 1.0f);
}

TSharedRef<SWidget> UCoMTutorialWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMTutorialWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	if (DismissButton)
	{
		DismissButton->OnClicked.AddDynamic(this, &UCoMTutorialWidget::OnDismissClicked);
	}
}

void UCoMTutorialWidget::OnDismissClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

void UCoMTutorialWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(TutColours::BgDim);
	BackgroundBorder->SetPadding(FMargin(0));

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(Root);

	// Centered modal panel.
	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(720.0f);
	PanelSize->SetHeightOverride(560.0f);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(TutColours::Panel);
	Panel->SetPadding(FMargin(36, 28));
	PanelSize->AddChild(Panel);

	UOverlaySlot* PSlot = Root->AddChildToOverlay(PanelSize);
	if (PSlot)
	{
		PSlot->SetHorizontalAlignment(HAlign_Center);
		PSlot->SetVerticalAlignment(VAlign_Center);
	}

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChild(ContentBox);

	// Title.
	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	TitleText->SetText(LOCTEXT("TutTitle", "Welcome to Shattered Arcana"));
	TitleText->SetColorAndOpacity(FSlateColor(TutColours::Gold));
	TitleText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = TitleText->GetFont(); F.Size = 26; TitleText->SetFont(F); }
	UVerticalBoxSlot* TS = ContentBox->AddChildToVerticalBox(TitleText);
	if (TS) { TS->SetHorizontalAlignment(HAlign_Center); TS->SetPadding(FMargin(0, 0, 0, 12)); }

	auto AddLine = [this](const FText& Body, bool bHeader)
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(Body);
		T->SetColorAndOpacity(FSlateColor(bHeader ? TutColours::Gold : TutColours::Silver));
		FSlateFontInfo F = T->GetFont();
		F.Size = bHeader ? 15 : 13;
		T->SetFont(F);
		T->SetAutoWrapText(true);
		UVerticalBoxSlot* S = ContentBox->AddChildToVerticalBox(T);
		if (S) { S->SetPadding(FMargin(0, bHeader ? 8 : 2, 0, 2)); }
	};

	AddLine(LOCTEXT("TutGoal", "Goal"), true);
	AddLine(LOCTEXT("TutGoalBody",
		"Become the dominant wizard across the eight planes. Win by casting "
		"the Spell of Mastery, by conquest, or by banishing every rival."), false);

	AddLine(LOCTEXT("TutEndTurn", "Each Turn"), true);
	AddLine(LOCTEXT("TutEndTurnBody",
		"Click End Turn to advance. Cities produce, armies move, mana flows in, "
		"and your research progresses every turn. AI wizards act too."), false);

	AddLine(LOCTEXT("TutCities", "Cities"), true);
	AddLine(LOCTEXT("TutCitiesBody",
		"Click a city for its panel. Set a build queue (Granary first for food, "
		"Marketplace next, then Smithy / Library / Barracks). Your population "
		"grows with food surplus."), false);

	AddLine(LOCTEXT("TutMagic", "Magic"), true);
	AddLine(LOCTEXT("TutMagicBody",
		"Open the Spell Book to research and cast. Damage and heal spells "
		"need a target; Global Enchantments and item-creation (Enchant Item / "
		"Create Artifact) cast directly. Capture mana nodes to fuel research."), false);

	AddLine(LOCTEXT("TutHeroes", "Heroes"), true);
	AddLine(LOCTEXT("TutHeroesBody",
		"Heroes appear in your taverns; recruit them, level them up, and equip "
		"forged items in their hero screen for combat bonuses."), false);

	AddLine(LOCTEXT("TutWorld", "Explore"), true);
	AddLine(LOCTEXT("TutWorldBody",
		"Lairs, ruins, and ancient towers hide gold, mana, and spells — "
		"send an army to clear them. The Tower of Wizardry on each plane "
		"opens a gateway to the next."), false);

	// Bottom bar with dismiss button.
	UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
	{
		USizeBox* BSize = WidgetTree->ConstructWidget<USizeBox>();
		BSize->SetWidthOverride(160.0f); BSize->SetHeightOverride(40.0f);

		DismissButton = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style = DismissButton->GetStyle();
		Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(TutColours::Btn);
		Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(TutColours::BtnHov);
		Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(TutColours::Btn);
		DismissButton->SetStyle(Style);

		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>();
		Lbl->SetText(LOCTEXT("TutDismiss", "Got it — let's play"));
		Lbl->SetColorAndOpacity(FSlateColor(TutColours::Gold));
		Lbl->SetJustification(ETextJustify::Center);
		{ FSlateFontInfo F = Lbl->GetFont(); F.Size = 14; Lbl->SetFont(F); }
		DismissButton->AddChild(Lbl);

		BSize->AddChild(DismissButton);
		Bottom->AddChildToHorizontalBox(BSize);
	}
	UVerticalBoxSlot* BS = ContentBox->AddChildToVerticalBox(Bottom);
	if (BS) { BS->SetHorizontalAlignment(HAlign_Center); BS->SetPadding(FMargin(0, 18, 0, 0)); }

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}
}

#undef LOCTEXT_NAMESPACE
