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
#include "HAL/IConsoleManager.h"

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

namespace
{
	struct FTutorialStep { const TCHAR* Header; const TCHAR* Body; };
	const TArray<FTutorialStep>& Steps()
	{
		static const TArray<FTutorialStep> S = {
			{ TEXT("Welcome, Wizard"),
			  TEXT("Shattered Arcana is a fantasy 4X strategy game inspired by Master of Magic. "
				   "You play a wizard competing with rivals for dominion across eight planes. "
				   "This quick tour shows you how the game flows. Click NEXT to continue.") },
			{ TEXT("Each Turn"),
			  TEXT("A turn cycles every wizard's actions. You move armies, queue city builds, set "
				   "research and casting, then click END TURN on the right HUD to advance. Buildings "
				   "complete, mana flows in, research progresses, AI wizards take their turns. ") },
			{ TEXT("Cities"),
			  TEXT("Click any of your cities on the overworld to open the city screen. Set a build "
				   "queue (Granary first, then Marketplace, then Smithy or Barracks). Cities grow "
				   "from food surplus and produce gold + mana from buildings.") },
			{ TEXT("Magic & Casting Skill"),
			  TEXT("Open the Spell Book to research and cast. Damage and heal spells need a target; "
				   "Global Enchantments and Forge Item cast directly. Your Casting Skill caps mana "
				   "per turn -- balance Skill against Research and Mana income in the Magic screen.") },
			{ TEXT("Armies & Combat"),
			  TEXT("Recruit combat units from cities with a Barracks. Move armies onto enemy stacks "
				   "to fight on a 12x8 tactical grid -- pick MOVE, ATTACK, DEFEND, or WAIT per unit. "
				   "AUTO-RESOLVE hands control to the AI if you'd rather not micro every battle.") },
			{ TEXT("Diplomacy & Espionage"),
			  TEXT("Open the Diplomacy panel to propose alliances, send gifts, or declare war. "
				   "Recruit spies in the Espionage panel for sabotage, theft, and assassination "
				   "missions. The HUD will toast war declarations and treaty resolutions.") },
			{ TEXT("Winning"),
			  TEXT("Win by Spell of Mastery (research the Arcane endgame), Domination (own a "
				   "majority of all cities), or Banishment (kill every rival wizard). At the turn "
				   "cap, the highest-scoring wizard wins by default. Good luck, Archmagus.") },
		};
		return S;
	}
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
	if (DismissButton) DismissButton->OnClicked.AddDynamic(this, &UCoMTutorialWidget::OnDismissClicked);
	if (NextButton)    NextButton->OnClicked.AddDynamic(this, &UCoMTutorialWidget::OnNextClicked);
	if (BackButton)    BackButton->OnClicked.AddDynamic(this, &UCoMTutorialWidget::OnBackClicked);
	ShowStep(0);
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
	RemoveFromParent();
}

void UCoMTutorialWidget::OnNextClicked()
{
	const int32 Last = Steps().Num() - 1;
	if (CurrentStep < Last) { ShowStep(CurrentStep + 1); }
	else { OnDismissClicked(); }
}

void UCoMTutorialWidget::OnBackClicked()
{
	if (CurrentStep > 0) { ShowStep(CurrentStep - 1); }
}

void UCoMTutorialWidget::ShowStep(int32 Index)
{
	CurrentStep = FMath::Clamp(Index, 0, Steps().Num() - 1);
	const FTutorialStep& S = Steps()[CurrentStep];
	if (StepHeader)  StepHeader->SetText(FText::FromString(S.Header));
	if (StepBody)    StepBody->SetText(FText::FromString(S.Body));
	if (StepCounter) StepCounter->SetText(FText::FromString(
		FString::Printf(TEXT("Step %d / %d"), CurrentStep + 1, Steps().Num())));
	if (BackButton) BackButton->SetIsEnabled(CurrentStep > 0);
	if (NextButton)
	{
		UTextBlock* T = NextButton->GetChildAt(0) ? Cast<UTextBlock>(NextButton->GetChildAt(0)) : nullptr;
		if (T)
		{
			T->SetText(FText::FromString(
				CurrentStep == Steps().Num() - 1 ? TEXT("Finish") : TEXT("Next →")));
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

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(740.0f);
	PanelSize->SetHeightOverride(520.0f);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(TutColours::Panel);
	Panel->SetPadding(FMargin(36, 28));
	PanelSize->AddChild(Panel);

	UOverlaySlot* PSlot = Root->AddChildToOverlay(PanelSize);
	if (PSlot) { PSlot->SetHorizontalAlignment(HAlign_Center); PSlot->SetVerticalAlignment(VAlign_Center); }

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChild(ContentBox);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	TitleText->SetText(LOCTEXT("TutTitle", "Welcome to Shattered Arcana"));
	TitleText->SetColorAndOpacity(FSlateColor(TutColours::Gold));
	TitleText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = TitleText->GetFont(); F.Size = 24; TitleText->SetFont(F); }
	UVerticalBoxSlot* TS = ContentBox->AddChildToVerticalBox(TitleText);
	if (TS) { TS->SetHorizontalAlignment(HAlign_Center); TS->SetPadding(FMargin(0, 0, 0, 8)); }

	StepCounter = WidgetTree->ConstructWidget<UTextBlock>();
	StepCounter->SetColorAndOpacity(FSlateColor(TutColours::Grey));
	StepCounter->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = StepCounter->GetFont(); F.Size = 11; StepCounter->SetFont(F); }
	UVerticalBoxSlot* SCS = ContentBox->AddChildToVerticalBox(StepCounter);
	if (SCS) { SCS->SetHorizontalAlignment(HAlign_Center); SCS->SetPadding(FMargin(0, 0, 0, 18)); }

	StepHeader = WidgetTree->ConstructWidget<UTextBlock>();
	StepHeader->SetColorAndOpacity(FSlateColor(TutColours::Gold));
	{ FSlateFontInfo F = StepHeader->GetFont(); F.Size = 18; StepHeader->SetFont(F); }
	ContentBox->AddChildToVerticalBox(StepHeader);

	StepBody = WidgetTree->ConstructWidget<UTextBlock>();
	StepBody->SetColorAndOpacity(FSlateColor(TutColours::Silver));
	StepBody->SetAutoWrapText(true);
	{ FSlateFontInfo F = StepBody->GetFont(); F.Size = 13; StepBody->SetFont(F); }
	UVerticalBoxSlot* BS = ContentBox->AddChildToVerticalBox(StepBody);
	if (BS) { BS->SetSize(ESlateSizeRule::Fill); BS->SetPadding(FMargin(0, 8, 0, 0)); }

	// Button row: Back | Skip | Next
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto MakeBtn = [this](TObjectPtr<UButton>& Out, const FString& Label, FLinearColor Tint)
	{
		Out = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle S = Out->GetStyle();
		S.Normal.DrawAs    = ESlateBrushDrawType::Box; S.Normal.TintColor    = FSlateColor(TutColours::Btn);
		S.Hovered.DrawAs   = ESlateBrushDrawType::Box; S.Hovered.TintColor   = FSlateColor(TutColours::BtnHov);
		Out->SetStyle(S);
		UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
		L->SetText(FText::FromString(Label));
		L->SetColorAndOpacity(FSlateColor(Tint));
		L->SetJustification(ETextJustify::Center);
		{ FSlateFontInfo F = L->GetFont(); F.Size = 13; L->SetFont(F); }
		Out->AddChild(L);
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(150.0f); SB->SetHeightOverride(38.0f);
		SB->AddChild(Out);
		return SB;
	};
	Row->AddChildToHorizontalBox(MakeBtn(BackButton,    TEXT("← Back"), TutColours::Silver));
	Row->AddChildToHorizontalBox(MakeBtn(DismissButton, TEXT("Skip"),   TutColours::Grey));
	Row->AddChildToHorizontalBox(MakeBtn(NextButton,    TEXT("Next →"), TutColours::Gold));
	UVerticalBoxSlot* RS = ContentBox->AddChildToVerticalBox(Row);
	if (RS) { RS->SetHorizontalAlignment(HAlign_Center); RS->SetPadding(FMargin(0, 18, 0, 0)); }

	if (WidgetTree) { WidgetTree->RootWidget = BackgroundBorder; }
}

// Console: open the tutorial overlay (single command, replaces the old one-pager
// open path through CoMUISubsystem).
static FAutoConsoleCommandWithWorldAndArgs GShowTutorialCmd(
	TEXT("com.show_tutorial"),
	TEXT("Open the multi-step tutorial overlay."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& /*Args*/, UWorld* World)
		{
			if (!World) return;
			UCoMTutorialWidget* W = CreateWidget<UCoMTutorialWidget>(
				World, UCoMTutorialWidget::StaticClass());
			if (W) { W->AddToViewport(108); }
		}));

#undef LOCTEXT_NAMESPACE
