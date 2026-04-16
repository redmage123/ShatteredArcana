// Copyright Mythforge Studios. All Rights Reserved.
// CoMQuestLogWidget.h -- Quest journal with active/completed/failed tabs.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMQuestLogWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UOverlay;
class UProgressBar;

/**
 * UCoMQuestLogWidget
 *
 * Quest journal panel with Active/Completed/Failed tabs. Shows a scrollable
 * list of quests with name, description, objective text, progress bar, and
 * reward text. Selected quest shows expanded details at bottom.
 */
UCLASS()
class COMUI_API UCoMQuestLogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Refresh the quest list for the given wizard. */
	UFUNCTION(BlueprintCallable, Category = "CoM|QuestLog")
	void RefreshQuests(int32 WizardId);

protected:
	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnActiveTabClicked();

	UFUNCTION()
	void OnCompletedTabClicked();

	UFUNCTION()
	void OnFailedTabClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> QuestScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailDescText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailRewardText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ActiveTab;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CompletedTab;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> FailedTab;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	void BuildLayout();
	UButton* CreateTabButton(const FString& Label);
	UButton* CreateActionButton(const FString& Label, float Width = 140.f);

	/** Add a quest entry to the scroll box. */
	void AddQuestEntry(const FString& Name, const FString& Description,
		const FString& Objective, float Progress, const FString& Reward);

	FString CurrentTab = TEXT("Active");
	int32 CurrentWizardId = -1;
};
