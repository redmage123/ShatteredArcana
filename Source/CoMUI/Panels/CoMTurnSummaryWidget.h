// Copyright Mythforge Studios. All Rights Reserved.
// CoMTurnSummaryWidget.h -- Turn end summary report overlay.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMTurnSummaryWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UOverlay;

/**
 * UCoMTurnSummaryWidget
 *
 * Full-screen overlay showing a grouped summary of all events that occurred
 * during the turn. Events are grouped by category (Cities, Research, Military,
 * Diplomacy, Events) with colored headers and icons. Dismissed via "Continue"
 * button which advances the turn.
 */
UCLASS()
class COMUI_API UCoMTurnSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// -- Public API ------------------------------------------------------------

	/** Add an event to the summary under the given category. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TurnSummary")
	void AddEvent(const FString& Category, const FString& Description);

	/** Clear all events from the summary. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TurnSummary")
	void ClearEvents();

	/** Populate the summary for the given turn number and display it. */
	UFUNCTION(BlueprintCallable, Category = "CoM|TurnSummary")
	void ShowSummary(int32 TurnNumber);

protected:
	UFUNCTION()
	void OnContinueClicked();

	// -- Widget references -----------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> EventScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ContentBox;

private:
	/** Build the entire layout in C++. */
	void BuildLayout();

	/** Create a styled button with a gold border. */
	UButton* CreateActionButton(const FString& Label, float Width = 160.f);

	/** Append a category section to the scroll box. */
	void AppendCategorySection(const FString& CatName, const FLinearColor& Color, const TArray<FString>& Events);

	/** Get the color for a category name. */
	static FLinearColor GetCategoryColor(const FString& Category);

	/** Stored events keyed by category. */
	TMap<FString, TArray<FString>> PendingEvents;

	int32 CurrentTurnNumber = 0;
};
