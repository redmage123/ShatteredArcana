// Copyright Mythforge Studios. All Rights Reserved.
// CoMResourceOverviewWidget.h -- Treasury and resource overview panel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMResourceOverviewWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UOverlay;

/**
 * UCoMResourceOverviewWidget
 *
 * Treasury and resources panel showing a summary row (gold, mana, research),
 * a scrollable city breakdown table, upkeep section, and net income line.
 */
UCLASS()
class COMUI_API UCoMResourceOverviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Refresh all resource data for the given wizard. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Resources")
	void RefreshOverview(int32 WizardId);

protected:
	UFUNCTION()
	void OnCloseClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> CityScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UpkeepText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NetIncomeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	void BuildLayout();
	UButton* CreateActionButton(const FString& Label, float Width = 140.f);

	/** Add a city row to the breakdown table. */
	void AddCityRow(const FString& CityName, int32 GoldVal, int32 Food, int32 Production,
		int32 Mana, int32 Research, int32 Pop, bool bIsTotalRow = false);

	/** Add a column header row. */
	void AddColumnHeaders();

	int32 CurrentWizardId = -1;
};
