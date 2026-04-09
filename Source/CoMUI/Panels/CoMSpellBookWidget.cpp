// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellBookWidget.cpp -- Spell book / research screen implementation.

#include "CoMSpellBookWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "Components/Slider.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMUI/CoMUISubsystem.h"

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

	// Bind action buttons.
	if (CastButton)     { CastButton->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnCastButtonClicked); }
	if (ResearchButton) { ResearchButton->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnResearchButtonClicked); }
	if (CloseButton)    { CloseButton->OnClicked.AddDynamic(this, &UCoMSpellBookWidget::OnCloseClicked); }

	// Bind mana slider.
	if (ManaAllocationSlider)
	{
		ManaAllocationSlider->OnValueChanged.AddDynamic(this, &UCoMSpellBookWidget::OnManaSliderChanged);
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

	// Mana allocation slider.
	if (ManaAllocationSlider && State.ManaPerTurn > 0)
	{
		float SliderValue = static_cast<float>(State.ResearchAllocation) /
			static_cast<float>(FMath::Max(State.ManaPerTurn, 1));
		ManaAllocationSlider->SetValue(FMath::Clamp(SliderValue, 0.f, 1.f));
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

	// Route through the spell targeting widget for target selection.
	// The targeting widget handles NoTarget spells (instant cast) as well as
	// spells that require tile/unit/city/army target selection.
	if (UCoMUISubsystem* UISS = GetGameInstance()->GetSubsystem<UCoMUISubsystem>())
	{
		UISS->BeginSpellTargeting(SelectedSpellId, CurrentWizardId);
		// Close the spell book to let the player select a target on the map.
		UISS->HideSpellBook();
	}
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
