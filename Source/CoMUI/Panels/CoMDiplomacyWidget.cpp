// Copyright Mythforge Studios. All Rights Reserved.
// CoMDiplomacyWidget.cpp -- Diplomacy screen implementation.

#include "CoMDiplomacyWidget.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"

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

// =============================================================================
// Subsystem Accessor
// =============================================================================

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

// =============================================================================
// Static Helpers
// =============================================================================

float UCoMDiplomacyWidget::ReputationToBarPercent(int32 Reputation)
{
	return FMath::Clamp((static_cast<float>(Reputation) + 1000.0f) / 2000.0f, 0.0f, 1.0f);
}

FLinearColor UCoMDiplomacyWidget::ReputationToColor(int32 Reputation)
{
	if (Reputation > 500) { return FLinearColor::Green; }
	if (Reputation > 0) { return FLinearColor(0.5f, 0.8f, 0.3f); }
	if (Reputation > -500) { return FLinearColor::Yellow; }
	return FLinearColor::Red;
}

FString UCoMDiplomacyWidget::TreatyToString(int32 TreatyValue)
{
	switch (TreatyValue)
	{
	case 0: return TEXT("None");
	case 1: return TEXT("Non-Aggression Pact");
	case 2: return TEXT("Alliance");
	case 3: return TEXT("War");
	default: return TEXT("Unknown");
	}
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMDiplomacyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	BuildLayout();
}

TSharedRef<SWidget> UCoMDiplomacyWidget::RebuildWidget()
{
	return Super::RebuildWidget();
}

// =============================================================================
// Layout Helpers
// =============================================================================

void UCoMDiplomacyWidget::BuildLayout()
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::BuildLayout stub called"));
}

UButton* UCoMDiplomacyWidget::CreateStyledButton(const FString& Label, UVerticalBox* Parent)
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::CreateStyledButton stub called — Label=%s"), *Label);
	return NewObject<UButton>(this);
}

UTextBlock* UCoMDiplomacyWidget::AddLabelToBox(const FString& Text, const FLinearColor& Color, int32 FontSize, UVerticalBox* Parent, const FMargin& Pad)
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::AddLabelToBox stub called — Text=%s"), *Text);
	return NewObject<UTextBlock>(this);
}

// =============================================================================
// Public API
// =============================================================================

void UCoMDiplomacyWidget::SetPlayerWizardId(int32 WizardId)
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::SetPlayerWizardId stub called — WizardId=%d"), WizardId);
	PlayerWizardId = WizardId;
}

void UCoMDiplomacyWidget::RefreshWizardList()
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::RefreshWizardList stub called"));
}

void UCoMDiplomacyWidget::OnWizardSelected(int32 WizardId)
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnWizardSelected stub called — WizardId=%d"), WizardId);
	SelectedTargetWizardId = WizardId;
}

// =============================================================================
// Public Diplomatic Action Callbacks
// =============================================================================

void UCoMDiplomacyWidget::OnDeclareWarClicked(int32 TargetWizardId)
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnDeclareWarClicked stub called — TargetWizardId=%d"), TargetWizardId);
}

void UCoMDiplomacyWidget::OnProposePeaceClicked(int32 TargetWizardId)
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnProposePeaceClicked stub called — TargetWizardId=%d"), TargetWizardId);
}

void UCoMDiplomacyWidget::OnProposeAllianceClicked(int32 TargetWizardId)
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnProposeAllianceClicked stub called — TargetWizardId=%d"), TargetWizardId);
}

void UCoMDiplomacyWidget::OnTradeClicked(int32 TargetWizardId)
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnTradeClicked stub called — TargetWizardId=%d"), TargetWizardId);
}

// =============================================================================
// Protected Button Callbacks (bound to UButton::OnClicked)
// =============================================================================

void UCoMDiplomacyWidget::OnCloseClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnCloseClicked stub called"));
}

void UCoMDiplomacyWidget::OnDeclareWarButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnDeclareWarButtonClicked stub called"));
	OnDeclareWarClicked(SelectedTargetWizardId);
}

void UCoMDiplomacyWidget::OnProposePeaceButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnProposePeaceButtonClicked stub called"));
	OnProposePeaceClicked(SelectedTargetWizardId);
}

void UCoMDiplomacyWidget::OnProposeAllianceButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnProposeAllianceButtonClicked stub called"));
	OnProposeAllianceClicked(SelectedTargetWizardId);
}

void UCoMDiplomacyWidget::OnTradeButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnTradeButtonClicked stub called"));
	OnTradeClicked(SelectedTargetWizardId);
}

void UCoMDiplomacyWidget::OnProposeTreatyButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnProposeTreatyButtonClicked stub called"));
}

void UCoMDiplomacyWidget::OnSendGiftButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMDiplomacyWidget::OnSendGiftButtonClicked stub called"));
}
