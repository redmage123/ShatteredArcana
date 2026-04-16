// Copyright Mythforge Studios. All Rights Reserved.
// CoMHistorianWidget.h -- Events log / game history panel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMHistorianWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

/**
 * UCoMHistorianWidget
 *
 * Chronicle of Ages — scrollable game event history with category filters
 * (All, Wars, Discoveries, Events, Diplomacy).
 */
UCLASS()
class COMUI_API UCoMHistorianWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Refresh the event log display. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Historian")
	void RefreshLog();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnFilterAllClicked();
	UFUNCTION() void OnFilterWarsClicked();
	UFUNCTION() void OnFilterDiscoveriesClicked();
	UFUNCTION() void OnFilterEventsClicked();
	UFUNCTION() void OnFilterDiplomacyClicked();

private:
	void BuildLayout();
	UButton* CreateFilterButton(UHorizontalBox* Parent, const FString& Label);
	UButton* CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width = 140.f);

	UPROPERTY() TObjectPtr<UBorder> BackgroundBorder;
	UPROPERTY() TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY() TObjectPtr<UScrollBox> EventLogScrollBox;
	UPROPERTY() TObjectPtr<UHorizontalBox> FilterRow;
	UPROPERTY() TObjectPtr<UButton> FilterAllButton;
	UPROPERTY() TObjectPtr<UButton> FilterWarsButton;
	UPROPERTY() TObjectPtr<UButton> FilterDiscoveriesButton;
	UPROPERTY() TObjectPtr<UButton> FilterEventsButton;
	UPROPERTY() TObjectPtr<UButton> FilterDiplomacyButton;
	UPROPERTY() TObjectPtr<UButton> CloseButton;

	FString CurrentFilter = TEXT("All");
};
