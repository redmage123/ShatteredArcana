// Copyright Mythforge Studios. All Rights Reserved.
// CoMCityScreenWidget.cpp -- City management screen implementation.

#include "CoMCityScreenWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/ProgressBar.h"
#include "Components/Overlay.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMUI/CoMUISubsystem.h"

void UCoMCityScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnCloseClicked);
	}

	if (AddToQueueButton)
	{
		AddToQueueButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnAddToQueueClicked);
	}

	if (BuildingsTabButton)
	{
		BuildingsTabButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnBuildingsTabClicked);
	}

	if (UnitsTabButton)
	{
		UnitsTabButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnUnitsTabClicked);
	}

	if (CloseBuildPickerButton)
	{
		CloseBuildPickerButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnCloseBuildPickerClicked);
	}

	// Bind city focus buttons.
	if (FocusManualButton)     { FocusManualButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnFocusManualClicked); }
	if (FocusGrowthButton)     { FocusGrowthButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnFocusGrowthClicked); }
	if (FocusMilitaryButton)   { FocusMilitaryButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnFocusMilitaryClicked); }
	if (FocusEconomyButton)    { FocusEconomyButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnFocusEconomyClicked); }
	if (FocusResearchButton)   { FocusResearchButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnFocusResearchClicked); }
	if (FocusProductionButton) { FocusProductionButton->OnClicked.AddDynamic(this, &UCoMCityScreenWidget::OnFocusProductionClicked); }

	// Hide the build picker overlay on startup.
	HideBuildPicker();
}

UCoMCitySubsystem* UCoMCityScreenWidget::GetCitySubsystem()
{
	if (CachedCitySubsystem.IsValid())
	{
		return CachedCitySubsystem.Get();
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI)
	{
		UCoMCitySubsystem* Subsystem = GI->GetSubsystem<UCoMCitySubsystem>();
		CachedCitySubsystem = Subsystem;
		return Subsystem;
	}
	return nullptr;
}

void UCoMCityScreenWidget::SetCity(int32 CityId)
{
	CurrentCityId = CityId;

	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (!CitySub)
	{
		return;
	}

	const FCoMCityData* City = CitySub->GetCity(CityId);
	if (!City)
	{
		return;
	}

	// City header
	if (CityNameText)
	{
		CityNameText->SetText(City->CityName);
	}
	if (PopulationText)
	{
		int32 PopCap = CitySub->GetCityPopulationCap(CityId);
		PopulationText->SetText(FText::FromString(
			FString::Printf(TEXT("Population: %d / %d"), City->Population, PopCap)));
	}

	// Resource outputs
	if (FoodText)
	{
		const FString FoodSign = (City->FoodSurplus >= 0) ? TEXT("+") : TEXT("");
		FoodText->SetText(FText::FromString(
			FString::Printf(TEXT("Food: %s%d per turn"), *FoodSign, City->FoodSurplus)));
	}
	if (GoldText)
	{
		GoldText->SetText(FText::FromString(
			FString::Printf(TEXT("Gold: +%d per turn"), City->GoldIncome)));
	}
	if (ProductionText)
	{
		ProductionText->SetText(FText::FromString(
			FString::Printf(TEXT("Production: +%d per turn"), City->ProductionOutput)));
	}
	if (ManaText)
	{
		ManaText->SetText(FText::FromString(
			FString::Printf(TEXT("Mana: +%d per turn"), City->ManaOutput)));
	}
	if (ResearchText)
	{
		ResearchText->SetText(FText::FromString(
			FString::Printf(TEXT("Research: +%d per turn"), City->ResearchOutput)));
	}
	if (UnrestText)
	{
		UnrestText->SetText(FText::FromString(
			FString::Printf(TEXT("Unrest: %d / %d"), City->Unrest, 10)));
	}

	// Population growth estimate.
	if (GrowthText)
	{
		if (City->FoodSurplus > 0)
		{
			// Rough estimate: food surplus / growth divisor gives growth per turn.
			// Population cap tells us when growth stops.
			const int32 GrowthRate = FMath::Max(1, City->FoodSurplus / 10);
			const int32 PopCap = CitySub ? CitySub->GetCityPopulationCap(CurrentCityId) : 25;
			const int32 Remaining = FMath::Max(0, PopCap - City->Population);
			const int32 TurnsToNext = (GrowthRate > 0 && Remaining > 0)
				? FMath::CeilToInt32(1.f / GrowthRate) : 0;

			if (TurnsToNext > 0)
			{
				GrowthText->SetText(FText::FromString(
					FString::Printf(TEXT("Growth: %d turns to next pop"), TurnsToNext)));
			}
			else
			{
				GrowthText->SetText(FText::FromString(TEXT("Growth: at capacity")));
			}
		}
		else
		{
			GrowthText->SetText(FText::FromString(TEXT("Growth: stagnant")));
		}
	}

	// Production queue display (legacy fallback in CurrentBuildText).
	if (CurrentBuildText)
	{
		if (City->ProductionQueue.Num() > 0)
		{
			const FCoMProductionItem& Front = City->ProductionQueue[0];
			CurrentBuildText->SetText(FText::FromString(
				FString::Printf(TEXT("Producing: %s (%d / %d)"),
					*Front.ItemID.ToString(), City->AccumulatedProduction, Front.ProductionCost)));
		}
		else if (City->CurrentBuildingID >= 0)
		{
			CurrentBuildText->SetText(FText::FromString(
				FString::Printf(TEXT("Building ID %d (Progress: %d)"),
					City->CurrentBuildingID, City->BuildingProgress)));
		}
		else
		{
			CurrentBuildText->SetText(FText::FromString(TEXT("Nothing in production")));
		}
	}

	if (BuildProgressBar)
	{
		if (City->ProductionQueue.Num() > 0)
		{
			const FCoMProductionItem& Front = City->ProductionQueue[0];
			const float Progress = FMath::Clamp(
				static_cast<float>(City->AccumulatedProduction) / FMath::Max(Front.ProductionCost, 1),
				0.f, 1.f);
			BuildProgressBar->SetPercent(Progress);
		}
		else
		{
			float Progress = (City->CurrentBuildingID >= 0 && City->ProductionOutput > 0)
				? FMath::Clamp(static_cast<float>(City->BuildingProgress) / FMath::Max(City->ProductionOutput * 10, 1), 0.f, 1.f)
				: 0.f;
			BuildProgressBar->SetPercent(Progress);
		}
	}

	RefreshBuildings();
	RefreshQueue();
	RefreshGarrison();
	RefreshFocusButtons();

	// Update queue header with "(Auto)" if governor is active.
	if (QueueHeaderText)
	{
		const ECoMCityFocus Focus = CitySub->GetCityFocus(CityId);
		if (Focus != ECoMCityFocus::Manual)
		{
			QueueHeaderText->SetText(FText::FromString(TEXT("Production Queue (Auto)")));
		}
		else
		{
			QueueHeaderText->SetText(FText::FromString(TEXT("Production Queue")));
		}
	}
}

void UCoMCityScreenWidget::RefreshBuildings()
{
	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (!CitySub || CurrentCityId < 0)
	{
		return;
	}

	const FCoMCityData* City = CitySub->GetCity(CurrentCityId);
	if (!City)
	{
		return;
	}

	// Existing buildings
	if (BuildingListScrollBox)
	{
		BuildingListScrollBox->ClearChildren();
		for (int32 BuildingID : City->BuildingIDs)
		{
			UTextBlock* Entry = NewObject<UTextBlock>(this);
			if (Entry)
			{
				Entry->SetText(FText::FromString(
					FString::Printf(TEXT("Building #%d"), BuildingID)));

				FSlateFontInfo FontInfo = Entry->GetFont();
				FontInfo.Size = 14;
				Entry->SetFont(FontInfo);

				BuildingListScrollBox->AddChild(Entry);
			}
		}
	}

	// Available buildings list (from the new availability query system).
	if (AvailableBuildingsScrollBox)
	{
		AvailableBuildingsScrollBox->ClearChildren();

		const TArray<FName> Available = CitySub->GetAvailableBuildings(CurrentCityId);
		if (Available.Num() == 0)
		{
			UTextBlock* Placeholder = NewObject<UTextBlock>(this);
			if (Placeholder)
			{
				Placeholder->SetText(FText::FromString(TEXT("(No buildings available)")));
				FSlateFontInfo FontInfo = Placeholder->GetFont();
				FontInfo.Size = 12;
				Placeholder->SetFont(FontInfo);
				AvailableBuildingsScrollBox->AddChild(Placeholder);
			}
		}
		else
		{
			for (const FName& BuildingName : Available)
			{
				UTextBlock* Entry = NewObject<UTextBlock>(this);
				if (Entry)
				{
					Entry->SetText(FText::FromString(BuildingName.ToString()));
					FSlateFontInfo FontInfo = Entry->GetFont();
					FontInfo.Size = 12;
					Entry->SetFont(FontInfo);
					AvailableBuildingsScrollBox->AddChild(Entry);
				}
			}
		}
	}
}

void UCoMCityScreenWidget::RefreshGarrison()
{
	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (!CitySub || !GarrisonScrollBox || CurrentCityId < 0)
	{
		return;
	}

	const FCoMCityData* City = CitySub->GetCity(CurrentCityId);
	if (!City)
	{
		return;
	}

	GarrisonScrollBox->ClearChildren();

	if (City->GarrisonArmyID < 0)
	{
		UTextBlock* Empty = NewObject<UTextBlock>(this);
		if (Empty)
		{
			Empty->SetText(FText::FromString(TEXT("No garrison")));
			GarrisonScrollBox->AddChild(Empty);
		}
		return;
	}

	// Query the unit subsystem for army composition.
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI)
	{
		return;
	}

	UCoMUnitSubsystem* UnitSub = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (!UnitSub)
	{
		return;
	}

	const FCoMArmyGroup* Army = UnitSub->GetArmy(City->GarrisonArmyID);
	if (!Army)
	{
		return;
	}

	for (int32 UnitID : Army->UnitIDs)
	{
		const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitID);
		if (!Unit)
		{
			continue;
		}

		UTextBlock* UnitEntry = NewObject<UTextBlock>(this);
		if (UnitEntry)
		{
			FString HeroTag = Unit->bIsHero ? TEXT(" [Hero]") : TEXT("");
			UnitEntry->SetText(FText::FromString(
				FString::Printf(TEXT("%s  HP: %d  Lv: %d%s"),
					*Unit->SpecID.ToString(), Unit->CurrentHP, Unit->Level, *HeroTag)));

			FSlateFontInfo FontInfo = UnitEntry->GetFont();
			FontInfo.Size = 12;
			UnitEntry->SetFont(FontInfo);

			GarrisonScrollBox->AddChild(UnitEntry);
		}
	}
}

void UCoMCityScreenWidget::RefreshQueue()
{
	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (!CitySub || !QueueScrollBox || CurrentCityId < 0)
	{
		return;
	}

	QueueScrollBox->ClearChildren();

	const TArray<FCoMProductionItem> Queue = CitySub->GetQueue(CurrentCityId);

	if (Queue.Num() == 0)
	{
		UTextBlock* EmptyText = NewObject<UTextBlock>(this);
		if (EmptyText)
		{
			EmptyText->SetText(FText::FromString(TEXT("Queue is empty")));
			FSlateFontInfo FontInfo = EmptyText->GetFont();
			FontInfo.Size = 12;
			EmptyText->SetFont(FontInfo);
			QueueScrollBox->AddChild(EmptyText);
		}
		return;
	}

	// Update the queue-level progress bar for the first item.
	if (QueueProgressBar && Queue.Num() > 0)
	{
		const FCoMProductionItem& Front = Queue[0];
		const float Progress = FMath::Clamp(
			static_cast<float>(CitySub->GetCity(CurrentCityId)->AccumulatedProduction) /
			FMath::Max(Front.ProductionCost, 1),
			0.f, 1.f);
		QueueProgressBar->SetPercent(Progress);
	}

	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		const FCoMProductionItem& Item = Queue[i];

		// Create a horizontal box for each queue row: name + turns + remove button.
		UHorizontalBox* Row = NewObject<UHorizontalBox>(this);
		if (!Row)
		{
			continue;
		}

		// Item name and type.
		UTextBlock* NameText = NewObject<UTextBlock>(this);
		if (NameText)
		{
			const FString TypeTag = Item.bIsUnit ? TEXT("[Unit]") : TEXT("[Bldg]");
			NameText->SetText(FText::FromString(
				FString::Printf(TEXT("%s %s"), *TypeTag, *Item.ItemID.ToString())));

			FSlateFontInfo FontInfo = NameText->GetFont();
			FontInfo.Size = 12;
			NameText->SetFont(FontInfo);

			UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(NameText);
			if (Slot)
			{
				Slot->SetPadding(FMargin(4.f, 2.f));
			}
		}

		// Turns remaining.
		UTextBlock* TurnsText = NewObject<UTextBlock>(this);
		if (TurnsText)
		{
			if (i == 0)
			{
				TurnsText->SetText(FText::FromString(
					FString::Printf(TEXT("(%d turns)"), Item.TurnsRemaining)));
			}
			else
			{
				TurnsText->SetText(FText::FromString(
					FString::Printf(TEXT("~%d turns"), Item.TurnsRemaining)));
			}

			FSlateFontInfo FontInfo = TurnsText->GetFont();
			FontInfo.Size = 11;
			TurnsText->SetFont(FontInfo);

			UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(TurnsText);
			if (Slot)
			{
				Slot->SetPadding(FMargin(8.f, 2.f));
			}
		}

		QueueScrollBox->AddChild(Row);
	}
}

void UCoMCityScreenWidget::ShowBuildPicker()
{
	if (BuildPickerOverlay)
	{
		BuildPickerOverlay->SetVisibility(ESlateVisibility::Visible);
	}

	bBuildPickerShowingUnits = false;

	// Populate with buildings by default.
	OnBuildingsTabClicked();
}

void UCoMCityScreenWidget::HideBuildPicker()
{
	if (BuildPickerOverlay)
	{
		BuildPickerOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCoMCityScreenWidget::OnBuildItemSelected(FName ItemID, bool bIsUnit)
{
	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (CitySub && CurrentCityId >= 0)
	{
		CitySub->AddToQueue(CurrentCityId, ItemID, bIsUnit);
		SetCity(CurrentCityId); // Full refresh.
	}

	HideBuildPicker();
}

void UCoMCityScreenWidget::OnAddToQueueClicked()
{
	ShowBuildPicker();
}

void UCoMCityScreenWidget::OnRemoveFromQueue(int32 Index)
{
	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (CitySub && CurrentCityId >= 0)
	{
		CitySub->RemoveFromQueue(CurrentCityId, Index);
		SetCity(CurrentCityId);
	}
}

void UCoMCityScreenWidget::OnMoveUp(int32 Index)
{
	if (Index <= 0)
	{
		return;
	}

	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (CitySub && CurrentCityId >= 0)
	{
		CitySub->MoveInQueue(CurrentCityId, Index, Index - 1);
		SetCity(CurrentCityId);
	}
}

void UCoMCityScreenWidget::OnMoveDown(int32 Index)
{
	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (!CitySub || CurrentCityId < 0)
	{
		return;
	}

	const TArray<FCoMProductionItem> Queue = CitySub->GetQueue(CurrentCityId);
	if (Index >= Queue.Num() - 1)
	{
		return;
	}

	CitySub->MoveInQueue(CurrentCityId, Index, Index + 1);
	SetCity(CurrentCityId);
}

void UCoMCityScreenWidget::OnBuildingsTabClicked()
{
	bBuildPickerShowingUnits = false;

	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (!CitySub || !BuildPickerScrollBox || CurrentCityId < 0)
	{
		return;
	}

	BuildPickerScrollBox->ClearChildren();

	const TArray<FName> Available = CitySub->GetAvailableBuildings(CurrentCityId);

	if (Available.Num() == 0)
	{
		UTextBlock* NoItems = NewObject<UTextBlock>(this);
		if (NoItems)
		{
			NoItems->SetText(FText::FromString(TEXT("No buildings available")));
			FSlateFontInfo FontInfo = NoItems->GetFont();
			FontInfo.Size = 12;
			NoItems->SetFont(FontInfo);
			BuildPickerScrollBox->AddChild(NoItems);
		}
		return;
	}

	for (const FName& BuildingID : Available)
	{
		UTextBlock* Entry = NewObject<UTextBlock>(this);
		if (Entry)
		{
			Entry->SetText(FText::FromString(BuildingID.ToString()));
			FSlateFontInfo FontInfo = Entry->GetFont();
			FontInfo.Size = 13;
			Entry->SetFont(FontInfo);
			BuildPickerScrollBox->AddChild(Entry);
		}
	}
}

void UCoMCityScreenWidget::OnUnitsTabClicked()
{
	bBuildPickerShowingUnits = true;

	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (!CitySub || !BuildPickerScrollBox || CurrentCityId < 0)
	{
		return;
	}

	BuildPickerScrollBox->ClearChildren();

	const TArray<FName> Available = CitySub->GetAvailableUnits(CurrentCityId);

	if (Available.Num() == 0)
	{
		UTextBlock* NoItems = NewObject<UTextBlock>(this);
		if (NoItems)
		{
			NoItems->SetText(FText::FromString(TEXT("No units available")));
			FSlateFontInfo FontInfo = NoItems->GetFont();
			FontInfo.Size = 12;
			NoItems->SetFont(FontInfo);
			BuildPickerScrollBox->AddChild(NoItems);
		}
		return;
	}

	for (const FName& UnitSpecID : Available)
	{
		UTextBlock* Entry = NewObject<UTextBlock>(this);
		if (Entry)
		{
			Entry->SetText(FText::FromString(UnitSpecID.ToString()));
			FSlateFontInfo FontInfo = Entry->GetFont();
			FontInfo.Size = 13;
			Entry->SetFont(FontInfo);
			BuildPickerScrollBox->AddChild(Entry);
		}
	}
}

void UCoMCityScreenWidget::OnCloseBuildPickerClicked()
{
	HideBuildPicker();
}

void UCoMCityScreenWidget::OnBuildClicked(int32 BuildingId)
{
	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (CitySub && CurrentCityId >= 0)
	{
		CitySub->SetBuildingQueue(CurrentCityId, BuildingId);
		// Refresh display to show new production queue.
		SetCity(CurrentCityId);
	}
}

void UCoMCityScreenWidget::OnCloseClicked()
{
	if (auto* UISS = GetGameInstance()->GetSubsystem<UCoMUISubsystem>())
	{
		UISS->HideCityScreen();
	}
}

// =============================================================================
// City Focus / Governor
// =============================================================================

void UCoMCityScreenWidget::SetCityFocus(ECoMCityFocus Focus)
{
	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (CitySub && CurrentCityId >= 0)
	{
		CitySub->SetCityFocus(CurrentCityId, Focus);
		SetCity(CurrentCityId); // Full refresh.
	}
}

void UCoMCityScreenWidget::RefreshFocusButtons()
{
	UCoMCitySubsystem* CitySub = GetCitySubsystem();
	if (!CitySub || CurrentCityId < 0)
	{
		return;
	}

	const ECoMCityFocus CurrentFocus = CitySub->GetCityFocus(CurrentCityId);

	// Gold color for active focus, default for inactive.
	const FLinearColor GoldColor(0.85f, 0.65f, 0.13f, 1.f);
	const FLinearColor DefaultColor(0.3f, 0.3f, 0.3f, 1.f);

	auto SetButtonColor = [&](UButton* Btn, ECoMCityFocus BtnFocus)
	{
		if (!Btn) return;
		const FLinearColor& Color = (CurrentFocus == BtnFocus) ? GoldColor : DefaultColor;
		Btn->SetBackgroundColor(Color);
	};

	SetButtonColor(FocusManualButton,     ECoMCityFocus::Manual);
	SetButtonColor(FocusGrowthButton,     ECoMCityFocus::BalancedGrowth);
	SetButtonColor(FocusMilitaryButton,   ECoMCityFocus::Military);
	SetButtonColor(FocusEconomyButton,    ECoMCityFocus::Economy);
	SetButtonColor(FocusResearchButton,   ECoMCityFocus::Research);
	SetButtonColor(FocusProductionButton, ECoMCityFocus::Production);

	// Update focus label.
	if (FocusLabelText)
	{
		const UEnum* FocusEnum = StaticEnum<ECoMCityFocus>();
		FString FocusName = FocusEnum
			? FocusEnum->GetDisplayNameTextByValue(static_cast<int64>(CurrentFocus)).ToString()
			: TEXT("Manual");
		FocusLabelText->SetText(FText::FromString(
			FString::Printf(TEXT("City Focus: %s"), *FocusName)));
	}
}

void UCoMCityScreenWidget::OnFocusManualClicked()     { SetCityFocus(ECoMCityFocus::Manual); }
void UCoMCityScreenWidget::OnFocusGrowthClicked()     { SetCityFocus(ECoMCityFocus::BalancedGrowth); }
void UCoMCityScreenWidget::OnFocusMilitaryClicked()   { SetCityFocus(ECoMCityFocus::Military); }
void UCoMCityScreenWidget::OnFocusEconomyClicked()    { SetCityFocus(ECoMCityFocus::Economy); }
void UCoMCityScreenWidget::OnFocusResearchClicked()   { SetCityFocus(ECoMCityFocus::Research); }
void UCoMCityScreenWidget::OnFocusProductionClicked() { SetCityFocus(ECoMCityFocus::Production); }
