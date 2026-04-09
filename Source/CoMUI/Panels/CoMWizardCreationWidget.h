// Copyright Mythforge Studios. All Rights Reserved.
// CoMWizardCreationWidget.h -- Wizard creation screen for new game setup.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoreTypes/CoMEnums.h"
#include "Framework/CoMGameInstance.h"
#include "CoMWizardCreationWidget.generated.h"

class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UButton;
class UTextBlock;
class UEditableTextBox;
class USizeBox;
class UOverlay;

/**
 * UCoMWizardCreationWidget
 *
 * Wizard creation screen — name, class, realm affinity, and difficulty.
 * Built entirely in C++ using UMG components. Dark fantasy aesthetic.
 */
UCLASS()
class COMUI_API UCoMWizardCreationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// -- Public API ---------------------------------------------------------------

	/** Construct the settings struct from current UI state. */
	UFUNCTION(BlueprintCallable, Category = "CoM|WizardCreation")
	FCoMNewGameSettings BuildSettings() const;

protected:
	virtual void NativeConstruct() override;

private:
	// -- Callbacks ----------------------------------------------------------------

	UFUNCTION()
	void OnClassWizardClicked();

	UFUNCTION()
	void OnClassPsykerClicked();

	UFUNCTION()
	void OnClassWarlockClicked();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void OnStartGameClicked();

	// Difficulty callbacks
	UFUNCTION()
	void OnDiffEasyClicked();

	UFUNCTION()
	void OnDiffNormalClicked();

	UFUNCTION()
	void OnDiffHardClicked();

	UFUNCTION()
	void OnDiffLunaticClicked();

	UFUNCTION()
	void OnDiffImpossibleClicked();

	// Realm callbacks
	UFUNCTION()
	void OnRealmLifeClicked();

	UFUNCTION()
	void OnRealmDeathClicked();

	UFUNCTION()
	void OnRealmChaosClicked();

	UFUNCTION()
	void OnRealmNatureClicked();

	UFUNCTION()
	void OnRealmSorceryClicked();

	UFUNCTION()
	void OnRealmArcaneClicked();

	UFUNCTION()
	void OnRealmBindingClicked();

	UFUNCTION()
	void OnRealmSpiritClicked();

	UFUNCTION()
	void OnRealmGlamourClicked();

	// -- Logic --------------------------------------------------------------------

	/** Highlights the selected class and shows/hides realm section. */
	void OnClassSelected(ECoMWizardClass Class);

	/** Toggle realm selection (max 3 selected). */
	void OnRealmToggled(ECoMSpellRealm Realm);

	/** Set the active difficulty level. */
	void OnDifficultySelected(int32 Level);

	// -- Layout construction ------------------------------------------------------

	void BuildLayout();

	/** Helper: create a styled section label. */
	UTextBlock* CreateSectionLabel(const FString& Text);

	/** Helper: create a styled button with given size. */
	UButton* CreateStyledButton(float Width, float Height);

	/** Helper: update class button visuals based on SelectedClass. */
	void UpdateClassButtonStyles();

	/** Helper: update difficulty button visuals. */
	void UpdateDifficultyButtonStyles();

	/** Helper: update realm button visuals. */
	void UpdateRealmButtonStyles();

	// -- State --------------------------------------------------------------------

	ECoMWizardClass SelectedClass = ECoMWizardClass::Wizard;
	int32 SelectedDifficulty = 1;
	TArray<ECoMSpellRealm> SelectedRealms;

	// -- Widget references --------------------------------------------------------

	UPROPERTY()
	UBorder* BackgroundBorder = nullptr;

	UPROPERTY()
	UVerticalBox* ContentBox = nullptr;

	UPROPERTY()
	UEditableTextBox* NameInputBox = nullptr;

	// Class buttons
	UPROPERTY()
	UButton* ClassWizardButton = nullptr;

	UPROPERTY()
	UButton* ClassPsykerButton = nullptr;

	UPROPERTY()
	UButton* ClassWarlockButton = nullptr;

	// Realm section (hidden for non-Wizard classes)
	UPROPERTY()
	UVerticalBox* RealmSectionBox = nullptr;

	// Realm buttons (9 total)
	UPROPERTY()
	UButton* RealmButtons[9] = {};

	UPROPERTY()
	UBorder* RealmButtonBorders[9] = {};

	// Difficulty buttons
	UPROPERTY()
	UButton* DifficultyButtons[5] = {};

	// Bottom buttons
	UPROPERTY()
	UButton* BackButton = nullptr;

	UPROPERTY()
	UButton* StartGameButton = nullptr;
};
