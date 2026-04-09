// Copyright Mythforge Studios. All Rights Reserved.
// CoMCreditsWidget.cpp -- Full-screen scrolling credits implementation.

#include "CoMCreditsWidget.h"

#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Spacer.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"

// =============================================================================
// Colour constants
// =============================================================================

namespace CreditsColours
{
	static const FLinearColor Background = FLinearColor(0.039f, 0.039f, 0.102f, 1.0f); // #0a0a1a
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f); // #daa520
	static const FLinearColor White      = FLinearColor::White;
	static const FLinearColor LightGrey  = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);       // #cccccc
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMCreditsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsFocusable = true;
	SetVisibility(ESlateVisibility::Visible);

	BuildCreditsContent();
}

void UCoMCreditsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsScrolling && CreditsScrollBox)
	{
		CurrentScrollOffset += ScrollSpeed * InDeltaTime;
		CreditsScrollBox->SetScrollOffset(CurrentScrollOffset);

		// Stop when we reach the end.
		const float MaxOffset = CreditsScrollBox->GetScrollOffsetOfEnd();
		if (CurrentScrollOffset >= MaxOffset)
		{
			bIsScrolling = false;
		}
	}
}

// =============================================================================
// Input handling
// =============================================================================

FReply UCoMCreditsWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	StopCredits();
	return FReply::Handled();
}

FReply UCoMCreditsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		StopCredits();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// =============================================================================
// Public API
// =============================================================================

void UCoMCreditsWidget::StartCredits()
{
	CurrentScrollOffset = 0.0f;
	if (CreditsScrollBox)
	{
		CreditsScrollBox->SetScrollOffset(0.0f);
	}
	bIsScrolling = true;
	SetKeyboardFocus();
}

void UCoMCreditsWidget::StopCredits()
{
	bIsScrolling = false;
	RemoveFromParent();
}

// =============================================================================
// Layout construction
// =============================================================================

void UCoMCreditsWidget::BuildCreditsContent()
{
	// -- Full-screen dark background ------------------------------------------

	BackgroundBorder = NewObject<UBorder>(this);
	BackgroundBorder->SetBrushColor(CreditsColours::Background);
	BackgroundBorder->SetPadding(FMargin(0.0f));

	// -- Scroll box fills the background --------------------------------------

	CreditsScrollBox = NewObject<UScrollBox>(this);
	CreditsScrollBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	CreditsScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
	CreditsScrollBox->SetOrientation(Orient_Vertical);

	BackgroundBorder->AddChild(CreditsScrollBox);

	// -- Vertical box holds all credit lines ----------------------------------

	ContentBox = NewObject<UVerticalBox>(this);
	CreditsScrollBox->AddChild(ContentBox);

	// -- Set the background as the root widget content ------------------------

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}

	// -- Build the credit lines -----------------------------------------------

	// Initial spacer (one full screen of blank so text scrolls up from bottom)
	AddSpacer(1080.0f);

	// Title
	AddTextLine(TEXT("SHATTERED ARCANA"), CreditsColours::Gold, 48, /*bBold=*/true);
	AddSpacer(24.0f);
	AddTextLine(TEXT("A Mythforge Studios Production"), CreditsColours::White, 24);
	AddSpacer(40.0f);

	AddSeparator();
	AddSpacer(40.0f);

	// Game Design
	AddTextLine(TEXT("GAME DESIGN & PROGRAMMING"), CreditsColours::White, 28, /*bBold=*/true);
	AddSpacer(12.0f);
	AddTextLine(TEXT("Braun Brelin"), CreditsColours::LightGrey, 22);
	AddSpacer(32.0f);

	// AI
	AddTextLine(TEXT("AI PROGRAMMING ASSISTANCE"), CreditsColours::White, 28, /*bBold=*/true);
	AddSpacer(12.0f);
	AddTextLine(TEXT("Claude \u2014 Anthropic"), CreditsColours::LightGrey, 22);
	AddSpacer(40.0f);

	AddSeparator();
	AddSpacer(40.0f);

	// Music
	AddTextLine(TEXT("MUSIC"), CreditsColours::White, 28, /*bBold=*/true);
	AddSpacer(20.0f);

	AddTextLine(TEXT("Patrick de Arteaga (patrickdearteaga.com)"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Licensed under Creative Commons Attribution 4.0"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("Tracks: Goliath's Foe, Boss Fight, Rey de Picas, Dangerous Dungeon,"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("Spring Village, Center Of The Earth, Yellow Forest, Never Surrender,"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("Happy Ending, Battleship, Not Giving Up, The Biggest Discovery,"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("Lyonesse, Child's Nightmare, Solve The Puzzle, Guerrero Colosal,"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("The True Story of Beelzebub, Las Dos Fronteras, Voices From Earth,"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("Forgotten Spaceship, Fantasia, Ruined Planet"), CreditsColours::LightGrey, 18);
	AddSpacer(24.0f);

	AddTextLine(TEXT("Matthew Pablo (matthewpablo.com)"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Licensed under Creative Commons Attribution 3.0"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("Tracks: Heroic Demise, Woodland Fantasy, Dark Descent"), CreditsColours::LightGrey, 18);
	AddSpacer(24.0f);

	AddTextLine(TEXT("Alexandr Zhelanov"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Licensed under Creative Commons Attribution 3.0"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("Track: Mystical Theme"), CreditsColours::LightGrey, 18);
	AddSpacer(24.0f);

	AddTextLine(TEXT("Pierre Bondoerffer (@pbondoer)"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Licensed under Creative Commons Attribution-ShareAlike 3.0"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("Track: A Journey Awaits"), CreditsColours::LightGrey, 18);
	AddSpacer(24.0f);

	AddTextLine(TEXT("cynicmusic \u2014 CC0 Public Domain"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Track: Battle Theme A"), CreditsColours::LightGrey, 18);
	AddSpacer(24.0f);

	AddTextLine(TEXT("vitalezzz \u2014 CC0 Public Domain"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Track: The Ancient Legend"), CreditsColours::LightGrey, 18);
	AddSpacer(40.0f);

	AddSeparator();
	AddSpacer(40.0f);

	// Sound Effects
	AddTextLine(TEXT("SOUND EFFECTS"), CreditsColours::White, 28, /*bBold=*/true);
	AddSpacer(20.0f);

	AddTextLine(TEXT("Kenney (kenney.nl)"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("CC0 Public Domain"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("UI Audio, Interface Sounds, Impact Sounds, RPG Audio, Voiceover Pack"), CreditsColours::LightGrey, 18);
	AddSpacer(24.0f);

	AddTextLine(TEXT("Mixkit (mixkit.co)"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Mixkit Free License"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("Magic and Spell Sound Effects"), CreditsColours::LightGrey, 18);
	AddSpacer(24.0f);

	AddTextLine(TEXT("OpenGameArt.org Contributors"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("CC0 Public Domain"), CreditsColours::LightGrey, 18);
	AddTextLine(TEXT("RPG Sound Pack by artisticdude"), CreditsColours::LightGrey, 18);
	AddSpacer(40.0f);

	AddSeparator();
	AddSpacer(40.0f);

	// Engines
	AddTextLine(TEXT("GAME ENGINE"), CreditsColours::White, 28, /*bBold=*/true);
	AddSpacer(12.0f);
	AddTextLine(TEXT("Unreal Engine 5.4 \u2014 Epic Games, Inc."), CreditsColours::LightGrey, 22);
	AddSpacer(32.0f);

	AddTextLine(TEXT("PROTOTYPE ENGINE"), CreditsColours::White, 28, /*bBold=*/true);
	AddSpacer(12.0f);
	AddTextLine(TEXT("Godot Engine 4.x \u2014 Godot Engine Contributors"), CreditsColours::LightGrey, 22);
	AddSpacer(40.0f);

	AddSeparator();
	AddSpacer(40.0f);

	// Middleware
	AddTextLine(TEXT("MIDDLEWARE & PLUGINS"), CreditsColours::White, 28, /*bBold=*/true);
	AddSpacer(12.0f);
	AddTextLine(TEXT("GameplayAbilities \u2014 Epic Games"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Enhanced Input \u2014 Epic Games"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Niagara Particle System \u2014 Epic Games"), CreditsColours::LightGrey, 22);
	AddSpacer(40.0f);

	AddSeparator();
	AddSpacer(40.0f);

	// Tools
	AddTextLine(TEXT("TOOLS USED"), CreditsColours::White, 28, /*bBold=*/true);
	AddSpacer(12.0f);
	AddTextLine(TEXT("Blender \u2014 Blender Foundation"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Mixamo \u2014 Adobe"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("MediaPipe \u2014 Google"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Playwright \u2014 Microsoft"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Three.js \u2014 mrdoob and contributors"), CreditsColours::LightGrey, 22);
	AddSpacer(40.0f);

	AddSeparator();
	AddSpacer(40.0f);

	// Special Thanks
	AddTextLine(TEXT("SPECIAL THANKS"), CreditsColours::White, 28, /*bBold=*/true);
	AddSpacer(12.0f);
	AddTextLine(TEXT("The Master of Magic community"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Caster of Magic modding community"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("Illwinter Game Design (inspiration)"), CreditsColours::LightGrey, 22);
	AddSpacer(40.0f);

	AddSeparator();
	AddSpacer(40.0f);

	// Copyright
	AddTextLine(FString(TEXT("\u00A9")) + TEXT(" 2026 Mythforge Studios"), CreditsColours::LightGrey, 22);
	AddTextLine(TEXT("All Rights Reserved"), CreditsColours::LightGrey, 22);
	AddSpacer(16.0f);
	AddTextLine(TEXT("mythforge.ai-elevate.ai"), CreditsColours::Gold, 20);

	// Trailing spacer so the last line can scroll fully off-screen
	AddSpacer(1080.0f);
}

// =============================================================================
// Helpers
// =============================================================================

void UCoMCreditsWidget::AddTextLine(const FString& Text, const FLinearColor& Color, int32 FontSize, bool bBold)
{
	if (!ContentBox)
	{
		return;
	}

	UTextBlock* TextBlock = NewObject<UTextBlock>(this);
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetJustification(ETextJustify::Center);

	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	if (bBold)
	{
		FontInfo.TypefaceFontName = FName(TEXT("Bold"));
	}
	TextBlock->SetFont(FontInfo);

	UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(TextBlock);
	if (Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetPadding(FMargin(0.0f, 2.0f));
	}
}

void UCoMCreditsWidget::AddSeparator()
{
	if (!ContentBox)
	{
		return;
	}

	UImage* Line = NewObject<UImage>(this);
	Line->SetColorAndOpacity(CreditsColours::Gold);
	Line->SetDesiredSizeOverride(FVector2D(400.0f, 2.0f));

	UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(Line);
	if (Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetPadding(FMargin(0.0f, 8.0f));
	}
}

void UCoMCreditsWidget::AddSpacer(float Height)
{
	if (!ContentBox)
	{
		return;
	}

	USpacer* SpacerWidget = NewObject<USpacer>(this);
	SpacerWidget->SetSize(FVector2D(1.0f, Height));

	UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(SpacerWidget);
	if (Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
	}
}
