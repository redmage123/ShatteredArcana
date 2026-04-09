// Copyright Mythforge Studios. All Rights Reserved.
// CoMArmyPanelWidget.cpp -- Army info panel implementation.

#include "CoMArmyPanelWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "CoMUI/CoMUISubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"

void UCoMArmyPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MoveButton)      { MoveButton->OnClicked.AddDynamic(this, &UCoMArmyPanelWidget::OnMoveButtonClicked); }
	if (MergeButton)     { MergeButton->OnClicked.AddDynamic(this, &UCoMArmyPanelWidget::OnMergeButtonClicked); }
	if (SplitButton)     { SplitButton->OnClicked.AddDynamic(this, &UCoMArmyPanelWidget::OnSplitButtonClicked); }
	if (FoundCityButton) { FoundCityButton->OnClicked.AddDynamic(this, &UCoMArmyPanelWidget::OnFoundCityButtonClicked); }
	if (CloseButton)     { CloseButton->OnClicked.AddDynamic(this, &UCoMArmyPanelWidget::OnCloseClicked); }
}

UCoMUnitSubsystem* UCoMArmyPanelWidget::GetUnitSubsystem()
{
	if (CachedUnitSubsystem.IsValid())
	{
		return CachedUnitSubsystem.Get();
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI)
	{
		UCoMUnitSubsystem* Sub = GI->GetSubsystem<UCoMUnitSubsystem>();
		CachedUnitSubsystem = Sub;
		return Sub;
	}
	return nullptr;
}

void UCoMArmyPanelWidget::SetArmy(int32 ArmyId)
{
	CurrentArmyId = ArmyId;

	UCoMUnitSubsystem* UnitSub = GetUnitSubsystem();
	if (!UnitSub)
	{
		return;
	}

	const FCoMArmyGroup* Army = UnitSub->GetArmy(ArmyId);
	if (!Army)
	{
		return;
	}

	// Header.
	if (ArmyHeaderText)
	{
		ArmyHeaderText->SetText(FText::FromString(
			FString::Printf(TEXT("Army #%d  (%d units)"),
				ArmyId, Army->UnitIDs.Num())));
	}

	// Movement points.
	if (MovementText)
	{
		MovementText->SetText(FText::FromString(
			FString::Printf(TEXT("Movement: %d"), Army->MovementRemaining)));
	}

	// Position.
	if (PositionText)
	{
		const UEnum* PlaneEnum = StaticEnum<ECoMPlane>();
		FString PlaneName = PlaneEnum
			? PlaneEnum->GetDisplayNameTextByValue(static_cast<int64>(Army->Plane)).ToString()
			: TEXT("Unknown");

		PositionText->SetText(FText::FromString(
			FString::Printf(TEXT("Position: (%d, %d) %s"),
				Army->Position.X, Army->Position.Y, *PlaneName)));
	}

	RefreshUnits();
	UpdateFoundCityButton();
}

void UCoMArmyPanelWidget::RefreshUnits()
{
	UCoMUnitSubsystem* UnitSub = GetUnitSubsystem();
	if (!UnitSub || !UnitListScrollBox || CurrentArmyId < 0)
	{
		return;
	}

	const FCoMArmyGroup* Army = UnitSub->GetArmy(CurrentArmyId);
	if (!Army)
	{
		return;
	}

	UnitListScrollBox->ClearChildren();

	// Track whether we found a hero for the detail panel.
	bool bFoundHero = false;

	for (int32 UnitID : Army->UnitIDs)
	{
		const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitID);
		if (!Unit)
		{
			continue;
		}

		// Create a horizontal row: [Name] [HP Bar] [Level]
		UHorizontalBox* Row = NewObject<UHorizontalBox>(this);
		if (!Row)
		{
			continue;
		}

		// Unit name.
		UTextBlock* NameText = NewObject<UTextBlock>(this);
		if (NameText)
		{
			FString HeroPrefix = Unit->bIsHero ? TEXT("[H] ") : TEXT("");
			NameText->SetText(FText::FromString(
				FString::Printf(TEXT("%s%s"), *HeroPrefix, *Unit->SpecID.ToString())));

			FSlateFontInfo FontInfo = NameText->GetFont();
			FontInfo.Size = 13;
			NameText->SetFont(FontInfo);

			UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText);
			if (NameSlot)
			{
				NameSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			}
		}

		// HP bar.
		UProgressBar* HPBar = NewObject<UProgressBar>(this);
		if (HPBar)
		{
			// We use HitPoints from the stat block as max; since we only have
			// CurrentHP on the instance, we estimate max from CurrentHP if the
			// unit is undamaged (level 1 default). A proper implementation would
			// look up the spec's HitPoints.
			float HPPercent = FMath::Clamp(
				static_cast<float>(Unit->CurrentHP) / FMath::Max(Unit->MaxHP, 1),
				0.f, 1.f);
			HPBar->SetPercent(HPPercent);

			// Color: green when healthy, red when low.
			FLinearColor BarColor = HPPercent > 0.5f
				? FLinearColor(0.2f, 0.8f, 0.2f, 1.f)
				: FLinearColor(0.8f, 0.2f, 0.2f, 1.f);
			HPBar->SetFillColorAndOpacity(BarColor);

			UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(HPBar);
			if (BarSlot)
			{
				BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				BarSlot->SetPadding(FMargin(4.f, 2.f, 4.f, 2.f));
			}
		}

		// Level text.
		UTextBlock* LevelText = NewObject<UTextBlock>(this);
		if (LevelText)
		{
			LevelText->SetText(FText::FromString(
				FString::Printf(TEXT("Lv%d"), Unit->Level)));

			FSlateFontInfo FontInfo = LevelText->GetFont();
			FontInfo.Size = 12;
			LevelText->SetFont(FontInfo);

			Row->AddChildToHorizontalBox(LevelText);
		}

		UnitListScrollBox->AddChild(Row);

		// Fill hero detail if this unit is a hero.
		if (Unit->bIsHero && !bFoundHero)
		{
			bFoundHero = true;

			if (HeroNameText)
			{
				HeroNameText->SetText(FText::FromString(Unit->SpecID.ToString()));
			}
			if (HeroLevelText)
			{
				HeroLevelText->SetText(FText::FromString(
					FString::Printf(TEXT("Level %d  |  XP: %d"),
						Unit->Level, Unit->Experience)));
			}
		}
	}

	// Show/hide hero detail panel.
	if (HeroDetailBox)
	{
		HeroDetailBox->SetVisibility(bFoundHero ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UCoMArmyPanelWidget::OnMoveClicked()
{
	if (CurrentArmyId >= 0)
	{
		OnArmyMoveRequested.Broadcast(CurrentArmyId);
	}
}

void UCoMArmyPanelWidget::OnMergeClicked()
{
	if (CurrentArmyId >= 0)
	{
		OnArmyMergeRequested.Broadcast(CurrentArmyId);
	}
}

void UCoMArmyPanelWidget::OnSplitClicked()
{
	if (CurrentArmyId >= 0)
	{
		OnArmySplitRequested.Broadcast(CurrentArmyId);
	}
}

void UCoMArmyPanelWidget::OnFoundCityClicked()
{
	if (CurrentArmyId < 0) return;

	const int32 SettlerId = FindSettlerInCurrentArmy();
	if (SettlerId >= 0)
	{
		UCoMUnitSubsystem* UnitSub = GetUnitSubsystem();
		if (UnitSub)
		{
			const int32 NewCityId = UnitSub->FoundCityWithSettler(CurrentArmyId, SettlerId);
			if (NewCityId >= 0)
			{
				OnFoundCityRequested.Broadcast(CurrentArmyId, SettlerId);

				// The army may have been disbanded if the settler was the only unit.
				// Refresh or close the panel.
				UCoMUnitSubsystem* Sub = GetUnitSubsystem();
				if (Sub && Sub->GetArmy(CurrentArmyId))
				{
					SetArmy(CurrentArmyId); // Refresh
				}
				else
				{
					RemoveFromParent(); // Army was disbanded
				}
			}
		}
	}
}

void UCoMArmyPanelWidget::OnMoveButtonClicked()       { OnMoveClicked(); }
void UCoMArmyPanelWidget::OnMergeButtonClicked()      { OnMergeClicked(); }
void UCoMArmyPanelWidget::OnSplitButtonClicked()      { OnSplitClicked(); }
void UCoMArmyPanelWidget::OnFoundCityButtonClicked()   { OnFoundCityClicked(); }
void UCoMArmyPanelWidget::OnCloseClicked()
{
	if (auto* UISS = GetGameInstance()->GetSubsystem<UCoMUISubsystem>())
	{
		UISS->HideArmyPanel();
	}
}

UCoMCitySubsystem* UCoMArmyPanelWidget::GetCitySubsystem()
{
	if (CachedCitySubsystem.IsValid())
	{
		return CachedCitySubsystem.Get();
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI)
	{
		UCoMCitySubsystem* Sub = GI->GetSubsystem<UCoMCitySubsystem>();
		CachedCitySubsystem = Sub;
		return Sub;
	}
	return nullptr;
}

int32 UCoMArmyPanelWidget::FindSettlerInCurrentArmy() const
{
	if (CurrentArmyId < 0) return -1;

	UCoMUnitSubsystem* UnitSub = const_cast<UCoMArmyPanelWidget*>(this)->GetUnitSubsystem();
	if (!UnitSub) return -1;

	const FCoMArmyGroup* Army = UnitSub->GetArmy(CurrentArmyId);
	if (!Army) return -1;

	for (int32 UnitID : Army->UnitIDs)
	{
		const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitID);
		if (Unit && Unit->bIsSettler)
		{
			return UnitID;
		}
	}
	return -1;
}

void UCoMArmyPanelWidget::UpdateFoundCityButton()
{
	if (!FoundCityButton)
	{
		return;
	}

	const int32 SettlerId = FindSettlerInCurrentArmy();
	if (SettlerId < 0)
	{
		// No settler in this army -- hide the button entirely.
		FoundCityButton->SetVisibility(ESlateVisibility::Collapsed);
		if (FoundCityTooltipText)
		{
			FoundCityTooltipText->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	// Army has a settler -- show the button.
	FoundCityButton->SetVisibility(ESlateVisibility::Visible);

	// Check if we can found a city at the current position.
	UCoMUnitSubsystem* UnitSub = GetUnitSubsystem();
	UCoMCitySubsystem* CitySub = GetCitySubsystem();

	if (!UnitSub || !CitySub)
	{
		FoundCityButton->SetIsEnabled(false);
		return;
	}

	const FCoMArmyGroup* Army = UnitSub->GetArmy(CurrentArmyId);
	const FCoMUnitInstance* Settler = UnitSub->GetUnit(SettlerId);
	if (!Army || !Settler)
	{
		FoundCityButton->SetIsEnabled(false);
		return;
	}

	const bool bCanFound = CitySub->CanFoundCityAt(
		Army->Plane, Army->Layer, Army->Position, Settler->RaceTag);

	FoundCityButton->SetIsEnabled(bCanFound);

	// Set tooltip explaining why the button is disabled.
	if (FoundCityTooltipText)
	{
		if (bCanFound)
		{
			FoundCityTooltipText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			FoundCityTooltipText->SetVisibility(ESlateVisibility::Visible);

			// Determine the reason for failure.
			FString Reason;
			if (CitySub->CanFoundCityAt(Army->Plane, Army->Layer, Army->Position, FGameplayTag()))
			{
				Reason = TEXT("Race cannot settle here");
			}
			else
			{
				// Check if it's a distance or terrain issue.
				// Simplified: we report the most common reasons.
				Reason = TEXT("Too close to another city or invalid terrain");
			}

			FoundCityTooltipText->SetText(FText::FromString(Reason));
		}
	}
}
