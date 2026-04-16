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
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

#include "CoMUISubsystem.h"
#include "Shared/CoMWidgetStyles.h"

static const FString GWizardNames[] = {
	TEXT("Merlin"), TEXT("Morgana"), TEXT("Zephyros"), TEXT("Hecate"),
	TEXT("Malachar"), TEXT("Lunara"), TEXT("Grimnar"), TEXT("Nekros"),
	TEXT("Gaia"), TEXT("Pyraxis"), TEXT("Glaciel"), TEXT("Aldric"),
	TEXT("Lilith"), TEXT("Solarius")
};

// Portrait callbacks
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

void UCoMWizardCreationWidget::SelectPortrait(int32 Index)
{
	SelectedPortraitIndex = FMath::Clamp(Index, 0, NumPortraits - 1);
	UpdatePortraitHighlights();

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
				PortraitBorders[i]->SetBrushColor(FLinearColor(1.0f, 0.82f, 0.2f, 1.0f));
				PortraitBorders[i]->SetPadding(FMargin(3.0f));
			}
			else
			{
				PortraitBorders[i]->SetBrushColor(FLinearColor(0.3f, 0.25f, 0.1f, 0.3f));
				PortraitBorders[i]->SetPadding(FMargin(1.0f));
			}
		}
	}
}

void UCoMWizardCreationWidget::OnCustomWizardClicked()
{
	if (UGameInstance* GI = GetGameInstance())
		if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
			UISS->ShowWizardConfig(-1);
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

TSharedRef<SWidget> UCoMWizardCreationWidget::RebuildWidget()
{
	if (WidgetTree) { BuildLayout(); }
	return Super::RebuildWidget();
}

void UCoMWizardCreationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->bShowMouseCursor = true;
			PC->SetInputMode(FInputModeUIOnly().SetWidgetToFocus(TakeWidget()));
		}
	}
}

void UCoMWizardCreationWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(CoMStyle::BgDarkest);
	BackgroundBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(RootOverlay);

	// Scrollable content so large portraits fit
	UScrollBox* ScrollRoot = WidgetTree->ConstructWidget<UScrollBox>();
	UOverlaySlot* ScrollSlotRef = RootOverlay->AddChildToOverlay(ScrollRoot);
	if (ScrollSlotRef)
	{
		ScrollSlotRef->SetHorizontalAlignment(HAlign_Center);
		ScrollSlotRef->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ScrollRoot->AddChild(ContentBox);

	// Title
	{
		UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>();
		Header->SetText(FText::FromString(TEXT("CHOOSE YOUR WIZARD")));
		Header->SetColorAndOpacity(FSlateColor(CoMStyle::GoldBright));
		Header->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = Header->GetFont();
		Font.Size = 36;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		Header->SetFont(Font);
		Header->SetShadowOffset(FVector2D(2, 2));
		Header->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.6f));

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(Header);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 20, 0, 15)); }
	}

	// Portrait grid: 4 columns x 4 rows (14 used, 2 empty)
	for (int32 Row = 0; Row < 4; ++Row)
	{
		UHorizontalBox* PortRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		for (int32 Col = 0; Col < 4; ++Col)
		{
			int32 Idx = Row * 4 + Col;
			if (Idx >= NumPortraits)
			{
				// Empty cell spacer
				USizeBox* EmptyCell = WidgetTree->ConstructWidget<USizeBox>();
				EmptyCell->SetWidthOverride(340.0f);
				EmptyCell->SetHeightOverride(400.0f);
				PortRow->AddChildToHorizontalBox(EmptyCell);
				continue;
			}

			// Gold border frame
			PortraitBorders[Idx] = WidgetTree->ConstructWidget<UBorder>();
			PortraitBorders[Idx]->SetBrushColor(FLinearColor(0.3f, 0.25f, 0.1f, 0.3f));
			PortraitBorders[Idx]->SetPadding(FMargin(1.0f));

			// Clickable button
			PortraitButtons[Idx] = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle PStyle = PortraitButtons[Idx]->GetStyle();
			PStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
			PStyle.Normal.TintColor = FSlateColor(CoMStyle::BgDark);
			PStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
			PStyle.Hovered.TintColor = FSlateColor(CoMStyle::BgMid);
			PStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
			PStyle.Pressed.TintColor = FSlateColor(CoMStyle::BgDarkest);
			PortraitButtons[Idx]->SetStyle(PStyle);

			UVerticalBox* PortContent = WidgetTree->ConstructWidget<UVerticalBox>();

			// Portrait image — 320x320
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
				Brush.ImageSize = FVector2D(320.0f, 320.0f);
				PortraitImages[Idx]->SetBrush(Brush);
			}

			USizeBox* ImgSize = WidgetTree->ConstructWidget<USizeBox>();
			ImgSize->SetWidthOverride(320.0f);
			ImgSize->SetHeightOverride(320.0f);
			ImgSize->AddChild(PortraitImages[Idx]);
			PortContent->AddChildToVerticalBox(ImgSize);

			// Wizard name
			UTextBlock* NameLabel = WidgetTree->ConstructWidget<UTextBlock>();
			NameLabel->SetText(FText::FromString(GWizardNames[Idx]));
			NameLabel->SetColorAndOpacity(FSlateColor(CoMStyle::TextSilver));
			NameLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo NFont = NameLabel->GetFont();
			NFont.Size = 16;
			NameLabel->SetFont(NFont);
			NameLabel->SetShadowOffset(FVector2D(1, 1));
			NameLabel->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.5f));
			UVerticalBoxSlot* NSlotRef = PortContent->AddChildToVerticalBox(NameLabel);
			if (NSlotRef) { NSlotRef->SetHorizontalAlignment(HAlign_Center); NSlotRef->SetPadding(FMargin(0, 4, 0, 0)); }

			PortraitButtons[Idx]->AddChild(PortContent);
			PortraitBorders[Idx]->AddChild(PortraitButtons[Idx]);

			USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>();
			CellSize->SetWidthOverride(340.0f);
			CellSize->SetHeightOverride(400.0f);
			CellSize->AddChild(PortraitBorders[Idx]);

			UHorizontalBoxSlot* CellSlotRef = PortRow->AddChildToHorizontalBox(CellSize);
			if (CellSlotRef) { CellSlotRef->SetPadding(FMargin(6.0f, 4.0f)); }
		}

		UVerticalBoxSlot* RowSlotRef = ContentBox->AddChildToVerticalBox(PortRow);
		if (RowSlotRef) { RowSlotRef->SetHorizontalAlignment(HAlign_Center); }
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

	// Bottom buttons
	{
		UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		USizeBox* BackContainer = nullptr;
		BackButton = CoMStyle::CreateStyledButton(WidgetTree, TEXT("Back"), 160.f, 44.f, 14, &BackContainer);
		BackButton->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnBackClicked);
		if (BackContainer)
		{
			UHorizontalBoxSlot* BSlotRef = BottomRow->AddChildToHorizontalBox(BackContainer);
			if (BSlotRef) { BSlotRef->SetPadding(FMargin(0, 0, 20, 0)); }
		}

		USizeBox* CustomContainer = nullptr;
		CustomWizardButton = CoMStyle::CreatePrimaryButton(WidgetTree, TEXT("Custom Wizard"), 200.f, 48.f, 16, &CustomContainer);
		CustomWizardButton->OnClicked.AddDynamic(this, &UCoMWizardCreationWidget::OnCustomWizardClicked);
		if (CustomContainer)
		{
			BottomRow->AddChildToHorizontalBox(CustomContainer);
		}

		UVerticalBoxSlot* BotSlotRef = ContentBox->AddChildToVerticalBox(BottomRow);
		if (BotSlotRef) { BotSlotRef->SetHorizontalAlignment(HAlign_Center); BotSlotRef->SetPadding(FMargin(0, 15, 0, 20)); }
	}
}
