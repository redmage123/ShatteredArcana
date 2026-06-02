// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMStatsScreenWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"

#include "CoMCore/Stats/CoMStatsSubsystem.h"

namespace StColours
{
	static const FLinearColor BgDim  = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);
	static const FLinearColor Panel  = FLinearColor(0.04f, 0.03f, 0.10f, 0.97f);
	static const FLinearColor Gold   = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver = FLinearColor(0.85f, 0.85f, 0.90f, 1.0f);
	static const FLinearColor Grey   = FLinearColor(0.55f, 0.55f, 0.60f, 1.0f);
	static const FLinearColor Locked = FLinearColor(0.35f, 0.30f, 0.30f, 1.0f);
	static const FLinearColor Win    = FLinearColor(0.30f, 0.85f, 0.30f, 1.0f);
}

TSharedRef<SWidget> UCoMStatsScreenWidget::RebuildWidget()
{
	if (WidgetTree) { BuildLayout(); }
	return Super::RebuildWidget();
}

void UCoMStatsScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &UCoMStatsScreenWidget::OnCloseClicked);
}

void UCoMStatsScreenWidget::OnCloseClicked()
{
	RemoveFromParent();
}

void UCoMStatsScreenWidget::BuildLayout()
{
	RootBorder = WidgetTree->ConstructWidget<UBorder>();
	RootBorder->SetBrushColor(StColours::BgDim);
	RootBorder->SetPadding(FMargin(0));

	UHorizontalBox* Frame = WidgetTree->ConstructWidget<UHorizontalBox>();
	RootBorder->AddChild(Frame);

	UCoMStatsSubsystem* StatsSub = nullptr;
	if (UGameInstance* GI = GetGameInstance()) { StatsSub = GI->GetSubsystem<UCoMStatsSubsystem>(); }
	const FCoMCareerStats* Stats = StatsSub ? &StatsSub->GetStats() : nullptr;

	auto AddRow = [&](UVerticalBox* Box, const FString& Label, const FString& Value, FLinearColor Tint)
	{
		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
		UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
		L->SetText(FText::FromString(Label));
		L->SetColorAndOpacity(FSlateColor(StColours::Grey));
		{ FSlateFontInfo F = L->GetFont(); F.Size = 12; L->SetFont(F); }
		UHorizontalBoxSlot* LS = H->AddChildToHorizontalBox(L);
		if (LS) { LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
		UTextBlock* V = WidgetTree->ConstructWidget<UTextBlock>();
		V->SetText(FText::FromString(Value));
		V->SetColorAndOpacity(FSlateColor(Tint));
		{ FSlateFontInfo F = V->GetFont(); F.Size = 14; V->SetFont(F); }
		H->AddChildToHorizontalBox(V);
		UVerticalBoxSlot* VS = Box->AddChildToVerticalBox(H);
		if (VS) { VS->SetPadding(FMargin(0, 4)); }
	};

	// --- Left: numbers ---
	{
		USizeBox* LeftBox = WidgetTree->ConstructWidget<USizeBox>();
		LeftBox->SetWidthOverride(380.0f);
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrushColor(StColours::Panel);
		Card->SetPadding(FMargin(24, 20));
		LeftBox->AddChild(Card);
		StatsColumn = WidgetTree->ConstructWidget<UVerticalBox>();
		Card->AddChild(StatsColumn);

		UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>();
		Header->SetText(FText::FromString(TEXT("Career Stats")));
		Header->SetColorAndOpacity(FSlateColor(StColours::Gold));
		{ FSlateFontInfo F = Header->GetFont(); F.Size = 22; Header->SetFont(F); }
		UVerticalBoxSlot* HS = StatsColumn->AddChildToVerticalBox(Header);
		if (HS) { HS->SetPadding(FMargin(0, 0, 0, 14)); }

		if (Stats)
		{
			AddRow(StatsColumn, TEXT("Games Played"),    FString::FromInt(Stats->GamesPlayed),     StColours::Silver);
			AddRow(StatsColumn, TEXT("Won"),             FString::FromInt(Stats->GamesWon),        StColours::Win);
			AddRow(StatsColumn, TEXT("Lost"),            FString::FromInt(Stats->GamesLost),       StColours::Silver);
			AddRow(StatsColumn, TEXT("Fastest Win"),
				Stats->FastestWinTurns < 0 ? TEXT("—") : FString::Printf(TEXT("%d turns"), Stats->FastestWinTurns),
				StColours::Silver);
			AddRow(StatsColumn, TEXT("Longest Game"),    FString::Printf(TEXT("%d turns"), Stats->LongestGameTurns), StColours::Silver);
			AddRow(StatsColumn, TEXT("Battles Fought"),  FString::FromInt(Stats->BattlesFought),   StColours::Silver);
			AddRow(StatsColumn, TEXT("Units Killed"),    FString::FromInt(Stats->UnitsKilled),     StColours::Silver);
			AddRow(StatsColumn, TEXT("Cities Captured"), FString::FromInt(Stats->CitiesCaptured),  StColours::Silver);
			AddRow(StatsColumn, TEXT("Spells Cast"),     FString::FromInt(Stats->SpellsCast),      StColours::Silver);
			AddRow(StatsColumn, TEXT("Mana Spent"),      FString::FromInt(Stats->ManaSpent),       StColours::Silver);
			AddRow(StatsColumn, TEXT("Sites Cleared"),   FString::FromInt(Stats->SitesCleared),    StColours::Silver);
			AddRow(StatsColumn, TEXT("Items Forged"),    FString::FromInt(Stats->ItemsForged),     StColours::Silver);
			AddRow(StatsColumn, TEXT("Heroes Recruited"),FString::FromInt(Stats->HeroesRecruited), StColours::Silver);
		}

		UHorizontalBoxSlot* LS = Frame->AddChildToHorizontalBox(LeftBox);
		if (LS) { LS->SetPadding(FMargin(20, 20, 10, 20)); }
	}

	// --- Right: achievements ---
	{
		USizeBox* RightBox = WidgetTree->ConstructWidget<USizeBox>();
		RightBox->SetWidthOverride(560.0f);
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrushColor(StColours::Panel);
		Card->SetPadding(FMargin(24, 20));
		RightBox->AddChild(Card);
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
		Card->AddChild(Col);

		UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>();
		UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>();
		Header->SetText(FText::FromString(TEXT("Achievements")));
		Header->SetColorAndOpacity(FSlateColor(StColours::Gold));
		{ FSlateFontInfo F = Header->GetFont(); F.Size = 22; Header->SetFont(F); }
		UHorizontalBoxSlot* HS1 = Top->AddChildToHorizontalBox(Header);
		if (HS1) { HS1->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

		CloseButton = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle CS = CloseButton->GetStyle();
		CS.Normal.DrawAs   = ESlateBrushDrawType::Box; CS.Normal.TintColor   = FSlateColor(FLinearColor(0.06f, 0.05f, 0.16f, 1));
		CS.Hovered.DrawAs  = ESlateBrushDrawType::Box; CS.Hovered.TintColor  = FSlateColor(FLinearColor(0.12f, 0.09f, 0.26f, 1));
		CloseButton->SetStyle(CS);
		UTextBlock* CT = WidgetTree->ConstructWidget<UTextBlock>();
		CT->SetText(FText::FromString(TEXT("Close")));
		CT->SetColorAndOpacity(FSlateColor(StColours::Gold));
		CT->SetJustification(ETextJustify::Center);
		CloseButton->AddChild(CT);
		USizeBox* CSB = WidgetTree->ConstructWidget<USizeBox>();
		CSB->SetWidthOverride(110.0f); CSB->SetHeightOverride(32.0f);
		CSB->AddChild(CloseButton);
		Top->AddChildToHorizontalBox(CSB);

		UVerticalBoxSlot* TS = Col->AddChildToVerticalBox(Top);
		if (TS) { TS->SetPadding(FMargin(0, 0, 0, 12)); }

		AchievementScroll = WidgetTree->ConstructWidget<UScrollBox>();
		UVerticalBoxSlot* AS = Col->AddChildToVerticalBox(AchievementScroll);
		if (AS) { AS->SetSize(ESlateSizeRule::Fill); }

		if (StatsSub)
		{
			for (const FCoMAchievement& A : StatsSub->GetAchievements())
			{
				UBorder* Row = WidgetTree->ConstructWidget<UBorder>();
				Row->SetBrushColor(A.bUnlocked
					? FLinearColor(0.10f, 0.30f, 0.10f, 1.0f)
					: FLinearColor(0.10f, 0.08f, 0.18f, 1.0f));
				Row->SetPadding(FMargin(10, 8));
				UVerticalBox* RowCol = WidgetTree->ConstructWidget<UVerticalBox>();
				Row->AddChild(RowCol);
				UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
				Name->SetText(FText::FromString(A.DisplayName));
				Name->SetColorAndOpacity(FSlateColor(A.bUnlocked ? StColours::Gold : StColours::Locked));
				{ FSlateFontInfo F = Name->GetFont(); F.Size = 14; Name->SetFont(F); }
				RowCol->AddChildToVerticalBox(Name);
				UTextBlock* Desc = WidgetTree->ConstructWidget<UTextBlock>();
				Desc->SetText(FText::FromString(A.Description));
				Desc->SetColorAndOpacity(FSlateColor(A.bUnlocked ? StColours::Silver : StColours::Locked));
				{ FSlateFontInfo F = Desc->GetFont(); F.Size = 11; Desc->SetFont(F); }
				RowCol->AddChildToVerticalBox(Desc);
				AchievementScroll->AddChild(Row);
			}
		}

		UHorizontalBoxSlot* RS = Frame->AddChildToHorizontalBox(RightBox);
		if (RS) { RS->SetPadding(FMargin(10, 20, 20, 20)); }
	}

	WidgetTree->RootWidget = RootBorder;
}

// Console: open the stats screen overlay.
static FAutoConsoleCommandWithWorldAndArgs GShowStatsScreenCmd(
	TEXT("com.show_stats_screen"),
	TEXT("Open the career-stats + achievements overlay."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& /*Args*/, UWorld* World)
		{
			if (!World) return;
			UCoMStatsScreenWidget* W = CreateWidget<UCoMStatsScreenWidget>(
				World, UCoMStatsScreenWidget::StaticClass());
			if (W) { W->AddToViewport(105); }
		}));
