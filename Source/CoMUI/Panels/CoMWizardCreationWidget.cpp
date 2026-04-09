// Copyright Mythforge Studios. All Rights Reserved.
// CoMWizardCreationWidget.cpp -- Wizard creation screen implementation.

#include "CoMWizardCreationWidget.h"

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
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"

#include "Kismet/GameplayStatics.h"
#include "Framework/CoMGameInstance.h"
#include "CoMUISubsystem.h"

// =============================================================================
// Colour constants
// =============================================================================

namespace WizCreationColours
{
	static const FLinearColor Background   = FLinearColor(0.039f, 0.039f, 0.102f, 1.0f); // #0a0a1a
	static const FLinearColor PanelBg      = FLinearColor(0.086f, 0.129f, 0.243f, 1.0f); // #16213e
	static const FLinearColor Gold         = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f); // #daa520
	static const FLinearColor GoldDim      = FLinearColor(0.855f, 0.647f, 0.125f, 0.4f);
	static const FLinearColor White        = FLinearColor::White;
	static const FLinearColor Grey         = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	static const FLinearColor LightGrey    = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
	static const FLinearColor DarkButton   = FLinearColor(0.06f, 0.06f, 0.15f, 1.0f);
	static const FLinearColor Purple       = FLinearColor(0.325f, 0.204f, 0.514f, 1.0f); // #533483
	static const FLinearColor ButtonHover  = FLinearColor(0.102f, 0.165f, 0.306f, 1.0f);

	// Realm colours
	static const FLinearColor RealmLife    = FLinearColor::White;                                     // #ffffff
	static const FLinearColor RealmDeath   = FLinearColor(0.4f, 0.0f, 0.4f, 1.0f);                  // #660066
	static const FLinearColor RealmChaos   = FLinearColor(1.0f, 0.2f, 0.0f, 1.0f);                  // #ff3300
	static const FLinearColor RealmNature  = FLinearColor(0.0f, 0.8f, 0.0f, 1.0f);                  // #00cc00
	static const FLinearColor RealmSorcery = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);                  // #3366ff
	static const FLinearColor RealmArcane  = FLinearColor(0.8f, 0.6f, 0.0f, 1.0f);                  // #cc9900
	static const FLinearColor RealmBinding = FLinearColor(0.6f, 0.0f, 0.0f, 1.0f);                  // #990000
	static const FLinearColor RealmSpirit  = FLinearColor(0.6f, 0.4f, 1.0f, 1.0f);                  // #9966ff
	static const FLinearColor RealmGlamour = FLinearColor(1.0f, 0.4f, 0.8f, 1.0f);                  // #ff66cc
}

// Realm metadata
struct FRealmInfo
{
	ECoMSpellRealm Realm;
	FString Name;
	FLinearColor Color;
};

static const FRealmInfo GRealmInfos[] =
{
	{ ECoMSpellRealm::Life,    TEXT("Life"),    WizCreationColours::RealmLife },
	{ ECoMSpellRealm::Death,   TEXT("Death"),   WizCreationColours::RealmDeath },
	{ ECoMSpellRealm::Chaos,   TEXT("Chaos"),   WizCreationColours::RealmChaos },
	{ ECoMSpellRealm::Nature,  TEXT("Nature"),  WizCreationColours::RealmNature },
	{ ECoMSpellRealm::Sorcery, TEXT("Sorcery"), WizCreationColours::RealmSorcery },
	{ ECoMSpellRealm::Arcane,  TEXT("Arcane"),  WizCreationColours::RealmArcane },
	{ ECoMSpellRealm::Binding, TEXT("Binding"), WizCreationColours::RealmBinding },
	{ ECoMSpellRealm::Spirit,  TEXT("Spirit"),  WizCreationColours::RealmSpirit },
	{ ECoMSpellRealm::Glamour, TEXT("Glamour"), WizCreationColours::RealmGlamour },
};

// Difficulty metadata
struct FDifficultyInfo
{
	FString Name;
	FString Description;
};

static const FDifficultyInfo GDifficultyInfos[] =
{
	{ TEXT("Easy"),       TEXT("AI gets penalties") },
	{ TEXT("Normal"),     TEXT("Standard") },
	{ TEXT("Hard"),       TEXT("+25% AI income") },
	{ TEXT("Lunatic"),    TEXT("+50% income, +25% combat") },
	{ TEXT("Impossible"), TEXT("+100% income, extra cities") },
};

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMWizardCreationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsFocusable = true;
	SetVisibility(ESlateVisibility::Visible);

	BuildLayout();
}

// =============================================================================
// Class callbacks (UFUNCTION wrappers)
// =============================================================================

void UCoMWizardCreationWidget::OnClassWizardClicked()  { OnClassSelected(ECoMWizardClass::Wizard); }
void UCoMWizardCreationWidget::OnClassPsykerClicked()  { OnClassSelected(ECoMWizardClass::Psyker); }
void UCoMWizardCreationWidget::OnClassWarlockClicked() { OnClassSelected(ECoMWizardClass::Warlock); }

// =============================================================================
// Difficulty callbacks
// =============================================================================

void UCoMWizardCreationWidget::OnDiffEasyClicked()       { OnDifficultySelected(0); }
void UCoMWizardCreationWidget::OnDiffNormalClicked()     { OnDifficultySelected(1); }
void UCoMWizardCreationWidget::OnDiffHardClicked()       { OnDifficultySelected(2); }
void UCoMWizardCreationWidget::OnDiffLunaticClicked()    { OnDifficultySelected(3); }
void UCoMWizardCreationWidget::OnDiffImpossibleClicked() { OnDifficultySelected(4); }

// =============================================================================
// Realm callbacks
// =============================================================================

void UCoMWizardCreationWidget::OnRealmLifeClicked()    { OnRealmToggled(ECoMSpellRealm::Life); }
void UCoMWizardCreationWidget::OnRealmDeathClicked()   { OnRealmToggled(ECoMSpellRealm::Death); }
void UCoMWizardCreationWidget::OnRealmChaosClicked()   { OnRealmToggled(ECoMSpellRealm::Chaos); }
void UCoMWizardCreationWidget::OnRealmNatureClicked()  { OnRealmToggled(ECoMSpellRealm::Nature); }
void UCoMWizardCreationWidget::OnRealmSorceryClicked() { OnRealmToggled(ECoMSpellRealm::Sorcery); }
void UCoMWizardCreationWidget::OnRealmArcaneClicked()  { OnRealmToggled(ECoMSpellRealm::Arcane); }
void UCoMWizardCreationWidget::OnRealmBindingClicked() { OnRealmToggled(ECoMSpellRealm::Binding); }
void UCoMWizardCreationWidget::OnRealmSpiritClicked()  { OnRealmToggled(ECoMSpellRealm::Spirit); }
void UCoMWizardCreationWidget::OnRealmGlamourClicked() { OnRealmToggled(ECoMSpellRealm::Glamour); }

// =============================================================================
// Logic
// =============================================================================

void UCoMWizardCreationWidget::OnClassSelected(ECoMWizardClass Class)
{
	SelectedClass = Class;
	UpdateClassButtonStyles();

	// Show realm section only for Wizard class.
	if (RealmSectionBox)
	{
		RealmSectionBox->SetVisibility(
			Class == ECoMWizardClass::Wizard ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UCoMWizardCreationWidget::OnRealmToggled(ECoMSpellRealm Realm)
{
	if (SelectedRealms.Contains(Realm))
	{
		SelectedRealms.Remove(Realm);
	}
	else if (SelectedRealms.Num() < 3)
	{
		SelectedRealms.Add(Realm);
	}

	UpdateRealmButtonStyles();
}

void UCoMWizardCreationWidget::OnDifficultySelected(int32 Level)
{
	SelectedDifficulty = FMath::Clamp(Level, 0, 4);
	UpdateDifficultyButtonStyles();
}

void UCoMWizardCreationWidget::OnBackClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->HideWizardCreation();
			UISS->ShowMainMenu();
		}
	}
}

void UCoMWizardCreationWidget::OnStartGameClicked()
{
	// Validate: name must not be empty.
	FText CurrentName = NameInputBox ? NameInputBox->GetText() : FText::GetEmpty();
	if (CurrentName.IsEmpty())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Wizard name cannot be empty!"));
		}
		return;
	}

	UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance());
	if (!CoMGI)
	{
		UE_LOG(LogTemp, Error, TEXT("CoMWizardCreationWidget: Could not get CoMGameInstance."));
		return;
	}

	// Populate settings.
	CoMGI->NewGameSettings = BuildSettings();
	CoMGI->LoadedSaveSlotName.Empty();

	// Transition to the overworld.
	UGameplayStatics::OpenLevel(this, FName(TEXT("L_Overworld")));
}

FCoMNewGameSettings UCoMWizardCreationWidget::BuildSettings() const
{
	FCoMNewGameSettings Settings;

	Settings.WizardName = NameInputBox ? NameInputBox->GetText() : FText::FromString(TEXT("Archmage"));
	Settings.WizardClass = SelectedClass;
	Settings.DifficultyLevel = SelectedDifficulty;

	// Convert selected realms to retort-style FName array for starting spellbook allocation.
	for (const ECoMSpellRealm& Realm : SelectedRealms)
	{
		// Store realm name as FName for the starting spells system.
		const UEnum* EnumPtr = StaticEnum<ECoMSpellRealm>();
		if (EnumPtr)
		{
			FString RealmName = EnumPtr->GetNameStringByValue(static_cast<int64>(Realm));
			Settings.ChosenRetorts.Add(FName(*RealmName));
		}
	}

	return Settings;
}

// =============================================================================
// Layout construction
// =============================================================================

UTextBlock* UCoMWizardCreationWidget::CreateSectionLabel(const FString& Text)
{
	UTextBlock* Label = NewObject<UTextBlock>(this);
	Label->SetText(FText::FromString(Text));
	Label->SetColorAndOpacity(FSlateColor(WizCreationColours::Gold));

	FSlateFontInfo FontInfo = Label->GetFont();
	FontInfo.Size = 18;
	FontInfo.TypefaceFontName = FName(TEXT("Bold"));
	Label->SetFont(FontInfo);

	return Label;
}

UButton* UCoMWizardCreationWidget::CreateStyledButton(float Width, float Height)
{
	UButton* Button = NewObject<UButton>(this);

	FButtonStyle Style = Button->GetStyle();

	Style.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
	Style.Normal.TintColor = FSlateColor(WizCreationColours::DarkButton);
	Style.Normal.OutlineSettings.Color = FSlateColor(WizCreationColours::GoldDim);
	Style.Normal.OutlineSettings.Width = 1.0f;

	Style.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
	Style.Hovered.TintColor = FSlateColor(WizCreationColours::ButtonHover);
	Style.Hovered.OutlineSettings.Color = FSlateColor(WizCreationColours::Gold);
	Style.Hovered.OutlineSettings.Width = 1.0f;

	Style.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
	Style.Pressed.TintColor = FSlateColor(WizCreationColours::DarkButton);
	Style.Pressed.OutlineSettings.Color = FSlateColor(WizCreationColours::Gold);
	Style.Pressed.OutlineSettings.Width = 1.0f;

	Button->SetStyle(Style);

	return Button;
}

void UCoMWizardCreationWidget::UpdateClassButtonStyles()
{
	UButton* ClassButtons[] = { ClassWizardButton, ClassPsykerButton, ClassWarlockButton };
	ECoMWizardClass Classes[] = { ECoMWizardClass::Wizard, ECoMWizardClass::Psyker, ECoMWizardClass::Warlock };

	for (int32 i = 0; i < 3; ++i)
	{
		if (!ClassButtons[i]) continue;

		FButtonStyle Style = ClassButtons[i]->GetStyle();
		bool bSelected = (Classes[i] == SelectedClass);

		FLinearColor OutlineColor = bSelected ? WizCreationColours::Gold : WizCreationColours::GoldDim;
		float OutlineWidth = bSelected ? 2.0f : 1.0f;
		FLinearColor BgColor = bSelected ? WizCreationColours::PanelBg : WizCreationColours::DarkButton;

		Style.Normal.TintColor = FSlateColor(BgColor);
		Style.Normal.OutlineSettings.Color = FSlateColor(OutlineColor);
		Style.Normal.OutlineSettings.Width = OutlineWidth;

		Style.Hovered.TintColor = FSlateColor(WizCreationColours::ButtonHover);
		Style.Hovered.OutlineSettings.Color = FSlateColor(WizCreationColours::Gold);
		Style.Hovered.OutlineSettings.Width = 2.0f;

		Style.Pressed.TintColor = FSlateColor(BgColor);
		Style.Pressed.OutlineSettings.Color = FSlateColor(WizCreationColours::Gold);
		Style.Pressed.OutlineSettings.Width = 2.0f;

		ClassButtons[i]->SetStyle(Style);
	}
}

void UCoMWizardCreationWidget::UpdateDifficultyButtonStyles()
{
	for (int32 i = 0; i < 5; ++i)
	{
		if (!DifficultyButtons[i]) continue;

		FButtonStyle Style = DifficultyButtons[i]->GetStyle();
		bool bSelected = (i == SelectedDifficulty);

		FLinearColor OutlineColor = bSelected ? WizCreationColours::Gold : WizCreationColours::GoldDim;
		float OutlineWidth = bSelected ? 2.0f : 1.0f;
		FLinearColor BgColor = bSelected ? WizCreationColours::PanelBg : WizCreationColours::DarkButton;

		Style.Normal.TintColor = FSlateColor(BgColor);
		Style.Normal.OutlineSettings.Color = FSlateColor(OutlineColor);
		Style.Normal.OutlineSettings.Width = OutlineWidth;

		Style.Hovered.OutlineSettings.Color = FSlateColor(WizCreationColours::Gold);
		Style.Hovered.OutlineSettings.Width = 2.0f;

		DifficultyButtons[i]->SetStyle(Style);
	}
}

void UCoMWizardCreationWidget::UpdateRealmButtonStyles()
{
	for (int32 i = 0; i < 9; ++i)
	{
		if (!RealmButtons[i] || !RealmButtonBorders[i]) continue;

		bool bSelected = SelectedRealms.Contains(GRealmInfos[i].Realm);

		FButtonStyle Style = RealmButtons[i]->GetStyle();

		FLinearColor OutlineColor = bSelected ? GRealmInfos[i].Color : WizCreationColours::GoldDim;
		float OutlineWidth = bSelected ? 2.0f : 1.0f;
		FLinearColor BgColor = bSelected ? WizCreationColours::PanelBg : WizCreationColours::DarkButton;

		Style.Normal.TintColor = FSlateColor(BgColor);
		Style.Normal.OutlineSettings.Color = FSlateColor(OutlineColor);
		Style.Normal.OutlineSettings.Width = OutlineWidth;

		Style.Hovered.OutlineSettings.Color = FSlateColor(GRealmInfos[i].Color);
		Style.Hovered.OutlineSettings.Width = 2.0f;

		RealmButtons[i]->SetStyle(Style);

		// Update the colour swatch border.
		RealmButtonBorders[i]->SetBrushColor(bSelected
			? GRealmInfos[i].Color
			: FLinearColor(GRealmInfos[i].Color.R, GRealmInfos[i].Color.G, GRealmInfos[i].Color.B, 0.4f));
	}
}

// =============================================================================
// Full layout build
// =============================================================================

void UCoMWizardCreationWidget::BuildLayout()
{
	// -- Full-screen dark background ------------------------------------------

	BackgroundBorder = NewObject<UBorder>(this);
	BackgroundBorder->SetBrushColor(WizCreationColours::Background);
	BackgroundBorder->SetPadding(FMargin(0.0f));

	// -- Center overlay -------------------------------------------------------

	UOverlay* RootOverlay = NewObject<UOverlay>(this);
	BackgroundBorder->AddChild(RootOverlay);

	// -- Scrollable content panel (700px wide) --------------------------------

	USizeBox* PanelSizeBox = NewObject<USizeBox>(this);
	PanelSizeBox->SetWidthOverride(700.0f);

	UOverlaySlot* PanelSlot = RootOverlay->AddChildToOverlay(PanelSizeBox);
	if (PanelSlot)
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	// Panel background border.
	UBorder* PanelBorder = NewObject<UBorder>(this);
	PanelBorder->SetBrushColor(WizCreationColours::PanelBg);
	PanelBorder->SetPadding(FMargin(32.0f, 24.0f));
	PanelSizeBox->AddChild(PanelBorder);

	// Scroll box for the panel contents (in case screen is small).
	UScrollBox* PanelScroll = NewObject<UScrollBox>(this);
	PanelScroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	PanelScroll->SetOrientation(Orient_Vertical);
	PanelBorder->AddChild(PanelScroll);

	ContentBox = NewObject<UVerticalBox>(this);
	PanelScroll->AddChild(ContentBox);

	// -- Set root widget ------------------------------------------------------

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}

	// =========================================================================
	// HEADER: "Create Your Wizard"
	// =========================================================================

	{
		UTextBlock* Header = NewObject<UTextBlock>(this);
		Header->SetText(FText::FromString(TEXT("Create Your Wizard")));
		Header->SetColorAndOpacity(FSlateColor(WizCreationColours::Gold));
		Header->SetJustification(ETextJustify::Center);

		FSlateFontInfo FontInfo = Header->GetFont();
		FontInfo.Size = 32;
		FontInfo.TypefaceFontName = FName(TEXT("Bold"));
		Header->SetFont(FontInfo);

		UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(Header);
		if (Slot)
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		}
	}

	// -- Gold separator -------------------------------------------------------

	{
		UImage* Sep = NewObject<UImage>(this);
		Sep->SetColorAndOpacity(WizCreationColours::Gold);
		Sep->SetDesiredSizeOverride(FVector2D(500.0f, 2.0f));

		UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(Sep);
		if (Slot)
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}
	}

	// =========================================================================
	// NAME SECTION
	// =========================================================================

	{
		UTextBlock* NameLabel = CreateSectionLabel(TEXT("Wizard Name"));
		UVerticalBoxSlot* LabelSlot = ContentBox->AddChildToVerticalBox(NameLabel);
		if (LabelSlot)
		{
			LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		NameInputBox = NewObject<UEditableTextBox>(this);
		NameInputBox->SetText(FText::FromString(TEXT("Archmage")));

		// Style the text box via WidgetStyle property.
		FEditableTextBoxStyle& TextBoxStyle = const_cast<FEditableTextBoxStyle&>(NameInputBox->WidgetStyle);
		TextBoxStyle.BackgroundImageNormal.TintColor = FSlateColor(WizCreationColours::DarkButton);
		TextBoxStyle.BackgroundImageHovered.TintColor = FSlateColor(WizCreationColours::ButtonHover);
		TextBoxStyle.BackgroundImageFocused.TintColor = FSlateColor(WizCreationColours::DarkButton);
		TextBoxStyle.ForegroundColor = FSlateColor(WizCreationColours::White);

		FSlateFontInfo InputFont = TextBoxStyle.TextStyle.Font;
		InputFont.Size = 16;
		TextBoxStyle.TextStyle.Font = InputFont;
		TextBoxStyle.TextStyle.ColorAndOpacity = FSlateColor(WizCreationColours::White);

		UVerticalBoxSlot* InputSlot = ContentBox->AddChildToVerticalBox(NameInputBox);
		if (InputSlot)
		{
			InputSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}
	}

	// =========================================================================
	// CLASS SECTION
	// =========================================================================

	{
		UTextBlock* ClassLabel = CreateSectionLabel(TEXT("Wizard Class"));
		UVerticalBoxSlot* LabelSlot = ContentBox->AddChildToVerticalBox(ClassLabel);
		if (LabelSlot)
		{
			LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		UHorizontalBox* ClassRow = NewObject<UHorizontalBox>(this);

		struct FClassInfo
		{
			FString Name;
			FString Desc;
		};

		FClassInfo ClassInfos[] =
		{
			{ TEXT("Wizard"),  TEXT("Master of spellbooks and research") },
			{ TEXT("Psyker"),  TEXT("Innate psychic powers, no spellbook") },
			{ TEXT("Warlock"), TEXT("Pact magic, entity bargaining, soul harvest") },
		};

		UButton** ClassButtonPtrs[] = { &ClassWizardButton, &ClassPsykerButton, &ClassWarlockButton };

		for (int32 i = 0; i < 3; ++i)
		{
			USizeBox* BtnSizeBox = NewObject<USizeBox>(this);
			BtnSizeBox->SetWidthOverride(200.0f);
			BtnSizeBox->SetHeightOverride(120.0f);

			UButton* Btn = CreateStyledButton(200.0f, 120.0f);
			*ClassButtonPtrs[i] = Btn;

			// Button content: vertical box with name + description.
			UVerticalBox* BtnContent = NewObject<UVerticalBox>(this);

			// Class name.
			UTextBlock* NameText = NewObject<UTextBlock>(this);
			NameText->SetText(FText::FromString(ClassInfos[i].Name));
			NameText->SetColorAndOpacity(FSlateColor(WizCreationColours::White));
			NameText->SetJustification(ETextJustify::Center);
			FSlateFontInfo NameFont = NameText->GetFont();
			NameFont.Size = 16;
			NameFont.TypefaceFontName = FName(TEXT("Bold"));
			NameText->SetFont(NameFont);

			UVerticalBoxSlot* NameSlot = BtnContent->AddChildToVerticalBox(NameText);
			if (NameSlot)
			{
				NameSlot->SetHorizontalAlignment(HAlign_Center);
				NameSlot->SetPadding(FMargin(8.0f, 16.0f, 8.0f, 4.0f));
			}

			// Description.
			UTextBlock* DescText = NewObject<UTextBlock>(this);
			DescText->SetText(FText::FromString(ClassInfos[i].Desc));
			DescText->SetColorAndOpacity(FSlateColor(WizCreationColours::Grey));
			DescText->SetJustification(ETextJustify::Center);
			DescText->SetAutoWrapText(true);
			FSlateFontInfo DescFont = DescText->GetFont();
			DescFont.Size = 10;
			DescText->SetFont(DescFont);

			UVerticalBoxSlot* DescSlot = BtnContent->AddChildToVerticalBox(DescText);
			if (DescSlot)
			{
				DescSlot->SetHorizontalAlignment(HAlign_Center);
				DescSlot->SetPadding(FMargin(8.0f, 0.0f, 8.0f, 8.0f));
			}

			Btn->AddChild(BtnContent);
			BtnSizeBox->AddChild(Btn);

			UHorizontalBoxSlot* HSlot = ClassRow->AddChildToHorizontalBox(BtnSizeBox);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(4.0f));
				HSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		// Bind click events.
		ClassWizardButton->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnClassWizardClicked);
		ClassPsykerButton->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnClassPsykerClicked);
		ClassWarlockButton->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnClassWarlockClicked);

		UVerticalBoxSlot* RowSlot = ContentBox->AddChildToVerticalBox(ClassRow);
		if (RowSlot)
		{
			RowSlot->SetHorizontalAlignment(HAlign_Center);
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}
	}

	// Apply default class selection style.
	UpdateClassButtonStyles();

	// =========================================================================
	// REALM AFFINITY SECTION (Wizard class only)
	// =========================================================================

	{
		RealmSectionBox = NewObject<UVerticalBox>(this);

		UTextBlock* RealmLabel = CreateSectionLabel(TEXT("Spell Realm Affinity (select up to 3)"));
		UVerticalBoxSlot* LabelSlot = RealmSectionBox->AddChildToVerticalBox(RealmLabel);
		if (LabelSlot)
		{
			LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		// 3x3 grid of realm buttons.
		// Callback function pointers in order.
		typedef void (UCoMWizardCreationWidget::*FRealmCallback)();
		FRealmCallback RealmCallbacks[] =
		{
			&UCoMWizardCreationWidget::OnRealmLifeClicked,
			&UCoMWizardCreationWidget::OnRealmDeathClicked,
			&UCoMWizardCreationWidget::OnRealmChaosClicked,
			&UCoMWizardCreationWidget::OnRealmNatureClicked,
			&UCoMWizardCreationWidget::OnRealmSorceryClicked,
			&UCoMWizardCreationWidget::OnRealmArcaneClicked,
			&UCoMWizardCreationWidget::OnRealmBindingClicked,
			&UCoMWizardCreationWidget::OnRealmSpiritClicked,
			&UCoMWizardCreationWidget::OnRealmGlamourClicked,
		};

		for (int32 Row = 0; Row < 3; ++Row)
		{
			UHorizontalBox* RowBox = NewObject<UHorizontalBox>(this);

			for (int32 Col = 0; Col < 3; ++Col)
			{
				int32 Idx = Row * 3 + Col;

				USizeBox* BtnSize = NewObject<USizeBox>(this);
				BtnSize->SetWidthOverride(140.0f);
				BtnSize->SetHeightOverride(50.0f);

				UButton* Btn = CreateStyledButton(140.0f, 50.0f);
				RealmButtons[Idx] = Btn;

				// Button content: horizontal box with colour swatch + name.
				UHorizontalBox* BtnContent = NewObject<UHorizontalBox>(this);

				// Colour swatch.
				UBorder* Swatch = NewObject<UBorder>(this);
				Swatch->SetBrushColor(FLinearColor(
					GRealmInfos[Idx].Color.R, GRealmInfos[Idx].Color.G, GRealmInfos[Idx].Color.B, 0.4f));
				Swatch->SetDesiredSizeScale(FVector2D(1.0f, 1.0f));
				RealmButtonBorders[Idx] = Swatch;

				USizeBox* SwatchSize = NewObject<USizeBox>(this);
				SwatchSize->SetWidthOverride(16.0f);
				SwatchSize->SetHeightOverride(16.0f);
				SwatchSize->AddChild(Swatch);

				UHorizontalBoxSlot* SwatchSlot = BtnContent->AddChildToHorizontalBox(SwatchSize);
				if (SwatchSlot)
				{
					SwatchSlot->SetVerticalAlignment(VAlign_Center);
					SwatchSlot->SetPadding(FMargin(8.0f, 0.0f, 6.0f, 0.0f));
				}

				// Realm name.
				UTextBlock* RealmName = NewObject<UTextBlock>(this);
				RealmName->SetText(FText::FromString(GRealmInfos[Idx].Name));
				RealmName->SetColorAndOpacity(FSlateColor(WizCreationColours::LightGrey));
				FSlateFontInfo RealmFont = RealmName->GetFont();
				RealmFont.Size = 12;
				RealmName->SetFont(RealmFont);

				UHorizontalBoxSlot* NameSlot = BtnContent->AddChildToHorizontalBox(RealmName);
				if (NameSlot)
				{
					NameSlot->SetVerticalAlignment(VAlign_Center);
				}

				Btn->AddChild(BtnContent);
				BtnSize->AddChild(Btn);

				Btn->OnClicked.AddDynamic(this, RealmCallbacks[Idx]);

				UHorizontalBoxSlot* ColSlot = RowBox->AddChildToHorizontalBox(BtnSize);
				if (ColSlot)
				{
					ColSlot->SetPadding(FMargin(4.0f, 2.0f));
				}
			}

			UVerticalBoxSlot* GridRowSlot = RealmSectionBox->AddChildToVerticalBox(RowBox);
			if (GridRowSlot)
			{
				GridRowSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		// Add realm section to main content.
		UVerticalBoxSlot* RealmSecSlot = ContentBox->AddChildToVerticalBox(RealmSectionBox);
		if (RealmSecSlot)
		{
			RealmSecSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}
	}

	UpdateRealmButtonStyles();

	// =========================================================================
	// DIFFICULTY SECTION
	// =========================================================================

	{
		UTextBlock* DiffLabel = CreateSectionLabel(TEXT("Difficulty"));
		UVerticalBoxSlot* LabelSlot = ContentBox->AddChildToVerticalBox(DiffLabel);
		if (LabelSlot)
		{
			LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		UHorizontalBox* DiffRow = NewObject<UHorizontalBox>(this);

		typedef void (UCoMWizardCreationWidget::*FDiffCallback)();
		FDiffCallback DiffCallbacks[] =
		{
			&UCoMWizardCreationWidget::OnDiffEasyClicked,
			&UCoMWizardCreationWidget::OnDiffNormalClicked,
			&UCoMWizardCreationWidget::OnDiffHardClicked,
			&UCoMWizardCreationWidget::OnDiffLunaticClicked,
			&UCoMWizardCreationWidget::OnDiffImpossibleClicked,
		};

		for (int32 i = 0; i < 5; ++i)
		{
			USizeBox* BtnSize = NewObject<USizeBox>(this);
			BtnSize->SetWidthOverride(120.0f);
			BtnSize->SetHeightOverride(70.0f);

			UButton* Btn = CreateStyledButton(120.0f, 70.0f);
			DifficultyButtons[i] = Btn;

			// Content: name + description.
			UVerticalBox* BtnContent = NewObject<UVerticalBox>(this);

			UTextBlock* DiffName = NewObject<UTextBlock>(this);
			DiffName->SetText(FText::FromString(GDifficultyInfos[i].Name));
			DiffName->SetColorAndOpacity(FSlateColor(WizCreationColours::White));
			DiffName->SetJustification(ETextJustify::Center);
			FSlateFontInfo DNameFont = DiffName->GetFont();
			DNameFont.Size = 12;
			DNameFont.TypefaceFontName = FName(TEXT("Bold"));
			DiffName->SetFont(DNameFont);

			UVerticalBoxSlot* DNameSlot = BtnContent->AddChildToVerticalBox(DiffName);
			if (DNameSlot)
			{
				DNameSlot->SetHorizontalAlignment(HAlign_Center);
				DNameSlot->SetPadding(FMargin(4.0f, 8.0f, 4.0f, 2.0f));
			}

			UTextBlock* DiffDesc = NewObject<UTextBlock>(this);
			DiffDesc->SetText(FText::FromString(GDifficultyInfos[i].Description));
			DiffDesc->SetColorAndOpacity(FSlateColor(WizCreationColours::Grey));
			DiffDesc->SetJustification(ETextJustify::Center);
			DiffDesc->SetAutoWrapText(true);
			FSlateFontInfo DDescFont = DiffDesc->GetFont();
			DDescFont.Size = 8;
			DiffDesc->SetFont(DDescFont);

			UVerticalBoxSlot* DDescSlot = BtnContent->AddChildToVerticalBox(DiffDesc);
			if (DDescSlot)
			{
				DDescSlot->SetHorizontalAlignment(HAlign_Center);
				DDescSlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 4.0f));
			}

			Btn->AddChild(BtnContent);
			BtnSize->AddChild(Btn);

			Btn->OnClicked.AddDynamic(this, DiffCallbacks[i]);

			UHorizontalBoxSlot* HSlot = DiffRow->AddChildToHorizontalBox(BtnSize);
			if (HSlot)
			{
				HSlot->SetPadding(FMargin(2.0f));
			}
		}

		UVerticalBoxSlot* DiffRowSlot = ContentBox->AddChildToVerticalBox(DiffRow);
		if (DiffRowSlot)
		{
			DiffRowSlot->SetHorizontalAlignment(HAlign_Center);
			DiffRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
		}
	}

	UpdateDifficultyButtonStyles();

	// =========================================================================
	// BOTTOM BUTTONS: Back + Start Game
	// =========================================================================

	{
		UImage* Sep = NewObject<UImage>(this);
		Sep->SetColorAndOpacity(WizCreationColours::Gold);
		Sep->SetDesiredSizeOverride(FVector2D(500.0f, 2.0f));

		UVerticalBoxSlot* SepSlot = ContentBox->AddChildToVerticalBox(Sep);
		if (SepSlot)
		{
			SepSlot->SetHorizontalAlignment(HAlign_Center);
			SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		}

		UHorizontalBox* BottomRow = NewObject<UHorizontalBox>(this);

		// -- Back button --
		{
			USizeBox* BackSize = NewObject<USizeBox>(this);
			BackSize->SetWidthOverride(150.0f);
			BackSize->SetHeightOverride(45.0f);

			BackButton = CreateStyledButton(150.0f, 45.0f);

			UTextBlock* BackLabel = NewObject<UTextBlock>(this);
			BackLabel->SetText(FText::FromString(TEXT("Back")));
			BackLabel->SetColorAndOpacity(FSlateColor(WizCreationColours::LightGrey));
			BackLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo BackFont = BackLabel->GetFont();
			BackFont.Size = 16;
			BackLabel->SetFont(BackFont);

			BackButton->AddChild(BackLabel);
			BackSize->AddChild(BackButton);

			BackButton->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnBackClicked);

			UHorizontalBoxSlot* HSlot = BottomRow->AddChildToHorizontalBox(BackSize);
			if (HSlot)
			{
				HSlot->SetHorizontalAlignment(HAlign_Left);
				HSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			}
		}

		// -- Spacer to push Start Game to the right --
		{
			USpacer* SpacerWidget = NewObject<USpacer>(this);
			SpacerWidget->SetSize(FVector2D(1.0f, 1.0f));
			UHorizontalBoxSlot* SpSlot = BottomRow->AddChildToHorizontalBox(SpacerWidget);
			if (SpSlot)
			{
				SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); // Fill
			}
		}

		// -- Start Game button (gold accent) --
		{
			USizeBox* StartSize = NewObject<USizeBox>(this);
			StartSize->SetWidthOverride(200.0f);
			StartSize->SetHeightOverride(45.0f);

			StartGameButton = NewObject<UButton>(this);

			FButtonStyle StartStyle = StartGameButton->GetStyle();

			StartStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
			StartStyle.Normal.TintColor = FSlateColor(WizCreationColours::Gold);
			StartStyle.Normal.OutlineSettings.Color = FSlateColor(WizCreationColours::Gold);
			StartStyle.Normal.OutlineSettings.Width = 2.0f;

			StartStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
			StartStyle.Hovered.TintColor = FSlateColor(
				FLinearColor(WizCreationColours::Gold.R * 1.2f, WizCreationColours::Gold.G * 1.2f,
				             WizCreationColours::Gold.B * 1.2f, 1.0f));
			StartStyle.Hovered.OutlineSettings.Color = FSlateColor(WizCreationColours::White);
			StartStyle.Hovered.OutlineSettings.Width = 2.0f;

			StartStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
			StartStyle.Pressed.TintColor = FSlateColor(
				FLinearColor(WizCreationColours::Gold.R * 0.8f, WizCreationColours::Gold.G * 0.8f,
				             WizCreationColours::Gold.B * 0.8f, 1.0f));
			StartStyle.Pressed.OutlineSettings.Color = FSlateColor(WizCreationColours::Gold);
			StartStyle.Pressed.OutlineSettings.Width = 2.0f;

			StartGameButton->SetStyle(StartStyle);

			UTextBlock* StartLabel = NewObject<UTextBlock>(this);
			StartLabel->SetText(FText::FromString(TEXT("Start Game")));
			StartLabel->SetColorAndOpacity(FSlateColor(WizCreationColours::Background));
			StartLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo StartFont = StartLabel->GetFont();
			StartFont.Size = 16;
			StartFont.TypefaceFontName = FName(TEXT("Bold"));
			StartLabel->SetFont(StartFont);

			StartGameButton->AddChild(StartLabel);
			StartSize->AddChild(StartGameButton);

			StartGameButton->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnStartGameClicked);

			UHorizontalBoxSlot* HSlot = BottomRow->AddChildToHorizontalBox(StartSize);
			if (HSlot)
			{
				HSlot->SetHorizontalAlignment(HAlign_Right);
				HSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			}
		}

		UVerticalBoxSlot* BottomSlot = ContentBox->AddChildToVerticalBox(BottomRow);
		if (BottomSlot)
		{
			BottomSlot->SetHorizontalAlignment(HAlign_Fill);
		}
	}
}
