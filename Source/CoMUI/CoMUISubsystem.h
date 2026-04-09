// Copyright Mythforge Studios. All Rights Reserved.
// CoMUISubsystem.h -- Central UI manager for widget creation and display.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMUISubsystem.generated.h"

class UCoMHUDWidget;
class UCoMCityScreenWidget;
class UCoMSpellBookWidget;
class UCoMDiplomacyWidget;
class UCoMArmyPanelWidget;
class UCoMCreditsWidget;
class UCoMSettingsWidget;
class UCoMMainMenuWidget;
class UCoMWizardCreationWidget;
class UCoMLoadScreenWidget;
class UCoMVictoryScreenWidget;
class UCoMTooltipWidget;
class UCoMTurnNotificationWidget;
class UCoMSpellTargetingWidget;
class UCoMPreBattleWidget;

/**
 * UCoMUISubsystem
 *
 * GameInstance subsystem that manages creation, display, and lifecycle of all
 * major UI panels: HUD, city screen, spell book, diplomacy, and army panel.
 *
 * Widget class references (TSubclassOf) should be set in Blueprint defaults
 * of a derived Blueprint subsystem, or assigned at runtime before showing.
 */
UCLASS()
class COMUI_API UCoMUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -- Main Menu -------------------------------------------------------------

	/** Create and show the main menu title screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowMainMenu();

	/** Remove the main menu from the viewport. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideMainMenu();

	// -- HUD -------------------------------------------------------------------

	/** Create and add the HUD widget to the viewport. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowHUD();

	/** Remove the HUD from the viewport. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideHUD();

	/** Get the active HUD widget (may be nullptr). */
	UFUNCTION(BlueprintPure, Category = "CoM|UI")
	UCoMHUDWidget* GetHUDWidget() const { return HUDWidgetInstance; }

	// -- City Screen -----------------------------------------------------------

	/** Create and show the city screen for the given city. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowCityScreen(int32 CityId);

	/** Close the city screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideCityScreen();

	// -- Spell Book ------------------------------------------------------------

	/** Create and show the spell book for the given wizard. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowSpellBook(int32 WizardId);

	/** Close the spell book. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideSpellBook();

	// -- Diplomacy -------------------------------------------------------------

	/** Create and show the diplomacy screen for the given wizard. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowDiplomacy(int32 WizardId);

	/** Close the diplomacy screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideDiplomacy();

	// -- Army Panel ------------------------------------------------------------

	/** Create and show the army panel for the given army. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowArmyPanel(int32 ArmyId);

	/** Close the army panel. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideArmyPanel();

	// -- Wizard Creation -------------------------------------------------------

	/** Create and show the wizard creation screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowWizardCreation();

	/** Close the wizard creation screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideWizardCreation();

	// -- Load Screen -----------------------------------------------------------

	/** Create and show the load game screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowLoadScreen();

	/** Close the load game screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideLoadScreen();

	// -- Victory / Defeat Screen -----------------------------------------------

	/** Show the full-screen victory overlay for the winning wizard. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowVictoryScreen(int32 WinnerWizardId, ECoMVictoryType VictoryType);

	/** Show the full-screen defeat overlay. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowDefeatScreen(int32 ConquerorWizardId);

	/** Close the victory/defeat screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideVictoryScreen();

	// -- Settings --------------------------------------------------------------

	/** Create and show the settings screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowSettings();

	/** Close the settings screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideSettings();

	// -- Credits ---------------------------------------------------------------

	/** Create and show the credits screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowCredits();

	/** Close the credits screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideCredits();

	// -- Tooltip ---------------------------------------------------------------

	/** Show a tile tooltip at the given map position. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowTooltipTile(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint TilePos, int32 WizardId);

	/** Show a city tooltip. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowTooltipCity(int32 CityId);

	/** Show an army tooltip. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowTooltipArmy(int32 ArmyId);

	/** Show a unit tooltip. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowTooltipUnit(int32 UnitId);

	/** Hide the tooltip. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideTooltip();

	/** Get the tooltip widget (may be nullptr). */
	UFUNCTION(BlueprintPure, Category = "CoM|UI")
	UCoMTooltipWidget* GetTooltipWidget() const { return TooltipInstance; }

	// -- Turn Notifications ----------------------------------------------------

	/** Get the turn notification widget (may be nullptr). */
	UFUNCTION(BlueprintPure, Category = "CoM|UI")
	UCoMTurnNotificationWidget* GetTurnNotificationWidget() const { return TurnNotificationInstance; }

	// -- Pre-Battle Popup ------------------------------------------------------

	/** Show the pre-battle choice popup for the given encounter. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowPreBattlePopup(int32 AttackerArmyId, int32 DefenderArmyId);

	/** Hide the pre-battle popup. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HidePreBattlePopup();

	// -- Spell Targeting -------------------------------------------------------

	/** Get the spell targeting widget (may be nullptr). */
	UFUNCTION(BlueprintPure, Category = "CoM|UI")
	UCoMSpellTargetingWidget* GetSpellTargetingWidget() const { return SpellTargetingInstance; }

	/** Begin spell targeting mode (called from spell book Cast button). */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void BeginSpellTargeting(FName SpellId, int32 CasterWizardId);

	/** Cancel spell targeting. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void CancelSpellTargeting();

	// -- Bulk Operations -------------------------------------------------------

	/** Close all panels except the HUD (includes victory screen). */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideAllPanels();

	/** Close everything including the HUD. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideAll();

	// -- Widget Class Configuration --------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMHUDWidget> HUDWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMCityScreenWidget> CityScreenWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMSpellBookWidget> SpellBookWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMDiplomacyWidget> DiplomacyWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMArmyPanelWidget> ArmyPanelWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMSettingsWidget> SettingsWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMCreditsWidget> CreditsWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMWizardCreationWidget> WizardCreationWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMLoadScreenWidget> LoadScreenWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMVictoryScreenWidget> VictoryScreenWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMTooltipWidget> TooltipWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMTurnNotificationWidget> TurnNotificationWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMPreBattleWidget> PreBattleWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMSpellTargetingWidget> SpellTargetingWidgetClass;

private:
	/** Helper: create a widget of the given class and add it to the viewport. */
	template<typename T>
	T* CreateAndShowWidget(TSubclassOf<T> WidgetClass, T*& InstanceRef, int32 ZOrder = 0);

	/** Helper: remove a widget from the viewport and null the reference. */
	template<typename T>
	void RemoveWidget(T*& InstanceRef);

	// -- Active widget instances -----------------------------------------------

	UPROPERTY()
	UCoMMainMenuWidget* MainMenuInstance = nullptr;

	UPROPERTY()
	UCoMHUDWidget* HUDWidgetInstance = nullptr;

	UPROPERTY()
	UCoMCityScreenWidget* CityScreenInstance = nullptr;

	UPROPERTY()
	UCoMSpellBookWidget* SpellBookInstance = nullptr;

	UPROPERTY()
	UCoMDiplomacyWidget* DiplomacyInstance = nullptr;

	UPROPERTY()
	UCoMArmyPanelWidget* ArmyPanelInstance = nullptr;

	UPROPERTY()
	UCoMSettingsWidget* SettingsInstance = nullptr;

	UPROPERTY()
	UCoMCreditsWidget* CreditsInstance = nullptr;

	UPROPERTY()
	UCoMWizardCreationWidget* WizardCreationInstance = nullptr;

	UPROPERTY()
	UCoMLoadScreenWidget* LoadScreenInstance = nullptr;

	UPROPERTY()
	UCoMVictoryScreenWidget* VictoryScreenInstance = nullptr;

	UPROPERTY()
	UCoMTooltipWidget* TooltipInstance = nullptr;

	UPROPERTY()
	UCoMTurnNotificationWidget* TurnNotificationInstance = nullptr;

	UPROPERTY()
	UCoMPreBattleWidget* PreBattleInstance = nullptr;

	UPROPERTY()
	UCoMSpellTargetingWidget* SpellTargetingInstance = nullptr;
};
