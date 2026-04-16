// Copyright Mythforge Studios. All Rights Reserved.
// CoMWizardConfigWidget.h -- Screen 2: Spell book allocation, retorts, and game start.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoreTypes/CoMEnums.h"
#include "Framework/CoMGameInstance.h"
#include "CoMWizardConfigWidget.generated.h"

class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UButton;
class UTextBlock;
class UEditableTextBox;
class USizeBox;
class UOverlay;
class UImage;
class UScrollBox;

/**
 * UCoMWizardConfigWidget
 *
 * Screen 2 of the two-screen wizard creation flow.
 * Handles spell book allocation (9 realms, max 13 per realm), retort selection,
 * difficulty choice, and game start. Shows selected wizard portrait in corner.
 */
UCLASS()
class COMUI_API UCoMWizardConfigWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the selected portrait index from Screen 1. -1 = custom wizard. */
	void SetPortraitIndex(int32 InPortraitIndex);

	/** Construct the settings struct from current UI state. */
	UFUNCTION(BlueprintCallable, Category = "CoM|WizardCreation")
	FCoMNewGameSettings BuildSettings() const;

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	// -- Layout construction ------------------------------------------------------

	void BuildLayout();

	/** Helper: create a styled section label. */
	UTextBlock* CreateSectionLabel(const FString& Text);

	/** Helper: create a styled button with Box draw type. */
	UButton* CreateStyledButton(float Width, float Height);

	/** Helper: create a small square button ([+] or [-]). */
	UButton* CreateSmallButton(const FString& Label, bool bEnabled);

	// -- Spell book add/remove callbacks (one per realm) --------------------------

	UFUNCTION() void OnAddBook0();   UFUNCTION() void OnRemoveBook0();
	UFUNCTION() void OnAddBook1();   UFUNCTION() void OnRemoveBook1();
	UFUNCTION() void OnAddBook2();   UFUNCTION() void OnRemoveBook2();
	UFUNCTION() void OnAddBook3();   UFUNCTION() void OnRemoveBook3();
	UFUNCTION() void OnAddBook4();   UFUNCTION() void OnRemoveBook4();
	UFUNCTION() void OnAddBook5();   UFUNCTION() void OnRemoveBook5();
	UFUNCTION() void OnAddBook6();   UFUNCTION() void OnRemoveBook6();
	UFUNCTION() void OnAddBook7();   UFUNCTION() void OnRemoveBook7();
	UFUNCTION() void OnAddBook8();   UFUNCTION() void OnRemoveBook8();

	// -- Retort callbacks (16 retorts) --------------------------------------------

	UFUNCTION() void OnRetort0();    UFUNCTION() void OnRetort1();
	UFUNCTION() void OnRetort2();    UFUNCTION() void OnRetort3();
	UFUNCTION() void OnRetort4();    UFUNCTION() void OnRetort5();
	UFUNCTION() void OnRetort6();    UFUNCTION() void OnRetort7();
	UFUNCTION() void OnRetort8();    UFUNCTION() void OnRetort9();
	UFUNCTION() void OnRetort10();   UFUNCTION() void OnRetort11();
	UFUNCTION() void OnRetort12();   UFUNCTION() void OnRetort13();
	UFUNCTION() void OnRetort14();   UFUNCTION() void OnRetort15();

	// -- Difficulty callbacks ------------------------------------------------------

	UFUNCTION() void OnDiffEasyClicked();
	UFUNCTION() void OnDiffNormalClicked();
	UFUNCTION() void OnDiffHardClicked();
	UFUNCTION() void OnDiffLunaticClicked();
	UFUNCTION() void OnDiffImpossibleClicked();

	// -- Navigation callbacks -----------------------------------------------------

	UFUNCTION() void OnBackClicked();
	UFUNCTION() void OnStartGameClicked();

	// -- Logic --------------------------------------------------------------------

	void OnAddBook(int32 RealmIndex);
	void OnRemoveBook(int32 RealmIndex);
	void OnRetortToggled(int32 RetortIndex);
	void OnDifficultySelected(int32 Level);

	// -- UI update helpers --------------------------------------------------------

	void UpdatePicksDisplay();
	void UpdateBookDisplay();
	void UpdateRetortButtons();
	void UpdateStartingSpells();
	void UpdateDifficultyButtonStyles();

	/** Returns the spell tier name for a given book count. */
	static FString GetTierName(int32 BookCount);

	/** Returns the number of picks currently spent. */
	int32 GetPicksSpent() const;

	/** Returns the number of picks remaining. */
	int32 GetPicksRemaining() const;

	// -- Constants ----------------------------------------------------------------

	static constexpr int32 TotalPicks = 11;
	static constexpr int32 MaxBooksPerRealm = 13;
	static constexpr int32 NumRealms = 9;
	static constexpr int32 NumRetorts = 16;
	static constexpr int32 NumPortraits = 14;

	// -- State --------------------------------------------------------------------

	int32 BookCounts[9] = {};
	TArray<int32> SelectedRetortIndices;
	int32 SelectedDifficulty = 1;
	int32 SelectedPortraitIndex = 0;

	// -- Widget references --------------------------------------------------------

	UPROPERTY() UBorder* BackgroundBorder = nullptr;
	UPROPERTY() UVerticalBox* ContentBox = nullptr;
	UPROPERTY() UEditableTextBox* NameInputBox = nullptr;

	// Top bar
	UPROPERTY() UImage* TopPortraitImage = nullptr;
	UPROPERTY() UTextBlock* PicksRemainingText = nullptr;

	// Spell book rows
	UPROPERTY() UButton* AddBookButtons[9] = {};
	UPROPERTY() UButton* RemoveBookButtons[9] = {};
	UPROPERTY() UTextBlock* BookCountTexts[9] = {};
	UPROPERTY() UTextBlock* TierTexts[9] = {};
	UBorder* BookSlotBorders[9][13] = {}; // No UPROPERTY — UHT doesn't support 2D arrays

	// Retorts
	UPROPERTY() UButton* RetortButtons[16] = {};
	UPROPERTY() UTextBlock* RetortLabels[16] = {};

	// Starting spells preview
	UPROPERTY() UTextBlock* StartingSpellsText = nullptr;

	// Difficulty buttons
	UPROPERTY() UButton* DifficultyButtons[5] = {};

	// Bottom buttons
	UPROPERTY() UButton* BackButton = nullptr;
	UPROPERTY() UButton* StartGameButton = nullptr;
};
