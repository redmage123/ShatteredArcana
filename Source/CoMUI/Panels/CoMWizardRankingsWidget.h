// Copyright Mythforge Studios. All Rights Reserved.
// CoMWizardRankingsWidget.h -- Power graph and wizard comparison panel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMWizardRankingsWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UOverlay;

/**
 * UCoMWizardRankingsWidget
 *
 * Displays wizard power rankings with a placeholder power graph and a
 * scrollable table of stats (rank, name, score, cities, military, mana,
 * spells, territory). Current player row is highlighted in gold.
 */
UCLASS()
class COMUI_API UCoMWizardRankingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Refresh the ranking table and power graph data. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Rankings")
	void RefreshRankings();

protected:
	UFUNCTION()
	void OnCloseClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> RankingScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GraphPlaceholderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreSummaryText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ContentBox;

private:
	void BuildLayout();
	UButton* CreateActionButton(const FString& Label, float Width = 140.f);

	/** Add a single ranking row to the scroll box. */
	void AddRankingRow(int32 Rank, const FString& WizardName, const FLinearColor& WizardColor,
		int32 Score, int32 Cities, int32 Military, int32 Mana, int32 Spells, int32 Territory,
		bool bIsPlayer);

	int32 CurrentTurnNumber = 0;
};
