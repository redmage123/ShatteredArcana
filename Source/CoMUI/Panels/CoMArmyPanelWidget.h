// Copyright Mythforge Studios. All Rights Reserved.
// CoMArmyPanelWidget.h -- Army info panel for selected armies on the overworld.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMArmyPanelWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UProgressBar;
class UBorder;
class USizeBox;
class UHorizontalBox;
class UOverlay;
class UCoMUnitSubsystem;
class UCoMCitySubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArmyMoveRequested, int32, ArmyId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArmyMergeRequested, int32, ArmyId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArmySplitRequested, int32, ArmyId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFoundCityRequested, int32, ArmyId, int32, SettlerUnitId);

/**
 * UCoMArmyPanelWidget
 *
 * Army info panel displayed when an army is selected on the overworld map.
 * Shows unit list with icons, HP bars, hero details, movement points,
 * and merge/split controls.
 */
UCLASS()
class COMUI_API UCoMArmyPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// -- Army Data -------------------------------------------------------------

	/** Load and display data for the specified army. */
	UFUNCTION(BlueprintCallable, Category = "CoM|ArmyPanel")
	void SetArmy(int32 ArmyId);

	/** Refresh the unit list display. */
	UFUNCTION(BlueprintCallable, Category = "CoM|ArmyPanel")
	void RefreshUnits();

	/** Get the currently displayed army ID. */
	UFUNCTION(BlueprintPure, Category = "CoM|ArmyPanel")
	int32 GetCurrentArmyId() const { return CurrentArmyId; }

	// -- Action Callbacks ------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "CoM|ArmyPanel")
	void OnMoveClicked();

	UFUNCTION(BlueprintCallable, Category = "CoM|ArmyPanel")
	void OnMergeClicked();

	UFUNCTION(BlueprintCallable, Category = "CoM|ArmyPanel")
	void OnSplitClicked();

	/** Attempt to found a city with the settler in this army. */
	UFUNCTION(BlueprintCallable, Category = "CoM|ArmyPanel")
	void OnFoundCityClicked();

	/** Toggle auto-explore for this army. */
	UFUNCTION(BlueprintCallable, Category = "CoM|ArmyPanel")
	void OnAutoExploreClicked();

	// -- Delegates -------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|ArmyPanel")
	FOnArmyMoveRequested OnArmyMoveRequested;

	UPROPERTY(BlueprintAssignable, Category = "CoM|ArmyPanel")
	FOnArmyMergeRequested OnArmyMergeRequested;

	UPROPERTY(BlueprintAssignable, Category = "CoM|ArmyPanel")
	FOnArmySplitRequested OnArmySplitRequested;

	UPROPERTY(BlueprintAssignable, Category = "CoM|ArmyPanel")
	FOnFoundCityRequested OnFoundCityRequested;

protected:
	UFUNCTION()
	void OnMoveButtonClicked();

	UFUNCTION()
	void OnMergeButtonClicked();

	UFUNCTION()
	void OnSplitButtonClicked();

	UFUNCTION()
	void OnFoundCityButtonClicked();

	UFUNCTION()
	void OnAutoExploreButtonClicked();

	UFUNCTION()
	void OnCloseClicked();

	// -- Widget references -----------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ArmyHeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MovementText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PositionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> UnitListScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> HeroDetailBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeroNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeroLevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MoveButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MergeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SplitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> FoundCityButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FoundCityTooltipText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AutoExploreButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AutoExploreStatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DisbandButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TotalStrengthText;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY()
	TObjectPtr<UBorder> BackgroundBorder;

	UFUNCTION()
	void OnDisbandButtonClicked();

private:
	/** Build the entire panel layout in C++. */
	void BuildLayout();

	/** Create a styled action button and add it to a parent vertical box. */
	UButton* CreateStyledButton(const FString& Label, UVerticalBox* Parent);

	/** Helper: add a label to a vertical box. */
	UTextBlock* AddLabelToBox(const FString& Text, const FLinearColor& Color, int32 FontSize, UVerticalBox* Parent, const FMargin& Pad = FMargin(0));

	UCoMUnitSubsystem* GetUnitSubsystem();
	UCoMCitySubsystem* GetCitySubsystem();

	/** Find the first settler unit in the currently displayed army. Returns -1 if none. */
	int32 FindSettlerInCurrentArmy() const;

	/** Update the FoundCity button visibility and enabled state. */
	void UpdateFoundCityButton();

	/** Update the auto-explore button state (active/inactive). */
	void UpdateAutoExploreButton();

	int32 CurrentArmyId = -1;

	TWeakObjectPtr<UCoMUnitSubsystem> CachedUnitSubsystem;
	TWeakObjectPtr<UCoMCitySubsystem> CachedCitySubsystem;
};
