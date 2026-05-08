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
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"

#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Texture2D.h"

#include "CoMUISubsystem.h"
#include "Audio/CoMAudioSubsystem.h"
#include "Shared/CoMWidgetStyles.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/ICursor.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

// =============================================================================
// Colour palette
// =============================================================================

namespace MC
{
	static const FLinearColor BgDark        = FLinearColor(0.015f, 0.010f, 0.040f, 1.0f);
	static const FLinearColor PanelBg       = FLinearColor(0.035f, 0.025f, 0.080f, 0.92f);
	static const FLinearColor Gold          = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldBright    = FLinearColor(1.000f, 0.843f, 0.000f, 1.0f);
	static const FLinearColor GoldDim       = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor LightPurple   = FLinearColor(0.700f, 0.600f, 0.850f, 1.0f);
	static const FLinearColor Silver        = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey          = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);

	static const FLinearColor BtnNormal     = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover      = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPressed    = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);
	static const FLinearColor BtnBorder     = FLinearColor(0.500f, 0.380f, 0.080f, 0.7f);
	static const FLinearColor BtnBorderH    = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
}

// Helper to make a solid-colour FSlateBrush
static FSlateBrush MakeSolidBrush(const FLinearColor& Color)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(Color);
	Brush.ImageSize = FVector2D(1.0f, 1.0f);
	return Brush;
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMMainMenuWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildMenuContent();
	}
	return Super::RebuildWidget();
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (NewGameButton)  { NewGameButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnNewGameClicked); }
	if (LoadGameButton) { LoadGameButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnLoadGameClicked); }
	if (SettingsButton) { SettingsButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnSettingsClicked); }
	if (CreditsButton)  { CreditsButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnCreditsClicked); }
	if (QuitButton)     { QuitButton->OnClicked.AddDynamic(this, &UCoMMainMenuWidget::OnQuitClicked); }

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->bShowMouseCursor = true;
			PC->SetInputMode(FInputModeUIOnly().SetWidgetToFocus(TakeWidget()));

			// Set magic wand hardware cursor from .cur file using Windows API
			FString CursorPath = FPaths::ProjectContentDir() / TEXT("Cursors/wand.cur");
			if (FPaths::FileExists(CursorPath))
			{
				HCURSOR WandCursor = ::LoadCursorFromFileW(*CursorPath);
				if (WandCursor)
				{
					TSharedPtr<ICursor> PlatformCursor = FSlateApplication::Get().GetPlatformCursor();
					if (PlatformCursor.IsValid())
					{
						PlatformCursor->SetTypeShape(EMouseCursor::Default, WandCursor);
						UE_LOG(LogTemp, Log, TEXT("[MainMenu] Wand cursor loaded: %s"), *CursorPath);
					}
				}
			}
		}
	}

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
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
			UI->ShowWizardCreation();
}

void UCoMMainMenuWidget::OnLoadGameClicked()
{
	if (UGameInstance* GI = GetGameInstance())
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
			UI->ShowLoadScreen();
}

void UCoMMainMenuWidget::OnSettingsClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			SetVisibility(ESlateVisibility::Collapsed);
			UI->ShowSettings();
		}
	}
}

void UCoMMainMenuWidget::OnCreditsClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			SetVisibility(ESlateVisibility::Collapsed);
			UI->ShowCredits();
		}
	}
}

void UCoMMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void UCoMMainMenuWidget::ShowNotification(const FString& Message)
{
	if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Message); }
}

// =============================================================================
// Layout
// =============================================================================

void UCoMMainMenuWidget::BuildMenuContent()
{
	// ── Full-screen dark background ──────────────────────────────────────────
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(MC::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	// ── Screen-level overlay ─────────────────────────────────────────────────
	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(ScreenOverlay);

	// ── Background image (establishing shot) ─────────────────────────────────
	{
		UTexture2D* BgTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/Textures/UI/T_MenuBackground.T_MenuBackground"));
		if (BgTexture)
		{
			UImage* BgImage = WidgetTree->ConstructWidget<UImage>();
			FSlateBrush BgBrush;
			BgBrush.SetResourceObject(BgTexture);
			BgBrush.ImageSize = FVector2D(1920.0f, 960.0f);
			BgBrush.DrawAs = ESlateBrushDrawType::Image;
			BgBrush.Tiling = ESlateBrushTileType::NoTile;
			// Keep it bright — tapestry style
			BgBrush.TintColor = FSlateColor(FLinearColor(0.85f, 0.85f, 0.9f, 1.0f));
			BgImage->SetBrush(BgBrush);

			UOverlaySlot* ImgSlot = ScreenOverlay->AddChildToOverlay(BgImage);
			if (ImgSlot)
			{
				ImgSlot->SetHorizontalAlignment(HAlign_Fill);
				ImgSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}

	// ── Dark vignette overlay on top of image ────────────────────────────────
	{
		UBorder* Vignette = WidgetTree->ConstructWidget<UBorder>();
		Vignette->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.02f, 0.2f));
		Vignette->SetPadding(FMargin(0.0f));
		UOverlaySlot* VSlot = ScreenOverlay->AddChildToOverlay(Vignette);
		if (VSlot)
		{
			VSlot->SetHorizontalAlignment(HAlign_Fill);
			VSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	// ── Gold accent line — top ───────────────────────────────────────────────
	{
		UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
		Line->SetBrushColor(MC::GoldDim);
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetHeightOverride(2.0f);
		Box->AddChild(Line);
		UOverlaySlot* S = ScreenOverlay->AddChildToOverlay(Box);
		if (S) { S->SetVerticalAlignment(VAlign_Top); S->SetHorizontalAlignment(HAlign_Fill); }
	}

	// ── Gold accent line — bottom ────────────────────────────────────────────
	{
		UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
		Line->SetBrushColor(MC::GoldDim);
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetHeightOverride(2.0f);
		Box->AddChild(Line);
		UOverlaySlot* S = ScreenOverlay->AddChildToOverlay(Box);
		if (S) { S->SetVerticalAlignment(VAlign_Bottom); S->SetHorizontalAlignment(HAlign_Fill); }
	}

	// ── Center panel ─────────────────────────────────────────────────────────
	// Panel border (gold colored thin border around the panel)
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(MC::GoldDim);
	PanelBorder->SetPadding(FMargin(1.5f)); // 1.5px gold border

	// Panel inner background
	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(MC::PanelBg);
	PanelInner->SetPadding(FMargin(50.0f, 35.0f));
	PanelBorder->AddChild(PanelInner);

	// Size constraint — wide enough for the title
	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(620.0f);
	PanelSize->AddChild(PanelBorder);

	UOverlaySlot* PanelSlot = ScreenOverlay->AddChildToOverlay(PanelSize);
	if (PanelSlot)
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	// ── Content column ───────────────────────────────────────────────────────
	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelInner->AddChild(ContentBox);

	// ── Decorative top symbols ───────────────────────────────────────────────
	AddCenteredText(TEXT("- - -"), MC::GoldDim, 16, FMargin(0, 0, 0, 10));

	// ── Title (Old English Gothic font) ──────────────────────────────────────
	{
		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
		Title->SetText(FText::FromString(TEXT("Shattered Arcana")));
		Title->SetColorAndOpacity(FSlateColor(MC::GoldBright));
		Title->SetJustification(ETextJustify::Center);

		// Use Old English font for the gothic title
		FString FontPath = FPaths::ProjectContentDir() / TEXT("Fonts/OldEnglish.ttf");
		if (!FPaths::FileExists(FontPath))
		{
			FontPath = TEXT("C:/Windows/Fonts/OLDENGL.TTF");
		}

		FSlateFontInfo GothicFont = Title->GetFont();
		GothicFont.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		GothicFont.Size = 48;

		// Try to use the Old English font via composite font
		const TSharedPtr<const FCompositeFont> OldEnglishFont = MakeShareable(new FStandaloneCompositeFont(
			NAME_None, FontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
		if (OldEnglishFont.IsValid())
		{
			GothicFont.FontObject = nullptr;
			GothicFont.CompositeFont = OldEnglishFont;
		}
		GothicFont.OutlineSettings.OutlineSize = 2;
		GothicFont.OutlineSettings.OutlineColor = FLinearColor(0.3f, 0.15f, 0.0f, 0.8f);
		Title->SetFont(GothicFont);
		Title->SetShadowOffset(FVector2D(3.0f, 3.0f));
		Title->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));

		UVerticalBoxSlot* S = ContentBox->AddChildToVerticalBox(Title);
		if (S) { S->SetHorizontalAlignment(HAlign_Center); S->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// ── Subtitle (italic serif) ──────────────────────────────────────────────
	{
		UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>();
		Subtitle->SetText(FText::FromString(TEXT("Eight Planes. A Thousand Spells. One Throne.")));
		Subtitle->SetColorAndOpacity(FSlateColor(MC::LightPurple));
		Subtitle->SetJustification(ETextJustify::Center);
		FSlateFontInfo SubFont = Subtitle->GetFont();
		SubFont.Size = 15;
		SubFont.TypefaceFontName = FName(TEXT("Italic"));
		Subtitle->SetFont(SubFont);
		Subtitle->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Subtitle->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
		UVerticalBoxSlot* S = ContentBox->AddChildToVerticalBox(Subtitle);
		if (S) { S->SetHorizontalAlignment(HAlign_Center); S->SetPadding(FMargin(0, 0, 0, 6)); }
	}

	// ── Divider ──────────────────────────────────────────────────────────────
	{
		UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
		Div->SetBrushColor(MC::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(1.0f);
		DivSize->SetWidthOverride(400.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* S = ContentBox->AddChildToVerticalBox(DivSize);
		if (S) { S->SetHorizontalAlignment(HAlign_Center); S->SetPadding(FMargin(0, 12, 0, 18)); }
	}

	// ── Buttons ──────────────────────────────────────────────────────────────
	{
		// New Game — primary action button (bright gold, prominent)
		USizeBox* NGContainer = nullptr;
		NewGameButton = CoMStyle::CreatePrimaryButton(WidgetTree, TEXT("New Game"), 440.f, 60.f, 22, &NGContainer);
		if (NGContainer)
		{
			UVerticalBoxSlot* S = ContentBox->AddChildToVerticalBox(NGContainer);
			if (S) { S->SetHorizontalAlignment(HAlign_Center); S->SetPadding(FMargin(0, 6, 0, 4)); }
		}
	}
	LoadGameButton = CreateMenuButton(TEXT("Load Game"));
	SettingsButton = CreateMenuButton(TEXT("Settings"));
	CreditsButton  = CreateMenuButton(TEXT("Credits"));
	QuitButton     = CreateMenuButton(TEXT("Quit"));

	// ── Bottom divider ───────────────────────────────────────────────────────
	{
		UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
		Div->SetBrushColor(MC::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(1.0f);
		DivSize->SetWidthOverride(400.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* S = ContentBox->AddChildToVerticalBox(DivSize);
		if (S) { S->SetHorizontalAlignment(HAlign_Center); S->SetPadding(FMargin(0, 14, 0, 10)); }
	}

	// ── Version + copyright ──────────────────────────────────────────────────
	AddCenteredText(TEXT("v0.1 Alpha"), MC::Grey, 11, FMargin(0, 0, 0, 2));
	AddCenteredText(FString(TEXT("\u00A9")) + TEXT(" 2026 Mythforge Studios"), MC::Grey, 10, FMargin(0));
}

// =============================================================================
// Helpers
// =============================================================================

void UCoMMainMenuWidget::AddCenteredText(const FString& Text, const FLinearColor& Color, int32 FontSize, const FMargin& Pad)
{
	UTextBlock* TB = WidgetTree->ConstructWidget<UTextBlock>();
	TB->SetText(FText::FromString(Text));
	TB->SetColorAndOpacity(FSlateColor(Color));
	TB->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = TB->GetFont();
	Font.Size = FontSize;
	TB->SetFont(Font);

	UVerticalBoxSlot* S = ContentBox->AddChildToVerticalBox(TB);
	if (S) { S->SetHorizontalAlignment(HAlign_Center); S->SetPadding(Pad); }
}

// =============================================================================
// Styled button — simple, clean, aligned
// =============================================================================

UButton* UCoMMainMenuWidget::CreateMenuButton(const FString& Label)
{
	USizeBox* Container = nullptr;
	UButton* Button = CoMStyle::CreateStyledButton(WidgetTree, Label, 420.f, 56.f, 20, &Container);

	if (Container)
	{
		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(Container);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Center);
			SlotRef->SetPadding(FMargin(0.0f, 5.0f));
		}
	}

	return Button;
}
