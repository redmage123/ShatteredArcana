// Copyright Mythforge Studios. All Rights Reserved.
// CoMWizardCreationWidget.h -- Screen 1: Wizard portrait selection grid.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMWizardCreationWidget.generated.h"

class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UButton;
class UTextBlock;
class USizeBox;
class UOverlay;
class UImage;

/**
 * UCoMWizardCreationWidget
 *
 * Screen 1 of the two-screen wizard creation flow.
 * Shows a fullscreen grid of 14 wizard portraits (7x2).
 * Clicking a portrait selects that wizard and transitions to the config screen.
 * "Custom Wizard" goes to config with empty settings.
 */
UCLASS()
class COMUI_API UCoMWizardCreationWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	// -- Layout construction ------------------------------------------------------

	void BuildLayout();

	// -- Portrait callbacks -------------------------------------------------------

	UFUNCTION() void OnPortrait0();  UFUNCTION() void OnPortrait1();
	UFUNCTION() void OnPortrait2();  UFUNCTION() void OnPortrait3();
	UFUNCTION() void OnPortrait4();  UFUNCTION() void OnPortrait5();
	UFUNCTION() void OnPortrait6();  UFUNCTION() void OnPortrait7();
	UFUNCTION() void OnPortrait8();  UFUNCTION() void OnPortrait9();
	UFUNCTION() void OnPortrait10(); UFUNCTION() void OnPortrait11();
	UFUNCTION() void OnPortrait12(); UFUNCTION() void OnPortrait13();

	void SelectPortrait(int32 Index);
	void UpdatePortraitHighlights();

	// -- Navigation callbacks -----------------------------------------------------

	UFUNCTION() void OnCustomWizardClicked();
	UFUNCTION() void OnBackClicked();

	// -- Constants ----------------------------------------------------------------

	static constexpr int32 NumPortraits = 14;

	// -- State --------------------------------------------------------------------

	int32 SelectedPortraitIndex = -1;

	// -- Widget references --------------------------------------------------------

	UPROPERTY() UBorder* BackgroundBorder = nullptr;
	UPROPERTY() UButton* PortraitButtons[14] = {};
	UPROPERTY() UBorder* PortraitBorders[14] = {};
	UPROPERTY() UImage* PortraitImages[14] = {};
	UPROPERTY() UButton* CustomWizardButton = nullptr;
	UPROPERTY() UButton* BackButton = nullptr;
};
