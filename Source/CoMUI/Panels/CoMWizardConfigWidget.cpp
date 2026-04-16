// Copyright Mythforge Studios. All Rights Reserved.
// CoMWizardConfigWidget.cpp -- Screen 2: Spell book allocation, retorts, and game start.

#include "CoMWizardConfigWidget.h"

#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Spacer.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

#include "Kismet/GameplayStatics.h"
#include "Framework/CoMGameInstance.h"
#include "Framework/CoMOverworldGameMode.h"
#include "CoMUISubsystem.h"

// =============================================================================
// Colour constants
// =============================================================================

namespace WizConfigColours
{
	static const FLinearColor Background   = FLinearColor(0.039f, 0.039f, 0.102f, 1.0f);
	static const FLinearColor PanelBg      = FLinearColor(0.086f, 0.129f, 0.243f, 1.0f);
	static const FLinearColor Gold         = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim      = FLinearColor(0.855f, 0.647f, 0.125f, 0.4f);
	static const FLinearColor White        = FLinearColor::White;
	static const FLinearColor Grey         = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	static const FLinearColor LightGrey    = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
	static const FLinearColor DarkButton   = FLinearColor(0.06f, 0.06f, 0.15f, 1.0f);
	static const FLinearColor ButtonHover  = FLinearColor(0.102f, 0.165f, 0.306f, 1.0f);
	static const FLinearColor DisabledGrey = FLinearColor(0.3f, 0.3f, 0.3f, 0.5f);
	static const FLinearColor SlotEmpty    = FLinearColor(0.15f, 0.15f, 0.2f, 0.5f);

	// Realm colours
	static const FLinearColor RealmLife    = FLinearColor(1.0f, 0.98f, 0.88f, 1.0f);
	static const FLinearColor RealmDeath   = FLinearColor(0.4f, 0.0f, 0.4f, 1.0f);
	static const FLinearColor RealmChaos   = FLinearColor(1.0f, 0.2f, 0.0f, 1.0f);
	static const FLinearColor RealmNature  = FLinearColor(0.0f, 0.8f, 0.0f, 1.0f);
	static const FLinearColor RealmSorcery = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);
	static const FLinearColor RealmArcane  = FLinearColor(0.8f, 0.6f, 0.0f, 1.0f);
	static const FLinearColor RealmBinding = FLinearColor(0.6f, 0.0f, 0.0f, 1.0f);
	static const FLinearColor RealmSpirit  = FLinearColor(0.6f, 0.4f, 1.0f, 1.0f);
	static const FLinearColor RealmGlamour = FLinearColor(1.0f, 0.4f, 0.8f, 1.0f);
}

// =============================================================================
// Realm metadata
// =============================================================================

struct FRealmInfo
{
	ECoMSpellRealm Realm;
	FString Name;
	FLinearColor Color;
};

static const FRealmInfo GRealmInfos[] =
{
	{ ECoMSpellRealm::Life,    TEXT("Life"),    WizConfigColours::RealmLife },
	{ ECoMSpellRealm::Death,   TEXT("Death"),   WizConfigColours::RealmDeath },
	{ ECoMSpellRealm::Chaos,   TEXT("Chaos"),   WizConfigColours::RealmChaos },
	{ ECoMSpellRealm::Nature,  TEXT("Nature"),  WizConfigColours::RealmNature },
	{ ECoMSpellRealm::Sorcery, TEXT("Sorcery"), WizConfigColours::RealmSorcery },
	{ ECoMSpellRealm::Arcane,  TEXT("Arcane"),  WizConfigColours::RealmArcane },
	{ ECoMSpellRealm::Binding, TEXT("Binding"), WizConfigColours::RealmBinding },
	{ ECoMSpellRealm::Spirit,  TEXT("Spirit"),  WizConfigColours::RealmSpirit },
	{ ECoMSpellRealm::Glamour, TEXT("Glamour"), WizConfigColours::RealmGlamour },
};

// =============================================================================
// Retort metadata
// =============================================================================

struct FRetortInfo
{
	FName ID;
	FString DisplayName;
	int32 PickCost;
	FString Description;
};

static const FRetortInfo GRetortInfos[] =
{
	{ FName(TEXT("Archmage")),       TEXT("Archmage"),       2, TEXT("Spell research +50%, casting skill +10") },
	{ FName(TEXT("Channeler")),      TEXT("Channeler"),      2, TEXT("Spell casting cost -25% in combat and overworld") },
	{ FName(TEXT("SageMaster")),     TEXT("Sage Master"),    2, TEXT("Research output +25% from all sources") },
	{ FName(TEXT("Famous")),         TEXT("Famous"),         2, TEXT("Start with extra fame, heroes cost less") },
	{ FName(TEXT("Myrran")),         TEXT("Myrran"),         3, TEXT("Start on Noctharion with rare units") },
	{ FName(TEXT("Warlord")),        TEXT("Warlord"),        2, TEXT("All units gain +1 level, elite units easier") },
	{ FName(TEXT("ChaosMastery")),   TEXT("Chaos Mastery"),  1, TEXT("Chaos spells cost -15%, chaos nodes yield +50%") },
	{ FName(TEXT("LifeMastery")),    TEXT("Life Mastery"),   1, TEXT("Life spells cost -15%, life nodes yield +50%") },
	{ FName(TEXT("DeathMastery")),   TEXT("Death Mastery"),  1, TEXT("Death spells cost -15%, death nodes yield +50%") },
	{ FName(TEXT("NatureMastery")),  TEXT("Nature Mastery"), 1, TEXT("Nature spells cost -15%, nature nodes yield +50%") },
	{ FName(TEXT("SorceryMastery")), TEXT("Sorcery Mastery"),1, TEXT("Sorcery spells cost -15%, sorcery nodes yield +50%") },
	{ FName(TEXT("DivinePower")),    TEXT("Divine Power"),   2, TEXT("Life/Spirit realm power +50%") },
	{ FName(TEXT("InfernalPower")),  TEXT("Infernal Power"), 2, TEXT("Death/Chaos/Binding realm power +50%") },
	{ FName(TEXT("Conjurer")),       TEXT("Conjurer"),       2, TEXT("Summoning spells cost -25%") },
	{ FName(TEXT("NodeMastery")),    TEXT("Node Mastery"),   1, TEXT("Meld any node regardless of realm") },
	{ FName(TEXT("Artificer")),      TEXT("Artificer"),      2, TEXT("Create magic items, enchant equipment") },
};

// Difficulty metadata
struct FDifficultyInfo
{
	FString Name;
	FString Description;
};

static const FDifficultyInfo GDifficultyInfos[] =
{
	{ TEXT("Easy"),       TEXT("AI gets penalties") },
	{ TEXT("Normal"),     TEXT("Standard") },
	{ TEXT("Hard"),       TEXT("+25% AI income") },
	{ TEXT("Lunatic"),    TEXT("+50% income, +25% combat") },
	{ TEXT("Impossible"), TEXT("+100% income, extra cities") },
};

// Wizard portrait names
static const FString GWizardNames[] = {
	TEXT("Merlin"), TEXT("Morgana"), TEXT("Zephyros"), TEXT("Hecate"),
	TEXT("Malachar"), TEXT("Lunara"), TEXT("Grimnar"), TEXT("Nekros"),
	TEXT("Gaia"), TEXT("Pyraxis"), TEXT("Glaciel"), TEXT("Aldric"),
	TEXT("Lilith"), TEXT("Solarius")
};

// Retort icon symbols
static const FString GRetortIcons[] = {
	TEXT("\u2726"), // Archmage
	TEXT("\u26A1"), // Channeler
	TEXT("\u2622"), // Sage Master
	TEXT("\u2605"), // Famous
	TEXT("\u263D"), // Myrran
	TEXT("\u2694"), // Warlord
	TEXT("\u2668"), // Chaos Mastery
	TEXT("\u2600"), // Life Mastery
	TEXT("\u2620"), // Death Mastery
	TEXT("\u2618"), // Nature Mastery
	TEXT("\u2728"), // Sorcery Mastery
	TEXT("\u271D"), // Divine Power
	TEXT("\u2666"), // Infernal Power
	TEXT("\u2721"), // Conjurer
	TEXT("\u2699"), // Node Mastery
	TEXT("\u2692"), // Artificer
};

// =============================================================================
// Public interface
// =============================================================================

void UCoMWizardConfigWidget::SetPortraitIndex(int32 InPortraitIndex)
{
	SelectedPortraitIndex = InPortraitIndex;
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMWizardConfigWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
}

TSharedRef<SWidget> UCoMWizardConfigWidget::RebuildWidget()
{
	BuildLayout();
	return Super::RebuildWidget();
}

// =============================================================================
// Spell book add/remove callbacks
// =============================================================================

void UCoMWizardConfigWidget::OnAddBook0()    { OnAddBook(0); }
void UCoMWizardConfigWidget::OnAddBook1()    { OnAddBook(1); }
void UCoMWizardConfigWidget::OnAddBook2()    { OnAddBook(2); }
void UCoMWizardConfigWidget::OnAddBook3()    { OnAddBook(3); }
void UCoMWizardConfigWidget::OnAddBook4()    { OnAddBook(4); }
void UCoMWizardConfigWidget::OnAddBook5()    { OnAddBook(5); }
void UCoMWizardConfigWidget::OnAddBook6()    { OnAddBook(6); }
void UCoMWizardConfigWidget::OnAddBook7()    { OnAddBook(7); }
void UCoMWizardConfigWidget::OnAddBook8()    { OnAddBook(8); }

void UCoMWizardConfigWidget::OnRemoveBook0() { OnRemoveBook(0); }
void UCoMWizardConfigWidget::OnRemoveBook1() { OnRemoveBook(1); }
void UCoMWizardConfigWidget::OnRemoveBook2() { OnRemoveBook(2); }
void UCoMWizardConfigWidget::OnRemoveBook3() { OnRemoveBook(3); }
void UCoMWizardConfigWidget::OnRemoveBook4() { OnRemoveBook(4); }
void UCoMWizardConfigWidget::OnRemoveBook5() { OnRemoveBook(5); }
void UCoMWizardConfigWidget::OnRemoveBook6() { OnRemoveBook(6); }
void UCoMWizardConfigWidget::OnRemoveBook7() { OnRemoveBook(7); }
void UCoMWizardConfigWidget::OnRemoveBook8() { OnRemoveBook(8); }

// =============================================================================
// Retort callbacks
// =============================================================================

void UCoMWizardConfigWidget::OnRetort0()  { OnRetortToggled(0); }
void UCoMWizardConfigWidget::OnRetort1()  { OnRetortToggled(1); }
void UCoMWizardConfigWidget::OnRetort2()  { OnRetortToggled(2); }
void UCoMWizardConfigWidget::OnRetort3()  { OnRetortToggled(3); }
void UCoMWizardConfigWidget::OnRetort4()  { OnRetortToggled(4); }
void UCoMWizardConfigWidget::OnRetort5()  { OnRetortToggled(5); }
void UCoMWizardConfigWidget::OnRetort6()  { OnRetortToggled(6); }
void UCoMWizardConfigWidget::OnRetort7()  { OnRetortToggled(7); }
void UCoMWizardConfigWidget::OnRetort8()  { OnRetortToggled(8); }
void UCoMWizardConfigWidget::OnRetort9()  { OnRetortToggled(9); }
void UCoMWizardConfigWidget::OnRetort10() { OnRetortToggled(10); }
void UCoMWizardConfigWidget::OnRetort11() { OnRetortToggled(11); }
void UCoMWizardConfigWidget::OnRetort12() { OnRetortToggled(12); }
void UCoMWizardConfigWidget::OnRetort13() { OnRetortToggled(13); }
void UCoMWizardConfigWidget::OnRetort14() { OnRetortToggled(14); }
void UCoMWizardConfigWidget::OnRetort15() { OnRetortToggled(15); }

// =============================================================================
// Difficulty callbacks
// =============================================================================

void UCoMWizardConfigWidget::OnDiffEasyClicked()       { OnDifficultySelected(0); }
void UCoMWizardConfigWidget::OnDiffNormalClicked()     { OnDifficultySelected(1); }
void UCoMWizardConfigWidget::OnDiffHardClicked()       { OnDifficultySelected(2); }
void UCoMWizardConfigWidget::OnDiffLunaticClicked()    { OnDifficultySelected(3); }
void UCoMWizardConfigWidget::OnDiffImpossibleClicked() { OnDifficultySelected(4); }

// =============================================================================
// Logic -- spell books
// =============================================================================

void UCoMWizardConfigWidget::OnAddBook(int32 RealmIndex)
{
	if (RealmIndex < 0 || RealmIndex >= NumRealms) return;
	if (GetPicksRemaining() <= 0) return;
	if (BookCounts[RealmIndex] >= MaxBooksPerRealm) return;

	BookCounts[RealmIndex]++;

	UpdatePicksDisplay();
	UpdateBookDisplay();
	UpdateRetortButtons();
	UpdateStartingSpells();
}

void UCoMWizardConfigWidget::OnRemoveBook(int32 RealmIndex)
{
	if (RealmIndex < 0 || RealmIndex >= NumRealms) return;
	if (BookCounts[RealmIndex] <= 0) return;

	BookCounts[RealmIndex]--;

	UpdatePicksDisplay();
	UpdateBookDisplay();
	UpdateRetortButtons();
	UpdateStartingSpells();
}

// =============================================================================
// Logic -- retorts
// =============================================================================

void UCoMWizardConfigWidget::OnRetortToggled(int32 RetortIndex)
{
	if (RetortIndex < 0 || RetortIndex >= NumRetorts) return;

	if (SelectedRetortIndices.Contains(RetortIndex))
	{
		SelectedRetortIndices.Remove(RetortIndex);
	}
	else
	{
		int32 Cost = GRetortInfos[RetortIndex].PickCost;
		if (GetPicksRemaining() >= Cost)
		{
			SelectedRetortIndices.Add(RetortIndex);
		}
	}

	UpdatePicksDisplay();
	UpdateBookDisplay();
	UpdateRetortButtons();
	UpdateStartingSpells();
}

// =============================================================================
// Logic -- difficulty
// =============================================================================

void UCoMWizardConfigWidget::OnDifficultySelected(int32 Level)
{
	SelectedDifficulty = FMath::Clamp(Level, 0, 4);
	UpdateDifficultyButtonStyles();
}

// =============================================================================
// Navigation
// =============================================================================

void UCoMWizardConfigWidget::OnBackClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->HideWizardConfig();
			// Portrait selection screen should still be visible underneath
		}
	}
}

void UCoMWizardConfigWidget::OnStartGameClicked()
{
	FText CurrentName = NameInputBox ? NameInputBox->GetText() : FText::GetEmpty();
	if (CurrentName.IsEmpty())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Wizard name cannot be empty!"));
		}
		return;
	}

	UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance());
	if (!CoMGI)
	{
		UE_LOG(LogTemp, Error, TEXT("CoMWizardConfigWidget: Could not get CoMGameInstance."));
		return;
	}

	CoMGI->NewGameSettings = BuildSettings();
	CoMGI->LoadedSaveSlotName.Empty();

	UWorld* World = GetWorld();
	AGameModeBase* GM = World ? World->GetAuthGameMode() : nullptr;
	if (ACoMOverworldGameMode* OverworldGM = Cast<ACoMOverworldGameMode>(GM))
	{
		OverworldGM->StartNewGame(1);
	}
	else
	{
		UGameplayStatics::OpenLevel(this, FName(TEXT("L_Overworld")));
	}
}

// =============================================================================
// BuildSettings
// =============================================================================

FCoMNewGameSettings UCoMWizardConfigWidget::BuildSettings() const
{
	FCoMNewGameSettings Settings;

	Settings.WizardName = NameInputBox ? NameInputBox->GetText() : FText::FromString(TEXT("Wizard"));
	Settings.WizardClass = ECoMWizardClass::Wizard;
	Settings.DifficultyLevel = SelectedDifficulty;

	// Store portrait path
	if (SelectedPortraitIndex >= 0 && SelectedPortraitIndex < NumPortraits)
	{
		Settings.WizardPortraitPath = FSoftObjectPath(FString::Printf(
			TEXT("/Game/Textures/Wizards/wizard_%02d.wizard_%02d"),
			SelectedPortraitIndex + 1, SelectedPortraitIndex + 1));
	}

	// Populate spell book allocation
	const UEnum* RealmEnum = StaticEnum<ECoMSpellRealm>();
	for (int32 i = 0; i < NumRealms; ++i)
	{
		if (BookCounts[i] > 0 && RealmEnum)
		{
			FString RealmName = RealmEnum->GetNameStringByValue(static_cast<int64>(GRealmInfos[i].Realm));
			for (int32 b = 0; b < BookCounts[i]; ++b)
			{
				Settings.StartingSpells.Add(FName(*FString::Printf(TEXT("Book_%s"), *RealmName)));
			}
		}
	}

	// Store selected retorts
	for (int32 Idx : SelectedRetortIndices)
	{
		if (Idx >= 0 && Idx < NumRetorts)
		{
			Settings.ChosenRetorts.Add(GRetortInfos[Idx].ID);
		}
	}

	return Settings;
}

// =============================================================================
// Pick accounting
// =============================================================================

int32 UCoMWizardConfigWidget::GetPicksSpent() const
{
	int32 Spent = 0;

	for (int32 i = 0; i < NumRealms; ++i)
	{
		Spent += BookCounts[i];
	}

	for (int32 Idx : SelectedRetortIndices)
	{
		if (Idx >= 0 && Idx < NumRetorts)
		{
			Spent += GRetortInfos[Idx].PickCost;
		}
	}

	return Spent;
}

int32 UCoMWizardConfigWidget::GetPicksRemaining() const
{
	return TotalPicks - GetPicksSpent();
}

// =============================================================================
// Tier name helper
// =============================================================================

FString UCoMWizardConfigWidget::GetTierName(int32 BookCount)
{
	if (BookCount <= 0) return TEXT("None");
	if (BookCount == 1) return TEXT("Common");
	if (BookCount <= 3) return TEXT("Uncommon");
	if (BookCount <= 5) return TEXT("Rare");
	return TEXT("Very Rare");
}

// =============================================================================
// UI update helpers
// =============================================================================

void UCoMWizardConfigWidget::UpdatePicksDisplay()
{
	if (PicksRemainingText)
	{
		int32 Remaining = GetPicksRemaining();
		PicksRemainingText->SetText(FText::FromString(
			FString::Printf(TEXT("Picks Remaining: %d / %d"), Remaining, TotalPicks)));

		PicksRemainingText->SetColorAndOpacity(FSlateColor(
			Remaining > 0 ? WizConfigColours::Gold : FLinearColor(1.0f, 0.3f, 0.3f, 1.0f)));
	}
}

void UCoMWizardConfigWidget::UpdateBookDisplay()
{
	int32 Remaining = GetPicksRemaining();

	for (int32 i = 0; i < NumRealms; ++i)
	{
		// Update book count text
		if (BookCountTexts[i])
		{
			BookCountTexts[i]->SetText(FText::FromString(
				FString::Printf(TEXT("%d/%d"), BookCounts[i], MaxBooksPerRealm)));
		}

		// Update tier text
		if (TierTexts[i])
		{
			FString Tier = GetTierName(BookCounts[i]);
			TierTexts[i]->SetText(FText::FromString(Tier));

			FLinearColor TierColor = WizConfigColours::Grey;
			if (BookCounts[i] >= 6) TierColor = FLinearColor(1.0f, 0.85f, 0.0f, 1.0f);
			else if (BookCounts[i] >= 4) TierColor = FLinearColor(0.6f, 0.4f, 1.0f, 1.0f);
			else if (BookCounts[i] >= 2) TierColor = FLinearColor(0.3f, 0.7f, 1.0f, 1.0f);
			else if (BookCounts[i] >= 1) TierColor = WizConfigColours::LightGrey;
			TierTexts[i]->SetColorAndOpacity(FSlateColor(TierColor));
		}

		// Update book slot visuals
		for (int32 s = 0; s < MaxBooksPerRealm; ++s)
		{
			if (BookSlotBorders[i][s])
			{
				if (s < BookCounts[i])
				{
					BookSlotBorders[i][s]->SetBrushColor(GRealmInfos[i].Color);

					if (UVerticalBox* BookVBox = Cast<UVerticalBox>(BookSlotBorders[i][s]->GetParent()->GetParent()))
					{
						if (USizeBox* BodySizeBox = Cast<USizeBox>(BookVBox->GetChildAt(0)))
						{
							if (UBorder* Body = Cast<UBorder>(BodySizeBox->GetChildAt(0)))
							{
								FLinearColor BodyColor = GRealmInfos[i].Color * 0.4f;
								BodyColor.A = 1.0f;
								Body->SetBrushColor(BodyColor);
							}
						}
					}
				}
				else
				{
					BookSlotBorders[i][s]->SetBrushColor(WizConfigColours::SlotEmpty);

					if (UVerticalBox* BookVBox = Cast<UVerticalBox>(BookSlotBorders[i][s]->GetParent()->GetParent()))
					{
						if (USizeBox* BodySizeBox = Cast<USizeBox>(BookVBox->GetChildAt(0)))
						{
							if (UBorder* Body = Cast<UBorder>(BodySizeBox->GetChildAt(0)))
							{
								Body->SetBrushColor(FLinearColor(0.08f, 0.06f, 0.12f, 0.3f));
							}
						}
					}
				}
			}
		}

		// Enable/disable [+] button
		if (AddBookButtons[i])
		{
			bool bCanAdd = (Remaining > 0) && (BookCounts[i] < MaxBooksPerRealm);
			AddBookButtons[i]->SetIsEnabled(bCanAdd);
		}

		// Enable/disable [-] button
		if (RemoveBookButtons[i])
		{
			RemoveBookButtons[i]->SetIsEnabled(BookCounts[i] > 0);
		}
	}
}

void UCoMWizardConfigWidget::UpdateRetortButtons()
{
	int32 Remaining = GetPicksRemaining();

	for (int32 i = 0; i < NumRetorts; ++i)
	{
		if (!RetortButtons[i]) continue;

		bool bSelected = SelectedRetortIndices.Contains(i);
		bool bCanAfford = (Remaining >= GRetortInfos[i].PickCost);

		FButtonStyle Style = RetortButtons[i]->GetStyle();
		if (bSelected)
		{
			Style.Normal.TintColor = FSlateColor(FLinearColor(0.15f, 0.10f, 0.02f, 1.0f));
			Style.Hovered.TintColor = FSlateColor(FLinearColor(0.20f, 0.14f, 0.04f, 1.0f));
		}
		else
		{
			Style.Normal.TintColor = FSlateColor(FLinearColor(0.05f, 0.04f, 0.10f, 1.0f));
			Style.Hovered.TintColor = FSlateColor(FLinearColor(0.08f, 0.06f, 0.16f, 1.0f));
		}
		RetortButtons[i]->SetStyle(Style);

		// Update the card border color
		if (UBorder* CardBorder = Cast<UBorder>(RetortButtons[i]->GetParent()))
		{
			if (bSelected)
			{
				CardBorder->SetBrushColor(FLinearColor(0.855f, 0.647f, 0.125f, 1.0f));
			}
			else
			{
				CardBorder->SetBrushColor(FLinearColor(0.25f, 0.20f, 0.08f, 0.4f));
			}
		}

		RetortButtons[i]->SetIsEnabled(bSelected || bCanAfford);

		if (RetortLabels[i])
		{
			if (bSelected)
			{
				RetortLabels[i]->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.3f, 1.0f)));
			}
			else if (!bCanAfford)
			{
				RetortLabels[i]->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.3f, 0.35f, 1.0f)));
			}
			else
			{
				RetortLabels[i]->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.75f, 1.0f)));
			}
		}
	}
}

void UCoMWizardConfigWidget::UpdateStartingSpells()
{
	if (!StartingSpellsText) return;

	FString Preview;

	for (int32 i = 0; i < NumRealms; ++i)
	{
		if (BookCounts[i] <= 0) continue;

		FString Tier = GetTierName(BookCounts[i]);
		int32 SpellCount = BookCounts[i];

		Preview += FString::Printf(TEXT("%s (%d books): %d starting %s spells\n"),
			*GRealmInfos[i].Name, BookCounts[i], SpellCount, *Tier);
	}

	if (Preview.IsEmpty())
	{
		Preview = TEXT("No spell books allocated.\nAdd books to see starting spells.");
	}

	StartingSpellsText->SetText(FText::FromString(Preview));
}

void UCoMWizardConfigWidget::UpdateDifficultyButtonStyles()
{
	for (int32 i = 0; i < 5; ++i)
	{
		if (!DifficultyButtons[i]) continue;

		FButtonStyle Style = DifficultyButtons[i]->GetStyle();
		bool bSelected = (i == SelectedDifficulty);

		FLinearColor OutlineColor = bSelected ? WizConfigColours::Gold : WizConfigColours::GoldDim;
		float OutlineWidth = bSelected ? 2.0f : 1.0f;
		FLinearColor BgColor = bSelected ? WizConfigColours::PanelBg : WizConfigColours::DarkButton;

		Style.Normal.TintColor = FSlateColor(BgColor);
		Style.Normal.OutlineSettings.Color = FSlateColor(OutlineColor);
		Style.Normal.OutlineSettings.Width = OutlineWidth;

		Style.Hovered.OutlineSettings.Color = FSlateColor(WizConfigColours::Gold);
		Style.Hovered.OutlineSettings.Width = 2.0f;

		DifficultyButtons[i]->SetStyle(Style);
	}
}

// =============================================================================
// Styled button helpers
// =============================================================================

UTextBlock* UCoMWizardConfigWidget::CreateSectionLabel(const FString& Text)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(FText::FromString(Text));
	Label->SetColorAndOpacity(FSlateColor(WizConfigColours::Gold));

	FSlateFontInfo FontInfo = Label->GetFont();
	FontInfo.Size = 18;
	FontInfo.TypefaceFontName = FName(TEXT("Bold"));
	Label->SetFont(FontInfo);

	return Label;
}

UButton* UCoMWizardConfigWidget::CreateStyledButton(float Width, float Height)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();

	FButtonStyle Style = Button->GetStyle();

	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(WizConfigColours::DarkButton);
	Style.Normal.OutlineSettings.Color = FSlateColor(WizConfigColours::GoldDim);
	Style.Normal.OutlineSettings.Width = 1.0f;

	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(WizConfigColours::ButtonHover);
	Style.Hovered.OutlineSettings.Color = FSlateColor(WizConfigColours::Gold);
	Style.Hovered.OutlineSettings.Width = 1.0f;

	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(WizConfigColours::DarkButton);
	Style.Pressed.OutlineSettings.Color = FSlateColor(WizConfigColours::Gold);
	Style.Pressed.OutlineSettings.Width = 1.0f;

	Button->SetStyle(Style);

	return Button;
}

UButton* UCoMWizardConfigWidget::CreateSmallButton(const FString& Label, bool bEnabled)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();

	FButtonStyle Style = Button->GetStyle();

	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(WizConfigColours::DarkButton);
	Style.Normal.OutlineSettings.Color = FSlateColor(WizConfigColours::GoldDim);
	Style.Normal.OutlineSettings.Width = 1.0f;

	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(WizConfigColours::ButtonHover);
	Style.Hovered.OutlineSettings.Color = FSlateColor(WizConfigColours::Gold);
	Style.Hovered.OutlineSettings.Width = 1.0f;

	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(WizConfigColours::DarkButton);
	Style.Pressed.OutlineSettings.Color = FSlateColor(WizConfigColours::Gold);
	Style.Pressed.OutlineSettings.Width = 1.0f;

	Button->SetStyle(Style);
	Button->SetIsEnabled(bEnabled);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(WizConfigColours::White));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 14;
	BtnFont.TypefaceFontName = FName(TEXT("Bold"));
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);

	return Button;
}

// =============================================================================
// Full layout build
// =============================================================================

void UCoMWizardConfigWidget::BuildLayout()
{
	// -- Full-screen dark background ------------------------------------------

	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(WizConfigColours::Background);
	BackgroundBorder->SetPadding(FMargin(0.0f));

	// -- Center overlay -------------------------------------------------------

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(RootOverlay);

	// -- Scrollable content panel (920px wide) --------------------------------

	USizeBox* PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>();
	PanelSizeBox->SetWidthOverride(920.0f);

	UOverlaySlot* PanelSlot = RootOverlay->AddChildToOverlay(PanelSizeBox);
	if (PanelSlot)
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	// Panel background border
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(WizConfigColours::PanelBg);
	PanelBorder->SetPadding(FMargin(28.0f, 20.0f));
	PanelSizeBox->AddChild(PanelBorder);

	// Scroll box for panel contents
	UScrollBox* PanelScroll = WidgetTree->ConstructWidget<UScrollBox>();
	PanelScroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	PanelScroll->SetOrientation(Orient_Vertical);
	PanelBorder->AddChild(PanelScroll);

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelScroll->AddChild(ContentBox);

	// -- Set root widget ------------------------------------------------------

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}

	// =========================================================================
	// TOP BAR: Portrait (100x100) + Name (editable) + Picks Remaining
	// =========================================================================

	{
		UHorizontalBox* TopBar = WidgetTree->ConstructWidget<UHorizontalBox>();

		// -- Selected wizard portrait (100x100) --
		{
			TopPortraitImage = WidgetTree->ConstructWidget<UImage>();

			FSlateBrush PortBrush;
			if (SelectedPortraitIndex >= 0 && SelectedPortraitIndex < NumPortraits)
			{
				FString AssetPath = FString::Printf(
					TEXT("/Game/Textures/Wizards/wizard_%02d.wizard_%02d"),
					SelectedPortraitIndex + 1, SelectedPortraitIndex + 1);
				UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *AssetPath);
				if (Tex)
				{
					PortBrush.SetResourceObject(Tex);
				}
				else
				{
					PortBrush.DrawAs = ESlateBrushDrawType::Box;
					float Hue = static_cast<float>(SelectedPortraitIndex) / 14.0f;
					PortBrush.TintColor = FSlateColor(FLinearColor::MakeFromHSV8(
						static_cast<uint8>(Hue * 255), 140, 180));
				}
			}
			else
			{
				PortBrush.DrawAs = ESlateBrushDrawType::Box;
				PortBrush.TintColor = FSlateColor(WizConfigColours::Grey);
			}
			PortBrush.ImageSize = FVector2D(100.0f, 100.0f);
			TopPortraitImage->SetBrush(PortBrush);

			UBorder* PortBorder = WidgetTree->ConstructWidget<UBorder>();
			PortBorder->SetBrushColor(WizConfigColours::Gold);
			PortBorder->SetPadding(FMargin(2.0f));

			USizeBox* PortSize = WidgetTree->ConstructWidget<USizeBox>();
			PortSize->SetWidthOverride(104.0f);
			PortSize->SetHeightOverride(104.0f);
			PortSize->AddChild(TopPortraitImage);
			PortBorder->AddChild(PortSize);

			UHorizontalBoxSlot* PortSlot = TopBar->AddChildToHorizontalBox(PortBorder);
			if (PortSlot) { PortSlot->SetVerticalAlignment(VAlign_Center); PortSlot->SetPadding(FMargin(0, 0, 16, 0)); }
		}

		// -- Name input + Picks remaining (vertical) --
		{
			UVerticalBox* NamePicksBox = WidgetTree->ConstructWidget<UVerticalBox>();

			// Wizard name label
			UTextBlock* NameLabel = WidgetTree->ConstructWidget<UTextBlock>();
			NameLabel->SetText(FText::FromString(TEXT("Wizard Name")));
			NameLabel->SetColorAndOpacity(FSlateColor(WizConfigColours::LightGrey));
			FSlateFontInfo NLFont = NameLabel->GetFont();
			NLFont.Size = 11;
			NameLabel->SetFont(NLFont);
			UVerticalBoxSlot* NLSlot = NamePicksBox->AddChildToVerticalBox(NameLabel);
			if (NLSlot) { NLSlot->SetPadding(FMargin(0, 0, 0, 2)); }

			// Name input box
			NameInputBox = WidgetTree->ConstructWidget<UEditableTextBox>();
			FString DefaultName = (SelectedPortraitIndex >= 0 && SelectedPortraitIndex < NumPortraits)
				? GWizardNames[SelectedPortraitIndex] : TEXT("Custom");
			NameInputBox->SetText(FText::FromString(DefaultName));

			FEditableTextBoxStyle& TextBoxStyle = const_cast<FEditableTextBoxStyle&>(NameInputBox->WidgetStyle);
			TextBoxStyle.BackgroundImageNormal.TintColor = FSlateColor(WizConfigColours::DarkButton);
			TextBoxStyle.BackgroundImageHovered.TintColor = FSlateColor(WizConfigColours::ButtonHover);
			TextBoxStyle.BackgroundImageFocused.TintColor = FSlateColor(WizConfigColours::DarkButton);
			TextBoxStyle.ForegroundColor = FSlateColor(WizConfigColours::White);
			FSlateFontInfo InputFont = TextBoxStyle.TextStyle.Font;
			InputFont.Size = 14;
			TextBoxStyle.TextStyle.Font = InputFont;
			TextBoxStyle.TextStyle.ColorAndOpacity = FSlateColor(WizConfigColours::White);

			USizeBox* NameInputSize = WidgetTree->ConstructWidget<USizeBox>();
			NameInputSize->SetWidthOverride(250.0f);
			NameInputSize->AddChild(NameInputBox);
			UVerticalBoxSlot* InputSlot = NamePicksBox->AddChildToVerticalBox(NameInputSize);
			if (InputSlot) { InputSlot->SetPadding(FMargin(0, 0, 0, 8)); }

			// Picks remaining display
			PicksRemainingText = WidgetTree->ConstructWidget<UTextBlock>();
			PicksRemainingText->SetText(FText::FromString(
				FString::Printf(TEXT("Picks Remaining: %d / %d"), TotalPicks, TotalPicks)));
			PicksRemainingText->SetColorAndOpacity(FSlateColor(WizConfigColours::Gold));
			FSlateFontInfo PickFont = PicksRemainingText->GetFont();
			PickFont.Size = 18;
			PickFont.TypefaceFontName = FName(TEXT("Bold"));
			PicksRemainingText->SetFont(PickFont);
			UVerticalBoxSlot* PickSlot = NamePicksBox->AddChildToVerticalBox(PicksRemainingText);
			if (PickSlot) { PickSlot->SetPadding(FMargin(0, 4, 0, 0)); }

			UHorizontalBoxSlot* RightSlot = TopBar->AddChildToHorizontalBox(NamePicksBox);
			if (RightSlot) { RightSlot->SetVerticalAlignment(VAlign_Center); }
		}

		UVerticalBoxSlot* TopBarSlot = ContentBox->AddChildToVerticalBox(TopBar);
		if (TopBarSlot) { TopBarSlot->SetPadding(FMargin(0, 0, 0, 12)); }
	}

	// -- Gold separator -------------------------------------------------------

	{
		UImage* Sep = WidgetTree->ConstructWidget<UImage>();
		Sep->SetColorAndOpacity(WizConfigColours::Gold);
		Sep->SetDesiredSizeOverride(FVector2D(700.0f, 1.0f));

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(Sep);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Center);
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}
	}

	// =========================================================================
	// SPELL BOOK ALLOCATION
	// =========================================================================

	{
		UTextBlock* BookSectionLabel = CreateSectionLabel(TEXT("Spell Book Allocation"));
		UVerticalBoxSlot* LabelSlot = ContentBox->AddChildToVerticalBox(BookSectionLabel);
		if (LabelSlot) { LabelSlot->SetPadding(FMargin(0, 0, 0, 6)); }

		// One row per realm
		for (int32 i = 0; i < NumRealms; ++i)
		{
			UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>();

			// -- Realm color swatch (20x20) --
			UBorder* Swatch = WidgetTree->ConstructWidget<UBorder>();
			Swatch->SetBrushColor(GRealmInfos[i].Color);
			USizeBox* SwatchSize = WidgetTree->ConstructWidget<USizeBox>();
			SwatchSize->SetWidthOverride(20.0f);
			SwatchSize->SetHeightOverride(20.0f);
			SwatchSize->AddChild(Swatch);
			UHorizontalBoxSlot* SwatchSlot = MainRow->AddChildToHorizontalBox(SwatchSize);
			if (SwatchSlot) { SwatchSlot->SetVerticalAlignment(VAlign_Center); SwatchSlot->SetPadding(FMargin(0, 0, 6, 0)); }

			// -- Realm name (bold, 90px wide) --
			UTextBlock* RealmNameText = WidgetTree->ConstructWidget<UTextBlock>();
			RealmNameText->SetText(FText::FromString(GRealmInfos[i].Name));
			RealmNameText->SetColorAndOpacity(FSlateColor(GRealmInfos[i].Color));
			FSlateFontInfo RealmFont = RealmNameText->GetFont();
			RealmFont.Size = 12;
			RealmFont.TypefaceFontName = FName(TEXT("Bold"));
			RealmNameText->SetFont(RealmFont);

			USizeBox* NameSize = WidgetTree->ConstructWidget<USizeBox>();
			NameSize->SetWidthOverride(90.0f);
			NameSize->AddChild(RealmNameText);
			UHorizontalBoxSlot* NameSlotRef = MainRow->AddChildToHorizontalBox(NameSize);
			if (NameSlotRef) { NameSlotRef->SetVerticalAlignment(VAlign_Center); NameSlotRef->SetPadding(FMargin(0, 0, 8, 0)); }

			// -- 13 book icons (24px wide x 36px tall) --
			UHorizontalBox* SlotsRow = WidgetTree->ConstructWidget<UHorizontalBox>();
			for (int32 s = 0; s < MaxBooksPerRealm; ++s)
			{
				// Each "book" is a vertical stack: body + colored spine at bottom (6px)
				UVerticalBox* BookVBox = WidgetTree->ConstructWidget<UVerticalBox>();

				// Book body (main rectangle)
				UBorder* BookBody = WidgetTree->ConstructWidget<UBorder>();
				BookBody->SetBrushColor(WizConfigColours::SlotEmpty);
				BookBody->SetPadding(FMargin(0.0f));

				USizeBox* BodySize = WidgetTree->ConstructWidget<USizeBox>();
				BodySize->SetWidthOverride(24.0f);
				BodySize->SetHeightOverride(30.0f);
				BodySize->AddChild(BookBody);
				BookVBox->AddChildToVerticalBox(BodySize);

				// Book spine (colored bottom strip, 6px)
				BookSlotBorders[i][s] = WidgetTree->ConstructWidget<UBorder>();
				BookSlotBorders[i][s]->SetBrushColor(WizConfigColours::SlotEmpty);
				BookSlotBorders[i][s]->SetPadding(FMargin(0.0f));

				USizeBox* SpineSize = WidgetTree->ConstructWidget<USizeBox>();
				SpineSize->SetWidthOverride(24.0f);
				SpineSize->SetHeightOverride(6.0f);
				SpineSize->AddChild(BookSlotBorders[i][s]);
				BookVBox->AddChildToVerticalBox(SpineSize);

				USizeBox* BookSize = WidgetTree->ConstructWidget<USizeBox>();
				BookSize->SetWidthOverride(26.0f);
				BookSize->SetHeightOverride(38.0f);
				BookSize->AddChild(BookVBox);

				UHorizontalBoxSlot* SSlotRef = SlotsRow->AddChildToHorizontalBox(BookSize);
				if (SSlotRef) { SSlotRef->SetPadding(FMargin(1.0f, 0.0f)); SSlotRef->SetVerticalAlignment(VAlign_Bottom); }
			}
			UHorizontalBoxSlot* SlotsSlotRef = MainRow->AddChildToHorizontalBox(SlotsRow);
			if (SlotsSlotRef) { SlotsSlotRef->SetVerticalAlignment(VAlign_Center); SlotsSlotRef->SetPadding(FMargin(0, 0, 8, 0)); }

			// -- [+] button --
			{
				USizeBox* AddSize = WidgetTree->ConstructWidget<USizeBox>();
				AddSize->SetWidthOverride(28.0f);
				AddSize->SetHeightOverride(22.0f);

				AddBookButtons[i] = CreateSmallButton(TEXT("+"), true);
				AddSize->AddChild(AddBookButtons[i]);

				UHorizontalBoxSlot* AddSlotRef = MainRow->AddChildToHorizontalBox(AddSize);
				if (AddSlotRef) { AddSlotRef->SetVerticalAlignment(VAlign_Center); AddSlotRef->SetPadding(FMargin(0, 0, 2, 0)); }
			}

			// -- [-] button --
			{
				USizeBox* RemSize = WidgetTree->ConstructWidget<USizeBox>();
				RemSize->SetWidthOverride(28.0f);
				RemSize->SetHeightOverride(22.0f);

				RemoveBookButtons[i] = CreateSmallButton(TEXT("-"), false);
				RemSize->AddChild(RemoveBookButtons[i]);

				UHorizontalBoxSlot* RemSlotRef = MainRow->AddChildToHorizontalBox(RemSize);
				if (RemSlotRef) { RemSlotRef->SetVerticalAlignment(VAlign_Center); RemSlotRef->SetPadding(FMargin(0, 0, 8, 0)); }
			}

			// -- Tier text (inline, after buttons) --
			TierTexts[i] = WidgetTree->ConstructWidget<UTextBlock>();
			TierTexts[i]->SetText(FText::FromString(TEXT("None")));
			TierTexts[i]->SetColorAndOpacity(FSlateColor(WizConfigColours::Grey));
			FSlateFontInfo TierFont = TierTexts[i]->GetFont();
			TierFont.Size = 10;
			TierTexts[i]->SetFont(TierFont);

			USizeBox* TierSize = WidgetTree->ConstructWidget<USizeBox>();
			TierSize->SetWidthOverride(60.0f);
			TierSize->AddChild(TierTexts[i]);
			UHorizontalBoxSlot* TierSlotRef = MainRow->AddChildToHorizontalBox(TierSize);
			if (TierSlotRef) { TierSlotRef->SetVerticalAlignment(VAlign_Center); }

			UVerticalBoxSlot* RealmRowSlot = ContentBox->AddChildToVerticalBox(MainRow);
			if (RealmRowSlot) { RealmRowSlot->SetPadding(FMargin(0, 2, 0, 2)); }
		}

		// Bind add/remove book callbacks
		AddBookButtons[0]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnAddBook0);
		AddBookButtons[1]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnAddBook1);
		AddBookButtons[2]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnAddBook2);
		AddBookButtons[3]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnAddBook3);
		AddBookButtons[4]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnAddBook4);
		AddBookButtons[5]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnAddBook5);
		AddBookButtons[6]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnAddBook6);
		AddBookButtons[7]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnAddBook7);
		AddBookButtons[8]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnAddBook8);

		RemoveBookButtons[0]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRemoveBook0);
		RemoveBookButtons[1]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRemoveBook1);
		RemoveBookButtons[2]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRemoveBook2);
		RemoveBookButtons[3]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRemoveBook3);
		RemoveBookButtons[4]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRemoveBook4);
		RemoveBookButtons[5]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRemoveBook5);
		RemoveBookButtons[6]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRemoveBook6);
		RemoveBookButtons[7]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRemoveBook7);
		RemoveBookButtons[8]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRemoveBook8);
	}

	// -- Separator ------------------------------------------------------------

	{
		UImage* Sep = WidgetTree->ConstructWidget<UImage>();
		Sep->SetColorAndOpacity(WizConfigColours::Gold);
		Sep->SetDesiredSizeOverride(FVector2D(700.0f, 1.0f));
		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(Sep);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 8, 0, 8)); }
	}

	// =========================================================================
	// RETORT SECTION
	// =========================================================================

	{
		UTextBlock* RetortLabel = CreateSectionLabel(TEXT("Retorts"));
		FSlateFontInfo RetortLabelFont = RetortLabel->GetFont();
		RetortLabelFont.Size = 14;
		RetortLabel->SetFont(RetortLabelFont);
		UVerticalBoxSlot* RLSlot = ContentBox->AddChildToVerticalBox(RetortLabel);
		if (RLSlot) { RLSlot->SetPadding(FMargin(0, 0, 0, 4)); }

		UVerticalBox* RetortGrid = WidgetTree->ConstructWidget<UVerticalBox>();

		// Build retort cards in 2-column grid
		for (int32 Row = 0; Row < (NumRetorts + 1) / 2; ++Row)
		{
			UHorizontalBox* RetortRow = WidgetTree->ConstructWidget<UHorizontalBox>();

			for (int32 Col = 0; Col < 2; ++Col)
			{
				int32 Idx = Row * 2 + Col;
				if (Idx >= NumRetorts) break;

				// Retort card: outer border
				UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>();
				CardBorder->SetBrushColor(FLinearColor(0.25f, 0.20f, 0.08f, 0.5f));
				CardBorder->SetPadding(FMargin(1.5f));

				RetortButtons[Idx] = WidgetTree->ConstructWidget<UButton>();
				FButtonStyle RStyle = RetortButtons[Idx]->GetStyle();
				RStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
				RStyle.Normal.TintColor = FSlateColor(FLinearColor(0.05f, 0.04f, 0.10f, 1.0f));
				RStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
				RStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.08f, 0.06f, 0.16f, 1.0f));
				RStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
				RStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.04f, 0.03f, 0.08f, 1.0f));
				RetortButtons[Idx]->SetStyle(RStyle);

				// Card content: [Icon] [Name] [Cost Badge]
				UHorizontalBox* CardContent = WidgetTree->ConstructWidget<UHorizontalBox>();

				// Icon symbol
				UTextBlock* IconText = WidgetTree->ConstructWidget<UTextBlock>();
				IconText->SetText(FText::FromString(GRetortIcons[Idx]));
				IconText->SetColorAndOpacity(FSlateColor(WizConfigColours::Gold));
				FSlateFontInfo IconFont = IconText->GetFont();
				IconFont.Size = 18;
				IconText->SetFont(IconFont);

				USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
				IconSize->SetWidthOverride(28.0f);
				IconSize->AddChild(IconText);
				UHorizontalBoxSlot* IconSlotRef = CardContent->AddChildToHorizontalBox(IconSize);
				if (IconSlotRef) { IconSlotRef->SetVerticalAlignment(VAlign_Center); IconSlotRef->SetPadding(FMargin(4, 0, 4, 0)); }

				// Name text
				RetortLabels[Idx] = WidgetTree->ConstructWidget<UTextBlock>();
				RetortLabels[Idx]->SetText(FText::FromString(GRetortInfos[Idx].DisplayName));
				RetortLabels[Idx]->SetColorAndOpacity(FSlateColor(WizConfigColours::LightGrey));
				FSlateFontInfo RFont = RetortLabels[Idx]->GetFont();
				RFont.Size = 11;
				RetortLabels[Idx]->SetFont(RFont);
				RetortLabels[Idx]->SetShadowOffset(FVector2D(1, 1));
				RetortLabels[Idx]->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.5f));

				UHorizontalBoxSlot* NameSlotRef = CardContent->AddChildToHorizontalBox(RetortLabels[Idx]);
				if (NameSlotRef) { NameSlotRef->SetVerticalAlignment(VAlign_Center); NameSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

				// Cost badge
				UBorder* CostBadge = WidgetTree->ConstructWidget<UBorder>();
				CostBadge->SetBrushColor(FLinearColor(0.5f, 0.38f, 0.08f, 0.8f));
				CostBadge->SetPadding(FMargin(4, 1, 4, 1));

				UTextBlock* CostText = WidgetTree->ConstructWidget<UTextBlock>();
				CostText->SetText(FText::FromString(FString::Printf(TEXT("%d"), GRetortInfos[Idx].PickCost)));
				CostText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.9f, 0.6f, 1.0f)));
				CostText->SetJustification(ETextJustify::Center);
				FSlateFontInfo CostFont = CostText->GetFont();
				CostFont.Size = 10;
				CostFont.TypefaceFontName = FName(TEXT("Bold"));
				CostText->SetFont(CostFont);
				CostBadge->AddChild(CostText);

				UHorizontalBoxSlot* CostSlotRef = CardContent->AddChildToHorizontalBox(CostBadge);
				if (CostSlotRef) { CostSlotRef->SetVerticalAlignment(VAlign_Center); CostSlotRef->SetPadding(FMargin(4, 0, 4, 0)); }

				RetortButtons[Idx]->AddChild(CardContent);
				CardBorder->AddChild(RetortButtons[Idx]);

				// Card size: 280x44
				USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>();
				CardSize->SetWidthOverride(280.0f);
				CardSize->SetHeightOverride(44.0f);
				CardSize->AddChild(CardBorder);

				UHorizontalBoxSlot* ColSlotRef = RetortRow->AddChildToHorizontalBox(CardSize);
				if (ColSlotRef) { ColSlotRef->SetPadding(FMargin(3.0f, 2.0f)); }
			}

			RetortGrid->AddChildToVerticalBox(RetortRow);
		}

		// Bind retort callbacks
		RetortButtons[0]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort0);
		RetortButtons[1]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort1);
		RetortButtons[2]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort2);
		RetortButtons[3]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort3);
		RetortButtons[4]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort4);
		RetortButtons[5]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort5);
		RetortButtons[6]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort6);
		RetortButtons[7]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort7);
		RetortButtons[8]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort8);
		RetortButtons[9]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort9);
		RetortButtons[10]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort10);
		RetortButtons[11]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort11);
		RetortButtons[12]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort12);
		RetortButtons[13]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort13);
		RetortButtons[14]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort14);
		RetortButtons[15]->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnRetort15);

		UVerticalBoxSlot* RetortGridSlot = ContentBox->AddChildToVerticalBox(RetortGrid);
		if (RetortGridSlot) { RetortGridSlot->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// -- Separator ------------------------------------------------------------

	{
		UImage* Sep = WidgetTree->ConstructWidget<UImage>();
		Sep->SetColorAndOpacity(WizConfigColours::Gold);
		Sep->SetDesiredSizeOverride(FVector2D(700.0f, 1.0f));
		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(Sep);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// =========================================================================
	// DIFFICULTY SECTION (compact row)
	// =========================================================================

	{
		UTextBlock* DiffLabel = CreateSectionLabel(TEXT("Difficulty"));
		FSlateFontInfo DLFont = DiffLabel->GetFont();
		DLFont.Size = 14;
		DiffLabel->SetFont(DLFont);
		UVerticalBoxSlot* LabelSlot = ContentBox->AddChildToVerticalBox(DiffLabel);
		if (LabelSlot) { LabelSlot->SetPadding(FMargin(0, 0, 0, 4)); }

		UHorizontalBox* DiffRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		for (int32 i = 0; i < 5; ++i)
		{
			USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>();
			BtnSize->SetWidthOverride(110.0f);
			BtnSize->SetHeightOverride(40.0f);

			UButton* Btn = CreateStyledButton(110.0f, 40.0f);
			DifficultyButtons[i] = Btn;

			UTextBlock* DiffName = WidgetTree->ConstructWidget<UTextBlock>();
			DiffName->SetText(FText::FromString(GDifficultyInfos[i].Name));
			DiffName->SetColorAndOpacity(FSlateColor(WizConfigColours::White));
			DiffName->SetJustification(ETextJustify::Center);
			FSlateFontInfo DNameFont = DiffName->GetFont();
			DNameFont.Size = 11;
			DNameFont.TypefaceFontName = FName(TEXT("Bold"));
			DiffName->SetFont(DNameFont);

			Btn->AddChild(DiffName);
			BtnSize->AddChild(Btn);

			switch (i)
			{
			case 0: Btn->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnDiffEasyClicked); break;
			case 1: Btn->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnDiffNormalClicked); break;
			case 2: Btn->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnDiffHardClicked); break;
			case 3: Btn->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnDiffLunaticClicked); break;
			case 4: Btn->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnDiffImpossibleClicked); break;
			}

			UHorizontalBoxSlot* HSlot = DiffRow->AddChildToHorizontalBox(BtnSize);
			if (HSlot) { HSlot->SetPadding(FMargin(2.0f)); }
		}

		UVerticalBoxSlot* DiffRowSlot = ContentBox->AddChildToVerticalBox(DiffRow);
		if (DiffRowSlot) { DiffRowSlot->SetHorizontalAlignment(HAlign_Center); DiffRowSlot->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	UpdateDifficultyButtonStyles();

	// -- Separator ------------------------------------------------------------

	{
		UImage* Sep = WidgetTree->ConstructWidget<UImage>();
		Sep->SetColorAndOpacity(WizConfigColours::Gold);
		Sep->SetDesiredSizeOverride(FVector2D(700.0f, 1.0f));
		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(Sep);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// =========================================================================
	// STARTING SPELLS PREVIEW
	// =========================================================================

	{
		UTextBlock* SpellsLabel = CreateSectionLabel(TEXT("Starting Spells Preview"));
		FSlateFontInfo SpellsLabelFont = SpellsLabel->GetFont();
		SpellsLabelFont.Size = 14;
		SpellsLabel->SetFont(SpellsLabelFont);
		UVerticalBoxSlot* SLSlot = ContentBox->AddChildToVerticalBox(SpellsLabel);
		if (SLSlot) { SLSlot->SetPadding(FMargin(0, 0, 0, 4)); }

		UBorder* SpellsBg = WidgetTree->ConstructWidget<UBorder>();
		SpellsBg->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.08f, 1.0f));
		SpellsBg->SetPadding(FMargin(8.0f));

		StartingSpellsText = WidgetTree->ConstructWidget<UTextBlock>();
		StartingSpellsText->SetText(FText::FromString(TEXT("No spell books allocated.\nAdd books to see starting spells.")));
		StartingSpellsText->SetColorAndOpacity(FSlateColor(WizConfigColours::LightGrey));
		StartingSpellsText->SetAutoWrapText(true);
		FSlateFontInfo SpellFont = StartingSpellsText->GetFont();
		SpellFont.Size = 10;
		StartingSpellsText->SetFont(SpellFont);

		SpellsBg->AddChild(StartingSpellsText);

		USizeBox* SpellsSize = WidgetTree->ConstructWidget<USizeBox>();
		SpellsSize->SetHeightOverride(120.0f);
		SpellsSize->AddChild(SpellsBg);

		UVerticalBoxSlot* SpellsSlot = ContentBox->AddChildToVerticalBox(SpellsSize);
		if (SpellsSlot) { SpellsSlot->SetPadding(FMargin(0, 0, 0, 12)); }
	}

	// =========================================================================
	// BOTTOM: Back + Start Game
	// =========================================================================

	{
		UImage* Sep = WidgetTree->ConstructWidget<UImage>();
		Sep->SetColorAndOpacity(WizConfigColours::Gold);
		Sep->SetDesiredSizeOverride(FVector2D(700.0f, 2.0f));
		UVerticalBoxSlot* SepSlot = ContentBox->AddChildToVerticalBox(Sep);
		if (SepSlot) { SepSlot->SetHorizontalAlignment(HAlign_Center); SepSlot->SetPadding(FMargin(0, 0, 0, 12)); }

		UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		// -- Back button --
		{
			USizeBox* BackSize = WidgetTree->ConstructWidget<USizeBox>();
			BackSize->SetWidthOverride(150.0f);
			BackSize->SetHeightOverride(40.0f);

			BackButton = CreateStyledButton(150.0f, 40.0f);

			UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>();
			BackLabel->SetText(FText::FromString(TEXT("Back")));
			BackLabel->SetColorAndOpacity(FSlateColor(WizConfigColours::LightGrey));
			BackLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo BackFont = BackLabel->GetFont();
			BackFont.Size = 14;
			BackLabel->SetFont(BackFont);

			BackButton->AddChild(BackLabel);
			BackSize->AddChild(BackButton);
			BackButton->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnBackClicked);

			UHorizontalBoxSlot* HSlot = BottomRow->AddChildToHorizontalBox(BackSize);
			if (HSlot) { HSlot->SetHorizontalAlignment(HAlign_Left); HSlot->SetPadding(FMargin(0, 0, 8, 0)); }
		}

		// -- Spacer --
		{
			USpacer* SpacerWidget = WidgetTree->ConstructWidget<USpacer>();
			SpacerWidget->SetSize(FVector2D(1.0f, 1.0f));
			UHorizontalBoxSlot* SpSlot = BottomRow->AddChildToHorizontalBox(SpacerWidget);
			if (SpSlot) { SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
		}

		// -- Start Game button (gold) --
		{
			USizeBox* StartSize = WidgetTree->ConstructWidget<USizeBox>();
			StartSize->SetWidthOverride(200.0f);
			StartSize->SetHeightOverride(40.0f);

			StartGameButton = WidgetTree->ConstructWidget<UButton>();

			FButtonStyle StartStyle = StartGameButton->GetStyle();

			StartStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
			StartStyle.Normal.TintColor = FSlateColor(WizConfigColours::Gold);
			StartStyle.Normal.OutlineSettings.Color = FSlateColor(WizConfigColours::Gold);
			StartStyle.Normal.OutlineSettings.Width = 2.0f;

			StartStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
			StartStyle.Hovered.TintColor = FSlateColor(FLinearColor(
				WizConfigColours::Gold.R * 1.2f, WizConfigColours::Gold.G * 1.2f,
				WizConfigColours::Gold.B * 0.8f, 1.0f));
			StartStyle.Hovered.OutlineSettings.Color = FSlateColor(WizConfigColours::White);
			StartStyle.Hovered.OutlineSettings.Width = 2.0f;

			StartStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
			StartStyle.Pressed.TintColor = FSlateColor(FLinearColor(
				WizConfigColours::Gold.R * 0.8f, WizConfigColours::Gold.G * 0.8f,
				WizConfigColours::Gold.B * 0.6f, 1.0f));

			StartGameButton->SetStyle(StartStyle);

			UTextBlock* StartLabel = WidgetTree->ConstructWidget<UTextBlock>();
			StartLabel->SetText(FText::FromString(TEXT("Start Game")));
			StartLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.03f, 0.0f, 1.0f)));
			StartLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo StartFont = StartLabel->GetFont();
			StartFont.Size = 16;
			StartFont.TypefaceFontName = FName(TEXT("Bold"));
			StartLabel->SetFont(StartFont);

			StartGameButton->AddChild(StartLabel);
			StartSize->AddChild(StartGameButton);
			StartGameButton->OnClicked.AddDynamic(this, &UCoMWizardConfigWidget::OnStartGameClicked);

			UHorizontalBoxSlot* HSlot = BottomRow->AddChildToHorizontalBox(StartSize);
			if (HSlot) { HSlot->SetHorizontalAlignment(HAlign_Right); }
		}

		UVerticalBoxSlot* BottomSlot = ContentBox->AddChildToVerticalBox(BottomRow);
		if (BottomSlot) { BottomSlot->SetPadding(FMargin(0, 0, 0, 8)); }
	}
}
