// Copyright Mythforge Studios. All Rights Reserved.
// CoMCityScreenWidget.h -- City management screen.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCityScreenWidget.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;
class UScrollBox;
class UProgressBar;
class UOverlay;
class UWidgetSwitcher;
class UCoMCitySubsystem;

struct FCoMProductionItem;

/**
 * UCoMCityScreenWidget
 *
 * City management screen displaying city name, population, building list,
 * production queue, garrison, and build controls.
 */
UCLASS()
class COMUI_API UCoMCityScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// -- City Data -------------------------------------------------------------

	/** Load and display data for the specified city. */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityScreen")
	void SetCity(int32 CityId);

	/** Refresh the building list display. */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityScreen")
	void RefreshBuildings();

	/** Refresh the garrison unit display. */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityScreen")
	void RefreshGarrison();

	/** Refresh the production queue display from city data. */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityScreen")
	void RefreshQueue();

	/** Request to build the specified building (legacy single-item). */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityScreen")
	void OnBuildClicked(int32 BuildingId);

	/** Show the build picker overlay. */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityScreen")
	void ShowBuildPicker();

	/** Hide the build picker overlay. */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityScreen")
	void HideBuildPicker();

	/** Called when a build item is selected from the picker. */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityScreen")
	void OnBuildItemSelected(FName ItemID, bool bIsUnit);

	/** Get the currently displayed city ID. */
	UFUNCTION(BlueprintPure, Category = "CoM|CityScreen")
	int32 GetCurrentCityId() const { return CurrentCityId; }

protected:
	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnAddToQueueClicked();

	UFUNCTION()
	void OnRemoveFromQueue(int32 Index);

	UFUNCTION()
	void OnMoveUp(int32 Index);

	UFUNCTION()
	void OnMoveDown(int32 Index);

	UFUNCTION()
	void OnBuildingsTabClicked();

	UFUNCTION()
	void OnUnitsTabClicked();

	UFUNCTION()
	void OnCloseBuildPickerClicked();

	// -- Widget references (bound from UMG) ------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CityNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PopulationText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FoodText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProductionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ManaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnrestText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentBuildText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BuildProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> BuildingListScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> AvailableBuildingsScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> GarrisonScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// -- Production Queue UI ---------------------------------------------------

	/** Text showing "Production Queue" header. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QueueHeaderText;

	/** Scrollable list of queued production items. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> QueueScrollBox;

	/** Progress bar for the item currently being produced. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> QueueProgressBar;

	/** Button to open the build picker. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AddToQueueButton;

	// -- Build Picker Overlay --------------------------------------------------

	/** Overlay containing the build picker popup. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> BuildPickerOverlay;

	/** Tab button for buildings in the build picker. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BuildingsTabButton;

	/** Tab button for units in the build picker. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> UnitsTabButton;

	/** Scroll box for the build picker item grid. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> BuildPickerScrollBox;

	/** Button to close the build picker. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseBuildPickerButton;

	// -- Resource Output Display -----------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResearchText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GrowthText;

private:
	/** The city currently being viewed. */
	int32 CurrentCityId = -1;

	/** Whether the build picker is currently showing units (true) or buildings (false). */
	bool bBuildPickerShowingUnits = false;

	/** Cached pointer to the city subsystem. */
	TWeakObjectPtr<UCoMCitySubsystem> CachedCitySubsystem;

	/** Resolve and cache the city subsystem. */
	UCoMCitySubsystem* GetCitySubsystem();
};
