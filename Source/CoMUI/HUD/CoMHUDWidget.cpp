// Copyright Mythforge Studios. All Rights Reserved.
// CoMHUDWidget.cpp -- Main HUD overlay implementation.

#include "CoMHUDWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"

void UCoMHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind button click events to our handler methods.
	if (EndTurnButton)
	{
		EndTurnButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnEndTurnClicked);
	}
	if (SpellBookButton)
	{
		SpellBookButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnSpellBookClicked);
	}
	if (CityListButton)
	{
		CityListButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnCityListClicked);
	}
	if (ArmyManagerButton)
	{
		ArmyManagerButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnArmyManagerClicked);
	}
	if (DiplomacyButton)
	{
		DiplomacyButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnDiplomacyClicked);
	}

	// Set initial display values.
	UpdateResources(0, 0, 0, 0);
	UpdateTurnInfo(1, TEXT("Unknown Wizard"));
}

void UCoMHUDWidget::UpdateResources(int32 Gold, int32 Mana, int32 Food, int32 Production)
{
	if (GoldText)
	{
		GoldText->SetText(FText::FromString(FString::Printf(TEXT("Gold: %d"), Gold)));
	}
	if (ManaText)
	{
		ManaText->SetText(FText::FromString(FString::Printf(TEXT("Mana: %d"), Mana)));
	}
	if (FoodText)
	{
		FoodText->SetText(FText::FromString(FString::Printf(TEXT("Food: %d"), Food)));
	}
	if (ProductionText)
	{
		ProductionText->SetText(FText::FromString(FString::Printf(TEXT("Prod: %d"), Production)));
	}
}

void UCoMHUDWidget::UpdateTurnInfo(int32 TurnNumber, const FString& WizardName)
{
	if (TurnInfoText)
	{
		TurnInfoText->SetText(FText::FromString(
			FString::Printf(TEXT("Turn %d - %s"), TurnNumber, *WizardName)));
	}
}

void UCoMHUDWidget::AddNotification(const FString& Message)
{
	if (!NotificationScrollBox)
	{
		return;
	}

	// Create a new text block for the notification.
	UTextBlock* NotifText = NewObject<UTextBlock>(this);
	if (NotifText)
	{
		NotifText->SetText(FText::FromString(Message));

		FSlateFontInfo FontInfo = NotifText->GetFont();
		FontInfo.Size = 12;
		NotifText->SetFont(FontInfo);

		NotificationScrollBox->AddChild(NotifText);

		// Trim oldest notifications if we exceed the cap.
		while (NotificationScrollBox->GetChildrenCount() > MaxNotifications)
		{
			UWidget* OldestChild = NotificationScrollBox->GetChildAt(0);
			if (OldestChild)
			{
				NotificationScrollBox->RemoveChild(OldestChild);
			}
		}

		// Scroll to the bottom to show the newest notification.
		NotificationScrollBox->ScrollToEnd();
	}
}

void UCoMHUDWidget::ClearNotifications()
{
	if (NotificationScrollBox)
	{
		NotificationScrollBox->ClearChildren();
	}
}

void UCoMHUDWidget::OnEndTurnClicked()
{
	OnEndTurnRequested.Broadcast();
}

void UCoMHUDWidget::OnSpellBookClicked()
{
	OnSpellBookRequested.Broadcast();
}

void UCoMHUDWidget::OnCityListClicked()
{
	OnCityListRequested.Broadcast();
}

void UCoMHUDWidget::OnArmyManagerClicked()
{
	OnArmyManagerRequested.Broadcast();
}

void UCoMHUDWidget::OnDiplomacyClicked()
{
	OnDiplomacyRequested.Broadcast();
}
