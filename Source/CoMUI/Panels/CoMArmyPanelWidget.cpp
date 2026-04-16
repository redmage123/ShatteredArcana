// Copyright Mythforge Studios. All Rights Reserved.
// CoMArmyPanelWidget.cpp -- Army info panel implementation.

#include "CoMArmyPanelWidget.h"

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

#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"

// =============================================================================
// Subsystem Accessors
// =============================================================================

UCoMUnitSubsystem* UCoMArmyPanelWidget::GetUnitSubsystem()
{
	if (CachedUnitSubsystem.IsValid()) { return CachedUnitSubsystem.Get(); }
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedUnitSubsystem = GI->GetSubsystem<UCoMUnitSubsystem>();
		return CachedUnitSubsystem.Get();
	}
	return nullptr;
}

UCoMCitySubsystem* UCoMArmyPanelWidget::GetCitySubsystem()
{
	if (CachedCitySubsystem.IsValid()) { return CachedCitySubsystem.Get(); }
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedCitySubsystem = GI->GetSubsystem<UCoMCitySubsystem>();
		return CachedCitySubsystem.Get();
	}
	return nullptr;
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMArmyPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	BuildLayout();
}

TSharedRef<SWidget> UCoMArmyPanelWidget::RebuildWidget()
{
	return Super::RebuildWidget();
}

// =============================================================================
// Layout Helpers
// =============================================================================

void UCoMArmyPanelWidget::BuildLayout()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::BuildLayout stub called"));
}

UButton* UCoMArmyPanelWidget::CreateStyledButton(const FString& Label, UVerticalBox* Parent)
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::CreateStyledButton stub called — Label=%s"), *Label);
	return NewObject<UButton>(this);
}

UTextBlock* UCoMArmyPanelWidget::AddLabelToBox(const FString& Text, const FLinearColor& Color, int32 FontSize, UVerticalBox* Parent, const FMargin& Pad)
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::AddLabelToBox stub called — Text=%s"), *Text);
	return NewObject<UTextBlock>(this);
}

int32 UCoMArmyPanelWidget::FindSettlerInCurrentArmy() const
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::FindSettlerInCurrentArmy stub called"));
	return -1;
}

void UCoMArmyPanelWidget::UpdateFoundCityButton()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::UpdateFoundCityButton stub called"));
}

void UCoMArmyPanelWidget::UpdateAutoExploreButton()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::UpdateAutoExploreButton stub called"));
}

// =============================================================================
// Public API
// =============================================================================

void UCoMArmyPanelWidget::SetArmy(int32 ArmyId)
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::SetArmy stub called — ArmyId=%d"), ArmyId);
	CurrentArmyId = ArmyId;
}

void UCoMArmyPanelWidget::RefreshUnits()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::RefreshUnits stub called"));
}

// =============================================================================
// Public Action Callbacks
// =============================================================================

void UCoMArmyPanelWidget::OnMoveClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnMoveClicked stub called"));
	OnArmyMoveRequested.Broadcast(CurrentArmyId);
}

void UCoMArmyPanelWidget::OnMergeClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnMergeClicked stub called"));
	OnArmyMergeRequested.Broadcast(CurrentArmyId);
}

void UCoMArmyPanelWidget::OnSplitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnSplitClicked stub called"));
	OnArmySplitRequested.Broadcast(CurrentArmyId);
}

void UCoMArmyPanelWidget::OnFoundCityClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnFoundCityClicked stub called"));
	int32 SettlerId = FindSettlerInCurrentArmy();
	OnFoundCityRequested.Broadcast(CurrentArmyId, SettlerId);
}

void UCoMArmyPanelWidget::OnAutoExploreClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnAutoExploreClicked stub called"));
}

// =============================================================================
// Protected Button Callbacks (bound to UButton::OnClicked)
// =============================================================================

void UCoMArmyPanelWidget::OnMoveButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnMoveButtonClicked stub called"));
	OnMoveClicked();
}

void UCoMArmyPanelWidget::OnMergeButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnMergeButtonClicked stub called"));
	OnMergeClicked();
}

void UCoMArmyPanelWidget::OnSplitButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnSplitButtonClicked stub called"));
	OnSplitClicked();
}

void UCoMArmyPanelWidget::OnFoundCityButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnFoundCityButtonClicked stub called"));
	OnFoundCityClicked();
}

void UCoMArmyPanelWidget::OnAutoExploreButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnAutoExploreButtonClicked stub called"));
	OnAutoExploreClicked();
}

void UCoMArmyPanelWidget::OnCloseClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnCloseClicked stub called"));
}

void UCoMArmyPanelWidget::OnDisbandButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMArmyPanelWidget::OnDisbandButtonClicked stub called"));
}
