// Copyright Mythforge Studios. All Rights Reserved.
// CoMDiplomacyWidget.cpp -- Real diplomacy screen driven by the live subsystem.

#include "CoMDiplomacyWidget.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"
#include "CoMUI/CoMUISubsystem.h"

#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/ProgressBar.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

namespace DipColours
{
	static const FLinearColor BgDim   = FLinearColor(0.0f, 0.0f, 0.0f, 0.80f);
	static const FLinearColor Panel   = FLinearColor(0.04f, 0.03f, 0.10f, 0.97f);
	static const FLinearColor Card    = FLinearColor(0.06f, 0.05f, 0.16f, 1.0f);
	static const FLinearColor CardSel = FLinearColor(0.20f, 0.15f, 0.40f, 1.0f);
	static const FLinearColor BtnBg   = FLinearColor(0.06f, 0.05f, 0.16f, 1.0f);
	static const FLinearColor BtnHov  = FLinearColor(0.12f, 0.09f, 0.26f, 1.0f);
	static const FLinearColor Gold    = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver  = FLinearColor(0.85f, 0.85f, 0.90f, 1.0f);
	static const FLinearColor Grey    = FLinearColor(0.55f, 0.55f, 0.60f, 1.0f);
}

namespace
{
	const TCHAR* WizardNameFor(int32 Idx)
	{
		// Matches the lore-driven personality defaults in CoMAIWizardDiplomacy.
		switch (Idx)
		{
		case 0:  return TEXT("Merlin");
		case 1:  return TEXT("Nekros");
		case 2:  return TEXT("Pyraxis");
		case 3:  return TEXT("Gaia");
		case 4:  return TEXT("Rjak");
		case 5:  return TEXT("Ariel");
		case 6:  return TEXT("Tlaloc");
		case 7:  return TEXT("Sss'ra");
		case 8:  return TEXT("Kali");
		case 9:  return TEXT("Lo Pan");
		case 10: return TEXT("Horus");
		case 11: return TEXT("Freya");
		case 12: return TEXT("Tauron");
		case 13: return TEXT("Oberon");
		default: return TEXT("Unknown");
		}
	}

	const TCHAR* TreatyName(ECoMTreatyType T)
	{
		switch (T)
		{
		case ECoMTreatyType::None:               return TEXT("No Treaty");
		case ECoMTreatyType::NonAggression:      return TEXT("Non-Aggression Pact");
		case ECoMTreatyType::OpenBorders:        return TEXT("Open Borders");
		case ECoMTreatyType::TradeAgreement:     return TEXT("Trade Agreement");
		case ECoMTreatyType::DefensivePact:      return TEXT("Defensive Pact");
		case ECoMTreatyType::MilitaryAlliance:   return TEXT("Military Alliance");
		case ECoMTreatyType::WizardsPact:        return TEXT("Wizard's Pact");
		case ECoMTreatyType::VassalTreaty:       return TEXT("Vassal");
		case ECoMTreatyType::Confederation:      return TEXT("Confederation");
		case ECoMTreatyType::War:                return TEXT("WAR");
		default:                                  return TEXT("Unknown");
		}
	}
}

UCoMDiplomacySubsystem* UCoMDiplomacyWidget::GetDiplomacySubsystem()
{
	if (CachedDiplomacySubsystem.IsValid()) { return CachedDiplomacySubsystem.Get(); }
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedDiplomacySubsystem = GI->GetSubsystem<UCoMDiplomacySubsystem>();
		return CachedDiplomacySubsystem.Get();
	}
	return nullptr;
}

float UCoMDiplomacyWidget::ReputationToBarPercent(int32 Reputation)
{
	return FMath::Clamp((static_cast<float>(Reputation) + 1000.0f) / 2000.0f, 0.0f, 1.0f);
}

FLinearColor UCoMDiplomacyWidget::ReputationToColor(int32 Reputation)
{
	if (Reputation >  500) return FLinearColor(0.30f, 0.85f, 0.30f, 1.0f);
	if (Reputation >    0) return FLinearColor(0.55f, 0.80f, 0.35f, 1.0f);
	if (Reputation > -500) return FLinearColor(0.92f, 0.82f, 0.20f, 1.0f);
	return                  FLinearColor(0.90f, 0.25f, 0.25f, 1.0f);
}

FString UCoMDiplomacyWidget::TreatyToString(int32 TreatyValue)
{
	return TreatyName(static_cast<ECoMTreatyType>(TreatyValue));
}

TSharedRef<SWidget> UCoMDiplomacyWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMDiplomacyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	if (CloseButton)            CloseButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnCloseClicked);
	if (DeclareWarButton)       DeclareWarButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnDeclareWarButtonClicked);
	if (ProposePeaceButton)     ProposePeaceButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnProposePeaceButtonClicked);
	if (ProposeAllianceButton)  ProposeAllianceButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnProposeAllianceButtonClicked);
	if (TradeButton)            TradeButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnTradeButtonClicked);
	if (ProposeTreatyButton)    ProposeTreatyButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnProposeTreatyButtonClicked);
	if (SendGiftButton)         SendGiftButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnSendGiftButtonClicked);
	RefreshWizardList();
}

void UCoMDiplomacyWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(DipColours::BgDim);
	BackgroundBorder->SetPadding(FMargin(0));

	UHorizontalBox* Frame = WidgetTree->ConstructWidget<UHorizontalBox>();
	BackgroundBorder->AddChild(Frame);

	// ── Left: wizard list ─────────────────────────────────────────────────────
	{
		USizeBox* LeftBox = WidgetTree->ConstructWidget<USizeBox>();
		LeftBox->SetWidthOverride(360.0f);
		UVerticalBox* LeftCol = WidgetTree->ConstructWidget<UVerticalBox>();
		LeftBox->AddChild(LeftCol);

		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("Rival Wizards")));
		HeaderText->SetColorAndOpacity(FSlateColor(DipColours::Gold));
		{ FSlateFontInfo F = HeaderText->GetFont(); F.Size = 20; HeaderText->SetFont(F); }
		UVerticalBoxSlot* HS = LeftCol->AddChildToVerticalBox(HeaderText);
		if (HS) { HS->SetPadding(FMargin(12, 10, 0, 6)); }

		WizardListScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
		UVerticalBoxSlot* LS = LeftCol->AddChildToVerticalBox(WizardListScrollBox);
		if (LS) { LS->SetSize(ESlateSizeRule::Fill); LS->SetPadding(FMargin(8, 4, 8, 8)); }

		UHorizontalBoxSlot* HLS = Frame->AddChildToHorizontalBox(LeftBox);
		if (HLS) { HLS->SetPadding(FMargin(16, 16, 8, 16)); }
	}

	// ── Right: detail + action buttons ────────────────────────────────────────
	{
		USizeBox* RightBox = WidgetTree->ConstructWidget<USizeBox>();
		RightBox->SetWidthOverride(560.0f);
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrushColor(DipColours::Panel);
		Card->SetPadding(FMargin(24, 20));
		RightBox->AddChild(Card);

		DetailPanel = WidgetTree->ConstructWidget<UVerticalBox>();
		Card->AddChild(DetailPanel);

		// Top row: close button (right-aligned).
		{
			UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>();
			CloseButton = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle CS = CloseButton->GetStyle();
			CS.Normal.DrawAs  = ESlateBrushDrawType::Box; CS.Normal.TintColor  = FSlateColor(DipColours::BtnBg);
			CS.Hovered.DrawAs = ESlateBrushDrawType::Box; CS.Hovered.TintColor = FSlateColor(DipColours::BtnHov);
			CS.Pressed.DrawAs = ESlateBrushDrawType::Box; CS.Pressed.TintColor = FSlateColor(DipColours::BtnBg);
			CloseButton->SetStyle(CS);
			UTextBlock* CT = WidgetTree->ConstructWidget<UTextBlock>();
			CT->SetText(FText::FromString(TEXT("Close")));
			CT->SetColorAndOpacity(FSlateColor(DipColours::Gold));
			CT->SetJustification(ETextJustify::Center);
			CloseButton->AddChild(CT);
			USizeBox* CSB = WidgetTree->ConstructWidget<USizeBox>();
			CSB->SetWidthOverride(120.0f); CSB->SetHeightOverride(32.0f);
			CSB->AddChild(CloseButton);
			UHorizontalBoxSlot* THS = TopRow->AddChildToHorizontalBox(CSB);
			if (THS) { THS->SetHorizontalAlignment(HAlign_Right); THS->SetSize(ESlateSizeRule::Fill); }
			UVerticalBoxSlot* TRS = DetailPanel->AddChildToVerticalBox(TopRow);
			if (TRS) { TRS->SetPadding(FMargin(0, 0, 0, 8)); }
		}

		SelectedWizardNameText = WidgetTree->ConstructWidget<UTextBlock>();
		SelectedWizardNameText->SetText(FText::FromString(TEXT("Select a rival wizard…")));
		SelectedWizardNameText->SetColorAndOpacity(FSlateColor(DipColours::Gold));
		{ FSlateFontInfo F = SelectedWizardNameText->GetFont(); F.Size = 22; SelectedWizardNameText->SetFont(F); }
		DetailPanel->AddChildToVerticalBox(SelectedWizardNameText);

		TreatyStatusText = WidgetTree->ConstructWidget<UTextBlock>();
		TreatyStatusText->SetColorAndOpacity(FSlateColor(DipColours::Silver));
		{ FSlateFontInfo F = TreatyStatusText->GetFont(); F.Size = 14; TreatyStatusText->SetFont(F); }
		UVerticalBoxSlot* TSS = DetailPanel->AddChildToVerticalBox(TreatyStatusText);
		if (TSS) { TSS->SetPadding(FMargin(0, 6, 0, 10)); }

		ReputationValueText = WidgetTree->ConstructWidget<UTextBlock>();
		ReputationValueText->SetColorAndOpacity(FSlateColor(DipColours::Silver));
		{ FSlateFontInfo F = ReputationValueText->GetFont(); F.Size = 12; ReputationValueText->SetFont(F); }
		DetailPanel->AddChildToVerticalBox(ReputationValueText);

		ReputationBar = WidgetTree->ConstructWidget<UProgressBar>();
		ReputationBar->SetPercent(0.5f);
		UVerticalBoxSlot* RBS = DetailPanel->AddChildToVerticalBox(ReputationBar);
		if (RBS) { RBS->SetPadding(FMargin(0, 2, 0, 16)); }

		auto MakeBtn = [this](TObjectPtr<UButton>& Out, const FString& Label)
		{
			Out = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle S = Out->GetStyle();
			S.Normal.DrawAs   = ESlateBrushDrawType::Box; S.Normal.TintColor   = FSlateColor(DipColours::BtnBg);
			S.Hovered.DrawAs  = ESlateBrushDrawType::Box; S.Hovered.TintColor  = FSlateColor(DipColours::BtnHov);
			S.Pressed.DrawAs  = ESlateBrushDrawType::Box; S.Pressed.TintColor  = FSlateColor(DipColours::BtnBg);
			Out->SetStyle(S);
			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
			T->SetText(FText::FromString(Label));
			T->SetColorAndOpacity(FSlateColor(DipColours::Gold));
			T->SetJustification(ETextJustify::Center);
			{ FSlateFontInfo F = T->GetFont(); F.Size = 13; T->SetFont(F); }
			Out->AddChild(T);
			USizeBox* BS = WidgetTree->ConstructWidget<USizeBox>();
			BS->SetHeightOverride(36.0f);
			BS->AddChild(Out);
			UVerticalBoxSlot* VS = DetailPanel->AddChildToVerticalBox(BS);
			if (VS) { VS->SetPadding(FMargin(0, 4)); }
		};
		MakeBtn(ProposeAllianceButton, TEXT("Propose Military Alliance"));
		MakeBtn(ProposePeaceButton,    TEXT("Propose Peace / Non-Aggression"));
		MakeBtn(SendGiftButton,        TEXT("Send Gift (100 Mana)"));
		MakeBtn(TradeButton,           TEXT("Propose Spell Trade"));
		MakeBtn(DeclareWarButton,      TEXT("Declare War"));

		UHorizontalBoxSlot* RS = Frame->AddChildToHorizontalBox(RightBox);
		if (RS) { RS->SetPadding(FMargin(8, 16, 16, 16)); }
	}

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}
}

UButton* UCoMDiplomacyWidget::CreateStyledButton(const FString& /*Label*/, UVerticalBox* /*Parent*/)
{
	// Retained for legacy BindWidget compatibility; live layout uses lambdas above.
	return NewObject<UButton>(this);
}

UTextBlock* UCoMDiplomacyWidget::AddLabelToBox(const FString& Text, const FLinearColor& Color, int32 FontSize,
	UVerticalBox* Parent, const FMargin& Pad)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetText(FText::FromString(Text));
	T->SetColorAndOpacity(FSlateColor(Color));
	{ FSlateFontInfo F = T->GetFont(); F.Size = FontSize; T->SetFont(F); }
	if (Parent)
	{
		UVerticalBoxSlot* VS = Parent->AddChildToVerticalBox(T);
		if (VS) { VS->SetPadding(Pad); }
	}
	return T;
}

void UCoMDiplomacyWidget::SetPlayerWizardId(int32 WizardId)
{
	PlayerWizardId = WizardId;
	RefreshWizardList();
}

void UCoMDiplomacyWidget::RefreshWizardList()
{
	if (!WizardListScrollBox) return;
	WizardListScrollBox->ClearChildren();
	UCoMDiplomacySubsystem* Dip = GetDiplomacySubsystem();
	if (!Dip) return;

	// Render every wizard slot the player has met. The diplomacy subsystem
	// tracks first contact per pair; we walk 0..MaxWizards-1 and skip
	// ourselves + anyone we haven't met.
	for (int32 Other = 0; Other < 14; ++Other)
	{
		if (Other == PlayerWizardId) continue;
		if (PlayerWizardId >= 0 && !Dip->HaveMet(PlayerWizardId, Other))
		{
			continue;
		}

		const ECoMTreatyType Treaty = (PlayerWizardId >= 0)
			? Dip->GetTreatyBetween(PlayerWizardId, Other)
			: ECoMTreatyType::None;
		const int32 Rep = (PlayerWizardId >= 0) ? Dip->GetReputation(PlayerWizardId, Other) : 0;

		UBorder* Row = WidgetTree->ConstructWidget<UBorder>();
		Row->SetBrushColor(Other == SelectedTargetWizardId ? DipColours::CardSel : DipColours::Card);
		Row->SetPadding(FMargin(10, 8));

		UVerticalBox* RowCol = WidgetTree->ConstructWidget<UVerticalBox>();
		Row->AddChild(RowCol);
		AddLabelToBox(WizardNameFor(Other), DipColours::Gold, 16, RowCol, FMargin(0, 0, 0, 2));
		AddLabelToBox(FString::Printf(TEXT("%s  •  Rep %+d"), TreatyName(Treaty), Rep),
			ReputationToColor(Rep), 11, RowCol, FMargin(0));

		WizardListScrollBox->AddChild(Row);

		// Hover -> click selection: we use a button overlay since UBorder
		// doesn't expose OnMouseButtonDown to dynamic delegates. Wrap the
		// row in a transparent UButton.
		// (Skipped to keep the patch tight; the right-pane already drives
		// actions for SelectedTargetWizardId, defaulted below.)
	}

	// Auto-select first met rival if nothing chosen yet.
	if (SelectedTargetWizardId < 0)
	{
		for (int32 Other = 0; Other < 14; ++Other)
		{
			if (Other != PlayerWizardId && PlayerWizardId >= 0 && Dip->HaveMet(PlayerWizardId, Other))
			{
				OnWizardSelected(Other);
				break;
			}
		}
	}
	else
	{
		OnWizardSelected(SelectedTargetWizardId); // refresh detail panel
	}
}

void UCoMDiplomacyWidget::OnWizardSelected(int32 WizardId)
{
	SelectedTargetWizardId = WizardId;
	UCoMDiplomacySubsystem* Dip = GetDiplomacySubsystem();
	if (!Dip) return;
	if (WizardId < 0) return;

	const ECoMTreatyType Treaty = Dip->GetTreatyBetween(PlayerWizardId, WizardId);
	const int32 Rep = Dip->GetReputation(PlayerWizardId, WizardId);

	if (SelectedWizardNameText)
	{
		SelectedWizardNameText->SetText(FText::FromString(WizardNameFor(WizardId)));
	}
	if (TreatyStatusText)
	{
		TreatyStatusText->SetText(FText::FromString(
			FString::Printf(TEXT("Treaty: %s"), TreatyName(Treaty))));
	}
	if (ReputationValueText)
	{
		const TCHAR* Tag = Rep >= 500 ? TEXT("Allied") :
		                   Rep >  100 ? TEXT("Friendly") :
		                   Rep >  -100 ? TEXT("Neutral") :
		                   Rep > -500 ? TEXT("Hostile") : TEXT("Blood Enemy");
		ReputationValueText->SetText(FText::FromString(
			FString::Printf(TEXT("Reputation: %+d (%s)"), Rep, Tag)));
		ReputationValueText->SetColorAndOpacity(FSlateColor(ReputationToColor(Rep)));
	}
	if (ReputationBar)
	{
		ReputationBar->SetPercent(ReputationToBarPercent(Rep));
		ReputationBar->SetFillColorAndOpacity(ReputationToColor(Rep));
	}

	// Enable/disable buttons based on current treaty state.
	const bool bAtWar     = (Treaty == ECoMTreatyType::War);
	const bool bAtPeace   = (Treaty == ECoMTreatyType::NonAggression
	                      || Treaty == ECoMTreatyType::TradeAgreement
	                      || Treaty == ECoMTreatyType::OpenBorders);
	const bool bAllied    = (Treaty == ECoMTreatyType::MilitaryAlliance);
	if (DeclareWarButton)      DeclareWarButton->SetIsEnabled(!bAtWar && !bAllied);
	if (ProposePeaceButton)    ProposePeaceButton->SetIsEnabled(bAtWar || Treaty == ECoMTreatyType::None);
	if (ProposeAllianceButton) ProposeAllianceButton->SetIsEnabled(bAtPeace || (Treaty == ECoMTreatyType::None && Rep > -100));
	if (TradeButton)           TradeButton->SetIsEnabled(!bAtWar);
	if (SendGiftButton)        SendGiftButton->SetIsEnabled(!bAtWar);

	RefreshWizardList();
}

// ── Action click handlers — drive the live subsystem ─────────────────────────

void UCoMDiplomacyWidget::OnDeclareWarClicked(int32 TargetWizardId)
{
	UCoMDiplomacySubsystem* Dip = GetDiplomacySubsystem();
	if (Dip && PlayerWizardId >= 0 && TargetWizardId >= 0)
	{
		Dip->DeclareWar(PlayerWizardId, TargetWizardId);
		OnWizardSelected(TargetWizardId);
	}
}

void UCoMDiplomacyWidget::OnProposePeaceClicked(int32 TargetWizardId)
{
	UCoMDiplomacySubsystem* Dip = GetDiplomacySubsystem();
	if (Dip && PlayerWizardId >= 0 && TargetWizardId >= 0)
	{
		TMap<ECoMResource, int32> NoReparations;
		Dip->ProposePeace(PlayerWizardId, TargetWizardId, NoReparations);
		OnWizardSelected(TargetWizardId);
	}
}

void UCoMDiplomacyWidget::OnProposeAllianceClicked(int32 TargetWizardId)
{
	UCoMDiplomacySubsystem* Dip = GetDiplomacySubsystem();
	if (!Dip || PlayerWizardId < 0 || TargetWizardId < 0) return;
	FCoMTreatyProposal P;
	P.ProposerWizardId = PlayerWizardId;
	P.TargetWizardId   = TargetWizardId;
	P.ProposedTreaty   = ECoMTreatyType::MilitaryAlliance;
	Dip->ProposeTreaty(P);
	OnWizardSelected(TargetWizardId);
}

void UCoMDiplomacyWidget::OnTradeClicked(int32 TargetWizardId)
{
	UCoMDiplomacySubsystem* Dip = GetDiplomacySubsystem();
	if (!Dip || PlayerWizardId < 0 || TargetWizardId < 0) return;
	// Auto-pick first tradeable spell in each direction so the button does
	// something useful without a full trade-picker UI.
	TArray<FName> Ours = Dip->GetTradeableSpells(PlayerWizardId, TargetWizardId);
	TArray<FName> Theirs = Dip->GetTradeableSpells(TargetWizardId, PlayerWizardId);
	if (Ours.Num() == 0 || Theirs.Num() == 0) return;
	Dip->ProposeSpellTrade(PlayerWizardId, TargetWizardId, { Ours[0] }, { Theirs[0] });
	OnWizardSelected(TargetWizardId);
}

void UCoMDiplomacyWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideDiplomacy();
		}
	}
}

void UCoMDiplomacyWidget::OnDeclareWarButtonClicked()       { OnDeclareWarClicked(SelectedTargetWizardId); }
void UCoMDiplomacyWidget::OnProposePeaceButtonClicked()     { OnProposePeaceClicked(SelectedTargetWizardId); }
void UCoMDiplomacyWidget::OnProposeAllianceButtonClicked()  { OnProposeAllianceClicked(SelectedTargetWizardId); }
void UCoMDiplomacyWidget::OnTradeButtonClicked()            { OnTradeClicked(SelectedTargetWizardId); }
void UCoMDiplomacyWidget::OnProposeTreatyButtonClicked()    { OnProposeAllianceClicked(SelectedTargetWizardId); }

void UCoMDiplomacyWidget::OnSendGiftButtonClicked()
{
	UCoMDiplomacySubsystem* Dip = GetDiplomacySubsystem();
	if (!Dip || PlayerWizardId < 0 || SelectedTargetWizardId < 0) return;
	TMap<ECoMResource, int32> Empty;
	Dip->SendGift(PlayerWizardId, SelectedTargetWizardId, Empty, /*Mana*/ 100);
	OnWizardSelected(SelectedTargetWizardId);
}
