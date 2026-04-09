// Copyright Mythforge Studios. All Rights Reserved.
// CoMUISubsystem.cpp -- Central UI manager implementation.

#include "CoMUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "HUD/CoMHUDWidget.h"
#include "Panels/CoMCityScreenWidget.h"
#include "Panels/CoMSpellBookWidget.h"
#include "Panels/CoMDiplomacyWidget.h"
#include "Panels/CoMArmyPanelWidget.h"
#include "Panels/CoMCreditsWidget.h"
#include "Panels/CoMWizardCreationWidget.h"
#include "HUD/CoMMainMenuWidget.h"
#include "Framework/CoMGameInstance.h"

void UCoMUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Bind to the main menu delegate so ACoMMainMenuGameMode can trigger
	// ShowMainMenu() without a compile-time dependency on CoMUI.
	if (UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance()))
	{
		CoMGI->OnMainMenuRequested.AddUObject(this, &UCoMUISubsystem::ShowMainMenu);
	}
}

void UCoMUISubsystem::Deinitialize()
{
	// Unbind delegate before teardown.
	if (UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance()))
	{
		CoMGI->OnMainMenuRequested.RemoveAll(this);
	}

	HideAll();
	Super::Deinitialize();
}

// =============================================================================
// Template helpers
// =============================================================================

template<typename T>
T* UCoMUISubsystem::CreateAndShowWidget(TSubclassOf<T> WidgetClass, T*& InstanceRef, int32 ZOrder)
{
	// If already showing, just return the existing instance.
	if (InstanceRef && InstanceRef->IsInViewport())
	{
		return InstanceRef;
	}

	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMUISubsystem: Widget class not set for %s"), *T::StaticClass()->GetName());
		return nullptr;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}

	UWorld* World = GI->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	InstanceRef = CreateWidget<T>(World, WidgetClass);
	if (InstanceRef)
	{
		InstanceRef->AddToViewport(ZOrder);
	}

	return InstanceRef;
}

template<typename T>
void UCoMUISubsystem::RemoveWidget(T*& InstanceRef)
{
	if (InstanceRef)
	{
		if (InstanceRef->IsInViewport())
		{
			InstanceRef->RemoveFromParent();
		}
		InstanceRef = nullptr;
	}
}

// =============================================================================
// Main Menu
// =============================================================================

void UCoMUISubsystem::ShowMainMenu()
{
	CreateAndShowWidget<UCoMMainMenuWidget>(MainMenuWidgetClass, MainMenuInstance, 100);
}

void UCoMUISubsystem::HideMainMenu()
{
	RemoveWidget(MainMenuInstance);
}

// =============================================================================
// HUD
// =============================================================================

void UCoMUISubsystem::ShowHUD()
{
	CreateAndShowWidget<UCoMHUDWidget>(HUDWidgetClass, HUDWidgetInstance, 0);
}

void UCoMUISubsystem::HideHUD()
{
	RemoveWidget(HUDWidgetInstance);
}

// =============================================================================
// City Screen
// =============================================================================

void UCoMUISubsystem::ShowCityScreen(int32 CityId)
{
	// Close any other panel (except HUD) before opening city screen.
	HideAllPanels();

	UCoMCityScreenWidget* Widget = CreateAndShowWidget<UCoMCityScreenWidget>(
		CityScreenWidgetClass, CityScreenInstance, 10);

	if (Widget)
	{
		Widget->SetCity(CityId);
	}
}

void UCoMUISubsystem::HideCityScreen()
{
	RemoveWidget(CityScreenInstance);
}

// =============================================================================
// Spell Book
// =============================================================================

void UCoMUISubsystem::ShowSpellBook(int32 WizardId)
{
	HideAllPanels();

	UCoMSpellBookWidget* Widget = CreateAndShowWidget<UCoMSpellBookWidget>(
		SpellBookWidgetClass, SpellBookInstance, 10);

	if (Widget)
	{
		Widget->SetWizardId(WizardId);
	}
}

void UCoMUISubsystem::HideSpellBook()
{
	RemoveWidget(SpellBookInstance);
}

// =============================================================================
// Diplomacy
// =============================================================================

void UCoMUISubsystem::ShowDiplomacy(int32 WizardId)
{
	HideAllPanels();

	UCoMDiplomacyWidget* Widget = CreateAndShowWidget<UCoMDiplomacyWidget>(
		DiplomacyWidgetClass, DiplomacyInstance, 10);

	if (Widget)
	{
		Widget->SetPlayerWizardId(WizardId);
	}
}

void UCoMUISubsystem::HideDiplomacy()
{
	RemoveWidget(DiplomacyInstance);
}

// =============================================================================
// Army Panel
// =============================================================================

void UCoMUISubsystem::ShowArmyPanel(int32 ArmyId)
{
	// Army panel can coexist with the HUD, but close other full-screen panels.
	HideCityScreen();
	HideSpellBook();
	HideDiplomacy();

	UCoMArmyPanelWidget* Widget = CreateAndShowWidget<UCoMArmyPanelWidget>(
		ArmyPanelWidgetClass, ArmyPanelInstance, 5);

	if (Widget)
	{
		Widget->SetArmy(ArmyId);
	}
}

void UCoMUISubsystem::HideArmyPanel()
{
	RemoveWidget(ArmyPanelInstance);
}

// =============================================================================
// Wizard Creation
// =============================================================================

void UCoMUISubsystem::ShowWizardCreation()
{
	HideAllPanels();
	HideMainMenu();
	CreateAndShowWidget<UCoMWizardCreationWidget>(
		WizardCreationWidgetClass, WizardCreationInstance, 100);
}

void UCoMUISubsystem::HideWizardCreation()
{
	RemoveWidget(WizardCreationInstance);
}

// =============================================================================
// Credits
// =============================================================================

void UCoMUISubsystem::ShowCredits()
{
	HideAllPanels();

	UCoMCreditsWidget* Widget = CreateAndShowWidget<UCoMCreditsWidget>(
		CreditsWidgetClass, CreditsInstance, 20);

	if (Widget)
	{
		Widget->StartCredits();
	}
}

void UCoMUISubsystem::HideCredits()
{
	RemoveWidget(CreditsInstance);
}

// =============================================================================
// Bulk operations
// =============================================================================

void UCoMUISubsystem::HideAllPanels()
{
	HideCityScreen();
	HideSpellBook();
	HideDiplomacy();
	HideArmyPanel();
	HideCredits();
	HideWizardCreation();
}

void UCoMUISubsystem::HideAll()
{
	HideMainMenu();
	HideAllPanels();
	HideHUD();
}
