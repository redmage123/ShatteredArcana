// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellBookWidget.cpp -- Spell book / research screen implementation.

#include "CoMSpellBookWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/Data/CoMSpellDatabase.h"
#include "CoMUI/CoMUISubsystem.h"

// Realm colours for tabs
namespace SpellColours
{
	static const FLinearColor BgDark     = FLinearColor(0.015f, 0.010f, 0.040f, 1.0f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);

	// Realm-specific colours
	static const FLinearColor Life       = FLinearColor(1.0f, 1.0f, 0.9f, 1.0f);
	static const FLinearColor Death      = FLinearColor(0.5f, 0.0f, 0.5f, 1.0f);
	static const FLinearColor Chaos      = FLinearColor(1.0f, 0.2f, 0.0f, 1.0f);
	static const FLinearColor Nature     = FLinearColor(0.0f, 0.8f, 0.0f, 1.0f);
	static const FLinearColor Sorcery    = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);
	static const FLinearColor Arcane     = FLinearColor(0.8f, 0.6f, 0.0f, 1.0f);
	static const FLinearColor Binding    = FLinearColor(0.6f, 0.0f, 0.0f, 1.0f);
	static const FLinearColor Spirit     = FLinearColor(0.6f, 0.4f, 1.0f, 1.0f);
	static const FLinearColor Glamour    = FLinearColor(1.0f, 0.4f, 0.8f, 1.0f);
}

// =============================================================================
// RebuildWidget — build the spell book layout before Slate finalizes
// =============================================================================

TSharedRef<SWidget> UCoMSpellBookWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMSpellBookWidget::BuildLayout()
{
	// ── Full-screen dark background ──────────────────────────────────────
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(SpellColours::BgDark);
	Background->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = Background;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->AddChild(ScreenOverlay);

	// ── Open-book panel: leather-tome backdrop with the content on the pages ─
	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(960.0f);
	PanelSize->SetHeightOverride(640.0f); // 3:2 to match the book art

	UOverlaySlot* PanelSlot = ScreenOverlay->AddChildToOverlay(PanelSize);
	if (PanelSlot)
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UOverlay* BookOverlay = WidgetTree->ConstructWidget<UOverlay>();
	PanelSize->AddChild(BookOverlay);

	// Open leather-tome art (falls back to a parchment tint if not imported yet).
	BookBackground = WidgetTree->ConstructWidget<UImage>();
	if (UTexture2D* BookTex = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/UI/SpellBook/spellbook_bg.spellbook_bg")))
	{
		BookBackground->SetBrushFromTexture(BookTex);
	}
	else
	{
		BookBackground->SetColorAndOpacity(FLinearColor(0.10f, 0.07f, 0.04f, 1.0f));
	}
	if (UOverlaySlot* BgSlot = BookOverlay->AddChildToOverlay(BookBackground))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// Content inset onto the open pages. A faint dark tint keeps the light text
	// readable over the parchment art.
	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(FLinearColor(0.03f, 0.02f, 0.015f, 0.45f));
	PanelInner->SetPadding(FMargin(64.0f, 44.0f));
	if (UOverlaySlot* InnerSlot = BookOverlay->AddChildToOverlay(PanelInner))
	{
		InnerSlot->SetHorizontalAlignment(HAlign_Fill);
		InnerSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// ── Content column ───────────────────────────────────────────────────
	RootContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelInner->AddChild(RootContentBox);

	// ── Header: "Spell Book" + wizard info ───────────────────────────────
	{
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		WizardNameText = WidgetTree->ConstructWidget<UTextBlock>();
		WizardNameText->SetText(FText::FromString(TEXT("SPELL BOOK")));
		WizardNameText->SetColorAndOpacity(FSlateColor(SpellColours::Gold));
		FSlateFontInfo TitleFont = WizardNameText->GetFont();
		TitleFont.Size = 28;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		WizardNameText->SetFont(TitleFont);

		UHorizontalBoxSlot* TitleSlotRef = HeaderRow->AddChildToHorizontalBox(WizardNameText);
		if (TitleSlotRef) { TitleSlotRef->SetVerticalAlignment(VAlign_Center); }

		// Mana display on the right
		ManaText = WidgetTree->ConstructWidget<UTextBlock>();
		ManaText->SetText(FText::FromString(TEXT("Mana: -- / --")));
		ManaText->SetColorAndOpacity(FSlateColor(SpellColours::Silver));
		FSlateFontInfo ManaFont = ManaText->GetFont();
		ManaFont.Size = 14;
		ManaText->SetFont(ManaFont);

		UHorizontalBoxSlot* ManaSlotRef = HeaderRow->AddChildToHorizontalBox(ManaText);
		if (ManaSlotRef)
		{
			ManaSlotRef->SetVerticalAlignment(VAlign_Center);
			ManaSlotRef->SetHorizontalAlignment(HAlign_Right);
			ManaSlotRef->SetPadding(FMargin(20.0f, 0.0f, 0.0f, 0.0f));
		}

		UVerticalBoxSlot* HeaderSlotRef = RootContentBox->AddChildToVerticalBox(HeaderRow);
		if (HeaderSlotRef) { HeaderSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// ── Realm tabs ───────────────────────────────────────────────────────
	{
		RealmTabsBox = WidgetTree->ConstructWidget<UHorizontalBox>();

		LifeTab    = CreateRealmTab(TEXT("Life"),    SpellColours::Life);
		DeathTab   = CreateRealmTab(TEXT("Death"),   SpellColours::Death);
		ChaosTab   = CreateRealmTab(TEXT("Chaos"),   SpellColours::Chaos);
		NatureTab  = CreateRealmTab(TEXT("Nature"),  SpellColours::Nature);
		SorceryTab = CreateRealmTab(TEXT("Sorcery"), SpellColours::Sorcery);
		ArcaneTab  = CreateRealmTab(TEXT("Arcane"),  SpellColours::Arcane);
		BindingTab = CreateRealmTab(TEXT("Binding"), SpellColours::Binding);
		SpiritTab  = CreateRealmTab(TEXT("Spirit"),  SpellColours::Spirit);
		GlamourTab = CreateRealmTab(TEXT("Glamour"), SpellColours::Glamour);

		UVerticalBoxSlot* TabSlotRef = RootContentBox->AddChildToVerticalBox(RealmTabsBox);
		if (TabSlotRef) { TabSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Chapter header: ◄ <Realm> ► page-turn arrows ─────────────────────
	{
		UHorizontalBox* ChapterRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		USizeBox* PrevBox = nullptr;
		PrevPageButton = CreateActionButton(TEXT("<"), 44.f, &PrevBox);
		if (PrevBox)
		{
			UHorizontalBoxSlot* S = ChapterRow->AddChildToHorizontalBox(PrevBox);
			if (S) { S->SetVerticalAlignment(VAlign_Center); S->SetPadding(FMargin(0, 0, 12, 0)); }
		}

		CurrentRealmText = WidgetTree->ConstructWidget<UTextBlock>();
		CurrentRealmText->SetText(FText::FromString(TEXT("Arcane")));
		CurrentRealmText->SetColorAndOpacity(FSlateColor(SpellColours::Gold));
		CurrentRealmText->SetJustification(ETextJustify::Center);
		{
			FSlateFontInfo RealmFont = CurrentRealmText->GetFont();
			RealmFont.Size = 20;
			RealmFont.TypefaceFontName = FName(TEXT("Bold"));
			CurrentRealmText->SetFont(RealmFont);
		}
		if (UHorizontalBoxSlot* RS = ChapterRow->AddChildToHorizontalBox(CurrentRealmText))
		{
			RS->SetVerticalAlignment(VAlign_Center);
			RS->SetHorizontalAlignment(HAlign_Center);
			RS->SetSize(ESlateSizeRule::Fill);
		}

		USizeBox* NextBox = nullptr;
		NextPageButton = CreateActionButton(TEXT(">"), 44.f, &NextBox);
		if (NextBox)
		{
			UHorizontalBoxSlot* S = ChapterRow->AddChildToHorizontalBox(NextBox);
			if (S) { S->SetVerticalAlignment(VAlign_Center); S->SetPadding(FMargin(12, 0, 0, 0)); }
		}

		UVerticalBoxSlot* RealmSlotRef = RootContentBox->AddChildToVerticalBox(ChapterRow);
		if (RealmSlotRef) { RealmSlotRef->SetPadding(FMargin(0, 0, 0, 6)); }
	}

	// ── Research progress ────────────────────────────────────────────────
	{
		ResearchText = WidgetTree->ConstructWidget<UTextBlock>();
		ResearchText->SetText(FText::FromString(TEXT("No active research")));
		ResearchText->SetColorAndOpacity(FSlateColor(SpellColours::Silver));
		FSlateFontInfo ResFont = ResearchText->GetFont();
		ResFont.Size = 13;
		ResearchText->SetFont(ResFont);

		UVerticalBoxSlot* ResSlotRef = RootContentBox->AddChildToVerticalBox(ResearchText);
		if (ResSlotRef) { ResSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

		ResearchProgressBar = WidgetTree->ConstructWidget<UProgressBar>();
		ResearchProgressBar->SetPercent(0.0f);
		ResearchProgressBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.5f, 1.0f, 1.0f));

		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>();
		BarSize->SetHeightOverride(16.0f);
		BarSize->AddChild(ResearchProgressBar);

		UVerticalBoxSlot* BarSlotRef = RootContentBox->AddChildToVerticalBox(BarSize);
		if (BarSlotRef) { BarSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// ── Now casting (overworld spell in progress) ────────────────────────
	{
		CastingText = WidgetTree->ConstructWidget<UTextBlock>();
		CastingText->SetText(FText::FromString(TEXT("Not casting")));
		CastingText->SetColorAndOpacity(FSlateColor(SpellColours::Silver));
		FSlateFontInfo CastFont = CastingText->GetFont();
		CastFont.Size = 13;
		CastingText->SetFont(CastFont);

		UVerticalBoxSlot* CastSlotRef = RootContentBox->AddChildToVerticalBox(CastingText);
		if (CastSlotRef) { CastSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

		CastingProgressBar = WidgetTree->ConstructWidget<UProgressBar>();
		CastingProgressBar->SetPercent(0.0f);
		CastingProgressBar->SetFillColorAndOpacity(FLinearColor(0.85f, 0.5f, 0.15f, 1.0f));

		USizeBox* CastBarSize = WidgetTree->ConstructWidget<USizeBox>();
		CastBarSize->SetHeightOverride(16.0f);
		CastBarSize->AddChild(CastingProgressBar);

		UVerticalBoxSlot* CastBarSlotRef = RootContentBox->AddChildToVerticalBox(CastBarSize);
		if (CastBarSlotRef) { CastBarSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// ── Spell list (scrollable) ──────────────────────────────────────────
	{
		SpellListScrollBox = WidgetTree->ConstructWidget<UScrollBox>();

		USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
		ListSize->SetHeightOverride(300.0f);
		ListSize->AddChild(SpellListScrollBox);

		UVerticalBoxSlot* ListSlotRef = RootContentBox->AddChildToVerticalBox(ListSize);
		if (ListSlotRef) { ListSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// ── Selected spell info ──────────────────────────────────────────────
	{
		SelectedSpellText = WidgetTree->ConstructWidget<UTextBlock>();
		SelectedSpellText->SetText(FText::FromString(TEXT("No spell selected")));
		SelectedSpellText->SetColorAndOpacity(FSlateColor(SpellColours::Silver));
		FSlateFontInfo SelFont = SelectedSpellText->GetFont();
		SelFont.Size = 14;
		SelectedSpellText->SetFont(SelFont);

		UVerticalBoxSlot* SelSlotRef = RootContentBox->AddChildToVerticalBox(SelectedSpellText);
		if (SelSlotRef) { SelSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Mana allocation slider ───────────────────────────────────────────
	{
		ManaAllocationLabel = WidgetTree->ConstructWidget<UTextBlock>();
		ManaAllocationLabel->SetText(FText::FromString(TEXT("Mana Allocation: Research / Casting")));
		ManaAllocationLabel->SetColorAndOpacity(FSlateColor(SpellColours::Grey));
		FSlateFontInfo AllocFont = ManaAllocationLabel->GetFont();
		AllocFont.Size = 12;
		ManaAllocationLabel->SetFont(AllocFont);

		UVerticalBoxSlot* AllocLabelSlotRef = RootContentBox->AddChildToVerticalBox(ManaAllocationLabel);
		if (AllocLabelSlotRef) { AllocLabelSlotRef->SetPadding(FMargin(0, 0, 0, 2)); }

		ManaAllocationSlider = WidgetTree->ConstructWidget<USlider>();
		ManaAllocationSlider->SetValue(0.5f);

		USizeBox* SliderSize = WidgetTree->ConstructWidget<USizeBox>();
		SliderSize->SetHeightOverride(24.0f);
		SliderSize->AddChild(ManaAllocationSlider);

		UVerticalBoxSlot* SliderSlotRef = RootContentBox->AddChildToVerticalBox(SliderSize);
		if (SliderSlotRef) { SliderSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// ── Bottom buttons: Research / Cast / Close ──────────────────────────
	{
		UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		USizeBox* ResBox = nullptr;
		USizeBox* CastBox = nullptr;
		USizeBox* CloseBox = nullptr;
		ResearchButton = CreateActionButton(TEXT("Research"), 140.f, &ResBox);
		CastButton     = CreateActionButton(TEXT("Cast Spell"), 140.f, &CastBox);
		CloseButton    = CreateActionButton(TEXT("Close"), 140.f, &CloseBox);

		if (ResBox)   { UHorizontalBoxSlot* S = ButtonRow->AddChildToHorizontalBox(ResBox);   if (S) S->SetPadding(FMargin(0, 0, 10, 0)); }
		if (CastBox)  { UHorizontalBoxSlot* S = ButtonRow->AddChildToHorizontalBox(CastBox);  if (S) S->SetPadding(FMargin(0, 0, 10, 0)); }
		if (CloseBox) { UHorizontalBoxSlot* S = ButtonRow->AddChildToHorizontalBox(CloseBox); if (S) S->SetHorizontalAlignment(HAlign_Right); }

		UVerticalBoxSlot* BtnRowSlotRef = RootContentBox->AddChildToVerticalBox(ButtonRow);
		if (BtnRowSlotRef) { BtnRowSlotRef->SetPadding(FMargin(0, 5, 0, 0)); }
	}
}

UButton* UCoMSpellBookWidget::CreateRealmTab(const FString& Label, const FLinearColor& Color)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();

	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(SpellColours::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(SpellColours::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(SpellColours::BtnNormal);
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

UButton* UCoMSpellBookWidget::CreateActionButton(const FString& Label, float Width, USizeBox** OutSizeBox)
{
	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(SpellColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(SpellColours::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(SpellColours::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(SpellColours::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(SpellColours::Silver));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 15;
	Text->SetFont(Font);
	Text->SetShadowOffset(FVector2D(1, 1));
	Text->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.4f));

	Btn->AddChild(Text);
	BtnBorder->AddChild(Btn);

	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(Width);
	SizeBox->SetHeightOverride(40.0f);
	SizeBox->AddChild(BtnBorder);

	if (OutSizeBox) { *OutSizeBox = SizeBox; }
	return Btn;
}

// =============================================================================
// NativeConstruct — bind delegates (widgets already created by RebuildWidget)
// =============================================================================

void UCoMSpellBookWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind realm tab buttons.
	if (LifeTab)    { LifeTab->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnLifeTabClicked); }
	if (DeathTab)   { DeathTab->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnDeathTabClicked); }
	if (ChaosTab)   { ChaosTab->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnChaosTabClicked); }
	if (NatureTab)  { NatureTab->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnNatureTabClicked); }
	if (SorceryTab) { SorceryTab->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnSorceryTabClicked); }
	if (ArcaneTab)  { ArcaneTab->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnArcaneTabClicked); }
	if (BindingTab) { BindingTab->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnBindingTabClicked); }
	if (SpiritTab)  { SpiritTab->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnSpiritTabClicked); }
	if (GlamourTab) { GlamourTab->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnGlamourTabClicked); }

	// Bind page-turn arrows.
	if (PrevPageButton) { PrevPageButton->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnPrevPageClicked); }
	if (NextPageButton) { NextPageButton->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnNextPageClicked); }

	// Bind action buttons.
	if (CastButton)     { CastButton->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnCastButtonClicked); }
	if (ResearchButton) { ResearchButton->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnResearchButtonClicked); }
	if (CloseButton)    { CloseButton->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnCloseClicked); }

	// Bind mana slider.
	if (ManaAllocationSlider)
	{
		ManaAllocationSlider->OnValueChanged.AddDynamic(this, &UCoMSpellBookWidget::OnManaSliderChanged);
	}

	// Bind auto-research checkbox.
	if (AutoResearchCheckBox)
	{
		AutoResearchCheckBox->OnCheckStateChanged.AddDynamic(this, &UCoMSpellBookWidget::OnAutoResearchToggled);
	}
}

UCoMMagicSubsystem* UCoMSpellBookWidget::GetMagicSubsystem()
{
	if (CachedMagicSubsystem.IsValid())
	{
		return CachedMagicSubsystem.Get();
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI)
	{
		UCoMMagicSubsystem* Sub = GI->GetSubsystem<UCoMMagicSubsystem>();
		CachedMagicSubsystem = Sub;
		return Sub;
	}
	return nullptr;
}

void UCoMSpellBookWidget::SetWizardId(int32 WizardId)
{
	CurrentWizardId = WizardId;

	UCoMMagicSubsystem* MagicSub = GetMagicSubsystem();
	if (!MagicSub)
	{
		return;
	}

	FCoMWizardMagicState& State = MagicSub->GetWizardMagic(WizardId);

	// Display wizard mana info.
	if (ManaText)
	{
		ManaText->SetText(FText::FromString(
			FString::Printf(TEXT("Mana: %d / %d  (+%d/turn)"),
				State.CurrentMana, State.MaxMana, State.ManaPerTurn)));
	}

	// Research progress.
	int32 ProgressPct = MagicSub->GetResearchProgress(WizardId);
	if (ResearchText)
	{
		if (State.CurrentResearchSpell != NAME_None)
		{
			int32 ETA = MagicSub->GetResearchETATurns(WizardId);
			ResearchText->SetText(FText::FromString(
				FString::Printf(TEXT("Researching: %s  (%d%%, ~%d turns)"),
					*State.CurrentResearchSpell.ToString(), ProgressPct, ETA)));
		}
		else
		{
			ResearchText->SetText(FText::FromString(TEXT("No active research")));
		}
	}

	if (ResearchProgressBar)
	{
		ResearchProgressBar->SetPercent(static_cast<float>(ProgressPct) / 100.f);
	}

	// Now casting (overworld spell in progress) — the other half of the CoM
	// magic screen: shows the spell being cast this turn and its progress.
	const FCoMSpellCast CurrentCast = MagicSub->GetCurrentCasting(WizardId);
	if (CastingText)
	{
		if (!CurrentCast.SpellId.IsNone())
		{
			const FCoMSpellInfo CastInfo = CoMSpellDatabase::GetSpellInfo(CurrentCast.SpellId);
			const FString CastName = CastInfo.DisplayName.IsEmpty()
				? CurrentCast.SpellId.ToString() : CastInfo.DisplayName.ToString();
			if (CurrentCast.TurnsRemaining > 0)
			{
				CastingText->SetText(FText::FromString(FString::Printf(
					TEXT("Now casting: %s  (~%d turns)"), *CastName, CurrentCast.TurnsRemaining)));
			}
			else
			{
				CastingText->SetText(FText::FromString(FString::Printf(TEXT("Now casting: %s"), *CastName)));
			}
		}
		else
		{
			CastingText->SetText(FText::FromString(TEXT("Not casting")));
		}
	}

	if (CastingProgressBar)
	{
		float CastPct = 0.f;
		if (!CurrentCast.SpellId.IsNone() && CurrentCast.CastingTime > 0)
		{
			CastPct = static_cast<float>(CurrentCast.CastingTime - CurrentCast.TurnsRemaining)
				/ static_cast<float>(CurrentCast.CastingTime);
		}
		CastingProgressBar->SetPercent(FMath::Clamp(CastPct, 0.f, 1.f));
	}

	// Mana allocation slider.
	if (ManaAllocationSlider && State.ManaPerTurn > 0)
	{
		float SliderValue = static_cast<float>(State.ResearchAllocation) /
			static_cast<float>(FMath::Max(State.ManaPerTurn, 1));
		ManaAllocationSlider->SetValue(FMath::Clamp(SliderValue, 0.f, 1.f));
	}

	// Auto-research toggle.
	if (AutoResearchCheckBox)
	{
		AutoResearchCheckBox->SetIsChecked(MagicSub->IsAutoResearchEnabled(WizardId));
	}
	if (AutoResearchLabel)
	{
		if (MagicSub->IsAutoResearchEnabled(WizardId) && State.CurrentResearchSpell != NAME_None)
		{
			AutoResearchLabel->SetText(FText::FromString(
				FString::Printf(TEXT("Auto-researching: %s"),
					*State.CurrentResearchSpell.ToString())));
		}
		else if (MagicSub->IsAutoResearchEnabled(WizardId))
		{
			AutoResearchLabel->SetText(FText::FromString(TEXT("Auto-Research: ON")));
		}
		else
		{
			AutoResearchLabel->SetText(FText::FromString(TEXT("Auto-Research: OFF")));
		}
	}

	// Default to the wizard's primary realm.
	SelectRealm(State.PrimaryRealm);
}

void UCoMSpellBookWidget::SelectRealm(ECoMSpellRealm Realm)
{
	CurrentRealm = Realm;

	if (CurrentRealmText)
	{
		const UEnum* RealmEnum = StaticEnum<ECoMSpellRealm>();
		FString RealmName = RealmEnum
			? RealmEnum->GetDisplayNameTextByValue(static_cast<int64>(Realm)).ToString()
			: TEXT("Unknown");
		CurrentRealmText->SetText(FText::FromString(RealmName));
	}

	RefreshSpellList();
}

void UCoMSpellBookWidget::RefreshSpellList()
{
	if (!SpellListScrollBox)
	{
		return;
	}

	SpellListScrollBox->ClearChildren();

	UCoMMagicSubsystem* MagicSub = GetMagicSubsystem();
	if (!MagicSub || CurrentWizardId < 0)
	{
		return;
	}

	FCoMWizardMagicState& State = MagicSub->GetWizardMagic(CurrentWizardId);

	// Show known spells for the current realm.
	for (const FName& SpellName : State.KnownSpells)
	{
		// In a full implementation we would look up the spell's realm from a data
		// asset. For now, display all known spells and let realm filtering be
		// implemented when the spell database is available.
		UTextBlock* SpellEntry = NewObject<UTextBlock>(this);
		if (SpellEntry)
		{
			SpellEntry->SetText(FText::FromString(
				FString::Printf(TEXT("[Known] %s"), *SpellName.ToString())));

			FSlateFontInfo FontInfo = SpellEntry->GetFont();
			FontInfo.Size = 14;
			SpellEntry->SetFont(FontInfo);
			SpellEntry->SetColorAndOpacity(FSlateColor(FLinearColor::White));

			SpellListScrollBox->AddChild(SpellEntry);
		}
	}

	// Show researchable spells (dimmed).
	TArray<FName> Researchable = MagicSub->GetResearchableSpells(CurrentWizardId);
	for (const FName& SpellName : Researchable)
	{
		UTextBlock* SpellEntry = NewObject<UTextBlock>(this);
		if (SpellEntry)
		{
			SpellEntry->SetText(FText::FromString(
				FString::Printf(TEXT("[Research] %s"), *SpellName.ToString())));

			FSlateFontInfo FontInfo = SpellEntry->GetFont();
			FontInfo.Size = 14;
			SpellEntry->SetFont(FontInfo);
			SpellEntry->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.f)));

			SpellListScrollBox->AddChild(SpellEntry);
		}
	}
}

void UCoMSpellBookWidget::OnSpellClicked(FName SpellId)
{
	SelectedSpellId = SpellId;

	if (SelectedSpellText)
	{
		SelectedSpellText->SetText(FText::FromString(
			FString::Printf(TEXT("Selected: %s"), *SpellId.ToString())));
	}
}

void UCoMSpellBookWidget::OnCastClicked()
{
	if (SelectedSpellId == NAME_None || CurrentWizardId < 0)
	{
		return;
	}

	UCoMUISubsystem* UISS = GetGameInstance()->GetSubsystem<UCoMUISubsystem>();
	if (!UISS) return;

	// Item-creation spells skip the targeting widget and go directly to the
	// forge. The forge handles its own mana spending on confirm.
	if (SelectedSpellId == FName(TEXT("enchant_item")))
	{
		UISS->ShowItemForge(CurrentWizardId, /*bArtifact*/ false);
		return;
	}
	if (SelectedSpellId == FName(TEXT("create_artifact")))
	{
		UISS->ShowItemForge(CurrentWizardId, /*bArtifact*/ true);
		return;
	}

	// Route through the spell targeting widget for target selection.
	// The targeting widget handles NoTarget spells (instant cast) as well as
	// spells that require tile/unit/city/army target selection.
	UISS->BeginSpellTargeting(SelectedSpellId, CurrentWizardId);
	// Close the spell book to let the player select a target on the map.
	UISS->HideSpellBook();
}

void UCoMSpellBookWidget::OnResearchClicked()
{
	if (SelectedSpellId == NAME_None || CurrentWizardId < 0)
	{
		return;
	}

	UCoMMagicSubsystem* MagicSub = GetMagicSubsystem();
	if (MagicSub)
	{
		MagicSub->StartResearch(CurrentWizardId, SelectedSpellId);
		SetWizardId(CurrentWizardId);
	}
}

void UCoMSpellBookWidget::OnManaSliderChanged(float Value)
{
	UCoMMagicSubsystem* MagicSub = GetMagicSubsystem();
	if (!MagicSub || CurrentWizardId < 0)
	{
		return;
	}

	FCoMWizardMagicState& State = MagicSub->GetWizardMagic(CurrentWizardId);
	int32 NewAllocation = FMath::RoundToInt32(Value * static_cast<float>(State.ManaPerTurn));
	MagicSub->SetResearchAllocation(CurrentWizardId, NewAllocation);

	if (ManaAllocationLabel)
	{
		ManaAllocationLabel->SetText(FText::FromString(
			FString::Printf(TEXT("Research: %d / Cast Reserve: %d"),
				NewAllocation, State.ManaPerTurn - NewAllocation)));
	}
}

void UCoMSpellBookWidget::OnCastButtonClicked()    { OnCastClicked(); }
void UCoMSpellBookWidget::OnResearchButtonClicked() { OnResearchClicked(); }
void UCoMSpellBookWidget::OnCloseClicked()
{
	if (auto* UISS = GetGameInstance()->GetSubsystem<UCoMUISubsystem>())
	{
		UISS->HideSpellBook();
	}
}

void UCoMSpellBookWidget::OnLifeTabClicked()    { SelectRealm(ECoMSpellRealm::Life); }
void UCoMSpellBookWidget::OnDeathTabClicked()   { SelectRealm(ECoMSpellRealm::Death); }
void UCoMSpellBookWidget::OnChaosTabClicked()   { SelectRealm(ECoMSpellRealm::Chaos); }
void UCoMSpellBookWidget::OnNatureTabClicked()  { SelectRealm(ECoMSpellRealm::Nature); }
void UCoMSpellBookWidget::OnSorceryTabClicked() { SelectRealm(ECoMSpellRealm::Sorcery); }
void UCoMSpellBookWidget::OnArcaneTabClicked()  { SelectRealm(ECoMSpellRealm::Arcane); }
void UCoMSpellBookWidget::OnBindingTabClicked() { SelectRealm(ECoMSpellRealm::Binding); }
void UCoMSpellBookWidget::OnSpiritTabClicked()  { SelectRealm(ECoMSpellRealm::Spirit); }
void UCoMSpellBookWidget::OnGlamourTabClicked() { SelectRealm(ECoMSpellRealm::Glamour); }

// Page-turn arrows cycle through the realm "chapters" in tab order.
void UCoMSpellBookWidget::OnPrevPageClicked() { CycleRealm(-1); }
void UCoMSpellBookWidget::OnNextPageClicked() { CycleRealm(+1); }

void UCoMSpellBookWidget::CycleRealm(int32 Dir)
{
	// Chapter order matches the realm tab row.
	static const ECoMSpellRealm Order[] = {
		ECoMSpellRealm::Life,    ECoMSpellRealm::Death,   ECoMSpellRealm::Chaos,
		ECoMSpellRealm::Nature,  ECoMSpellRealm::Sorcery, ECoMSpellRealm::Arcane,
		ECoMSpellRealm::Binding, ECoMSpellRealm::Spirit,  ECoMSpellRealm::Glamour
	};
	const int32 Count = UE_ARRAY_COUNT(Order);

	int32 Idx = 0;
	for (int32 i = 0; i < Count; ++i)
	{
		if (Order[i] == CurrentRealm) { Idx = i; break; }
	}
	Idx = ((Idx + Dir) % Count + Count) % Count; // wrap both directions
	SelectRealm(Order[Idx]);
}

void UCoMSpellBookWidget::OnAutoResearchToggled(bool bIsChecked)
{
	UCoMMagicSubsystem* MagicSub = GetMagicSubsystem();
	if (!MagicSub || CurrentWizardId < 0)
	{
		return;
	}

	MagicSub->SetAutoResearch(CurrentWizardId, bIsChecked);

	// If enabling and nothing is being researched, auto-pick now.
	if (bIsChecked)
	{
		FCoMWizardMagicState& State = MagicSub->GetWizardMagic(CurrentWizardId);
		if (State.CurrentResearchSpell.IsNone())
		{
			MagicSub->AutoPickResearch(CurrentWizardId);
		}
	}

	// Refresh display.
	SetWizardId(CurrentWizardId);
}
