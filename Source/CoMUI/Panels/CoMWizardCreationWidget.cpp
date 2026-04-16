// Copyright Mythforge Studios. All Rights Reserved.
// CoMWizardCreationWidget.cpp -- Screen 1: Wizard portrait selection grid.

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
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

#include "CoMUISubsystem.h"

// =============================================================================
// Colour constants
// =============================================================================

namespace WizPortraitColours
{
	static const FLinearColor Background   = FLinearColor(0.039f, 0.039f, 0.102f, 1.0f);
	static const FLinearColor Gold         = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim      = FLinearColor(0.855f, 0.647f, 0.125f, 0.4f);
	static const FLinearColor GoldBright   = FLinearColor(1.0f, 0.82f, 0.2f, 1.0f);
	static const FLinearColor White        = FLinearColor::White;
	static const FLinearColor LightGrey    = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
	static const FLinearColor DarkButton   = FLinearColor(0.06f, 0.06f, 0.15f, 1.0f);
	static const FLinearColor ButtonHover  = FLinearColor(0.102f, 0.165f, 0.306f, 1.0f);
	static const FLinearColor DimBorder    = FLinearColor(0.3f, 0.25f, 0.1f, 0.3f);
}

// Wizard portrait names
static const FString GWizardNames[] = {
	TEXT("Merlin"), TEXT("Morgana"), TEXT("Zephyros"), TEXT("Hecate"),
	TEXT("Malachar"), TEXT("Lunara"), TEXT("Grimnar"), TEXT("Nekros"),
	TEXT("Gaia"), TEXT("Pyraxis"), TEXT("Glaciel"), TEXT("Aldric"),
	TEXT("Lilith"), TEXT("Solarius")
};

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMWizardCreationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
}

TSharedRef<SWidget> UCoMWizardCreationWidget::RebuildWidget()
{
	BuildLayout();
	return Super::RebuildWidget();
}

// =============================================================================
// Portrait callbacks
// =============================================================================

void UCoMWizardCreationWidget::OnPortrait0()  { SelectPortrait(0); }
void UCoMWizardCreationWidget::OnPortrait1()  { SelectPortrait(1); }
void UCoMWizardCreationWidget::OnPortrait2()  { SelectPortrait(2); }
void UCoMWizardCreationWidget::OnPortrait3()  { SelectPortrait(3); }
void UCoMWizardCreationWidget::OnPortrait4()  { SelectPortrait(4); }
void UCoMWizardCreationWidget::OnPortrait5()  { SelectPortrait(5); }
void UCoMWizardCreationWidget::OnPortrait6()  { SelectPortrait(6); }
void UCoMWizardCreationWidget::OnPortrait7()  { SelectPortrait(7); }
void UCoMWizardCreationWidget::OnPortrait8()  { SelectPortrait(8); }
void UCoMWizardCreationWidget::OnPortrait9()  { SelectPortrait(9); }
void UCoMWizardCreationWidget::OnPortrait10() { SelectPortrait(10); }
void UCoMWizardCreationWidget::OnPortrait11() { SelectPortrait(11); }
void UCoMWizardCreationWidget::OnPortrait12() { SelectPortrait(12); }
void UCoMWizardCreationWidget::OnPortrait13() { SelectPortrait(13); }

// =============================================================================
// Portrait selection — highlight then transition to config screen
// =============================================================================

void UCoMWizardCreationWidget::SelectPortrait(int32 Index)
{
	SelectedPortraitIndex = FMath::Clamp(Index, 0, NumPortraits - 1);
	UpdatePortraitHighlights();

	// Transition to Screen 2 (wizard config) via UISubsystem
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->ShowWizardConfig(SelectedPortraitIndex);
		}
	}
}

void UCoMWizardCreationWidget::UpdatePortraitHighlights()
{
	for (int32 i = 0; i < NumPortraits; ++i)
	{
		if (PortraitBorders[i])
		{
			if (i == SelectedPortraitIndex)
			{
				PortraitBorders[i]->SetBrushColor(WizPortraitColours::GoldBright);
				PortraitBorders[i]->SetPadding(FMargin(3.0f));
			}
			else
			{
				PortraitBorders[i]->SetBrushColor(WizPortraitColours::DimBorder);
				PortraitBorders[i]->SetPadding(FMargin(1.0f));
			}
		}
	}
}

// =============================================================================
// Navigation
// =============================================================================

void UCoMWizardCreationWidget::OnCustomWizardClicked()
{
	// Go to config with no pre-selected portrait (index -1)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->ShowWizardConfig(-1);
		}
	}
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

// =============================================================================
// Full layout build
// =============================================================================

void UCoMWizardCreationWidget::BuildLayout()
{
	// -- Full-screen dark background ------------------------------------------

	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(WizPortraitColours::Background);
	BackgroundBorder->SetPadding(FMargin(0.0f));

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}

	// -- Root overlay for centering content -----------------------------------

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(RootOverlay);

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	UOverlaySlot* ContentSlot = RootOverlay->AddChildToOverlay(ContentBox);
	if (ContentSlot)
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	// =========================================================================
	// TITLE: "CHOOSE YOUR WIZARD"
	// =========================================================================

	{
		UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>();
		Header->SetText(FText::FromString(TEXT("CHOOSE YOUR WIZARD")));
		Header->SetColorAndOpacity(FSlateColor(WizPortraitColours::Gold));
		Header->SetJustification(ETextJustify::Center);

		FSlateFontInfo FontInfo = Header->GetFont();
		FontInfo.Size = 32;
		FontInfo.TypefaceFontName = FName(TEXT("Bold"));
		Header->SetFont(FontInfo);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(Header);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Center);
			SlotRef->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 8.0f));
		}
	}

	// -- Gold separator -------------------------------------------------------

	{
		UImage* Sep = WidgetTree->ConstructWidget<UImage>();
		Sep->SetColorAndOpacity(WizPortraitColours::Gold);
		Sep->SetDesiredSizeOverride(FVector2D(900.0f, 2.0f));

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(Sep);
		if (SlotRef)
		{
			SlotRef->SetHorizontalAlignment(HAlign_Center);
			SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}
	}

	// =========================================================================
	// PORTRAIT GRID: 7 columns x 2 rows = 14 wizards
	// =========================================================================

	{
		UVerticalBox* PortraitGrid = WidgetTree->ConstructWidget<UVerticalBox>();

		for (int32 Row = 0; Row < 2; ++Row)
		{
			UHorizontalBox* PortRow = WidgetTree->ConstructWidget<UHorizontalBox>();

			for (int32 Col = 0; Col < 7; ++Col)
			{
				int32 Idx = Row * 7 + Col;

				// Outer border — changes thickness/color when selected
				PortraitBorders[Idx] = WidgetTree->ConstructWidget<UBorder>();
				PortraitBorders[Idx]->SetBrushColor(WizPortraitColours::DimBorder);
				PortraitBorders[Idx]->SetPadding(FMargin(1.0f));

				// Clickable button
				PortraitButtons[Idx] = WidgetTree->ConstructWidget<UButton>();
				FButtonStyle PStyle = PortraitButtons[Idx]->GetStyle();
				PStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
				PStyle.Normal.TintColor = FSlateColor(FLinearColor(0.04f, 0.03f, 0.08f, 1.0f));
				PStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
				PStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.08f, 0.06f, 0.15f, 1.0f));
				PStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
				PStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.03f, 0.02f, 0.06f, 1.0f));
				PortraitButtons[Idx]->SetStyle(PStyle);

				// Portrait content: image + name label
				UVerticalBox* PortContent = WidgetTree->ConstructWidget<UVerticalBox>();

				// Portrait image (170x170)
				PortraitImages[Idx] = WidgetTree->ConstructWidget<UImage>();
				{
					FString AssetPath = FString::Printf(
						TEXT("/Game/Textures/Wizards/wizard_%02d.wizard_%02d"), Idx + 1, Idx + 1);
					UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *AssetPath);
					FSlateBrush Brush;
					if (Tex)
					{
						Brush.SetResourceObject(Tex);
					}
					else
					{
						Brush.DrawAs = ESlateBrushDrawType::Box;
						float Hue = static_cast<float>(Idx) / 14.0f;
						Brush.TintColor = FSlateColor(FLinearColor::MakeFromHSV8(
							static_cast<uint8>(Hue * 255), 140, 180));
					}
					Brush.ImageSize = FVector2D(170.0f, 170.0f);
					PortraitImages[Idx]->SetBrush(Brush);
				}

				USizeBox* ImgSize = WidgetTree->ConstructWidget<USizeBox>();
				ImgSize->SetWidthOverride(170.0f);
				ImgSize->SetHeightOverride(170.0f);
				ImgSize->AddChild(PortraitImages[Idx]);

				UVerticalBoxSlot* ImgSlot = PortContent->AddChildToVerticalBox(ImgSize);
				if (ImgSlot) { ImgSlot->SetHorizontalAlignment(HAlign_Center); }

				// Wizard name (size 14, centered)
				UTextBlock* NameLabel = WidgetTree->ConstructWidget<UTextBlock>();
				NameLabel->SetText(FText::FromString(GWizardNames[Idx]));
				NameLabel->SetColorAndOpacity(FSlateColor(WizPortraitColours::LightGrey));
				NameLabel->SetJustification(ETextJustify::Center);
				FSlateFontInfo NFont = NameLabel->GetFont();
				NFont.Size = 14;
				NameLabel->SetFont(NFont);

				UVerticalBoxSlot* NSlotRef = PortContent->AddChildToVerticalBox(NameLabel);
				if (NSlotRef) { NSlotRef->SetHorizontalAlignment(HAlign_Center); NSlotRef->SetPadding(FMargin(0, 4, 0, 0)); }

				PortraitButtons[Idx]->AddChild(PortContent);
				PortraitBorders[Idx]->AddChild(PortraitButtons[Idx]);

				// Cell size box: 180x220
				USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>();
				CellSize->SetWidthOverride(180.0f);
				CellSize->SetHeightOverride(220.0f);
				CellSize->AddChild(PortraitBorders[Idx]);

				UHorizontalBoxSlot* CellSlotRef = PortRow->AddChildToHorizontalBox(CellSize);
				if (CellSlotRef) { CellSlotRef->SetPadding(FMargin(5.0f, 4.0f)); }
			}

			UVerticalBoxSlot* RowSlot = PortraitGrid->AddChildToVerticalBox(PortRow);
			if (RowSlot) { RowSlot->SetHorizontalAlignment(HAlign_Center); }
		}

		// Bind portrait clicks
		PortraitButtons[0]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait0);
		PortraitButtons[1]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait1);
		PortraitButtons[2]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait2);
		PortraitButtons[3]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait3);
		PortraitButtons[4]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait4);
		PortraitButtons[5]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait5);
		PortraitButtons[6]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait6);
		PortraitButtons[7]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait7);
		PortraitButtons[8]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait8);
		PortraitButtons[9]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait9);
		PortraitButtons[10]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait10);
		PortraitButtons[11]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait11);
		PortraitButtons[12]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait12);
		PortraitButtons[13]->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnPortrait13);

		UVerticalBoxSlot* GridSlot = ContentBox->AddChildToVerticalBox(PortraitGrid);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}
	}

	// =========================================================================
	// BOTTOM BUTTONS: Custom Wizard + Back
	// =========================================================================

	{
		UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		// -- Back button --
		{
			USizeBox* BackSize = WidgetTree->ConstructWidget<USizeBox>();
			BackSize->SetWidthOverride(150.0f);
			BackSize->SetHeightOverride(44.0f);

			BackButton = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle BackStyle = BackButton->GetStyle();
			BackStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
			BackStyle.Normal.TintColor = FSlateColor(WizPortraitColours::DarkButton);
			BackStyle.Normal.OutlineSettings.Color = FSlateColor(WizPortraitColours::GoldDim);
			BackStyle.Normal.OutlineSettings.Width = 1.0f;
			BackStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
			BackStyle.Hovered.TintColor = FSlateColor(WizPortraitColours::ButtonHover);
			BackStyle.Hovered.OutlineSettings.Color = FSlateColor(WizPortraitColours::Gold);
			BackStyle.Hovered.OutlineSettings.Width = 1.0f;
			BackStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
			BackStyle.Pressed.TintColor = FSlateColor(WizPortraitColours::DarkButton);
			BackButton->SetStyle(BackStyle);

			UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>();
			BackLabel->SetText(FText::FromString(TEXT("Back")));
			BackLabel->SetColorAndOpacity(FSlateColor(WizPortraitColours::LightGrey));
			BackLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo BackFont = BackLabel->GetFont();
			BackFont.Size = 14;
			BackLabel->SetFont(BackFont);

			BackButton->AddChild(BackLabel);
			BackSize->AddChild(BackButton);
			BackButton->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnBackClicked);

			UHorizontalBoxSlot* HSlot = BottomRow->AddChildToHorizontalBox(BackSize);
			if (HSlot) { HSlot->SetPadding(FMargin(0, 0, 12, 0)); }
		}

		// -- Spacer --
		{
			USpacer* SpacerWidget = WidgetTree->ConstructWidget<USpacer>();
			SpacerWidget->SetSize(FVector2D(1.0f, 1.0f));
			UHorizontalBoxSlot* SpSlot = BottomRow->AddChildToHorizontalBox(SpacerWidget);
			if (SpSlot) { SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
		}

		// -- Custom Wizard button --
		{
			USizeBox* CustomSize = WidgetTree->ConstructWidget<USizeBox>();
			CustomSize->SetWidthOverride(200.0f);
			CustomSize->SetHeightOverride(44.0f);

			CustomWizardButton = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle CStyle = CustomWizardButton->GetStyle();
			CStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
			CStyle.Normal.TintColor = FSlateColor(WizPortraitColours::DarkButton);
			CStyle.Normal.OutlineSettings.Color = FSlateColor(WizPortraitColours::Gold);
			CStyle.Normal.OutlineSettings.Width = 1.0f;
			CStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
			CStyle.Hovered.TintColor = FSlateColor(WizPortraitColours::ButtonHover);
			CStyle.Hovered.OutlineSettings.Color = FSlateColor(WizPortraitColours::GoldBright);
			CStyle.Hovered.OutlineSettings.Width = 2.0f;
			CStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
			CStyle.Pressed.TintColor = FSlateColor(WizPortraitColours::DarkButton);
			CustomWizardButton->SetStyle(CStyle);

			UTextBlock* CustomLabel = WidgetTree->ConstructWidget<UTextBlock>();
			CustomLabel->SetText(FText::FromString(TEXT("Custom Wizard")));
			CustomLabel->SetColorAndOpacity(FSlateColor(WizPortraitColours::Gold));
			CustomLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo CustomFont = CustomLabel->GetFont();
			CustomFont.Size = 14;
			CustomFont.TypefaceFontName = FName(TEXT("Bold"));
			CustomLabel->SetFont(CustomFont);

			CustomWizardButton->AddChild(CustomLabel);
			CustomSize->AddChild(CustomWizardButton);
			CustomWizardButton->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnCustomWizardClicked);

			UHorizontalBoxSlot* HSlot = BottomRow->AddChildToHorizontalBox(CustomSize);
			if (HSlot) { HSlot->SetHorizontalAlignment(HAlign_Right); }
		}

		UVerticalBoxSlot* BottomSlot = ContentBox->AddChildToVerticalBox(BottomRow);
		if (BottomSlot)
		{
			BottomSlot->SetPadding(FMargin(80.0f, 0.0f, 80.0f, 16.0f));
		}
	}
}
