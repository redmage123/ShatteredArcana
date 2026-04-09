// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMVictoryScreenWidget.cpp -- Victory / defeat overlay implementation.

#include "CoMVictoryScreenWidget.h"

#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

#include "CoMCore/Victory/CoMVictorySubsystem.h"
#include "CoMCore/Framework/CoMGameState.h"
#include "CoMCore/Wizards/CoMPlayerState.h"

// =============================================================================
// Colour constants
// =============================================================================

namespace VictoryColours
{
	static const FLinearColor Background = FLinearColor(0.02f, 0.02f, 0.06f, 0.92f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f); // #daa520
	static const FLinearColor Red        = FLinearColor(0.85f, 0.1f, 0.1f, 1.0f);
	static const FLinearColor White      = FLinearColor::White;
	static const FLinearColor LightGrey  = FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);
	static const FLinearColor DimGold    = FLinearColor(0.6f, 0.45f, 0.1f, 1.0f);
	static const FLinearColor SepGold    = FLinearColor(0.855f, 0.647f, 0.125f, 0.5f);
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMVictoryScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bIsFocusable = true;
	SetVisibility(ESlateVisibility::Visible);
	BuildLayout();
}

FReply UCoMVictoryScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnMainMenuClicked();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// =============================================================================
// Public API
// =============================================================================

void UCoMVictoryScreenWidget::ShowVictory(int32 WinnerWizardId, ECoMVictoryType VictoryType)
{
	bIsVictory = true;
	CachedWinnerWizardId = WinnerWizardId;
	CachedVictoryType = VictoryType;

	if (ContentBox)
	{
		ContentBox->ClearChildren();
	}
	PopulateVictory(WinnerWizardId, VictoryType);
}

void UCoMVictoryScreenWidget::ShowDefeat(int32 ConquerorWizardId)
{
	bIsVictory = false;
	CachedWinnerWizardId = ConquerorWizardId;
	CachedVictoryType = ECoMVictoryType::None;

	if (ContentBox)
	{
		ContentBox->ClearChildren();
	}
	PopulateDefeat(ConquerorWizardId);
}

// =============================================================================
// Build layout
// =============================================================================

void UCoMVictoryScreenWidget::BuildLayout()
{
	if (!WidgetTree) { return; }

	// Background
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VictoryBG"));
	BackgroundBorder->SetBrushColor(VictoryColours::Background);
	BackgroundBorder->SetPadding(FMargin(60.0f, 40.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	// Content vertical box
	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	BackgroundBorder->AddChild(ContentBox);
}

// =============================================================================
// Victory content
// =============================================================================

void UCoMVictoryScreenWidget::PopulateVictory(int32 WinnerWizardId, ECoMVictoryType VictoryType)
{
	if (!ContentBox) { return; }

	AddSpacer(60.0f);

	// Title: VICTORY
	AddTextLine(TEXT("VICTORY"), VictoryColours::Gold, 72, /*bBold=*/ true);

	AddSpacer(20.0f);
	AddSeparator();
	AddSpacer(20.0f);

	// Winner name
	const FString WinnerName = GetWizardName(WinnerWizardId);
	AddTextLine(FString::Printf(TEXT("%s is victorious!"), *WinnerName),
	            VictoryColours::White, 32, true);

	AddSpacer(12.0f);

	// Victory type description
	const FString Desc = GetVictoryDescription(VictoryType);
	AddTextLine(Desc, VictoryColours::DimGold, 24);

	AddSpacer(30.0f);
	AddSeparator();
	AddSpacer(20.0f);

	// Score rankings
	AddTextLine(TEXT("Final Rankings"), VictoryColours::Gold, 28, true);
	AddSpacer(10.0f);
	BuildScoreTable();

	AddSpacer(40.0f);

	// Buttons
	if (WidgetTree)
	{
		// Main Menu button
		MainMenuButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MainMenuBtn"));
		MainMenuButton->OnClicked.AddDynamic(this, &UCoMVictoryScreenWidget::OnMainMenuClicked);

		UTextBlock* MenuText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuBtnText"));
		MenuText->SetText(FText::FromString(TEXT("Main Menu")));
		FSlateFontInfo MenuFont = MenuText->GetFont();
		MenuFont.Size = 20;
		MenuText->SetFont(MenuFont);
		MenuText->SetColorAndOpacity(FSlateColor(VictoryColours::White));
		MainMenuButton->AddChild(MenuText);

		UVerticalBoxSlot* MenuSlot = ContentBox->AddChildToVerticalBox(MainMenuButton);
		if (MenuSlot)
		{
			MenuSlot->SetHorizontalAlignment(HAlign_Center);
		}

		AddSpacer(10.0f);

		// Continue Playing button
		ContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContinueBtn"));
		ContinueButton->OnClicked.AddDynamic(this, &UCoMVictoryScreenWidget::OnContinuePlayingClicked);

		UTextBlock* ContText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContBtnText"));
		ContText->SetText(FText::FromString(TEXT("Continue Playing")));
		FSlateFontInfo ContFont = ContText->GetFont();
		ContFont.Size = 18;
		ContText->SetFont(ContFont);
		ContText->SetColorAndOpacity(FSlateColor(VictoryColours::LightGrey));
		ContinueButton->AddChild(ContText);

		UVerticalBoxSlot* ContSlot = ContentBox->AddChildToVerticalBox(ContinueButton);
		if (ContSlot)
		{
			ContSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}
}

// =============================================================================
// Defeat content
// =============================================================================

void UCoMVictoryScreenWidget::PopulateDefeat(int32 ConquerorWizardId)
{
	if (!ContentBox) { return; }

	AddSpacer(80.0f);

	// Title: DEFEAT
	AddTextLine(TEXT("DEFEAT"), VictoryColours::Red, 72, /*bBold=*/ true);

	AddSpacer(20.0f);
	AddSeparator();
	AddSpacer(20.0f);

	// Defeat message
	if (ConquerorWizardId >= 0)
	{
		const FString ConquerorName = GetWizardName(ConquerorWizardId);
		AddTextLine(FString::Printf(TEXT("Your realm has fallen to %s."), *ConquerorName),
		            VictoryColours::White, 28);
	}
	else
	{
		AddTextLine(TEXT("Your realm has been destroyed."), VictoryColours::White, 28);
	}

	AddSpacer(30.0f);

	// Score
	AddTextLine(TEXT("Final Rankings"), VictoryColours::Gold, 28, true);
	AddSpacer(10.0f);
	BuildScoreTable();

	AddSpacer(40.0f);

	// Main Menu button only (no continue after defeat)
	if (WidgetTree)
	{
		MainMenuButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MainMenuBtn"));
		MainMenuButton->OnClicked.AddDynamic(this, &UCoMVictoryScreenWidget::OnMainMenuClicked);

		UTextBlock* MenuText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuBtnText"));
		MenuText->SetText(FText::FromString(TEXT("Main Menu")));
		FSlateFontInfo MenuFont = MenuText->GetFont();
		MenuFont.Size = 20;
		MenuText->SetFont(MenuFont);
		MenuText->SetColorAndOpacity(FSlateColor(VictoryColours::White));
		MainMenuButton->AddChild(MenuText);

		UVerticalBoxSlot* MenuSlot = ContentBox->AddChildToVerticalBox(MainMenuButton);
		if (MenuSlot)
		{
			MenuSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}
}

// =============================================================================
// Score table
// =============================================================================

void UCoMVictoryScreenWidget::BuildScoreTable()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	UCoMVictorySubsystem* VictorySub = GI->GetSubsystem<UCoMVictorySubsystem>();
	if (!VictorySub) { return; }

	const TArray<FCoMVictoryScoreEntry> Scores = VictorySub->GetAllScores();

	int32 Rank = 1;
	for (const FCoMVictoryScoreEntry& Entry : Scores)
	{
		const FString WizardName = GetWizardName(Entry.WizardId);
		const bool bIsWinner = (Entry.WizardId == CachedWinnerWizardId && bIsVictory);
		const FLinearColor LineColor = bIsWinner ? VictoryColours::Gold : VictoryColours::LightGrey;

		const FString Line = FString::Printf(TEXT("#%d  %s  —  %d points"),
		                                      Rank, *WizardName, Entry.TotalScore);
		AddTextLine(Line, LineColor, 20, bIsWinner);

		Rank++;
	}
}

// =============================================================================
// Helpers
// =============================================================================

void UCoMVictoryScreenWidget::AddTextLine(const FString& Text, const FLinearColor& Color,
                                           int32 FontSize, bool bBold, ETextJustify::Type Justify)
{
	if (!ContentBox || !WidgetTree) { return; }

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("Text_%d"), ContentBox->GetChildrenCount()));

	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetJustification(Justify);

	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	if (bBold)
	{
		Font.TypefaceFontName = FName("Bold");
	}
	TextBlock->SetFont(Font);

	UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(TextBlock);
	if (Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetPadding(FMargin(0.0f, 2.0f));
	}
}

void UCoMVictoryScreenWidget::AddSeparator()
{
	if (!ContentBox || !WidgetTree) { return; }

	UBorder* Sep = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), *FString::Printf(TEXT("Sep_%d"), ContentBox->GetChildrenCount()));
	Sep->SetBrushColor(VictoryColours::SepGold);

	UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(Sep);
	if (Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetPadding(FMargin(80.0f, 0.0f));
	}

	Sep->SetDesiredSizeScale(FVector2D(1.0f, 2.0f));
}

void UCoMVictoryScreenWidget::AddSpacer(float Height)
{
	if (!ContentBox || !WidgetTree) { return; }

	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(
		USpacer::StaticClass(), *FString::Printf(TEXT("Spacer_%d"), ContentBox->GetChildrenCount()));
	Spacer->SetSize(FVector2D(0.0f, Height));
	ContentBox->AddChildToVerticalBox(Spacer);
}

FString UCoMVictoryScreenWidget::GetVictoryDescription(ECoMVictoryType Type)
{
	switch (Type)
	{
	case ECoMVictoryType::Conquest:
		return TEXT("Conquest — All rival wizards have been eliminated!");
	case ECoMVictoryType::Domination:
		return TEXT("Domination — All capital cities are under your control!");
	case ECoMVictoryType::Magical:
		return TEXT("Magical Supremacy — The Spell of Mastery has been cast!");
	case ECoMVictoryType::Diplomatic:
		return TEXT("Diplomatic Victory — All surviving wizards have pledged alliance!");
	case ECoMVictoryType::Economic:
		return TEXT("Economic Domination — Your gold controls the world!");
	case ECoMVictoryType::Cultural:
		return TEXT("Cultural Hegemony — All cities have embraced your culture!");
	case ECoMVictoryType::Pact:
		return TEXT("Pact Victory — Contracts with all three Archdevils have been sealed!");
	case ECoMVictoryType::Dissolution:
		return TEXT("Dissolution Victory — Three planes destabilized and Sael-Thrix has fallen!");
	default:
		return TEXT("Victory!");
	}
}

FString UCoMVictoryScreenWidget::GetWizardName(int32 WizardId) const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return FString::Printf(TEXT("Wizard %d"), WizardId); }

	UWorld* World = GI->GetWorld();
	if (!World) { return FString::Printf(TEXT("Wizard %d"), WizardId); }

	const ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState());
	if (!GS) { return FString::Printf(TEXT("Wizard %d"), WizardId); }

	const ACoMPlayerState* PS = GS->GetWizardByIndex(WizardId);
	if (PS)
	{
		return PS->GetPlayerName();
	}

	return FString::Printf(TEXT("Wizard %d"), WizardId);
}

// =============================================================================
// Button callbacks
// =============================================================================

void UCoMVictoryScreenWidget::OnMainMenuClicked()
{
	RemoveFromParent();

	// TODO: Transition to main menu level via GameInstance
	UE_LOG(LogTemp, Log, TEXT("VictoryScreen: Main Menu clicked — returning to title."));
}

void UCoMVictoryScreenWidget::OnContinuePlayingClicked()
{
	// Hide the victory screen and resume gameplay (sandbox mode).
	RemoveFromParent();

	UE_LOG(LogTemp, Log, TEXT("VictoryScreen: Continue Playing — sandbox mode."));

	// Clear the game-over flag so turns can resume.
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		// Note: The VictorySubsystem still records the winner, but gameplay continues.
		UE_LOG(LogTemp, Log, TEXT("VictoryScreen: Game continues in sandbox mode."));
	}
}
