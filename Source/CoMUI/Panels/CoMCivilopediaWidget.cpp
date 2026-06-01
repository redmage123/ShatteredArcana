// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMCivilopediaWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

#include "CoMCore/Data/CoMSpellDatabase.h"
#include "CoMCore/Data/CoMUnitDatabase.h"
#include "CoMCore/Data/CoMBuildingDatabase.h"
#include "CoMCore/Data/CoMGlobalEnchantmentData.h"
#include "CoMUI/CoMUISubsystem.h"

#define LOCTEXT_NAMESPACE "CoMCivilopedia"

namespace CivColours
{
	static const FLinearColor BgDim   = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);
	static const FLinearColor Panel   = FLinearColor(0.04f, 0.03f, 0.10f, 0.97f);
	static const FLinearColor Tab     = FLinearColor(0.06f, 0.05f, 0.15f, 1.0f);
	static const FLinearColor TabHov  = FLinearColor(0.10f, 0.08f, 0.22f, 1.0f);
	static const FLinearColor TabSel  = FLinearColor(0.20f, 0.15f, 0.40f, 1.0f);
	static const FLinearColor Gold    = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver  = FLinearColor(0.85f, 0.85f, 0.90f, 1.0f);
	static const FLinearColor Grey    = FLinearColor(0.55f, 0.55f, 0.60f, 1.0f);
}

// ─── Per-category data accessors ──────────────────────────────────────────────

namespace
{
	struct FEntryRow
	{
		FName  ID;
		FString Display;
		FString Subline;     // realm/race/category hint shown in the list
	};

	const TCHAR* RealmTag(ECoMSpellRealm R)
	{
		switch (R)
		{
		case ECoMSpellRealm::Life:    return TEXT("Life");
		case ECoMSpellRealm::Death:   return TEXT("Death");
		case ECoMSpellRealm::Chaos:   return TEXT("Chaos");
		case ECoMSpellRealm::Nature:  return TEXT("Nature");
		case ECoMSpellRealm::Sorcery: return TEXT("Sorcery");
		case ECoMSpellRealm::Arcane:  return TEXT("Arcane");
		case ECoMSpellRealm::Binding: return TEXT("Binding");
		case ECoMSpellRealm::Spirit:  return TEXT("Spirit");
		case ECoMSpellRealm::Glamour: return TEXT("Glamour");
		default:                      return TEXT("None");
		}
	}

	const TCHAR* RarityTag(ECoMSpellRarity R)
	{
		switch (R)
		{
		case ECoMSpellRarity::Common:    return TEXT("Common");
		case ECoMSpellRarity::Uncommon:  return TEXT("Uncommon");
		case ECoMSpellRarity::Rare:      return TEXT("Rare");
		case ECoMSpellRarity::VeryRare:  return TEXT("Very Rare");
		default:                         return TEXT("");
		}
	}

	// Race lore — 15 races. Two-line hooks; expand later for full lore.
	struct FRaceLore { const TCHAR* Display; const TCHAR* Lore; };
	const TMap<FString, FRaceLore>& RaceLore()
	{
		static const TMap<FString, FRaceLore> M = {
			{ TEXT("HighMen"),    { TEXT("High Men"),    TEXT("Disciplined human realms of the sunlit plains. Balanced unit roster with strong infantry and cavalry. Founded the first Wizard's Guild.") }},
			{ TEXT("HighElves"),  { TEXT("High Elves"),  TEXT("Long-lived archers and mages of the silver forests. Higher upkeep, but units carry +1 attack and +1 defense after the first level.") }},
			{ TEXT("Dwarves"),    { TEXT("Dwarves"),     TEXT("Mountain craftsmen with the only Engineer of their tier. Cities are slow to grow but enormous when fed. Ranged stat is low; melee is iron.") }},
			{ TEXT("Draconians"), { TEXT("Draconians"),  TEXT("Winged warm-blooded reptiles. Their entire roster flies, ignoring rivers and rough terrain. Susceptible to Cold damage.") }},
			{ TEXT("DarkElves"),  { TEXT("Dark Elves"),  TEXT("Underground kin with deathless discipline. Every unit casts Doom Bolt once per battle. Sun magic burns them.") }},
			{ TEXT("Demons"),     { TEXT("Demons"),      TEXT("Infernyx natives only. High attack, low resistance. Their Summoning Circle is far cheaper than other races'.") }},
			{ TEXT("Merfolk"),    { TEXT("Merfolk"),     TEXT("Aquatic. Cities must be coastal or sunken. Naval units start with +2 movement. Cannot recruit cavalry.") }},
			{ TEXT("Halflings"),  { TEXT("Halflings"),   TEXT("Small but irrepressibly cheerful. +1 happiness in every city. Cap at 14 population per city.") }},
			{ TEXT("Orcs"),       { TEXT("Orcs"),        TEXT("Brutal melee specialists. Infantry costs 30% less production. Cannot research Life magic.") }},
			{ TEXT("Gnolls"),     { TEXT("Gnolls"),      TEXT("Scavenging plains stalkers. Gain 1 gold per enemy unit killed. No library, no smithy.") }},
			{ TEXT("Lizardmen"),  { TEXT("Lizardmen"),   TEXT("Swamp dwellers. Cities ignore swamp food penalty. Hatch full-strength replacements after every battle survival.") }},
			{ TEXT("Undead"),     { TEXT("Undead"),      TEXT("Animated remains. Immune to disease and poison. Cannot grow population organically; recruit-via-raise instead.") }},
			{ TEXT("Trolls"),     { TEXT("Trolls"),      TEXT("Regenerate 1 HP per round. Slow population growth. Weak to fire — Wall of Fire is devastating.") }},
			{ TEXT("Nomads"),     { TEXT("Nomads"),      TEXT("Desert wanderers. All units have +1 movement on desert. Caravan trade routes generate 50% more gold.") }},
			{ TEXT("Beastmen"),   { TEXT("Beastmen"),    TEXT("Horned plains-folk allied with primal spirits. Engineers can build sacred groves that buff adjacent armies.") }},
		};
		return M;
	}

	struct FMechanicEntry { const TCHAR* Display; const TCHAR* Body; };
	const TMap<FString, FMechanicEntry>& MechanicEntries()
	{
		static const TMap<FString, FMechanicEntry> M = {
			{ TEXT("Turns"),        { TEXT("Each Turn"),
				TEXT("A turn is a single overworld pulse. All wizards take their actions, then cities produce, armies regain movement, mana flows in, and research advances. Click End Turn at the bottom of the HUD to advance.") }},
			{ TEXT("Cities"),       { TEXT("Cities"),
				TEXT("Cities are your engine of production. Population grows from food surplus; gold and production come from buildings. Each city has a Focus (Growth / Military / Economy / Research / Production / Manual) that biases its automatic queue.") }},
			{ TEXT("Spells"),       { TEXT("Spells & Casting Skill"),
				TEXT("Each wizard has a Casting Skill (mana spent per turn on instant spells) and Research progress. Open the Spell Book to set what you research and what you cast. Combat spells fire from heroes or your own casting skill; global enchantments cost mana up front and a smaller amount per turn.") }},
			{ TEXT("Mana"),         { TEXT("Mana & Power"),
				TEXT("Mana fuels casting. It comes from your Power Base — a sum of Wizard's Guild, Shrine of Magic, and captured mana nodes. Power is split between Mana income, Research, and Skill points per turn in the Magic screen.") }},
			{ TEXT("Heroes"),       { TEXT("Heroes & Items"),
				TEXT("Heroes appear in taverns. Each hero has a kit of skills, a level cap (Adventurer → Hero → Champion → Demigod), and equip slots for forged artifacts. Use the Item Forge to create artifacts with skills like Flame Blade or Vampiric.") }},
			{ TEXT("Combat"),       { TEXT("Tactical Combat"),
				TEXT("When two hostile armies meet, combat opens on a 12x8 tactical grid. Each unit acts in initiative order. Ranged units shoot until pinned; melee charges; defenders gain +50% defense for the round. Heroes act first.") }},
			{ TEXT("Sieges"),       { TEXT("Sieges"),
				TEXT("Sit a hostile army adjacent to a rival city to begin a siege. After several turns of siege the city's garrison surrenders and the besieger captures it (population minus an unrest spike). City walls extend the siege duration.") }},
			{ TEXT("Planes"),       { TEXT("The Eight Planes"),
				TEXT("Aurelith and the seven sister planes are reachable via Ley Portals or the Earth Gate spell. Each plane has unique terrain, native races, and magical climate. Some races are home-plane locked.") }},
			{ TEXT("Diplomacy"),    { TEXT("Diplomacy"),
				TEXT("Rivals have personalities (aggressor / builder / scholar / zealot). They remember broken alliances and shared enemies. Trade spells, propose treaties, or declare war from the Diplomacy panel.") }},
			{ TEXT("Victory"),      { TEXT("Victory Conditions"),
				TEXT("Win by Spell of Mastery (research the Arcane endgame), Domination (own a majority of all cities), or Banishment (kill every rival wizard). At the turn cap, the highest-scoring wizard wins by default.") }},
		};
		return M;
	}

	const TCHAR* RealmFolder(ECoMSpellRealm R)
	{
		switch (R)
		{
		case ECoMSpellRealm::Life:    return TEXT("life");
		case ECoMSpellRealm::Death:   return TEXT("death");
		case ECoMSpellRealm::Chaos:   return TEXT("chaos");
		case ECoMSpellRealm::Nature:  return TEXT("nature");
		case ECoMSpellRealm::Sorcery: return TEXT("sorcery");
		case ECoMSpellRealm::Arcane:  return TEXT("arcane");
		case ECoMSpellRealm::Binding: return TEXT("binding");
		case ECoMSpellRealm::Spirit:  return TEXT("spirit");
		case ECoMSpellRealm::Glamour: return TEXT("glamour");
		default:                      return TEXT("universal");
		}
	}
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────

TSharedRef<SWidget> UCoMCivilopediaWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMCivilopediaWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	if (CloseButton)  CloseButton->OnClicked.AddDynamic(this, &UCoMCivilopediaWidget::OnCloseClicked);
	if (FilterBox)    FilterBox->OnTextChanged.AddDynamic(this, &UCoMCivilopediaWidget::OnFilterChanged);
	RebuildList();
}

void UCoMCivilopediaWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

void UCoMCivilopediaWidget::OnSpellsClicked()       { SetCategory(ECoMCivilopediaCategory::Spells); }
void UCoMCivilopediaWidget::OnUnitsClicked()        { SetCategory(ECoMCivilopediaCategory::Units); }
void UCoMCivilopediaWidget::OnBuildingsClicked()    { SetCategory(ECoMCivilopediaCategory::Buildings); }
void UCoMCivilopediaWidget::OnEnchantmentsClicked() { SetCategory(ECoMCivilopediaCategory::Enchantments); }
void UCoMCivilopediaWidget::OnRacesClicked()        { SetCategory(ECoMCivilopediaCategory::Races); }
void UCoMCivilopediaWidget::OnMechanicsClicked()    { SetCategory(ECoMCivilopediaCategory::Mechanics); }

void UCoMCivilopediaWidget::OnFilterChanged(const FText& Text)
{
	SetFilter(Text.ToString());
}

void UCoMCivilopediaWidget::SetCategory(ECoMCivilopediaCategory NewCategory)
{
	CurrentCategory = NewCategory;
	SelectedEntry = NAME_None;
	RebuildList();
	RebuildDetail();
}

void UCoMCivilopediaWidget::SetFilter(const FString& NewFilter)
{
	CurrentFilter = NewFilter;
	RebuildList();
}

void UCoMCivilopediaWidget::ShowEntry(FName EntryID)
{
	SelectedEntry = EntryID;
	RebuildDetail();
}

// ─── Layout ───────────────────────────────────────────────────────────────────

UButton* UCoMCivilopediaWidget::MakeCategoryButton(const FString& Label, ECoMCivilopediaCategory Cat)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle S = B->GetStyle();
	const FLinearColor BgC = (Cat == CurrentCategory) ? CivColours::TabSel : CivColours::Tab;
	S.Normal.DrawAs    = ESlateBrushDrawType::Box; S.Normal.TintColor    = FSlateColor(BgC);
	S.Hovered.DrawAs   = ESlateBrushDrawType::Box; S.Hovered.TintColor   = FSlateColor(CivColours::TabHov);
	S.Pressed.DrawAs   = ESlateBrushDrawType::Box; S.Pressed.TintColor   = FSlateColor(CivColours::TabSel);
	B->SetStyle(S);
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetText(FText::FromString(Label));
	T->SetColorAndOpacity(FSlateColor(CivColours::Gold));
	T->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = T->GetFont(); F.Size = 14; T->SetFont(F); }
	B->AddChild(T);
	return B;
}

void UCoMCivilopediaWidget::BuildLayout()
{
	RootBorder = WidgetTree->ConstructWidget<UBorder>();
	RootBorder->SetBrushColor(CivColours::BgDim);
	RootBorder->SetPadding(FMargin(0));

	UHorizontalBox* Frame = WidgetTree->ConstructWidget<UHorizontalBox>();
	RootBorder->AddChild(Frame);

	// --- Left column: category buttons ---
	{
		USizeBox* LeftBox = WidgetTree->ConstructWidget<USizeBox>();
		LeftBox->SetWidthOverride(160.0f);
		CategoryColumn = WidgetTree->ConstructWidget<UVerticalBox>();
		LeftBox->AddChild(CategoryColumn);
		UHorizontalBoxSlot* LS = Frame->AddChildToHorizontalBox(LeftBox);
		if (LS) { LS->SetPadding(FMargin(12)); }

		struct FTab { const TCHAR* Label; ECoMCivilopediaCategory Cat; void (UCoMCivilopediaWidget::*Cb)(); };
		const FTab Tabs[] = {
			{ TEXT("Spells"),       ECoMCivilopediaCategory::Spells,       &UCoMCivilopediaWidget::OnSpellsClicked },
			{ TEXT("Units"),        ECoMCivilopediaCategory::Units,        &UCoMCivilopediaWidget::OnUnitsClicked },
			{ TEXT("Buildings"),    ECoMCivilopediaCategory::Buildings,    &UCoMCivilopediaWidget::OnBuildingsClicked },
			{ TEXT("Enchantments"), ECoMCivilopediaCategory::Enchantments, &UCoMCivilopediaWidget::OnEnchantmentsClicked },
			{ TEXT("Races"),        ECoMCivilopediaCategory::Races,        &UCoMCivilopediaWidget::OnRacesClicked },
			{ TEXT("Mechanics"),    ECoMCivilopediaCategory::Mechanics,    &UCoMCivilopediaWidget::OnMechanicsClicked },
		};
		for (const FTab& T : Tabs)
		{
			UButton* B = MakeCategoryButton(T.Label, T.Cat);
			B->OnClicked.AddDynamic(this, T.Cb);
			UVerticalBoxSlot* VS = CategoryColumn->AddChildToVerticalBox(B);
			if (VS) { VS->SetPadding(FMargin(0, 4)); }
		}
	}

	// --- Middle column: search + list ---
	{
		USizeBox* MidBox = WidgetTree->ConstructWidget<USizeBox>();
		MidBox->SetWidthOverride(260.0f);
		UVerticalBox* MidCol = WidgetTree->ConstructWidget<UVerticalBox>();
		MidBox->AddChild(MidCol);

		FilterBox = WidgetTree->ConstructWidget<UEditableTextBox>();
		FilterBox->SetHintText(LOCTEXT("FilterHint", "Filter…"));
		UVerticalBoxSlot* FS = MidCol->AddChildToVerticalBox(FilterBox);
		if (FS) { FS->SetPadding(FMargin(0, 0, 0, 8)); }

		EntryListScroll = WidgetTree->ConstructWidget<UScrollBox>();
		MidCol->AddChildToVerticalBox(EntryListScroll);

		UHorizontalBoxSlot* MS = Frame->AddChildToHorizontalBox(MidBox);
		if (MS) { MS->SetPadding(FMargin(0, 12, 12, 12)); }
	}

	// --- Right column: detail ---
	{
		USizeBox* RightBox = WidgetTree->ConstructWidget<USizeBox>();
		RightBox->SetWidthOverride(560.0f);
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrushColor(CivColours::Panel);
		Card->SetPadding(FMargin(24));
		RightBox->AddChild(Card);

		UVerticalBox* DetailCol = WidgetTree->ConstructWidget<UVerticalBox>();
		Card->AddChild(DetailCol);

		// Close button
		CloseButton = WidgetTree->ConstructWidget<UButton>();
		{
			FButtonStyle CS = CloseButton->GetStyle();
			CS.Normal.DrawAs  = ESlateBrushDrawType::Box; CS.Normal.TintColor  = FSlateColor(CivColours::Tab);
			CS.Hovered.DrawAs = ESlateBrushDrawType::Box; CS.Hovered.TintColor = FSlateColor(CivColours::TabHov);
			CS.Pressed.DrawAs = ESlateBrushDrawType::Box; CS.Pressed.TintColor = FSlateColor(CivColours::Tab);
			CloseButton->SetStyle(CS);
			UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>();
			Lbl->SetText(LOCTEXT("Close", "Close"));
			Lbl->SetColorAndOpacity(FSlateColor(CivColours::Gold));
			Lbl->SetJustification(ETextJustify::Center);
			{ FSlateFontInfo F = Lbl->GetFont(); F.Size = 13; Lbl->SetFont(F); }
			CloseButton->AddChild(Lbl);
			USizeBox* CSB = WidgetTree->ConstructWidget<USizeBox>();
			CSB->SetWidthOverride(120.0f); CSB->SetHeightOverride(32.0f);
			CSB->AddChild(CloseButton);
			UVerticalBoxSlot* CS2 = DetailCol->AddChildToVerticalBox(CSB);
			if (CS2) { CS2->SetHorizontalAlignment(HAlign_Right); CS2->SetPadding(FMargin(0, 0, 0, 8)); }
		}

		DetailTitle = WidgetTree->ConstructWidget<UTextBlock>();
		DetailTitle->SetColorAndOpacity(FSlateColor(CivColours::Gold));
		{ FSlateFontInfo F = DetailTitle->GetFont(); F.Size = 22; DetailTitle->SetFont(F); }
		UVerticalBoxSlot* TS = DetailCol->AddChildToVerticalBox(DetailTitle);
		if (TS) { TS->SetPadding(FMargin(0, 0, 0, 4)); }

		DetailSubtitle = WidgetTree->ConstructWidget<UTextBlock>();
		DetailSubtitle->SetColorAndOpacity(FSlateColor(CivColours::Grey));
		{ FSlateFontInfo F = DetailSubtitle->GetFont(); F.Size = 12; DetailSubtitle->SetFont(F); }
		UVerticalBoxSlot* SS = DetailCol->AddChildToVerticalBox(DetailSubtitle);
		if (SS) { SS->SetPadding(FMargin(0, 0, 0, 12)); }

		// Optional art slot.
		USizeBox* ArtBox = WidgetTree->ConstructWidget<USizeBox>();
		ArtBox->SetWidthOverride(420.0f); ArtBox->SetHeightOverride(180.0f);
		DetailArt = WidgetTree->ConstructWidget<UImage>();
		ArtBox->AddChild(DetailArt);
		UVerticalBoxSlot* AS = DetailCol->AddChildToVerticalBox(ArtBox);
		if (AS) { AS->SetPadding(FMargin(0, 0, 0, 12)); AS->SetHorizontalAlignment(HAlign_Left); }

		DetailScroll = WidgetTree->ConstructWidget<UScrollBox>();
		DetailBody = WidgetTree->ConstructWidget<UVerticalBox>();
		DetailScroll->AddChild(DetailBody);
		UVerticalBoxSlot* DSS = DetailCol->AddChildToVerticalBox(DetailScroll);
		if (DSS) { DSS->SetSize(ESlateSizeRule::Fill); }

		UHorizontalBoxSlot* RS = Frame->AddChildToHorizontalBox(RightBox);
		if (RS) { RS->SetPadding(FMargin(0, 12, 12, 12)); }
	}

	if (WidgetTree)
	{
		WidgetTree->RootWidget = RootBorder;
	}
}

// ─── List + detail rebuilds ───────────────────────────────────────────────────

static void GatherEntries(ECoMCivilopediaCategory Cat, TArray<FEntryRow>& Out)
{
	Out.Reset();
	switch (Cat)
	{
	case ECoMCivilopediaCategory::Spells:
	{
		for (const FName& ID : CoMSpellDatabase::GetAllSpellIDs())
		{
			const FCoMSpellInfo Info = CoMSpellDatabase::GetSpellInfo(ID);
			FEntryRow R;
			R.ID = ID;
			R.Display = Info.DisplayName.IsEmpty() ? ID.ToString() : Info.DisplayName.ToString();
			R.Subline = FString::Printf(TEXT("%s · %s"), RealmTag(Info.Realm), RarityTag(Info.Rarity));
			Out.Add(R);
		}
		break;
	}
	case ECoMCivilopediaCategory::Units:
	{
		for (const FName& ID : CoMUnitDatabase::GetAllUnitSpecIDs())
		{
			const FCoMUnitSpecInfo& Info = CoMUnitDatabase::GetUnitSpec(ID);
			FEntryRow R;
			R.ID = ID;
			R.Display = Info.DisplayName.IsEmpty() ? ID.ToString() : Info.DisplayName.ToString();
			R.Subline = Info.RaceTag;
			Out.Add(R);
		}
		break;
	}
	case ECoMCivilopediaCategory::Buildings:
	{
		for (const FName& ID : CoMBuildingDatabase::GetAllBuildingIDs())
		{
			const FCoMBuildingInfo& Info = CoMBuildingDatabase::GetBuildingInfo(ID);
			FEntryRow R;
			R.ID = ID;
			R.Display = Info.DisplayName.IsEmpty() ? ID.ToString() : Info.DisplayName.ToString();
			R.Subline = FString::Printf(TEXT("%d production"), Info.ProductionCost);
			Out.Add(R);
		}
		break;
	}
	case ECoMCivilopediaCategory::Enchantments:
	{
		for (const FCoMGlobalEnchantmentDef& Def : CoMGlobalEnchantmentData::GetAll())
		{
			FEntryRow R;
			R.ID = Def.SpellID;
			R.Display = Def.DisplayName.IsEmpty() ? Def.SpellID.ToString() : Def.DisplayName.ToString();
			R.Subline = RealmTag(Def.Realm);
			Out.Add(R);
		}
		break;
	}
	case ECoMCivilopediaCategory::Races:
		for (const auto& P : RaceLore())
		{
			FEntryRow R;
			R.ID      = FName(*P.Key);
			R.Display = P.Value.Display;
			R.Subline = TEXT("Race");
			Out.Add(R);
		}
		break;
	case ECoMCivilopediaCategory::Mechanics:
		for (const auto& P : MechanicEntries())
		{
			FEntryRow R;
			R.ID      = FName(*P.Key);
			R.Display = P.Value.Display;
			R.Subline = TEXT("Mechanic");
			Out.Add(R);
		}
		break;
	}
	Out.Sort([](const FEntryRow& A, const FEntryRow& B) { return A.Display < B.Display; });
}

void UCoMCivilopediaWidget::RebuildList()
{
	if (!EntryListScroll) return;
	EntryListScroll->ClearChildren();

	TArray<FEntryRow> Rows;
	GatherEntries(CurrentCategory, Rows);

	const FString Lower = CurrentFilter.ToLower();
	int32 Shown = 0;
	for (const FEntryRow& R : Rows)
	{
		if (!Lower.IsEmpty()
			&& !R.Display.ToLower().Contains(Lower)
			&& !R.Subline.ToLower().Contains(Lower)
			&& !R.ID.ToString().ToLower().Contains(Lower))
		{
			continue;
		}
		UBorder* Row = WidgetTree->ConstructWidget<UBorder>();
		const FLinearColor RowBg = (R.ID == SelectedEntry) ? CivColours::TabSel : CivColours::Tab;
		Row->SetBrushColor(RowBg);
		Row->SetPadding(FMargin(8, 6));

		UVerticalBox* RowCol = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
		Title->SetText(FText::FromString(R.Display));
		Title->SetColorAndOpacity(FSlateColor(CivColours::Gold));
		{ FSlateFontInfo F = Title->GetFont(); F.Size = 13; Title->SetFont(F); }
		RowCol->AddChildToVerticalBox(Title);

		if (!R.Subline.IsEmpty())
		{
			UTextBlock* Sub = WidgetTree->ConstructWidget<UTextBlock>();
			Sub->SetText(FText::FromString(R.Subline));
			Sub->SetColorAndOpacity(FSlateColor(CivColours::Grey));
			{ FSlateFontInfo F = Sub->GetFont(); F.Size = 10; Sub->SetFont(F); }
			RowCol->AddChildToVerticalBox(Sub);
		}

		Row->AddChild(RowCol);
		EntryListScroll->AddChild(Row);

		// First entry selected on category switch, so detail isn't empty.
		if (Shown++ == 0 && SelectedEntry.IsNone())
		{
			SelectedEntry = R.ID;
		}
	}

	RebuildDetail();
}

void UCoMCivilopediaWidget::RebuildDetail()
{
	if (!DetailTitle || !DetailSubtitle || !DetailBody) return;
	DetailBody->ClearChildren();

	if (SelectedEntry.IsNone())
	{
		DetailTitle->SetText(LOCTEXT("Empty", "—"));
		DetailSubtitle->SetText(FText::GetEmpty());
		return;
	}

	auto AddPara = [this](const FString& Text)
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Text));
		T->SetColorAndOpacity(FSlateColor(CivColours::Silver));
		T->SetAutoWrapText(true);
		{ FSlateFontInfo F = T->GetFont(); F.Size = 12; T->SetFont(F); }
		UVerticalBoxSlot* S = DetailBody->AddChildToVerticalBox(T);
		if (S) { S->SetPadding(FMargin(0, 0, 0, 6)); }
	};
	auto SetArt = [this](const FString& AssetPath)
	{
		if (!DetailArt) return;
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *AssetPath);
		if (!Tex) { DetailArt->SetBrushFromTexture(nullptr); return; }
		DetailArt->SetBrushFromTexture(Tex);
	};

	switch (CurrentCategory)
	{
	case ECoMCivilopediaCategory::Spells:
	{
		const FCoMSpellInfo Info = CoMSpellDatabase::GetSpellInfo(SelectedEntry);
		DetailTitle->SetText(Info.DisplayName.IsEmpty() ? FText::FromName(SelectedEntry) : Info.DisplayName);
		DetailSubtitle->SetText(FText::FromString(FString::Printf(
			TEXT("%s realm · %s · Research %d · Cast %d"),
			RealmTag(Info.Realm), RarityTag(Info.Rarity), Info.ResearchCost, Info.CastingCost)));
		if (Info.DamageBase  > 0) AddPara(FString::Printf(TEXT("Base damage: %d"), Info.DamageBase));
		if (Info.HealAmount  > 0) AddPara(FString::Printf(TEXT("Healing: %d"), Info.HealAmount));
		if (Info.bAOE)            AddPara(FString::Printf(TEXT("Area effect (radius %d)"), Info.AOERadius));
		if (Info.bSummon)         AddPara(TEXT("Summons a creature for your army."));
		if (Info.bOngoing)        AddPara(FString::Printf(TEXT("Ongoing effect — %d mana upkeep per turn."), Info.UpkeepMana));
		AddPara(FString::Printf(TEXT("Range: %d. %s."),
			Info.Range, Info.bInstant ? TEXT("Instant") : TEXT("Sustained")));
		// VFX preview slot uses the realm-default sheet.
		SetArt(FString::Printf(TEXT("/Game/Textures/SpellVFX/%s/impact_flash_sheet.impact_flash_sheet"),
			RealmFolder(Info.Realm)));
		break;
	}
	case ECoMCivilopediaCategory::Units:
	{
		const FCoMUnitSpecInfo& Info = CoMUnitDatabase::GetUnitSpec(SelectedEntry);
		DetailTitle->SetText(Info.DisplayName.IsEmpty() ? FText::FromName(SelectedEntry) : Info.DisplayName);
		DetailSubtitle->SetText(FText::FromString(FString::Printf(
			TEXT("%s · %d figures · %d production"),
			*Info.RaceTag, Info.Figures, Info.ProductionCost)));
		AddPara(FString::Printf(TEXT("Melee %d · Ranged %d · Defense %d · Resistance %d · HP %d · Move %d"),
			Info.MeleeAttack, Info.RangedAttack, Info.Defense, Info.Resistance,
			Info.HitPoints, Info.Movement));
		if (Info.bEngineer) AddPara(TEXT("Engineer — can build roads and outpost mines on the overworld."));
		SetArt(FString::Printf(TEXT("/Game/UI/Units/%s.%s"), *SelectedEntry.ToString(), *SelectedEntry.ToString()));
		break;
	}
	case ECoMCivilopediaCategory::Buildings:
	{
		const FCoMBuildingInfo& Info = CoMBuildingDatabase::GetBuildingInfo(SelectedEntry);
		DetailTitle->SetText(Info.DisplayName.IsEmpty() ? FText::FromName(SelectedEntry) : Info.DisplayName);
		DetailSubtitle->SetText(FText::FromString(FString::Printf(
			TEXT("Cost %d · Upkeep %d gold"), Info.ProductionCost, Info.UpkeepGold)));
		FString Stats;
		auto Add = [&Stats](const TCHAR* Lbl, int32 V) { if (V != 0) Stats += FString::Printf(TEXT("%s %+d  "), Lbl, V); };
		Add(TEXT("Food"), Info.FoodBonus);
		Add(TEXT("Gold"), Info.GoldBonus);
		Add(TEXT("Prod"), Info.ProductionBonus);
		Add(TEXT("Mana"), Info.ManaBonus);
		Add(TEXT("Research"), Info.ResearchBonus);
		Add(TEXT("Unrest"), -Info.UnrestReduction);
		Add(TEXT("Happiness"), Info.HappinessBonus);
		Add(TEXT("PopCap"), Info.PopCapBonus);
		if (Info.WallHP > 0)               Stats += FString::Printf(TEXT("Wall HP %d  "), Info.WallHP);
		if (Info.GoldMultiplierPercent != 100) Stats += FString::Printf(TEXT("Gold x%.2f  "), Info.GoldMultiplierPercent / 100.0f);
		if (!Stats.IsEmpty()) AddPara(Stats);
		if (Info.bCoastalOnly)            AddPara(TEXT("Coastal cities only."));
		if (Info.bEnablesHeroRecruitment) AddPara(TEXT("Enables hero recruitment in this city."));
		if (Info.bEnablesSummoning)       AddPara(TEXT("Enables summoning spells in this city."));
		if (!Info.EnablesUnitTag.IsEmpty()) AddPara(FString::Printf(TEXT("Unlocks %s units."), *Info.EnablesUnitTag));
		SetArt(FString::Printf(TEXT("/Game/Textures/CityView/%s_complete.%s_complete"),
			*SelectedEntry.ToString(), *SelectedEntry.ToString()));
		break;
	}
	case ECoMCivilopediaCategory::Enchantments:
	{
		const FCoMGlobalEnchantmentDef& Def = CoMGlobalEnchantmentData::Get(SelectedEntry);
		DetailTitle->SetText(Def.DisplayName.IsEmpty() ? FText::FromName(SelectedEntry) : Def.DisplayName);
		DetailSubtitle->SetText(FText::FromString(RealmTag(Def.Realm)));
		if (!Def.FlavorText.IsEmpty()) AddPara(Def.FlavorText.ToString());
		AddPara(FString::Printf(TEXT("Magnitude: %d"), Def.Magnitude));
		if (!Def.CardImageSlug.IsEmpty())
		{
			SetArt(FString::Printf(TEXT("/Game/UI/Enchantments/%s.%s"),
				*Def.CardImageSlug, *Def.CardImageSlug));
		}
		break;
	}
	case ECoMCivilopediaCategory::Races:
	{
		const FRaceLore* Lore = RaceLore().Find(SelectedEntry.ToString());
		if (Lore)
		{
			DetailTitle->SetText(FText::FromString(Lore->Display));
			DetailSubtitle->SetText(LOCTEXT("RaceSub", "Playable race"));
			AddPara(Lore->Lore);
		}
		break;
	}
	case ECoMCivilopediaCategory::Mechanics:
	{
		const FMechanicEntry* M = MechanicEntries().Find(SelectedEntry.ToString());
		if (M)
		{
			DetailTitle->SetText(FText::FromString(M->Display));
			DetailSubtitle->SetText(LOCTEXT("MechanicSub", "How it works"));
			AddPara(M->Body);
		}
		break;
	}
	}
}

// Console: open the Civilopedia directly. No category arg needed; defaults to Spells.
//   com.show_civilopedia
static FAutoConsoleCommandWithWorldAndArgs GShowCivilopediaCmd(
	TEXT("com.show_civilopedia"),
	TEXT("Open the in-game encyclopedia (spells, units, buildings, races, mechanics)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& /*Args*/, UWorld* World)
		{
			if (!World) { return; }
			UCoMCivilopediaWidget* W = CreateWidget<UCoMCivilopediaWidget>(
				World, UCoMCivilopediaWidget::StaticClass());
			if (W)
			{
				W->AddToViewport(100);
			}
		}));

#undef LOCTEXT_NAMESPACE
