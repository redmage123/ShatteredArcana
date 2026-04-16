// Copyright Mythforge Studios. All Rights Reserved.
// CoMEncyclopediaWidget.cpp -- Encyclopedia / codex browser implementation.

#include "CoMEncyclopediaWidget.h"

#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/EditableTextBox.h"
#include "Blueprint/WidgetTree.h"

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMEncyclopediaWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	BuildLayout();
}

TSharedRef<SWidget> UCoMEncyclopediaWidget::RebuildWidget()
{
	return Super::RebuildWidget();
}

// =============================================================================
// Layout Helpers
// =============================================================================

void UCoMEncyclopediaWidget::BuildLayout()
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::BuildLayout stub called"));
}

UButton* UCoMEncyclopediaWidget::CreateCategoryButton(const FString& Label, UVerticalBox* Parent)
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::CreateCategoryButton stub called — Label=%s"), *Label);
	return NewObject<UButton>(this);
}

UButton* UCoMEncyclopediaWidget::CreateActionButton(const FString& Label, float Width)
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::CreateActionButton stub called — Label=%s"), *Label);
	return NewObject<UButton>(this);
}

void UCoMEncyclopediaWidget::AddCodexEntry(const FString& Name, const FString& Description)
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::AddCodexEntry stub called — Name=%s"), *Name);
}

void UCoMEncyclopediaWidget::PopulateCategory(const FString& Category)
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::PopulateCategory stub called — Category=%s"), *Category);
}

// =============================================================================
// Public API
// =============================================================================

void UCoMEncyclopediaWidget::SelectCategory(const FString& Category)
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::SelectCategory stub called — Category=%s"), *Category);
	CurrentCategory = Category;
	PopulateCategory(Category);
}

// =============================================================================
// Button Callbacks
// =============================================================================

void UCoMEncyclopediaWidget::OnCloseClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::OnCloseClicked stub called"));
}

void UCoMEncyclopediaWidget::OnUnitsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::OnUnitsClicked stub called"));
	SelectCategory(TEXT("Units"));
}

void UCoMEncyclopediaWidget::OnBuildingsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::OnBuildingsClicked stub called"));
	SelectCategory(TEXT("Buildings"));
}

void UCoMEncyclopediaWidget::OnSpellsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::OnSpellsClicked stub called"));
	SelectCategory(TEXT("Spells"));
}

void UCoMEncyclopediaWidget::OnRacesClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::OnRacesClicked stub called"));
	SelectCategory(TEXT("Races"));
}

void UCoMEncyclopediaWidget::OnPlanesClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::OnPlanesClicked stub called"));
	SelectCategory(TEXT("Planes"));
}

void UCoMEncyclopediaWidget::OnItemsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::OnItemsClicked stub called"));
	SelectCategory(TEXT("Items"));
}

void UCoMEncyclopediaWidget::OnDragonsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("CoMEncyclopediaWidget::OnDragonsClicked stub called"));
	SelectCategory(TEXT("Dragons"));
}
