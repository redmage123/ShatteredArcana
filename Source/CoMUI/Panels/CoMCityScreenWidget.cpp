// Copyright Mythforge Studios. All Rights Reserved.
// CoMCityScreenWidget.cpp -- City management screen implementation.

#include "CoMCityScreenWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/ProgressBar.h"
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
		FoodText->SetText(FText::FromString(
			FString::Printf(TEXT("Food: %d"), City->FoodSurplus)));
	}
	if (GoldText)
	{
		GoldText->SetText(FText::FromString(
			FString::Printf(TEXT("Gold: %d"), City->GoldIncome)));
	}
	if (ProductionText)
	{
		ProductionText->SetText(FText::FromString(
			FString::Printf(TEXT("Production: %d"), City->ProductionOutput)));
	}
	if (ManaText)
	{
		ManaText->SetText(FText::FromString(
			FString::Printf(TEXT("Mana: %d"), City->ManaOutput)));
	}
	if (UnrestText)
	{
		UnrestText->SetText(FText::FromString(
			FString::Printf(TEXT("Unrest: %d / %d"), City->Unrest, 10)));
	}

	// Production queue
	if (CurrentBuildText)
	{
		if (City->CurrentBuildingID >= 0)
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
		// Show progress as a rough percentage. Without knowing total cost here,
		// we display a proportional bar capped at 1.0.
		float Progress = (City->CurrentBuildingID >= 0 && City->ProductionOutput > 0)
			? FMath::Clamp(static_cast<float>(City->BuildingProgress) / FMath::Max(City->ProductionOutput * 10, 1), 0.f, 1.f)
			: 0.f;
		BuildProgressBar->SetPercent(Progress);
	}

	RefreshBuildings();
	RefreshGarrison();
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

	// Available buildings placeholder -- in a full implementation this would
	// query a building database and filter out already-built structures.
	if (AvailableBuildingsScrollBox)
	{
		AvailableBuildingsScrollBox->ClearChildren();

		UTextBlock* Placeholder = NewObject<UTextBlock>(this);
		if (Placeholder)
		{
			Placeholder->SetText(FText::FromString(TEXT("(Select a building to queue)")));
			FSlateFontInfo FontInfo = Placeholder->GetFont();
			FontInfo.Size = 12;
			Placeholder->SetFont(FontInfo);
			AvailableBuildingsScrollBox->AddChild(Placeholder);
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
