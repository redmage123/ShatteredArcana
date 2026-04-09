// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMVictoryScreenWidget.h -- Full-screen victory / defeat overlay.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMVictoryScreenWidget.generated.h"

class UBorder;
class UVerticalBox;
class UTextBlock;
class UButton;

struct FCoMVictoryScoreEntry;

/**
 * UCoMVictoryScreenWidget
 *
 * Full-screen overlay that displays victory or defeat information:
 * - Gold "VICTORY" or red "DEFEAT" title
 * - Victory type description
 * - Final score display
 * - Wizard rankings (sorted by score)
 * - "Main Menu" and "Continue Playing" buttons
 */
UCLASS()
class COMUI_API UCoMVictoryScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// -- Public API -----------------------------------------------------------

	/** Configure the widget for a victory screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	void ShowVictory(int32 WinnerWizardId, ECoMVictoryType VictoryType);

	/** Configure the widget for a defeat screen. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Victory")
	void ShowDefeat(int32 ConquerorWizardId);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// -- Build methods --------------------------------------------------------

	/** Build the entire UI layout in C++. */
	void BuildLayout();

	/** Populate the content for victory. */
	void PopulateVictory(int32 WinnerWizardId, ECoMVictoryType VictoryType);

	/** Populate the content for defeat. */
	void PopulateDefeat(int32 ConquerorWizardId);

	/** Add a text line to the content box. */
	void AddTextLine(const FString& Text, const FLinearColor& Color, int32 FontSize,
	                 bool bBold = false, ETextJustify::Type Justify = ETextJustify::Center);

	/** Add a horizontal separator line. */
	void AddSeparator();

	/** Add vertical spacing. */
	void AddSpacer(float Height);

	/** Build the score rankings table. */
	void BuildScoreTable();

	/** Get a human-readable description for a victory type. */
	static FString GetVictoryDescription(ECoMVictoryType Type);

	/** Get the wizard display name from game state. */
	FString GetWizardName(int32 WizardId) const;

	// -- Button callbacks -----------------------------------------------------

	UFUNCTION()
	void OnMainMenuClicked();

	UFUNCTION()
	void OnContinuePlayingClicked();

	// -- Widget references (created in C++) -----------------------------------

	UPROPERTY()
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY()
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY()
	TObjectPtr<UButton> ContinueButton;

	// -- State ----------------------------------------------------------------

	bool bIsVictory = true;
	int32 CachedWinnerWizardId = -1;
	ECoMVictoryType CachedVictoryType = ECoMVictoryType::None;
};
