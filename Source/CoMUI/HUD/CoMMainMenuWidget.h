// Copyright Mythforge Studios. All Rights Reserved.
// CoMMainMenuWidget.h -- Title screen / main menu widget.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMMainMenuWidget.generated.h"

class UBorder;
class UVerticalBox;
class UButton;

/**
 * UCoMMainMenuWidget
 *
 * Full-screen dark-fantasy title screen displayed on game launch.
 * Provides New Game, Load Game, Settings, Credits, and Quit buttons.
 * Built entirely in C++ (no Blueprint asset required for layout).
 */
UCLASS()
class COMUI_API UCoMMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	// -- Button callbacks --------------------------------------------------------

	UFUNCTION()
	void OnNewGameClicked();

	UFUNCTION()
	void OnLoadGameClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnCreditsClicked();

	UFUNCTION()
	void OnQuitClicked();

	// -- Layout construction -----------------------------------------------------

	/** Build the entire menu layout in C++. */
	void BuildMenuContent();

	/** Helper: create a styled menu button and add it to the content box. */
	UButton* CreateMenuButton(const FString& Label);

	/** Helper: show a temporary notification message on screen. */
	void ShowNotification(const FString& Message);

	// -- Widget references (created in C++) --------------------------------------

	UPROPERTY()
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY()
	TObjectPtr<UButton> NewGameButton;

	UPROPERTY()
	TObjectPtr<UButton> LoadGameButton;

	UPROPERTY()
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY()
	TObjectPtr<UButton> CreditsButton;

	UPROPERTY()
	TObjectPtr<UButton> QuitButton;
};
