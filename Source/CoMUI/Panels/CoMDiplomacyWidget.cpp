// Copyright Mythforge Studios. All Rights Reserved.
// CoMDiplomacyWidget.cpp -- Diplomacy screen implementation.

#include "CoMDiplomacyWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/ProgressBar.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"
#include "CoMUI/CoMUISubsystem.h"

void UCoMDiplomacyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (DeclareWarButton)       { DeclareWarButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnDeclareWarButtonClicked); }
	if (ProposePeaceButton)     { ProposePeaceButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnProposePeaceButtonClicked); }
	if (ProposeAllianceButton)  { ProposeAllianceButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnProposeAllianceButtonClicked); }
	if (TradeButton)            { TradeButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnTradeButtonClicked); }
	if (CloseButton)            { CloseButton->OnClicked.AddDynamic(this, &UCoMDiplomacyWidget::OnCloseClicked); }
}

UCoMDiplomacySubsystem* UCoMDiplomacyWidget::GetDiplomacySubsystem()
{
	if (CachedDiplomacySubsystem.IsValid())
	{
		return CachedDiplomacySubsystem.Get();
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI)
	{
		UCoMDiplomacySubsystem* Sub = GI->GetSubsystem<UCoMDiplomacySubsystem>();
		CachedDiplomacySubsystem = Sub;
		return Sub;
	}
	return nullptr;
}

void UCoMDiplomacyWidget::SetPlayerWizardId(int32 WizardId)
{
	PlayerWizardId = WizardId;
	RefreshWizardList();
}

void UCoMDiplomacyWidget::RefreshWizardList()
{
	if (!WizardListScrollBox)
	{
		return;
	}

	WizardListScrollBox->ClearChildren();

	UCoMDiplomacySubsystem* DipSub = GetDiplomacySubsystem();
	if (!DipSub || PlayerWizardId < 0)
	{
		return;
	}

	// Display allies and enemies as known wizards. In a full implementation
	// this would iterate over all wizards that have made first contact.
	TArray<int32> Allies = DipSub->GetAllies(PlayerWizardId);
	TArray<int32> Enemies = DipSub->GetEnemies(PlayerWizardId);

	// Combine into a unique set.
	TSet<int32> KnownWizards;
	for (int32 Id : Allies)  { KnownWizards.Add(Id); }
	for (int32 Id : Enemies) { KnownWizards.Add(Id); }

	for (int32 WizId : KnownWizards)
	{
		int32 Rep = DipSub->GetReputation(PlayerWizardId, WizId);
		ECoMTreatyType Treaty = DipSub->GetTreatyBetween(PlayerWizardId, WizId);

		// Build a summary line.
		const UEnum* TreatyEnum = StaticEnum<ECoMTreatyType>();
		FString TreatyStr = TreatyEnum
			? TreatyEnum->GetDisplayNameTextByValue(static_cast<int64>(Treaty)).ToString()
			: TEXT("Unknown");

		UTextBlock* Entry = NewObject<UTextBlock>(this);
		if (Entry)
		{
			Entry->SetText(FText::FromString(
				FString::Printf(TEXT("Wizard %d  |  %s  |  Rep: %d"),
					WizId, *TreatyStr, Rep)));

			FSlateFontInfo FontInfo = Entry->GetFont();
			FontInfo.Size = 14;
			Entry->SetFont(FontInfo);
			Entry->SetColorAndOpacity(FSlateColor(ReputationToColor(Rep)));

			WizardListScrollBox->AddChild(Entry);
		}
	}
}

void UCoMDiplomacyWidget::OnWizardSelected(int32 WizardId)
{
	SelectedTargetWizardId = WizardId;

	UCoMDiplomacySubsystem* DipSub = GetDiplomacySubsystem();
	if (!DipSub || PlayerWizardId < 0)
	{
		return;
	}

	int32 Rep = DipSub->GetReputation(PlayerWizardId, WizardId);
	ECoMTreatyType Treaty = DipSub->GetTreatyBetween(PlayerWizardId, WizardId);

	if (SelectedWizardNameText)
	{
		SelectedWizardNameText->SetText(FText::FromString(
			FString::Printf(TEXT("Wizard %d"), WizardId)));
	}

	if (TreatyStatusText)
	{
		const UEnum* TreatyEnum = StaticEnum<ECoMTreatyType>();
		FString TreatyStr = TreatyEnum
			? TreatyEnum->GetDisplayNameTextByValue(static_cast<int64>(Treaty)).ToString()
			: TEXT("None");
		TreatyStatusText->SetText(FText::FromString(
			FString::Printf(TEXT("Treaty: %s"), *TreatyStr)));
	}

	if (ReputationValueText)
	{
		FString RepLabel;
		if (Rep >= 500)       { RepLabel = TEXT("Devoted Ally"); }
		else if (Rep >= 200)  { RepLabel = TEXT("Friendly"); }
		else if (Rep >= -200) { RepLabel = TEXT("Neutral"); }
		else if (Rep >= -500) { RepLabel = TEXT("Hostile"); }
		else                  { RepLabel = TEXT("Blood Enemy"); }

		ReputationValueText->SetText(FText::FromString(
			FString::Printf(TEXT("Reputation: %d (%s)"), Rep, *RepLabel)));
		ReputationValueText->SetColorAndOpacity(FSlateColor(ReputationToColor(Rep)));
	}

	if (ReputationBar)
	{
		ReputationBar->SetPercent(ReputationToBarPercent(Rep));
		ReputationBar->SetFillColorAndOpacity(ReputationToColor(Rep));
	}

	// Enable/disable action buttons based on current treaty state.
	bool bAtWar = DipSub->AreAtWar(PlayerWizardId, WizardId);
	bool bAllied = DipSub->AreAllied(PlayerWizardId, WizardId);

	if (DeclareWarButton)       { DeclareWarButton->SetIsEnabled(!bAtWar); }
	if (ProposePeaceButton)     { ProposePeaceButton->SetIsEnabled(bAtWar); }
	if (ProposeAllianceButton)  { ProposeAllianceButton->SetIsEnabled(!bAllied && !bAtWar); }
	if (TradeButton)            { TradeButton->SetIsEnabled(!bAtWar); }

	if (DetailPanel)
	{
		DetailPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UCoMDiplomacyWidget::OnDeclareWarClicked(int32 TargetWizardId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySubsystem();
	if (DipSub && PlayerWizardId >= 0)
	{
		DipSub->DeclareWar(PlayerWizardId, TargetWizardId);
		OnWizardSelected(TargetWizardId);
		RefreshWizardList();
	}
}

void UCoMDiplomacyWidget::OnProposePeaceClicked(int32 TargetWizardId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySubsystem();
	if (DipSub && PlayerWizardId >= 0)
	{
		TMap<ECoMResource, int32> EmptyReparations;
		DipSub->ProposePeace(PlayerWizardId, TargetWizardId, EmptyReparations);
		OnWizardSelected(TargetWizardId);
	}
}

void UCoMDiplomacyWidget::OnProposeAllianceClicked(int32 TargetWizardId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySubsystem();
	if (DipSub && PlayerWizardId >= 0)
	{
		FCoMTreatyProposal Proposal;
		Proposal.ProposerWizardId = PlayerWizardId;
		Proposal.TargetWizardId = TargetWizardId;
		Proposal.ProposedTreaty = ECoMTreatyType::MilitaryAlliance;
		DipSub->ProposeTreaty(Proposal);
		OnWizardSelected(TargetWizardId);
	}
}

void UCoMDiplomacyWidget::OnTradeClicked(int32 TargetWizardId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySubsystem();
	if (DipSub && PlayerWizardId >= 0)
	{
		FCoMTreatyProposal Proposal;
		Proposal.ProposerWizardId = PlayerWizardId;
		Proposal.TargetWizardId = TargetWizardId;
		Proposal.ProposedTreaty = ECoMTreatyType::TradeAgreement;
		DipSub->ProposeTreaty(Proposal);
		OnWizardSelected(TargetWizardId);
	}
}

// -- Button delegates that forward to the parameterized methods ---------------

void UCoMDiplomacyWidget::OnDeclareWarButtonClicked()
{
	if (SelectedTargetWizardId >= 0) { OnDeclareWarClicked(SelectedTargetWizardId); }
}

void UCoMDiplomacyWidget::OnProposePeaceButtonClicked()
{
	if (SelectedTargetWizardId >= 0) { OnProposePeaceClicked(SelectedTargetWizardId); }
}

void UCoMDiplomacyWidget::OnProposeAllianceButtonClicked()
{
	if (SelectedTargetWizardId >= 0) { OnProposeAllianceClicked(SelectedTargetWizardId); }
}

void UCoMDiplomacyWidget::OnTradeButtonClicked()
{
	if (SelectedTargetWizardId >= 0) { OnTradeClicked(SelectedTargetWizardId); }
}

void UCoMDiplomacyWidget::OnCloseClicked()
{
	if (auto* UISS = GetGameInstance()->GetSubsystem<UCoMUISubsystem>())
	{
		UISS->HideDiplomacy();
	}
}

// -- Static helpers -----------------------------------------------------------

float UCoMDiplomacyWidget::ReputationToBarPercent(int32 Reputation)
{
	// Map -1000..+1000 to 0..1
	return FMath::Clamp((static_cast<float>(Reputation) + 1000.f) / 2000.f, 0.f, 1.f);
}

FLinearColor UCoMDiplomacyWidget::ReputationToColor(int32 Reputation)
{
	if (Reputation >= 500)       { return FLinearColor(0.1f, 0.8f, 0.2f, 1.f); } // Green — ally
	else if (Reputation >= 200)  { return FLinearColor(0.4f, 0.7f, 0.4f, 1.f); } // Light green — friendly
	else if (Reputation >= -200) { return FLinearColor(0.8f, 0.8f, 0.2f, 1.f); } // Yellow — neutral
	else if (Reputation >= -500) { return FLinearColor(0.9f, 0.4f, 0.1f, 1.f); } // Orange — hostile
	else                         { return FLinearColor(0.9f, 0.1f, 0.1f, 1.f); } // Red — blood enemy
}

FString UCoMDiplomacyWidget::TreatyToString(int32 TreatyValue)
{
	const UEnum* TreatyEnum = StaticEnum<ECoMTreatyType>();
	if (TreatyEnum)
	{
		return TreatyEnum->GetDisplayNameTextByValue(static_cast<int64>(TreatyValue)).ToString();
	}
	return TEXT("Unknown");
}
