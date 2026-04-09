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
class UCoMCitySubsystem;

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

	/** Request to build the specified building. */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityScreen")
	void OnBuildClicked(int32 BuildingId);

	/** Get the currently displayed city ID. */
	UFUNCTION(BlueprintPure, Category = "CoM|CityScreen")
	int32 GetCurrentCityId() const { return CurrentCityId; }

protected:
	UFUNCTION()
	void OnCloseClicked();

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

private:
	/** The city currently being viewed. */
	int32 CurrentCityId = -1;

	/** Cached pointer to the city subsystem. */
	TWeakObjectPtr<UCoMCitySubsystem> CachedCitySubsystem;

	/** Resolve and cache the city subsystem. */
	UCoMCitySubsystem* GetCitySubsystem();
};
