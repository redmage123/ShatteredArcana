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
#include "Panels/CoMSettingsWidget.h"
#include "Panels/CoMWizardCreationWidget.h"
#include "HUD/CoMMainMenuWidget.h"
#include "HUD/CoMLoadScreenWidget.h"
#include "Panels/CoMVictoryScreenWidget.h"
#include "HUD/CoMTooltipWidget.h"
#include "HUD/CoMTurnNotificationWidget.h"
#include "HUD/CoMSpellTargetingWidget.h"
#include "Panels/CoMPreBattleWidget.h"
#include "Framework/CoMGameInstance.h"
#include "CoMCore/Combat/CoMCombatSubsystem.h"

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

	// Create the turn notification overlay (lives on top of HUD).
	if (!TurnNotificationInstance)
	{
		UCoMTurnNotificationWidget* NotifWidget = CreateAndShowWidget<UCoMTurnNotificationWidget>(
			TurnNotificationWidgetClass, TurnNotificationInstance, 50);
		if (NotifWidget)
		{
			NotifWidget->BindToSubsystems();
		}
	}

	// Bind to combat subsystem's pre-battle delegate so we can show the popup.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMCombatSubsystem* CombatSub = GI->GetSubsystem<UCoMCombatSubsystem>())
		{
			CombatSub->OnPreBattleChoice.AddDynamic(this, &UCoMUISubsystem::ShowPreBattlePopup);
		}
	}

	// Create the tooltip widget (highest Z-order).
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}

	// Create the spell targeting widget (below tooltip, above panels).
	if (!SpellTargetingInstance)
	{
		CreateAndShowWidget<UCoMSpellTargetingWidget>(SpellTargetingWidgetClass, SpellTargetingInstance, 150);
	}
}

void UCoMUISubsystem::HideHUD()
{
	// Unbind notification delegates before removal.
	if (TurnNotificationInstance)
	{
		TurnNotificationInstance->UnbindFromSubsystems();
	}

	RemoveWidget(TurnNotificationInstance);
	RemoveWidget(TooltipInstance);
	RemoveWidget(SpellTargetingInstance);
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
// Load Screen
// =============================================================================

void UCoMUISubsystem::ShowLoadScreen()
{
	HideAllPanels();
	HideMainMenu();
	CreateAndShowWidget<UCoMLoadScreenWidget>(
		LoadScreenWidgetClass, LoadScreenInstance, 100);
}

void UCoMUISubsystem::HideLoadScreen()
{
	RemoveWidget(LoadScreenInstance);
}

// =============================================================================
// Settings
// =============================================================================

void UCoMUISubsystem::ShowSettings()
{
	HideAllPanels();
	CreateAndShowWidget<UCoMSettingsWidget>(SettingsWidgetClass, SettingsInstance, 20);
}

void UCoMUISubsystem::HideSettings()
{
	const bool bWasShowing = (SettingsInstance != nullptr && SettingsInstance->IsInViewport());

	RemoveWidget(SettingsInstance);

	// Re-show the main menu if it was collapsed when we opened settings.
	if (bWasShowing && MainMenuInstance && MainMenuInstance->IsInViewport())
	{
		MainMenuInstance->SetVisibility(ESlateVisibility::Visible);
	}
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
// Victory / Defeat Screen
// =============================================================================

void UCoMUISubsystem::ShowVictoryScreen(int32 WinnerWizardId, ECoMVictoryType VictoryType)
{
	HideAllPanels();

	UCoMVictoryScreenWidget* Widget = CreateAndShowWidget<UCoMVictoryScreenWidget>(
		VictoryScreenWidgetClass, VictoryScreenInstance, 50);

	if (Widget)
	{
		Widget->ShowVictory(WinnerWizardId, VictoryType);
	}
}

void UCoMUISubsystem::ShowDefeatScreen(int32 ConquerorWizardId)
{
	HideAllPanels();

	UCoMVictoryScreenWidget* Widget = CreateAndShowWidget<UCoMVictoryScreenWidget>(
		VictoryScreenWidgetClass, VictoryScreenInstance, 50);

	if (Widget)
	{
		Widget->ShowDefeat(ConquerorWizardId);
	}
}

void UCoMUISubsystem::HideVictoryScreen()
{
	RemoveWidget(VictoryScreenInstance);
}

// =============================================================================
// Tooltip
// =============================================================================

void UCoMUISubsystem::ShowTooltipTile(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint TilePos, int32 WizardId)
{
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}
	if (TooltipInstance)
	{
		TooltipInstance->ShowTileTooltip(Plane, Layer, TilePos, WizardId);
	}
}

void UCoMUISubsystem::ShowTooltipCity(int32 CityId)
{
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}
	if (TooltipInstance)
	{
		TooltipInstance->ShowCityTooltip(CityId);
	}
}

void UCoMUISubsystem::ShowTooltipArmy(int32 ArmyId)
{
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}
	if (TooltipInstance)
	{
		TooltipInstance->ShowArmyTooltip(ArmyId);
	}
}

void UCoMUISubsystem::ShowTooltipUnit(int32 UnitId)
{
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}
	if (TooltipInstance)
	{
		TooltipInstance->ShowUnitTooltip(UnitId);
	}
}

void UCoMUISubsystem::HideTooltip()
{
	if (TooltipInstance)
	{
		TooltipInstance->Hide();
	}
}

// =============================================================================
// Pre-Battle Popup
// =============================================================================

void UCoMUISubsystem::ShowPreBattlePopup(int32 AttackerArmyId, int32 DefenderArmyId)
{
	UCoMPreBattleWidget* Widget = CreateAndShowWidget<UCoMPreBattleWidget>(
		PreBattleWidgetClass, PreBattleInstance, 80);

	if (Widget)
	{
		Widget->SetBattle(AttackerArmyId, DefenderArmyId);
	}
}

void UCoMUISubsystem::HidePreBattlePopup()
{
	RemoveWidget(PreBattleInstance);
}

// =============================================================================
// Spell Targeting
// =============================================================================

void UCoMUISubsystem::BeginSpellTargeting(FName SpellId, int32 CasterWizardId)
{
	if (!SpellTargetingInstance)
	{
		CreateAndShowWidget<UCoMSpellTargetingWidget>(SpellTargetingWidgetClass, SpellTargetingInstance, 150);
	}
	if (SpellTargetingInstance)
	{
		SpellTargetingInstance->BeginTargeting(SpellId, CasterWizardId);
	}
}

void UCoMUISubsystem::CancelSpellTargeting()
{
	if (SpellTargetingInstance)
	{
		SpellTargetingInstance->CancelTargeting();
	}
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
	HideSettings();
	HideCredits();
	HideWizardCreation();
	HideLoadScreen();
	HideVictoryScreen();
}

void UCoMUISubsystem::HideAll()
{
	HideMainMenu();
	HideAllPanels();
	HideHUD();
}
