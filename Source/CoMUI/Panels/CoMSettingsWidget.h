// Copyright Mythforge Studios. All Rights Reserved.
// CoMSettingsWidget.h -- Full-screen settings panel with Audio/Graphics/Controls tabs.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMSettingsWidget.generated.h"

class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UOverlay;
class UButton;
class UTextBlock;
class USlider;
class UCheckBox;
class UComboBoxString;
class USizeBox;
class UWidgetSwitcher;

/**
 * UCoMSettingsWidget
 *
 * Full-screen settings panel with three tabs: Audio, Graphics, Controls.
 * Dark fantasy theme matching the rest of the Shattered Arcana UI.
 * Built entirely in C++ using UMG components.
 */
UCLASS()
class COMUI_API UCoMSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// -- Public API ---------------------------------------------------------------

	/** Close the settings panel via the UI subsystem. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Settings")
	void CloseSettings();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// -- Layout construction ------------------------------------------------------

	void BuildSettingsLayout();
	void BuildAudioTab();
	void BuildGraphicsTab();
	void BuildControlsTab();

	// -- Tab switching -------------------------------------------------------------

	UFUNCTION()
	void OnAudioTabClicked();

	UFUNCTION()
	void OnGraphicsTabClicked();

	UFUNCTION()
	void OnControlsTabClicked();

	void SwitchTab(int32 TabIndex);
	void UpdateTabButtonStyles();

	// -- Audio callbacks ----------------------------------------------------------

	UFUNCTION()
	void OnMasterVolumeChanged(float Value);

	UFUNCTION()
	void OnMusicVolumeChanged(float Value);

	UFUNCTION()
	void OnSFXVolumeChanged(float Value);

	UFUNCTION()
	void OnAmbientVolumeChanged(float Value);

	// -- Graphics callbacks -------------------------------------------------------

	UFUNCTION()
	void OnResolutionSelected(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnWindowModeFullscreen();

	UFUNCTION()
	void OnWindowModeWindowed();

	UFUNCTION()
	void OnWindowModeBorderless();

	UFUNCTION()
	void OnQualityLow();

	UFUNCTION()
	void OnQualityMedium();

	UFUNCTION()
	void OnQualityHigh();

	UFUNCTION()
	void OnQualityUltra();

	UFUNCTION()
	void OnVSyncToggled(bool bIsChecked);

	UFUNCTION()
	void OnFPSLimit30();

	UFUNCTION()
	void OnFPSLimit60();

	UFUNCTION()
	void OnFPSLimit120();

	UFUNCTION()
	void OnFPSLimitUnlimited();

	// -- Bottom bar callbacks -----------------------------------------------------

	UFUNCTION()
	void OnApplyClicked();

	UFUNCTION()
	void OnBackClicked();

	// -- Controls tab callbacks ---------------------------------------------------

	UFUNCTION()
	void OnResetDefaultsClicked();

	/** Called when any rebind button is clicked; scans to find which one. */
	UFUNCTION()
	void OnRebindButtonClicked();

	void PopulateKeybindingsList();

	/** Start listening for a key press to rebind an action. */
	void StartRebind(int32 BindingIndex);

	// -- Helper factories ---------------------------------------------------------

	/** Create a styled button matching the dark fantasy theme. */
	UButton* CreateStyledButton(const FString& Label, float Width = 120.0f, float Height = 40.0f);

	/** Create a small option button (for graphics presets, window modes, etc). */
	UButton* CreateOptionButton(const FString& Label, float Width = 100.0f);

	/** Create a labeled slider row for audio settings. Returns the slider. */
	USlider* CreateVolumeSliderRow(UVerticalBox* Parent, const FString& Label, UTextBlock*& OutValueText);

	/** Add a section label to a vertical box. */
	void AddSectionLabel(UVerticalBox* Parent, const FString& Label);

	/** Add vertical spacing to a vertical box. */
	void AddSpacer(UVerticalBox* Parent, float Height);

	/** Load current settings from engine and audio subsystem. */
	void LoadCurrentSettings();

	/** Save audio volumes to config. */
	void SaveAudioSettings();

	/** Load audio volumes from config. */
	void LoadAudioSettings();

	// -- Widget references --------------------------------------------------------

	UPROPERTY()
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	// Tab buttons
	UPROPERTY()
	TObjectPtr<UButton> AudioTabButton;

	UPROPERTY()
	TObjectPtr<UButton> GraphicsTabButton;

	UPROPERTY()
	TObjectPtr<UButton> ControlsTabButton;

	// Tab content panels
	UPROPERTY()
	TObjectPtr<UVerticalBox> AudioPanel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> GraphicsPanel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ControlsPanel;

	// Audio sliders
	UPROPERTY()
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY()
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY()
	TObjectPtr<USlider> SFXVolumeSlider;

	UPROPERTY()
	TObjectPtr<USlider> AmbientVolumeSlider;

	// Audio value labels
	UPROPERTY()
	TObjectPtr<UTextBlock> MasterVolumeText;

	UPROPERTY()
	TObjectPtr<UTextBlock> MusicVolumeText;

	UPROPERTY()
	TObjectPtr<UTextBlock> SFXVolumeText;

	UPROPERTY()
	TObjectPtr<UTextBlock> AmbientVolumeText;

	// Graphics widgets
	UPROPERTY()
	TObjectPtr<UComboBoxString> ResolutionCombo;

	UPROPERTY()
	TObjectPtr<UButton> FullscreenButton;

	UPROPERTY()
	TObjectPtr<UButton> WindowedButton;

	UPROPERTY()
	TObjectPtr<UButton> BorderlessButton;

	UPROPERTY()
	TObjectPtr<UButton> QualityLowButton;

	UPROPERTY()
	TObjectPtr<UButton> QualityMediumButton;

	UPROPERTY()
	TObjectPtr<UButton> QualityHighButton;

	UPROPERTY()
	TObjectPtr<UButton> QualityUltraButton;

	UPROPERTY()
	TObjectPtr<UCheckBox> VSyncCheckBox;

	UPROPERTY()
	TObjectPtr<UButton> FPS30Button;

	UPROPERTY()
	TObjectPtr<UButton> FPS60Button;

	UPROPERTY()
	TObjectPtr<UButton> FPS120Button;

	UPROPERTY()
	TObjectPtr<UButton> FPSUnlimitedButton;

	// Controls tab
	UPROPERTY()
	TObjectPtr<UVerticalBox> KeybindingsListBox;

	UPROPERTY()
	TObjectPtr<UButton> ResetDefaultsButton;

	// Bottom bar
	UPROPERTY()
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY()
	TObjectPtr<UButton> BackButton;

	// -- State --------------------------------------------------------------------

	int32 ActiveTab = 0;

	// Pending graphics settings (applied on "Apply" click)
	FIntPoint PendingResolution = FIntPoint(1920, 1080);
	int32 PendingWindowMode = 0; // 0=Fullscreen, 1=Windowed, 2=Borderless
	int32 PendingQualityLevel = 2; // 0=Low, 1=Medium, 2=High, 3=Ultra
	bool bPendingVSync = true;
	float PendingFPSLimit = 60.0f;

	// Audio volumes (0.0 - 1.0)
	float CachedMasterVolume = 0.8f;
	float CachedMusicVolume = 0.8f;
	float CachedSFXVolume = 0.8f;
	float CachedAmbientVolume = 0.8f;

	// Keybinding data for the controls tab
	struct FKeyBindingEntry
	{
		FString ActionName;
		FString DisplayName;
		FKey CurrentKey;
		bool bShift = false;
		bool bCtrl = false;
		bool bAlt = false;
		TObjectPtr<UTextBlock> KeyLabel;
		TObjectPtr<UButton> RebindButton;
	};

	TArray<FKeyBindingEntry> KeyBindings;

	/** Index of the binding currently waiting for a key press, or -1. */
	int32 RebindingIndex = -1;
};
