// Copyright Mythforge Studios. All Rights Reserved.
// CoMNotificationCenterWidget.h -- Slide-in notification history panel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMNotificationCenterWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UOverlay;

/**
 * UCoMNotificationCenterWidget
 *
 * Slide-in panel from the right side showing past notifications grouped by
 * turn and category. Filter buttons at top for All/Combat/City/Research/
 * Diplomacy. Supports "Clear All" to dismiss all entries.
 */
UCLASS()
class COMUI_API UCoMNotificationCenterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Add a notification entry. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notifications")
	void AddNotification(int32 Turn, const FString& Category, const FString& Message);

	/** Filter the visible list to a specific category (or "All"). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notifications")
	void FilterByCategory(const FString& Category);

protected:
	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnClearAllClicked();

	UFUNCTION()
	void OnFilterAllClicked();

	UFUNCTION()
	void OnFilterCombatClicked();

	UFUNCTION()
	void OnFilterCityClicked();

	UFUNCTION()
	void OnFilterResearchClicked();

	UFUNCTION()
	void OnFilterDiplomacyClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> NotifScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ClearAllButton;

	// Filter buttons
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> FilterAllBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> FilterCombatBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> FilterCityBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> FilterResearchBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> FilterDiplomacyBtn;

private:
	void BuildLayout();
	UButton* CreateFilterButton(const FString& Label, UHorizontalBox* Parent);
	UButton* CreateActionButton(const FString& Label, float Width = 140.f);

	/** Refresh the scroll box based on current filter. */
	void RefreshList();

	/** Get the color for a notification category. */
	static FLinearColor GetNotifCategoryColor(const FString& Category);

	/** Stored notification entries. */
	struct FNotifEntry
	{
		int32 Turn;
		FString Category;
		FString Message;
	};
	TArray<FNotifEntry> AllNotifications;
	FString ActiveFilter = TEXT("All");
};
