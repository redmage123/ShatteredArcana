// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMScenarioPickerWidget.h"

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
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

#include "CoMCore/Scenario/CoMScenarioDatabase.h"
#include "CoMCore/Playtest/CoMPlaytestSubsystem.h"

namespace ScColours
{
	static const FLinearColor BgDim   = FLinearColor(0.0f, 0.0f, 0.0f, 0.88f);
	static const FLinearColor Panel   = FLinearColor(0.04f, 0.03f, 0.10f, 0.97f);
	static const FLinearColor RowBg   = FLinearColor(0.06f, 0.05f, 0.16f, 1.0f);
	static const FLinearColor RowSel  = FLinearColor(0.20f, 0.15f, 0.40f, 1.0f);
	static const FLinearColor Gold    = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver  = FLinearColor(0.85f, 0.85f, 0.90f, 1.0f);
	static const FLinearColor Grey    = FLinearColor(0.55f, 0.55f, 0.60f, 1.0f);
}

TSharedRef<SWidget> UCoMScenarioPickerWidget::RebuildWidget()
{
	if (WidgetTree) { BuildLayout(); }
	return Super::RebuildWidget();
}

void UCoMScenarioPickerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &UCoMScenarioPickerWidget::OnCloseClicked);
	if (StartButton) StartButton->OnClicked.AddDynamic(this, &UCoMScenarioPickerWidget::OnStartClicked);

	const TArray<FCoMScenarioDef>& All = CoMScenarioDatabase::GetAll();
	if (All.Num() > 0) { SelectScenario(All[0].ScenarioID); }
	RebuildScenarioList();
}

void UCoMScenarioPickerWidget::OnCloseClicked() { RemoveFromParent(); }

void UCoMScenarioPickerWidget::OnStartClicked()
{
	if (SelectedScenarioID.IsNone()) return;
	const FCoMScenarioDef* Def = CoMScenarioDatabase::Find(SelectedScenarioID);
	if (!Def) return;
	UGameInstance* GI = GetGameInstance();
	UCoMPlaytestSubsystem* PT = GI ? GI->GetSubsystem<UCoMPlaytestSubsystem>() : nullptr;
	if (!PT) return;
	PT->RunPlaytest(/*Games=*/ 1, Def->MaxTurns,
		Def->Seed != 0 ? Def->Seed : 42,
		/*OutFilePath=*/ TEXT(""), Def->NumWizards);
	RemoveFromParent();
}

void UCoMScenarioPickerWidget::BuildLayout()
{
	RootBorder = WidgetTree->ConstructWidget<UBorder>();
	RootBorder->SetBrushColor(ScColours::BgDim);

	UHorizontalBox* Frame = WidgetTree->ConstructWidget<UHorizontalBox>();
	RootBorder->AddChild(Frame);

	// Left: scenario list
	{
		USizeBox* LeftBox = WidgetTree->ConstructWidget<USizeBox>();
		LeftBox->SetWidthOverride(340.0f);
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrushColor(ScColours::Panel);
		Card->SetPadding(FMargin(16));
		LeftBox->AddChild(Card);
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
		Card->AddChild(Col);

		UTextBlock* Hdr = WidgetTree->ConstructWidget<UTextBlock>();
		Hdr->SetText(FText::FromString(TEXT("Scenarios")));
		Hdr->SetColorAndOpacity(FSlateColor(ScColours::Gold));
		{ FSlateFontInfo F = Hdr->GetFont(); F.Size = 20; Hdr->SetFont(F); }
		UVerticalBoxSlot* HS = Col->AddChildToVerticalBox(Hdr);
		if (HS) { HS->SetPadding(FMargin(0, 0, 0, 12)); }

		ScenarioListScroll = WidgetTree->ConstructWidget<UScrollBox>();
		UVerticalBoxSlot* LS = Col->AddChildToVerticalBox(ScenarioListScroll);
		if (LS) { LS->SetSize(ESlateSizeRule::Fill); }

		UHorizontalBoxSlot* FS = Frame->AddChildToHorizontalBox(LeftBox);
		if (FS) { FS->SetPadding(FMargin(20, 20, 10, 20)); }
	}

	// Right: detail + start
	{
		USizeBox* RightBox = WidgetTree->ConstructWidget<USizeBox>();
		RightBox->SetWidthOverride(560.0f);
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrushColor(ScColours::Panel);
		Card->SetPadding(FMargin(24, 20));
		RightBox->AddChild(Card);
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
		Card->AddChild(Col);

		DetailNameText = WidgetTree->ConstructWidget<UTextBlock>();
		DetailNameText->SetText(FText::FromString(TEXT("Pick a scenario…")));
		DetailNameText->SetColorAndOpacity(FSlateColor(ScColours::Gold));
		{ FSlateFontInfo F = DetailNameText->GetFont(); F.Size = 24; DetailNameText->SetFont(F); }
		Col->AddChildToVerticalBox(DetailNameText);

		DetailSubText = WidgetTree->ConstructWidget<UTextBlock>();
		DetailSubText->SetColorAndOpacity(FSlateColor(ScColours::Grey));
		{ FSlateFontInfo F = DetailSubText->GetFont(); F.Size = 12; DetailSubText->SetFont(F); }
		UVerticalBoxSlot* SS = Col->AddChildToVerticalBox(DetailSubText);
		if (SS) { SS->SetPadding(FMargin(0, 4, 0, 16)); }

		DetailBodyText = WidgetTree->ConstructWidget<UTextBlock>();
		DetailBodyText->SetColorAndOpacity(FSlateColor(ScColours::Silver));
		DetailBodyText->SetAutoWrapText(true);
		{ FSlateFontInfo F = DetailBodyText->GetFont(); F.Size = 13; DetailBodyText->SetFont(F); }
		UVerticalBoxSlot* BS = Col->AddChildToVerticalBox(DetailBodyText);
		if (BS) { BS->SetSize(ESlateSizeRule::Fill); }

		UHorizontalBox* Btns = WidgetTree->ConstructWidget<UHorizontalBox>();
		auto MakeBtn = [this](TObjectPtr<UButton>& Out, const FString& Label, FLinearColor TextColor)
		{
			Out = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle S = Out->GetStyle();
			S.Normal.DrawAs   = ESlateBrushDrawType::Box; S.Normal.TintColor   = FSlateColor(ScColours::RowBg);
			S.Hovered.DrawAs  = ESlateBrushDrawType::Box; S.Hovered.TintColor  = FSlateColor(ScColours::RowSel);
			Out->SetStyle(S);
			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
			T->SetText(FText::FromString(Label));
			T->SetColorAndOpacity(FSlateColor(TextColor));
			T->SetJustification(ETextJustify::Center);
			Out->AddChild(T);
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(180.0f); SB->SetHeightOverride(38.0f);
			SB->AddChild(Out);
			return SB;
		};
		Btns->AddChildToHorizontalBox(MakeBtn(StartButton, TEXT("Start Scenario"), ScColours::Gold));
		Btns->AddChildToHorizontalBox(MakeBtn(CloseButton, TEXT("Cancel"), ScColours::Silver));
		UVerticalBoxSlot* BTNS = Col->AddChildToVerticalBox(Btns);
		if (BTNS) { BTNS->SetHorizontalAlignment(HAlign_Right); BTNS->SetPadding(FMargin(0, 16, 0, 0)); }

		UHorizontalBoxSlot* RS = Frame->AddChildToHorizontalBox(RightBox);
		if (RS) { RS->SetPadding(FMargin(10, 20, 20, 20)); }
	}

	WidgetTree->RootWidget = RootBorder;
}

void UCoMScenarioPickerWidget::RebuildScenarioList()
{
	if (!ScenarioListScroll) return;
	ScenarioListScroll->ClearChildren();
	for (const FCoMScenarioDef& Def : CoMScenarioDatabase::GetAll())
	{
		UBorder* Row = WidgetTree->ConstructWidget<UBorder>();
		Row->SetBrushColor(Def.ScenarioID == SelectedScenarioID ? ScColours::RowSel : ScColours::RowBg);
		Row->SetPadding(FMargin(12, 10));
		UVerticalBox* RowCol = WidgetTree->ConstructWidget<UVerticalBox>();
		Row->AddChild(RowCol);
		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
		Name->SetText(Def.DisplayName);
		Name->SetColorAndOpacity(FSlateColor(ScColours::Gold));
		{ FSlateFontInfo F = Name->GetFont(); F.Size = 15; Name->SetFont(F); }
		RowCol->AddChildToVerticalBox(Name);
		UTextBlock* Sub = WidgetTree->ConstructWidget<UTextBlock>();
		Sub->SetText(FText::FromString(FString::Printf(TEXT("%d wizards · %d turns"),
			Def.NumWizards, Def.MaxTurns)));
		Sub->SetColorAndOpacity(FSlateColor(ScColours::Grey));
		{ FSlateFontInfo F = Sub->GetFont(); F.Size = 11; Sub->SetFont(F); }
		RowCol->AddChildToVerticalBox(Sub);
		ScenarioListScroll->AddChild(Row);
	}
}

void UCoMScenarioPickerWidget::SelectScenario(FName ScenarioID)
{
	SelectedScenarioID = ScenarioID;
	const FCoMScenarioDef* Def = CoMScenarioDatabase::Find(ScenarioID);
	if (!Def) return;
	if (DetailNameText) DetailNameText->SetText(Def->DisplayName);
	if (DetailSubText)
	{
		DetailSubText->SetText(FText::FromString(FString::Printf(
			TEXT("%d wizards · %d-turn cap · seed %d"),
			Def->NumWizards, Def->MaxTurns, Def->Seed)));
	}
	if (DetailBodyText) DetailBodyText->SetText(Def->Synopsis);
}

// Console: open the scenario picker overlay.
static FAutoConsoleCommandWithWorldAndArgs GScenarioPickerCmd(
	TEXT("com.show_scenario_picker"),
	TEXT("Open the new-game scenario picker overlay."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& /*Args*/, UWorld* World)
		{
			if (!World) return;
			UCoMScenarioPickerWidget* W = CreateWidget<UCoMScenarioPickerWidget>(
				World, UCoMScenarioPickerWidget::StaticClass());
			if (W) { W->AddToViewport(106); }
		}));
