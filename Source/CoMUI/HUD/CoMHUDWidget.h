// Copyright Mythforge Studios. All Rights Reserved.
// CoMHUDWidget.h -- Main HUD overlay for overworld gameplay.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMHUDWidget.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;
class UHorizontalBox;
class UScrollBox;
class UBorder;
class UCoMMinimapWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndTurnRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpellBookRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCityListRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArmyManagerRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDiplomacyRequested);

/**
 * UCoMHUDWidget
 *
 * Main HUD overlay shown during overworld gameplay.
 * Displays resource bar (gold, mana, food, production), turn indicator,
 * minimap frame, action buttons, and a scrolling notification area.
 */
UCLASS()
class COMUI_API UCoMHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// -- Construction ----------------------------------------------------------

	virtual void NativeConstruct() override;

	// -- Resource Display ------------------------------------------------------

	/** Update the resource bar with current values. */
	UFUNCTION(BlueprintCallable, Category = "CoM|HUD")
	void UpdateResources(int32 Gold, int32 Mana, int32 Food, int32 Production);

	// -- Turn Display ----------------------------------------------------------

	/** Update the turn indicator text. */
	UFUNCTION(BlueprintCallable, Category = "CoM|HUD")
	void UpdateTurnInfo(int32 TurnNumber, const FString& WizardName);

	// -- Minimap ---------------------------------------------------------------

	/**
	 * Initialize the embedded minimap for a given wizard and starting plane.
	 * Call after NativeConstruct, once game state is ready.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|HUD")
	void InitializeMinimap(int32 WizardId, ECoMPlane StartingPlane);

	/** Access the minimap widget (e.g. to bind OnMinimapNavigate). */
	UFUNCTION(BlueprintPure, Category = "CoM|HUD")
	UCoMMinimapWidget* GetMinimapWidget() const { return MinimapWidget; }

	/** Trigger a full minimap refresh (e.g. on turn end). */
	UFUNCTION(BlueprintCallable, Category = "CoM|HUD")
	void RefreshMinimap();

	// -- Notifications ---------------------------------------------------------

	/** Add a notification message to the scrolling event list. */
	UFUNCTION(BlueprintCallable, Category = "CoM|HUD")
	void AddNotification(const FString& Message);

	/** Clear all notifications. */
	UFUNCTION(BlueprintCallable, Category = "CoM|HUD")
	void ClearNotifications();

	// -- Game Speed Control ----------------------------------------------------

	/** Set the game speed multiplier from the HUD speed buttons. */
	UFUNCTION(BlueprintCallable, Category = "CoM|HUD")
	void SetGameSpeed(float Speed);

	/** Update the speed button display to show the current speed. */
	UFUNCTION(BlueprintCallable, Category = "CoM|HUD")
	void UpdateSpeedDisplay(float CurrentSpeed);

	// -- Delegates (so the UI subsystem can bind) ------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|HUD")
	FOnEndTurnRequested OnEndTurnRequested;

	UPROPERTY(BlueprintAssignable, Category = "CoM|HUD")
	FOnSpellBookRequested OnSpellBookRequested;

	UPROPERTY(BlueprintAssignable, Category = "CoM|HUD")
	FOnCityListRequested OnCityListRequested;

	UPROPERTY(BlueprintAssignable, Category = "CoM|HUD")
	FOnArmyManagerRequested OnArmyManagerRequested;

	UPROPERTY(BlueprintAssignable, Category = "CoM|HUD")
	FOnDiplomacyRequested OnDiplomacyRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();

protected:
	// -- Button callbacks ------------------------------------------------------

	UFUNCTION()
	void OnEndTurnClicked();

	UFUNCTION()
	void OnSpellBookClicked();

	UFUNCTION()
	void OnCityListClicked();

	UFUNCTION()
	void OnArmyManagerClicked();

	UFUNCTION()
	void OnDiplomacyClicked();

	UFUNCTION()
	void OnSpeed1xClicked();

	UFUNCTION()
	void OnSpeed2xClicked();

	UFUNCTION()
	void OnSpeed4xClicked();

	// -- Widget references (bound from UMG or created in C++) ------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ManaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FoodText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProductionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TurnInfoText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EndTurnButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SpellBookButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CityListButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ArmyManagerButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DiplomacyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> NotificationScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Speed1xButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Speed2xButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Speed4xButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeedDisplayText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> MinimapFrame;

	/** The actual minimap widget, created in C++ and added to MinimapFrame. */
	UPROPERTY()
	TObjectPtr<UCoMMinimapWidget> MinimapWidget;

private:
	/** Maximum notifications to keep in the scroll list before trimming oldest. */
	static constexpr int32 MaxNotifications = 50;
};
