// Copyright Mythforge Studios. All Rights Reserved.
// CoMUISubsystem.h -- Central UI manager for widget creation and display.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "CoMUISubsystem.generated.h"

class UCoMHUDWidget;
class UCoMCityScreenWidget;
class UCoMSpellBookWidget;
class UCoMDiplomacyWidget;
class UCoMArmyPanelWidget;
class UCoMCreditsWidget;
class UCoMMainMenuWidget;
class UCoMWizardCreationWidget;
class UCoMLoadScreenWidget;

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

	// -- Credits ---------------------------------------------------------------

	/** Create and show the credits screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void ShowCredits();

	/** Close the credits screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UI")
	void HideCredits();

	// -- Bulk Operations -------------------------------------------------------

	/** Close all panels except the HUD. */
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
	TSubclassOf<UCoMCreditsWidget> CreditsWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMWizardCreationWidget> WizardCreationWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|UI|Classes")
	TSubclassOf<UCoMLoadScreenWidget> LoadScreenWidgetClass;

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
	UCoMCreditsWidget* CreditsInstance = nullptr;

	UPROPERTY()
	UCoMWizardCreationWidget* WizardCreationInstance = nullptr;

	UPROPERTY()
	UCoMLoadScreenWidget* LoadScreenInstance = nullptr;
};
