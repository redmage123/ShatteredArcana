// Copyright Mythforge Studios. All Rights Reserved.
// CoMMainMenuWidget.cpp -- Title screen / main menu implementation.

#include "CoMMainMenuWidget.h"

#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Spacer.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"

#include "Kismet/KismetSystemLibrary.h"

#include "CoMUISubsystem.h"
#include "Audio/CoMAudioSubsystem.h"

// =============================================================================
// Colour constants
// =============================================================================

namespace MenuColours
{
	static const FLinearColor BackgroundTop    = FLinearColor(0.020f, 0.020f, 0.063f, 1.0f); // #050510
	static const FLinearColor BackgroundBottom = FLinearColor(0.039f, 0.039f, 0.102f, 1.0f); // #0a0a1a
	static const FLinearColor Gold             = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f); // #daa520
	static const FLinearColor LightPurple      = FLinearColor(0.627f, 0.565f, 0.753f, 1.0f); // #a090c0
	static const FLinearColor White            = FLinearColor::White;
	static const FLinearColor Grey             = FLinearColor(0.400f, 0.400f, 0.400f, 1.0f); // #666666
	static const FLinearColor ButtonBg         = FLinearColor(0.086f, 0.129f, 0.243f, 1.0f); // #16213e
	static const FLinearColor ButtonHover      = FLinearColor(0.102f, 0.165f, 0.306f, 1.0f); // #1a2a4e
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsFocusable = true;
	SetVisibility(ESlateVisibility::Visible);

	BuildMenuContent();

	// Start menu music if audio subsystem is available.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMAudioSubsystem* Audio = GI->GetSubsystem<UCoMAudioSubsystem>())
		{
			Audio->PlayMusic(FName(TEXT("MainMenu")));
		}
	}
}

// =============================================================================
// Button callbacks
// =============================================================================

void UCoMMainMenuWidget::OnNewGameClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->ShowWizardCreation();
			return;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("CoMMainMenuWidget: Could not get CoMUISubsystem for wizard creation."));
}

void UCoMMainMenuWidget::OnLoadGameClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->ShowLoadScreen();
			return;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("CoMMainMenuWidget: Could not get CoMUISubsystem for load screen."));
}

void UCoMMainMenuWidget::OnSettingsClicked()
{
	ShowNotification(TEXT("Settings coming in a future update"));
}

void UCoMMainMenuWidget::OnCreditsClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			// Hide the main menu while credits are showing.
			SetVisibility(ESlateVisibility::Collapsed);
			UISS->ShowCredits();
		}
	}
}

void UCoMMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(
		this,
		nullptr,
		EQuitPreference::Quit,
		false);
}

// =============================================================================
// Notification helper
// =============================================================================

void UCoMMainMenuWidget::ShowNotification(const FString& Message)
{
	// Simple on-screen debug message. A proper notification system can replace this later.
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Message);
	}
}

// =============================================================================
// Layout construction
// =============================================================================

void UCoMMainMenuWidget::BuildMenuContent()
{
	// -- Full-screen dark background ------------------------------------------

	BackgroundBorder = NewObject<UBorder>(this);
	BackgroundBorder->SetBrushColor(MenuColours::BackgroundBottom);
	BackgroundBorder->SetPadding(FMargin(0.0f));

	// -- Center-aligned content via overlay + vertical box --------------------

	UOverlay* RootOverlay = NewObject<UOverlay>(this);
	BackgroundBorder->AddChild(RootOverlay);

	ContentBox = NewObject<UVerticalBox>(this);

	UOverlaySlot* ContentSlot = RootOverlay->AddChildToOverlay(ContentBox);
	if (ContentSlot)
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	// -- Set the background as the root widget --------------------------------

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}

	// -- Title ----------------------------------------------------------------

	{
		UTextBlock* TitleText = NewObject<UTextBlock>(this);
		TitleText->SetText(FText::FromString(TEXT("SHATTERED ARCANA")));
		TitleText->SetColorAndOpacity(FSlateColor(MenuColours::Gold));
		TitleText->SetJustification(ETextJustify::Center);

		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 48;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		TitleText->SetFont(TitleFont);

		UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(TitleText);
		if (Slot)
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	}

	// -- Subtitle -------------------------------------------------------------

	{
		UTextBlock* SubtitleText = NewObject<UTextBlock>(this);
		SubtitleText->SetText(FText::FromString(TEXT("Eight Planes. A Thousand Spells. One Throne.")));
		SubtitleText->SetColorAndOpacity(FSlateColor(MenuColours::LightPurple));
		SubtitleText->SetJustification(ETextJustify::Center);

		FSlateFontInfo SubFont = SubtitleText->GetFont();
		SubFont.Size = 16;
		SubFont.TypefaceFontName = FName(TEXT("Italic"));
		SubtitleText->SetFont(SubFont);

		UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(SubtitleText);
		if (Slot)
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}

	// -- Spacer (40px) --------------------------------------------------------

	{
		USpacer* SpacerWidget = NewObject<USpacer>(this);
		SpacerWidget->SetSize(FVector2D(1.0f, 40.0f));
		ContentBox->AddChildToVerticalBox(SpacerWidget);
	}

	// -- Menu Buttons ---------------------------------------------------------

	NewGameButton  = CreateMenuButton(TEXT("New Game"));
	LoadGameButton = CreateMenuButton(TEXT("Load Game"));
	SettingsButton = CreateMenuButton(TEXT("Settings"));
	CreditsButton  = CreateMenuButton(TEXT("Credits"));
	QuitButton     = CreateMenuButton(TEXT("Quit"));

	// Bind click events.
	NewGameButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnNewGameClicked);
	LoadGameButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnLoadGameClicked);
	SettingsButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnSettingsClicked);
	CreditsButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnCreditsClicked);
	QuitButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnQuitClicked);

	// -- Spacer (20px) --------------------------------------------------------

	{
		USpacer* SpacerWidget = NewObject<USpacer>(this);
		SpacerWidget->SetSize(FVector2D(1.0f, 20.0f));
		ContentBox->AddChildToVerticalBox(SpacerWidget);
	}

	// -- Version text ---------------------------------------------------------

	{
		UTextBlock* VersionText = NewObject<UTextBlock>(this);
		VersionText->SetText(FText::FromString(TEXT("v0.1 Alpha \u2014 Mythforge Studios")));
		VersionText->SetColorAndOpacity(FSlateColor(MenuColours::Grey));
		VersionText->SetJustification(ETextJustify::Center);

		FSlateFontInfo VerFont = VersionText->GetFont();
		VerFont.Size = 12;
		VersionText->SetFont(VerFont);

		UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(VersionText);
		if (Slot)
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		}
	}

	// -- Copyright text -------------------------------------------------------

	{
		UTextBlock* CopyrightText = NewObject<UTextBlock>(this);
		CopyrightText->SetText(FText::FromString(FString(TEXT("\u00A9")) + TEXT(" 2026 Mythforge Studios")));
		CopyrightText->SetColorAndOpacity(FSlateColor(MenuColours::Grey));
		CopyrightText->SetJustification(ETextJustify::Center);

		FSlateFontInfo CopyFont = CopyrightText->GetFont();
		CopyFont.Size = 10;
		CopyrightText->SetFont(CopyFont);

		UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(CopyrightText);
		if (Slot)
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
		}
	}
}

// =============================================================================
// Button factory
// =============================================================================

UButton* UCoMMainMenuWidget::CreateMenuButton(const FString& Label)
{
	// Size box enforces 300x50.
	USizeBox* SizeBox = NewObject<USizeBox>(this);
	SizeBox->SetWidthOverride(300.0f);
	SizeBox->SetHeightOverride(50.0f);

	// Button with dark background.
	UButton* Button = NewObject<UButton>(this);

	// Style the button -- dark background with gold tint accents.
	FButtonStyle Style = Button->GetStyle();

	// Normal state: dark background.
	Style.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
	Style.Normal.TintColor = FSlateColor(MenuColours::ButtonBg);
	Style.Normal.OutlineSettings.Color = FSlateColor(MenuColours::Gold);
	Style.Normal.OutlineSettings.Width = 1.0f;

	// Hovered state: slightly brighter.
	Style.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
	Style.Hovered.TintColor = FSlateColor(MenuColours::ButtonHover);
	Style.Hovered.OutlineSettings.Color = FSlateColor(MenuColours::Gold);
	Style.Hovered.OutlineSettings.Width = 1.0f;

	// Pressed state: same as normal.
	Style.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
	Style.Pressed.TintColor = FSlateColor(MenuColours::ButtonBg);
	Style.Pressed.OutlineSettings.Color = FSlateColor(MenuColours::Gold);
	Style.Pressed.OutlineSettings.Width = 1.0f;

	Button->SetStyle(Style);

	// Label text inside the button.
	UTextBlock* ButtonLabel = NewObject<UTextBlock>(this);
	ButtonLabel->SetText(FText::FromString(Label));
	ButtonLabel->SetColorAndOpacity(FSlateColor(MenuColours::White));
	ButtonLabel->SetJustification(ETextJustify::Center);

	FSlateFontInfo BtnFont = ButtonLabel->GetFont();
	BtnFont.Size = 16;
	ButtonLabel->SetFont(BtnFont);

	Button->AddChild(ButtonLabel);
	SizeBox->AddChild(Button);

	// Add to content box with 8px vertical spacing.
	UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(SizeBox);
	if (Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetPadding(FMargin(0.0f, 4.0f)); // 4px top + 4px bottom = 8px between buttons
	}

	return Button;
}
