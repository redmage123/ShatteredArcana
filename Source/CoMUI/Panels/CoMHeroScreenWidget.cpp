// Copyright Mythforge Studios. All Rights Reserved.
// CoMHeroScreenWidget.cpp -- Hero character sheet implementation.

#include "CoMHeroScreenWidget.h"

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
#include "Blueprint/WidgetTree.h"

// =============================================================================
// Subsystem Accessors
// =============================================================================

UCoMUnitSubsystem* UCoMHeroScreenWidget::GetUnitSubsystem()
{
	if (CachedUnitSubsystem.IsValid()) { return CachedUnitSubsystem.Get(); }
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedUnitSubsystem = GI->GetSubsystem<UCoMUnitSubsystem>();
		return CachedUnitSubsystem.Get();
	}
	return nullptr;
}

UCoMHeroSubsystem* UCoMHeroScreenWidget::GetHeroSubsystem()
{
	if (CachedHeroSubsystem.IsValid()) { return CachedHeroSubsystem.Get(); }
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedHeroSubsystem = GI->GetSubsystem<UCoMHeroSubsystem>();
		return CachedHeroSubsystem.Get();
	}
	return nullptr;
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMHeroScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	BuildHeroLayout();
}

TSharedRef<SWidget> UCoMHeroScreenWidget::RebuildWidget()
{
	return Super::RebuildWidget();
}

// =============================================================================
// Layout
// =============================================================================

void UCoMHeroScreenWidget::BuildHeroLayout()
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::BuildHeroLayout stub called"));
}

void UCoMHeroScreenWidget::BuildLeftColumn(UVerticalBox* LeftBox)
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::BuildLeftColumn stub called"));
}

void UCoMHeroScreenWidget::BuildRightColumn(UVerticalBox* RightBox)
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::BuildRightColumn stub called"));
}

UHorizontalBox* UCoMHeroScreenWidget::CreateStatPair(const FString& Label1, const FString& Value1,
                                                      const FString& Label2, const FString& Value2,
                                                      UVerticalBox* Parent)
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::CreateStatPair stub called"));
	return nullptr;
}

UButton* UCoMHeroScreenWidget::CreateStyledButton(const FString& Label, UVerticalBox* Parent)
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::CreateStyledButton stub called"));
	return NewObject<UButton>(this);
}

// =============================================================================
// Public API
// =============================================================================

void UCoMHeroScreenWidget::SetHero(int32 HeroUnitId)
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::SetHero stub called — HeroUnitId=%d"), HeroUnitId);
	CurrentHeroUnitId = HeroUnitId;
}

// =============================================================================
// Button Callbacks
// =============================================================================

void UCoMHeroScreenWidget::OnCloseClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::OnCloseClicked stub called"));
}

void UCoMHeroScreenWidget::OnItem0Clicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::OnItem0Clicked — Item slot 0 clicked"));
}

void UCoMHeroScreenWidget::OnItem1Clicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::OnItem1Clicked — Item slot 1 clicked"));
}

void UCoMHeroScreenWidget::OnItem2Clicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::OnItem2Clicked — Item slot 2 clicked"));
}

void UCoMHeroScreenWidget::OnItem3Clicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::OnItem3Clicked — Item slot 3 clicked"));
}

void UCoMHeroScreenWidget::OnItem4Clicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::OnItem4Clicked — Item slot 4 clicked"));
}

void UCoMHeroScreenWidget::OnItem5Clicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMHeroScreenWidget::OnItem5Clicked — Item slot 5 clicked"));
}
