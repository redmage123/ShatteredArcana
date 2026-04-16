// Copyright Mythforge Studios. All Rights Reserved.
// CoMSettingsWidget.cpp -- Full-screen settings panel implementation.

#include "CoMSettingsWidget.h"

#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Spacer.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"

#include "GameFramework/GameUserSettings.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"

#include "CoMUISubsystem.h"
#include "CoMCore/Audio/CoMAudioSubsystem.h"

// =============================================================================
// Colour constants (matching dark fantasy theme)
// =============================================================================

namespace SettingsColours
{
	static const FLinearColor Background   = FLinearColor(0.039f, 0.039f, 0.102f, 1.0f); // #0a0a1a
	static const FLinearColor Gold         = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f); // #daa520
	static const FLinearColor White        = FLinearColor::White;
	static const FLinearColor LightGrey    = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
	static const FLinearColor Grey         = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	static const FLinearColor ButtonBg     = FLinearColor(0.086f, 0.129f, 0.243f, 1.0f); // #16213e
	static const FLinearColor ButtonHover  = FLinearColor(0.102f, 0.165f, 0.306f, 1.0f); // #1a2a4e
	static const FLinearColor ActiveTab    = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f); // gold
	static const FLinearColor InactiveTab  = FLinearColor(0.086f, 0.129f, 0.243f, 1.0f); // #16213e
	static const FLinearColor SliderBar    = FLinearColor(0.15f, 0.15f, 0.25f, 1.0f);
	static const FLinearColor SliderThumb  = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f); // gold
}

// =============================================================================
// Lifecycle
// =============================================================================

TSharedRef<SWidget> UCoMSettingsWidget::RebuildWidget()
{
	BuildSettingsLayout();
	return Super::RebuildWidget();
}

void UCoMSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	LoadCurrentSettings();
	SwitchTab(0); // Start on Audio tab
}

FReply UCoMSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// If we are waiting for a rebind key press, capture it.
	if (RebindingIndex >= 0 && RebindingIndex < KeyBindings.Num())
	{
		FKey PressedKey = InKeyEvent.GetKey();

		// Escape cancels the rebind.
		if (PressedKey == EKeys::Escape)
		{
			if (KeyBindings[RebindingIndex].KeyLabel)
			{
				FString KeyDisplay = KeyBindings[RebindingIndex].CurrentKey.GetDisplayName().ToString();
				if (KeyBindings[RebindingIndex].bShift) KeyDisplay = TEXT("Shift+") + KeyDisplay;
				if (KeyBindings[RebindingIndex].bCtrl) KeyDisplay = TEXT("Ctrl+") + KeyDisplay;
				if (KeyBindings[RebindingIndex].bAlt) KeyDisplay = TEXT("Alt+") + KeyDisplay;
				KeyBindings[RebindingIndex].KeyLabel->SetText(FText::FromString(KeyDisplay));
			}
			RebindingIndex = -1;
			return FReply::Handled();
		}

		// Update the binding.
		FKeyBindingEntry& Entry = KeyBindings[RebindingIndex];
		Entry.CurrentKey = PressedKey;
		Entry.bShift = InKeyEvent.IsShiftDown();
		Entry.bCtrl = InKeyEvent.IsControlDown();
		Entry.bAlt = InKeyEvent.IsAltDown();

		// Update the display label.
		if (Entry.KeyLabel)
		{
			FString KeyDisplay = PressedKey.GetDisplayName().ToString();
			if (Entry.bShift) KeyDisplay = TEXT("Shift+") + KeyDisplay;
			if (Entry.bCtrl) KeyDisplay = TEXT("Ctrl+") + KeyDisplay;
			if (Entry.bAlt) KeyDisplay = TEXT("Alt+") + KeyDisplay;
			Entry.KeyLabel->SetText(FText::FromString(KeyDisplay));
		}

		// Apply to input settings.
		UInputSettings* InputSettings = UInputSettings::GetInputSettings();
		if (InputSettings)
		{
			// Remove old mappings for this action.
			TArray<FInputActionKeyMapping> ActionMappings;
			InputSettings->GetActionMappingByName(FName(*Entry.ActionName), ActionMappings);

			for (const FInputActionKeyMapping& Mapping : ActionMappings)
			{
				InputSettings->RemoveActionMapping(Mapping);
			}

			// Add new mapping.
			FInputActionKeyMapping NewMapping;
			NewMapping.ActionName = FName(*Entry.ActionName);
			NewMapping.Key = Entry.CurrentKey;
			NewMapping.bShift = Entry.bShift;
			NewMapping.bCtrl = Entry.bCtrl;
			NewMapping.bAlt = Entry.bAlt;
			InputSettings->AddActionMapping(NewMapping);
			InputSettings->SaveKeyMappings();
		}

		RebindingIndex = -1;
		return FReply::Handled();
	}

	// Escape closes the settings panel.
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseSettings();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// =============================================================================
// Public API
// =============================================================================

void UCoMSettingsWidget::CloseSettings()
{
	SaveAudioSettings();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->HideSettings();
			return;
		}
	}

	// Fallback: just remove self.
	RemoveFromParent();
}

// =============================================================================
// Layout construction
// =============================================================================

void UCoMSettingsWidget::BuildSettingsLayout()
{
	// -- Full-screen dark background ------------------------------------------

	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(SettingsColours::Background);
	BackgroundBorder->SetPadding(FMargin(0.0f));

	// Set the background as the root widget BEFORE adding children.
	WidgetTree->RootWidget = BackgroundBorder;

	// -- Center-aligned content via overlay -----------------------------------

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(RootOverlay);

	// Content area: 600px wide, centered
	USizeBox* ContentSizeBox = WidgetTree->ConstructWidget<USizeBox>();
	ContentSizeBox->SetWidthOverride(600.0f);

	UOverlaySlot* ContentSlot = RootOverlay->AddChildToOverlay(ContentSizeBox);
	if (ContentSlot)
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	RootBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ContentSizeBox->AddChild(RootBox);

	// -- Title ----------------------------------------------------------------

	{
		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>();
		TitleText->SetText(FText::FromString(TEXT("SETTINGS")));
		TitleText->SetColorAndOpacity(FSlateColor(SettingsColours::Gold));
		TitleText->SetJustification(ETextJustify::Center);

		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 36;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		TitleText->SetFont(TitleFont);

		UVerticalBoxSlot* SlotRef = RootBox->AddChildToVerticalBox(TitleText);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Center);
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		}
	}

	// -- Gold separator -------------------------------------------------------

	{
		UImage* Separator = WidgetTree->ConstructWidget<UImage>();
		Separator->SetColorAndOpacity(SettingsColours::Gold);
		Separator->SetDesiredSizeOverride(FVector2D(500.0f, 2.0f));

		UVerticalBoxSlot* SlotRef = RootBox->AddChildToVerticalBox(Separator);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Center);
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		}
	}

	// -- Tab bar --------------------------------------------------------------

	{
		UHorizontalBox* TabBar = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Audio tab
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(180.0f);
			SB->SetHeightOverride(44.0f);

			AudioTabButton = WidgetTree->ConstructWidget<UButton>();
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(FText::FromString(TEXT("Audio")));
			Label->SetColorAndOpacity(FSlateColor(SettingsColours::White));
			Label->SetJustification(ETextJustify::Center);
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = 16;
			Font.TypefaceFontName = FName(TEXT("Bold"));
			Label->SetFont(Font);
			AudioTabButton->AddChild(Label);
			SB->AddChild(AudioTabButton);
			AudioTabButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnAudioTabClicked);

			UHorizontalBoxSlot* HSlot = TabBar->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
				HSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		// Graphics tab
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(180.0f);
			SB->SetHeightOverride(44.0f);

			GraphicsTabButton = WidgetTree->ConstructWidget<UButton>();
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(FText::FromString(TEXT("Graphics")));
			Label->SetColorAndOpacity(FSlateColor(SettingsColours::White));
			Label->SetJustification(ETextJustify::Center);
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = 16;
			Font.TypefaceFontName = FName(TEXT("Bold"));
			Label->SetFont(Font);
			GraphicsTabButton->AddChild(Label);
			SB->AddChild(GraphicsTabButton);
			GraphicsTabButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnGraphicsTabClicked);

			UHorizontalBoxSlot* HSlot = TabBar->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
				HSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		// Controls tab
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(180.0f);
			SB->SetHeightOverride(44.0f);

			ControlsTabButton = WidgetTree->ConstructWidget<UButton>();
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(FText::FromString(TEXT("Controls")));
			Label->SetColorAndOpacity(FSlateColor(SettingsColours::White));
			Label->SetJustification(ETextJustify::Center);
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = 16;
			Font.TypefaceFontName = FName(TEXT("Bold"));
			Label->SetFont(Font);
			ControlsTabButton->AddChild(Label);
			SB->AddChild(ControlsTabButton);
			ControlsTabButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnControlsTabClicked);

			UHorizontalBoxSlot* HSlot = TabBar->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
				HSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		UVerticalBoxSlot* TabBarSlot = RootBox->AddChildToVerticalBox(TabBar);
		if (TabBarSlot)
		{
			TabBarSlot->SetHorizontalAlignment(HAlign_Center);
			TabBarSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		}
	}

	// -- Tab content panels ---------------------------------------------------

	// Audio panel
	AudioPanel = WidgetTree->ConstructWidget<UVerticalBox>();
	BuildAudioTab();
	{
		UVerticalBoxSlot* SlotRef = RootBox->AddChildToVerticalBox(AudioPanel);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Fill);
			SlotRef->SetPadding(FMargin(20.0f, 0.0f));
		}
	}

	// Graphics panel
	GraphicsPanel = WidgetTree->ConstructWidget<UVerticalBox>();
	BuildGraphicsTab();
	{
		UVerticalBoxSlot* SlotRef = RootBox->AddChildToVerticalBox(GraphicsPanel);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Fill);
			SlotRef->SetPadding(FMargin(20.0f, 0.0f));
		}
	}

	// Controls panel
	ControlsPanel = WidgetTree->ConstructWidget<UVerticalBox>();
	BuildControlsTab();
	{
		UVerticalBoxSlot* SlotRef = RootBox->AddChildToVerticalBox(ControlsPanel);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Fill);
			SlotRef->SetPadding(FMargin(20.0f, 0.0f));
		}
	}

	// -- Spacer before bottom bar ---------------------------------------------

	AddSpacer(RootBox, 24.0f);

	// -- Gold separator -------------------------------------------------------

	{
		UImage* Separator = WidgetTree->ConstructWidget<UImage>();
		Separator->SetColorAndOpacity(SettingsColours::Gold);
		Separator->SetDesiredSizeOverride(FVector2D(500.0f, 2.0f));

		UVerticalBoxSlot* SlotRef = RootBox->AddChildToVerticalBox(Separator);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Center);
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		}
	}

	// -- Bottom bar: Apply + Back ---------------------------------------------

	{
		UHorizontalBox* BottomBar = WidgetTree->ConstructWidget<UHorizontalBox>();

		ApplyButton = CreateStyledButton(TEXT("Apply"), 140.0f, 44.0f);
		ApplyButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnApplyClicked);

		BackButton = CreateStyledButton(TEXT("Back"), 140.0f, 44.0f);
		BackButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnBackClicked);

		// Apply button -- gold-tinted
		{
			FButtonStyle ApplyStyle = ApplyButton->GetStyle();
			ApplyStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
			ApplyStyle.Normal.TintColor = FSlateColor(SettingsColours::Gold);
			ApplyStyle.Normal.OutlineSettings.Color = FSlateColor(SettingsColours::Gold);
			ApplyStyle.Normal.OutlineSettings.Width = 1.0f;
			ApplyStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
			ApplyStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.95f, 0.75f, 0.2f, 1.0f));
			ApplyStyle.Hovered.OutlineSettings.Color = FSlateColor(SettingsColours::Gold);
			ApplyStyle.Hovered.OutlineSettings.Width = 1.0f;
			ApplyStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
			ApplyStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.7f, 0.5f, 0.1f, 1.0f));
			ApplyStyle.Pressed.OutlineSettings.Color = FSlateColor(SettingsColours::Gold);
			ApplyStyle.Pressed.OutlineSettings.Width = 1.0f;
			ApplyButton->SetStyle(ApplyStyle);
		}

		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(140.0f);
			SB->SetHeightOverride(44.0f);
			SB->AddChild(ApplyButton);
			UHorizontalBoxSlot* HSlot = BottomBar->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(8.0f, 0.0f));
			}
		}

		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(140.0f);
			SB->SetHeightOverride(44.0f);
			SB->AddChild(BackButton);
			UHorizontalBoxSlot* HSlot = BottomBar->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(8.0f, 0.0f));
			}
		}

		UVerticalBoxSlot* BottomSlot = RootBox->AddChildToVerticalBox(BottomBar);
		if (BottomSlot)
		{
			BottomSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}
}

// =============================================================================
// Audio Tab
// =============================================================================

void UCoMSettingsWidget::BuildAudioTab()
{
	AddSectionLabel(AudioPanel, TEXT("Volume"));
	AddSpacer(AudioPanel, 8.0f);

	MasterVolumeSlider = CreateVolumeSliderRow(AudioPanel, TEXT("Master Volume"), MasterVolumeText);
	MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UCoMSettingsWidget::OnMasterVolumeChanged);

	AddSpacer(AudioPanel, 8.0f);

	MusicVolumeSlider = CreateVolumeSliderRow(AudioPanel, TEXT("Music Volume"), MusicVolumeText);
	MusicVolumeSlider->OnValueChanged.AddDynamic(this, &UCoMSettingsWidget::OnMusicVolumeChanged);

	AddSpacer(AudioPanel, 8.0f);

	SFXVolumeSlider = CreateVolumeSliderRow(AudioPanel, TEXT("SFX Volume"), SFXVolumeText);
	SFXVolumeSlider->OnValueChanged.AddDynamic(this, &UCoMSettingsWidget::OnSFXVolumeChanged);

	AddSpacer(AudioPanel, 8.0f);

	AmbientVolumeSlider = CreateVolumeSliderRow(AudioPanel, TEXT("Ambient Volume"), AmbientVolumeText);
	AmbientVolumeSlider->OnValueChanged.AddDynamic(this, &UCoMSettingsWidget::OnAmbientVolumeChanged);
}

// =============================================================================
// Graphics Tab
// =============================================================================

void UCoMSettingsWidget::BuildGraphicsTab()
{
	// -- Resolution -----------------------------------------------------------

	AddSectionLabel(GraphicsPanel, TEXT("Resolution"));
	AddSpacer(GraphicsPanel, 4.0f);

	ResolutionCombo = WidgetTree->ConstructWidget<UComboBoxString>();
	ResolutionCombo->AddOption(TEXT("1280x720"));
	ResolutionCombo->AddOption(TEXT("1920x1080"));
	ResolutionCombo->AddOption(TEXT("2560x1440"));
	ResolutionCombo->AddOption(TEXT("3840x2160"));
	ResolutionCombo->SetSelectedOption(TEXT("1920x1080"));
	ResolutionCombo->OnSelectionChanged.AddDynamic(this, &UCoMSettingsWidget::OnResolutionSelected);

	{
		UVerticalBoxSlot* SlotRef = GraphicsPanel->AddChildToVerticalBox(ResolutionCombo);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Fill);
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
	}

	// -- Window Mode ----------------------------------------------------------

	AddSectionLabel(GraphicsPanel, TEXT("Window Mode"));
	AddSpacer(GraphicsPanel, 4.0f);

	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		FullscreenButton = CreateOptionButton(TEXT("Fullscreen"), 130.0f);
		FullscreenButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnWindowModeFullscreen);

		WindowedButton = CreateOptionButton(TEXT("Windowed"), 130.0f);
		WindowedButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnWindowModeWindowed);

		BorderlessButton = CreateOptionButton(TEXT("Borderless"), 130.0f);
		BorderlessButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnWindowModeBorderless);

		auto AddToRow = [&](UButton* Btn)
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(130.0f);
			SB->SetHeightOverride(36.0f);
			SB->AddChild(Btn);
			UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
			}
		};

		AddToRow(FullscreenButton);
		AddToRow(WindowedButton);
		AddToRow(BorderlessButton);

		UVerticalBoxSlot* SlotRef = GraphicsPanel->AddChildToVerticalBox(Row);
		if (SlotRef)
		{
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
	}

	// -- Quality Preset -------------------------------------------------------

	AddSectionLabel(GraphicsPanel, TEXT("Quality Preset"));
	AddSpacer(GraphicsPanel, 4.0f);

	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		QualityLowButton = CreateOptionButton(TEXT("Low"));
		QualityLowButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnQualityLow);

		QualityMediumButton = CreateOptionButton(TEXT("Medium"));
		QualityMediumButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnQualityMedium);

		QualityHighButton = CreateOptionButton(TEXT("High"));
		QualityHighButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnQualityHigh);

		QualityUltraButton = CreateOptionButton(TEXT("Ultra"));
		QualityUltraButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnQualityUltra);

		auto AddToRow = [&](UButton* Btn)
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(100.0f);
			SB->SetHeightOverride(36.0f);
			SB->AddChild(Btn);
			UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
			}
		};

		AddToRow(QualityLowButton);
		AddToRow(QualityMediumButton);
		AddToRow(QualityHighButton);
		AddToRow(QualityUltraButton);

		UVerticalBoxSlot* SlotRef = GraphicsPanel->AddChildToVerticalBox(Row);
		if (SlotRef)
		{
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
	}

	// -- VSync ----------------------------------------------------------------

	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		UTextBlock* VSyncLabel = WidgetTree->ConstructWidget<UTextBlock>();
		VSyncLabel->SetText(FText::FromString(TEXT("VSync")));
		VSyncLabel->SetColorAndOpacity(FSlateColor(SettingsColours::LightGrey));
		FSlateFontInfo Font = VSyncLabel->GetFont();
		Font.Size = 14;
		VSyncLabel->SetFont(Font);

		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(VSyncLabel);
		if (LabelSlot)
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
		}

		VSyncCheckBox = WidgetTree->ConstructWidget<UCheckBox>();
		VSyncCheckBox->SetIsChecked(true);
		VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &UCoMSettingsWidget::OnVSyncToggled);

		UHorizontalBoxSlot* CheckSlot = Row->AddChildToHorizontalBox(VSyncCheckBox);
		if (CheckSlot)
		{
			CheckSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* SlotRef = GraphicsPanel->AddChildToVerticalBox(Row);
		if (SlotRef)
		{
			SlotRef->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
		}
	}

	// -- FPS Limit ------------------------------------------------------------

	AddSectionLabel(GraphicsPanel, TEXT("FPS Limit"));
	AddSpacer(GraphicsPanel, 4.0f);

	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		FPS30Button = CreateOptionButton(TEXT("30"));
		FPS30Button->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnFPSLimit30);

		FPS60Button = CreateOptionButton(TEXT("60"));
		FPS60Button->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnFPSLimit60);

		FPS120Button = CreateOptionButton(TEXT("120"));
		FPS120Button->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnFPSLimit120);

		FPSUnlimitedButton = CreateOptionButton(TEXT("Unlimited"), 110.0f);
		FPSUnlimitedButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnFPSLimitUnlimited);

		auto AddToRow = [&](UButton* Btn, float Width = 100.0f)
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(Width);
			SB->SetHeightOverride(36.0f);
			SB->AddChild(Btn);
			UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
			}
		};

		AddToRow(FPS30Button);
		AddToRow(FPS60Button);
		AddToRow(FPS120Button);
		AddToRow(FPSUnlimitedButton, 110.0f);

		UVerticalBoxSlot* SlotRef = GraphicsPanel->AddChildToVerticalBox(Row);
		if (SlotRef)
		{
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	}
}

// =============================================================================
// Controls Tab
// =============================================================================

void UCoMSettingsWidget::BuildControlsTab()
{
	AddSectionLabel(ControlsPanel, TEXT("Key Bindings"));
	AddSpacer(ControlsPanel, 8.0f);

	// Header row
	{
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		auto AddHeader = [&](const FString& Text, float Width)
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(Width);

			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(FText::FromString(Text));
			Label->SetColorAndOpacity(FSlateColor(SettingsColours::Gold));
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = 13;
			Font.TypefaceFontName = FName(TEXT("Bold"));
			Label->SetFont(Font);

			SB->AddChild(Label);
			UHorizontalBoxSlot* HSlot = HeaderRow->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
				HSlot->SetVerticalAlignment(VAlign_Center);
			}
		};

		AddHeader(TEXT("Action"), 200.0f);
		AddHeader(TEXT("Key"), 150.0f);
		AddHeader(TEXT(""), 100.0f); // Rebind column

		UVerticalBoxSlot* SlotRef = ControlsPanel->AddChildToVerticalBox(HeaderRow);
		if (SlotRef)
		{
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		}
	}

	// Separator
	{
		UImage* Sep = WidgetTree->ConstructWidget<UImage>();
		Sep->SetColorAndOpacity(SettingsColours::Grey);
		Sep->SetDesiredSizeOverride(FVector2D(460.0f, 1.0f));

		UVerticalBoxSlot* SlotRef = ControlsPanel->AddChildToVerticalBox(Sep);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Left);
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		}
	}

	// Keybindings list box (populated dynamically)
	KeybindingsListBox = WidgetTree->ConstructWidget<UVerticalBox>();
	{
		UVerticalBoxSlot* SlotRef = ControlsPanel->AddChildToVerticalBox(KeybindingsListBox);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Fill);
		}
	}

	PopulateKeybindingsList();

	AddSpacer(ControlsPanel, 12.0f);

	// Reset Defaults button
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		ResetDefaultsButton = CreateStyledButton(TEXT("Reset Defaults"), 160.0f, 36.0f);
		ResetDefaultsButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnResetDefaultsClicked);

		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(160.0f);
		SB->SetHeightOverride(36.0f);
		SB->AddChild(ResetDefaultsButton);

		UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SB);
		if (HSlot)
		{
			HSlot->SetPadding(FMargin(0.0f));
		}

		UVerticalBoxSlot* SlotRef = ControlsPanel->AddChildToVerticalBox(Row);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Left);
		}
	}
}

// =============================================================================
// Tab switching
// =============================================================================

void UCoMSettingsWidget::OnAudioTabClicked()
{
	SwitchTab(0);
}

void UCoMSettingsWidget::OnGraphicsTabClicked()
{
	SwitchTab(1);
}

void UCoMSettingsWidget::OnControlsTabClicked()
{
	SwitchTab(2);
}

void UCoMSettingsWidget::SwitchTab(int32 TabIndex)
{
	ActiveTab = TabIndex;

	if (AudioPanel) AudioPanel->SetVisibility(TabIndex == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (GraphicsPanel) GraphicsPanel->SetVisibility(TabIndex == 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ControlsPanel) ControlsPanel->SetVisibility(TabIndex == 2 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	UpdateTabButtonStyles();
}

void UCoMSettingsWidget::UpdateTabButtonStyles()
{
	auto StyleTabButton = [](UButton* Btn, bool bActive)
	{
		if (!Btn) return;

		FButtonStyle Style = Btn->GetStyle();

		FLinearColor BgColor = bActive ? SettingsColours::ActiveTab : SettingsColours::InactiveTab;
		FLinearColor HoverColor = bActive
			? FLinearColor(0.95f, 0.75f, 0.2f, 1.0f)
			: SettingsColours::ButtonHover;

		Style.Normal.DrawAs = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(BgColor);
		Style.Normal.OutlineSettings.Color = FSlateColor(SettingsColours::Gold);
		Style.Normal.OutlineSettings.Width = 1.0f;

		Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(HoverColor);
		Style.Hovered.OutlineSettings.Color = FSlateColor(SettingsColours::Gold);
		Style.Hovered.OutlineSettings.Width = 1.0f;

		Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(BgColor);
		Style.Pressed.OutlineSettings.Color = FSlateColor(SettingsColours::Gold);
		Style.Pressed.OutlineSettings.Width = 1.0f;

		Btn->SetStyle(Style);
	};

	StyleTabButton(AudioTabButton, ActiveTab == 0);
	StyleTabButton(GraphicsTabButton, ActiveTab == 1);
	StyleTabButton(ControlsTabButton, ActiveTab == 2);
}

// =============================================================================
// Audio callbacks
// =============================================================================

void UCoMSettingsWidget::OnMasterVolumeChanged(float Value)
{
	CachedMasterVolume = Value;
	if (MasterVolumeText)
	{
		MasterVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
	}

	// Live preview via audio subsystem.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMAudioSubsystem* Audio = GI->GetSubsystem<UCoMAudioSubsystem>())
		{
			Audio->SetMasterVolume(Value);
		}
	}
}

void UCoMSettingsWidget::OnMusicVolumeChanged(float Value)
{
	CachedMusicVolume = Value;
	if (MusicVolumeText)
	{
		MusicVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMAudioSubsystem* Audio = GI->GetSubsystem<UCoMAudioSubsystem>())
		{
			Audio->SetMusicVolume(Value);
		}
	}
}

void UCoMSettingsWidget::OnSFXVolumeChanged(float Value)
{
	CachedSFXVolume = Value;
	if (SFXVolumeText)
	{
		SFXVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMAudioSubsystem* Audio = GI->GetSubsystem<UCoMAudioSubsystem>())
		{
			Audio->SetSFXVolume(Value);
		}
	}
}

void UCoMSettingsWidget::OnAmbientVolumeChanged(float Value)
{
	CachedAmbientVolume = Value;
	if (AmbientVolumeText)
	{
		AmbientVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
	}

	// Ambient volume is stored as config; no dedicated subsystem setter yet.
	// When CoMAudioSubsystem gets SetAmbientVolume(), wire it here.
}

// =============================================================================
// Graphics callbacks
// =============================================================================

void UCoMSettingsWidget::OnResolutionSelected(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectedItem == TEXT("1280x720"))        PendingResolution = FIntPoint(1280, 720);
	else if (SelectedItem == TEXT("1920x1080"))  PendingResolution = FIntPoint(1920, 1080);
	else if (SelectedItem == TEXT("2560x1440"))  PendingResolution = FIntPoint(2560, 1440);
	else if (SelectedItem == TEXT("3840x2160"))  PendingResolution = FIntPoint(3840, 2160);
}

void UCoMSettingsWidget::OnWindowModeFullscreen()
{
	PendingWindowMode = 0;
}

void UCoMSettingsWidget::OnWindowModeWindowed()
{
	PendingWindowMode = 1;
}

void UCoMSettingsWidget::OnWindowModeBorderless()
{
	PendingWindowMode = 2;
}

void UCoMSettingsWidget::OnQualityLow()
{
	PendingQualityLevel = 0;
}

void UCoMSettingsWidget::OnQualityMedium()
{
	PendingQualityLevel = 1;
}

void UCoMSettingsWidget::OnQualityHigh()
{
	PendingQualityLevel = 2;
}

void UCoMSettingsWidget::OnQualityUltra()
{
	PendingQualityLevel = 3;
}

void UCoMSettingsWidget::OnVSyncToggled(bool bIsChecked)
{
	bPendingVSync = bIsChecked;
}

void UCoMSettingsWidget::OnFPSLimit30()
{
	PendingFPSLimit = 30.0f;
}

void UCoMSettingsWidget::OnFPSLimit60()
{
	PendingFPSLimit = 60.0f;
}

void UCoMSettingsWidget::OnFPSLimit120()
{
	PendingFPSLimit = 120.0f;
}

void UCoMSettingsWidget::OnFPSLimitUnlimited()
{
	PendingFPSLimit = 0.0f; // 0 = unlimited
}

// =============================================================================
// Apply / Back
// =============================================================================

void UCoMSettingsWidget::OnApplyClicked()
{
	// Apply graphics settings via UGameUserSettings.
	if (GEngine)
	{
		UGameUserSettings* Settings = GEngine->GetGameUserSettings();
		if (Settings)
		{
			Settings->SetScreenResolution(PendingResolution);

			EWindowMode::Type WindowMode;
			switch (PendingWindowMode)
			{
			case 0:  WindowMode = EWindowMode::Fullscreen; break;
			case 1:  WindowMode = EWindowMode::Windowed; break;
			case 2:  WindowMode = EWindowMode::WindowedFullscreen; break;
			default: WindowMode = EWindowMode::Fullscreen; break;
			}
			Settings->SetFullscreenMode(WindowMode);

			Settings->SetOverallScalabilityLevel(PendingQualityLevel);
			Settings->SetVSyncEnabled(bPendingVSync);
			Settings->SetFrameRateLimit(PendingFPSLimit);

			Settings->ApplySettings(false);
			Settings->SaveSettings();
		}
	}

	// Save audio volumes.
	SaveAudioSettings();

	// Visual feedback.
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Settings applied."));
	}
}

void UCoMSettingsWidget::OnBackClicked()
{
	CloseSettings();
}

// =============================================================================
// Controls: keybindings
// =============================================================================

void UCoMSettingsWidget::PopulateKeybindingsList()
{
	KeyBindings.Empty();

	// Define the actions we want to show, with display-friendly names.
	struct FActionDef
	{
		FString ActionName;
		FString DisplayName;
	};

	const TArray<FActionDef> ActionDefs = {
		{ TEXT("EndTurn"),          TEXT("End Turn") },
		{ TEXT("CameraZoomIn"),    TEXT("Camera Zoom In") },
		{ TEXT("CameraZoomOut"),   TEXT("Camera Zoom Out") },
		{ TEXT("OpenMenu"),        TEXT("Open Menu") },
		{ TEXT("Select"),          TEXT("Select") },
		{ TEXT("ContextMenu"),     TEXT("Context Menu") },
		{ TEXT("QuickSave"),       TEXT("Quick Save") },
		{ TEXT("QuickLoad"),       TEXT("Quick Load") },
		{ TEXT("ToggleMinimap"),   TEXT("Toggle Minimap") },
		{ TEXT("ToggleSpellBook"), TEXT("Toggle Spell Book") },
		{ TEXT("ToggleDiplomacy"), TEXT("Toggle Diplomacy") },
		{ TEXT("ToggleCityList"),  TEXT("Toggle City List") },
		{ TEXT("ToggleResearch"),  TEXT("Toggle Research") },
	};

	UInputSettings* InputSettings = UInputSettings::GetInputSettings();

	for (const FActionDef& Def : ActionDefs)
	{
		FKeyBindingEntry Entry;
		Entry.ActionName = Def.ActionName;
		Entry.DisplayName = Def.DisplayName;
		Entry.CurrentKey = EKeys::Invalid;

		// Read the first matching key from input settings.
		if (InputSettings)
		{
			TArray<FInputActionKeyMapping> Mappings;
			InputSettings->GetActionMappingByName(FName(*Def.ActionName), Mappings);
			if (Mappings.Num() > 0)
			{
				Entry.CurrentKey = Mappings[0].Key;
				Entry.bShift = Mappings[0].bShift;
				Entry.bCtrl = Mappings[0].bCtrl;
				Entry.bAlt = Mappings[0].bAlt;
			}
		}

		KeyBindings.Add(Entry);
	}

	// Build UI rows for each binding.
	if (!KeybindingsListBox) return;

	for (int32 i = 0; i < KeyBindings.Num(); ++i)
	{
		FKeyBindingEntry& Entry = KeyBindings[i];

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Action name
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(200.0f);

			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(FText::FromString(Entry.DisplayName));
			Label->SetColorAndOpacity(FSlateColor(SettingsColours::LightGrey));
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = 13;
			Label->SetFont(Font);

			SB->AddChild(Label);
			UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
				HSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		// Current key
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(150.0f);

			Entry.KeyLabel = WidgetTree->ConstructWidget<UTextBlock>();
			FString KeyDisplay = Entry.CurrentKey.IsValid()
				? Entry.CurrentKey.GetDisplayName().ToString()
				: TEXT("Unbound");
			if (Entry.bShift) KeyDisplay = TEXT("Shift+") + KeyDisplay;
			if (Entry.bCtrl) KeyDisplay = TEXT("Ctrl+") + KeyDisplay;
			if (Entry.bAlt) KeyDisplay = TEXT("Alt+") + KeyDisplay;

			Entry.KeyLabel->SetText(FText::FromString(KeyDisplay));
			Entry.KeyLabel->SetColorAndOpacity(FSlateColor(SettingsColours::Gold));
			FSlateFontInfo Font = Entry.KeyLabel->GetFont();
			Font.Size = 13;
			Entry.KeyLabel->SetFont(Font);

			SB->AddChild(Entry.KeyLabel);
			UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
				HSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		// Rebind button
		{
			Entry.RebindButton = CreateOptionButton(TEXT("Rebind"), 80.0f);

			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
			SB->SetWidthOverride(80.0f);
			SB->SetHeightOverride(30.0f);
			SB->AddChild(Entry.RebindButton);

			UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SB);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f));
				HSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		UVerticalBoxSlot* RowSlot = KeybindingsListBox->AddChildToVerticalBox(Row);
		if (RowSlot)
		{
			RowSlot->SetPadding(FMargin(0.0f, 2.0f));
		}
	}

	// Bind all rebind buttons to a single UFUNCTION handler that scans
	// for the hovered button to determine which index was clicked.
	for (int32 i = 0; i < KeyBindings.Num(); ++i)
	{
		if (KeyBindings[i].RebindButton)
		{
			KeyBindings[i].RebindButton->OnClicked.AddDynamic(this, &UCoMSettingsWidget::OnRebindButtonClicked);
		}
	}
}

void UCoMSettingsWidget::OnRebindButtonClicked()
{
	// Determine which rebind button was clicked by checking IsHovered().
	// Only the clicked button will be hovered at the time OnClicked fires.
	for (int32 i = 0; i < KeyBindings.Num(); ++i)
	{
		if (KeyBindings[i].RebindButton && KeyBindings[i].RebindButton->IsHovered())
		{
			StartRebind(i);
			return;
		}
	}
}

void UCoMSettingsWidget::StartRebind(int32 BindingIndex)
{
	if (BindingIndex < 0 || BindingIndex >= KeyBindings.Num())
	{
		return;
	}

	RebindingIndex = BindingIndex;

	// Update the label to show "Press a key..."
	if (KeyBindings[BindingIndex].KeyLabel)
	{
		KeyBindings[BindingIndex].KeyLabel->SetText(FText::FromString(TEXT("Press a key...")));
		KeyBindings[BindingIndex].KeyLabel->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f, 1.0f)));
	}

	// Ensure we have keyboard focus to capture the next key.
	SetKeyboardFocus();
}

void UCoMSettingsWidget::OnResetDefaultsClicked()
{
	// Reload default input settings.
	UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (InputSettings)
	{
		// Remove all current action mappings.
		for (const FKeyBindingEntry& Entry : KeyBindings)
		{
			TArray<FInputActionKeyMapping> Mappings;
			InputSettings->GetActionMappingByName(FName(*Entry.ActionName), Mappings);
			for (const FInputActionKeyMapping& Mapping : Mappings)
			{
				InputSettings->RemoveActionMapping(Mapping);
			}
		}

		// Re-add defaults from DefaultInput.ini definitions.
		struct FDefaultBinding
		{
			FString Action;
			FKey Key;
			bool bShift;
		};

		const TArray<FDefaultBinding> Defaults = {
			{ TEXT("EndTurn"),          EKeys::Enter,           false },
			{ TEXT("EndTurn"),          EKeys::SpaceBar,        false },
			{ TEXT("CameraZoomIn"),     EKeys::MouseScrollUp,   false },
			{ TEXT("CameraZoomOut"),    EKeys::MouseScrollDown,  false },
			{ TEXT("CameraZoomIn"),     EKeys::PageUp,          false },
			{ TEXT("CameraZoomOut"),    EKeys::PageDown,         false },
			{ TEXT("OpenMenu"),         EKeys::Escape,          false },
			{ TEXT("OpenMenu"),         EKeys::F10,             false },
			{ TEXT("Select"),           EKeys::LeftMouseButton, false },
			{ TEXT("ContextMenu"),      EKeys::RightMouseButton,false },
			{ TEXT("QuickSave"),        EKeys::F5,              false },
			{ TEXT("QuickLoad"),        EKeys::F9,              false },
			{ TEXT("ToggleMinimap"),    EKeys::M,               false },
			{ TEXT("ToggleSpellBook"),  EKeys::S,               true  },
			{ TEXT("ToggleDiplomacy"),  EKeys::D,               true  },
			{ TEXT("ToggleCityList"),   EKeys::C,               false },
			{ TEXT("ToggleResearch"),   EKeys::R,               false },
		};

		for (const FDefaultBinding& Def : Defaults)
		{
			FInputActionKeyMapping Mapping;
			Mapping.ActionName = FName(*Def.Action);
			Mapping.Key = Def.Key;
			Mapping.bShift = Def.bShift;
			InputSettings->AddActionMapping(Mapping);
		}

		InputSettings->SaveKeyMappings();
	}

	// Rebuild the keybindings list UI.
	if (KeybindingsListBox)
	{
		KeybindingsListBox->ClearChildren();
	}
	PopulateKeybindingsList();
}

// =============================================================================
// Helper factories
// =============================================================================

UButton* UCoMSettingsWidget::CreateStyledButton(const FString& Label, float Width, float Height)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();

	FButtonStyle Style = Button->GetStyle();

	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(SettingsColours::ButtonBg);
	Style.Normal.OutlineSettings.Color = FSlateColor(SettingsColours::Gold);
	Style.Normal.OutlineSettings.Width = 1.0f;

	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(SettingsColours::ButtonHover);
	Style.Hovered.OutlineSettings.Color = FSlateColor(SettingsColours::Gold);
	Style.Hovered.OutlineSettings.Width = 1.0f;

	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(SettingsColours::ButtonBg);
	Style.Pressed.OutlineSettings.Color = FSlateColor(SettingsColours::Gold);
	Style.Pressed.OutlineSettings.Width = 1.0f;

	Button->SetStyle(Style);

	UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>();
	ButtonLabel->SetText(FText::FromString(Label));
	ButtonLabel->SetColorAndOpacity(FSlateColor(SettingsColours::White));
	ButtonLabel->SetJustification(ETextJustify::Center);

	FSlateFontInfo BtnFont = ButtonLabel->GetFont();
	BtnFont.Size = 14;
	ButtonLabel->SetFont(BtnFont);

	Button->AddChild(ButtonLabel);

	return Button;
}

UButton* UCoMSettingsWidget::CreateOptionButton(const FString& Label, float Width)
{
	return CreateStyledButton(Label, Width, 36.0f);
}

USlider* UCoMSettingsWidget::CreateVolumeSliderRow(UVerticalBox* Parent, const FString& Label, UTextBlock*& OutValueText)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

	// Label (160px wide)
	{
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(160.0f);

		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>();
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetColorAndOpacity(FSlateColor(SettingsColours::LightGrey));
		FSlateFontInfo Font = LabelText->GetFont();
		Font.Size = 14;
		LabelText->SetFont(Font);

		SB->AddChild(LabelText);
		UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SB);
		if (HSlot)
		{
			HSlot->SetVerticalAlignment(VAlign_Center);
			HSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}
	}

	// Slider
	USlider* Slider = WidgetTree->ConstructWidget<USlider>();
	Slider->SetMinValue(0.0f);
	Slider->SetMaxValue(1.0f);
	Slider->SetValue(0.8f);
	Slider->SetSliderBarColor(SettingsColours::SliderBar);
	Slider->SetSliderHandleColor(SettingsColours::SliderThumb);

	{
		UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(Slider);
		if (HSlot)
		{
			HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); // Fill
			HSlot->SetVerticalAlignment(VAlign_Center);
			HSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}
	}

	// Value text (60px)
	{
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(60.0f);

		OutValueText = WidgetTree->ConstructWidget<UTextBlock>();
		OutValueText->SetText(FText::FromString(TEXT("80%")));
		OutValueText->SetColorAndOpacity(FSlateColor(SettingsColours::Gold));
		OutValueText->SetJustification(ETextJustify::Right);
		FSlateFontInfo Font = OutValueText->GetFont();
		Font.Size = 14;
		OutValueText->SetFont(Font);

		SB->AddChild(OutValueText);
		UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(SB);
		if (HSlot)
		{
			HSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(Row);
	if (RowSlot)
	{
		RowSlot->SetPadding(FMargin(0.0f, 4.0f));
	}

	return Slider;
}

void UCoMSettingsWidget::AddSectionLabel(UVerticalBox* Parent, const FString& Label)
{
	if (!Parent) return;

	UTextBlock* SectionText = WidgetTree->ConstructWidget<UTextBlock>();
	SectionText->SetText(FText::FromString(Label));
	SectionText->SetColorAndOpacity(FSlateColor(SettingsColours::Gold));

	FSlateFontInfo Font = SectionText->GetFont();
	Font.Size = 16;
	Font.TypefaceFontName = FName(TEXT("Bold"));
	SectionText->SetFont(Font);

	UVerticalBoxSlot* SlotRef = Parent->AddChildToVerticalBox(SectionText);
	if (SlotRef)
	{
		SlotRef->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 2.0f));
	}
}

void UCoMSettingsWidget::AddSpacer(UVerticalBox* Parent, float Height)
{
	if (!Parent) return;

	USpacer* SpacerWidget = WidgetTree->ConstructWidget<USpacer>();
	SpacerWidget->SetSize(FVector2D(1.0f, Height));
	Parent->AddChildToVerticalBox(SpacerWidget);
}

// =============================================================================
// Settings persistence
// =============================================================================

void UCoMSettingsWidget::LoadCurrentSettings()
{
	// -- Load audio settings from config --
	LoadAudioSettings();

	// Apply cached values to sliders.
	if (MasterVolumeSlider)  MasterVolumeSlider->SetValue(CachedMasterVolume);
	if (MusicVolumeSlider)   MusicVolumeSlider->SetValue(CachedMusicVolume);
	if (SFXVolumeSlider)     SFXVolumeSlider->SetValue(CachedSFXVolume);
	if (AmbientVolumeSlider) AmbientVolumeSlider->SetValue(CachedAmbientVolume);

	// Update text labels.
	if (MasterVolumeText)  MasterVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(CachedMasterVolume * 100.0f))));
	if (MusicVolumeText)   MusicVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(CachedMusicVolume * 100.0f))));
	if (SFXVolumeText)     SFXVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(CachedSFXVolume * 100.0f))));
	if (AmbientVolumeText) AmbientVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(CachedAmbientVolume * 100.0f))));

	// -- Load graphics settings from UGameUserSettings --
	if (GEngine)
	{
		UGameUserSettings* Settings = GEngine->GetGameUserSettings();
		if (Settings)
		{
			PendingResolution = Settings->GetScreenResolution();

			// Set resolution combo to match.
			FString ResStr = FString::Printf(TEXT("%dx%d"), PendingResolution.X, PendingResolution.Y);
			if (ResolutionCombo)
			{
				// If the current resolution matches one of our options, select it.
				if (ResolutionCombo->FindOptionIndex(ResStr) != -1)
				{
					ResolutionCombo->SetSelectedOption(ResStr);
				}
			}

			EWindowMode::Type WinMode = Settings->GetFullscreenMode();
			switch (WinMode)
			{
			case EWindowMode::Fullscreen:          PendingWindowMode = 0; break;
			case EWindowMode::Windowed:            PendingWindowMode = 1; break;
			case EWindowMode::WindowedFullscreen:  PendingWindowMode = 2; break;
			default:                               PendingWindowMode = 0; break;
			}

			PendingQualityLevel = Settings->GetOverallScalabilityLevel();
			if (PendingQualityLevel < 0) PendingQualityLevel = 2; // Default to High if custom.

			bPendingVSync = Settings->IsVSyncEnabled();
			if (VSyncCheckBox) VSyncCheckBox->SetIsChecked(bPendingVSync);

			PendingFPSLimit = Settings->GetFrameRateLimit();
		}
	}
}

void UCoMSettingsWidget::SaveAudioSettings()
{
	// Save audio volumes to GameUserSettings.ini via GConfig.
	const FString ConfigPath = GGameUserSettingsIni;
	if (GConfig)
	{
		const TCHAR* Section = TEXT("/Script/CoMCore.CoMAudioSettings");

		GConfig->SetFloat(Section, TEXT("MasterVolume"), CachedMasterVolume, ConfigPath);
		GConfig->SetFloat(Section, TEXT("MusicVolume"), CachedMusicVolume, ConfigPath);
		GConfig->SetFloat(Section, TEXT("SFXVolume"), CachedSFXVolume, ConfigPath);
		GConfig->SetFloat(Section, TEXT("AmbientVolume"), CachedAmbientVolume, ConfigPath);

		GConfig->Flush(false, ConfigPath);
	}
}

void UCoMSettingsWidget::LoadAudioSettings()
{
	// Try to load from audio subsystem first (runtime state).
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMAudioSubsystem* Audio = GI->GetSubsystem<UCoMAudioSubsystem>())
		{
			CachedMasterVolume = Audio->GetMasterVolume();
			CachedMusicVolume = Audio->GetMusicVolume();
			CachedSFXVolume = Audio->GetSFXVolume();
		}
	}

	// Load ambient from config (no runtime getter yet).
	if (GConfig)
	{
		const FString ConfigPath = GGameUserSettingsIni;
		const TCHAR* Section = TEXT("/Script/CoMCore.CoMAudioSettings");
		float ConfigAmbient = 0.8f;
		if (GConfig->GetFloat(Section, TEXT("AmbientVolume"), ConfigAmbient, ConfigPath))
		{
			CachedAmbientVolume = ConfigAmbient;
		}
	}
}
