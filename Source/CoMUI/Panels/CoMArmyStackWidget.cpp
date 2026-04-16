// Copyright Mythforge Studios. All Rights Reserved.
// CoMArmyStackWidget.cpp -- Army stack grid implementation.

#include "CoMArmyStackWidget.h"

#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMUI/CoMUISubsystem.h"

// =============================================================================
// Colour palette — dark fantasy army theme
// =============================================================================

namespace AS
{
	static const FLinearColor BgDark        = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f);  // #0e0e1a
	static const FLinearColor PanelBg       = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold          = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);   // #daa520
	static const FLinearColor GoldDim       = FLinearColor(0.500f, 0.380f, 0.080f, 0.7f);
	static const FLinearColor Silver        = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor White         = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	static const FLinearColor Grey          = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor EmptySlot     = FLinearColor(0.08f, 0.07f, 0.12f, 1.0f);

	static const FLinearColor BtnNormal     = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover      = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPressed    = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);
	static const FLinearColor BtnBorder     = FLinearColor(0.500f, 0.380f, 0.080f, 0.7f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMArmyStackWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildArmyLayout();
	}
	return Super::RebuildWidget();
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMArmyStackWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	// Bind slot click callbacks.
	if (SlotButtons[0]) { SlotButtons[0]->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnSlot0Clicked); }
	if (SlotButtons[1]) { SlotButtons[1]->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnSlot1Clicked); }
	if (SlotButtons[2]) { SlotButtons[2]->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnSlot2Clicked); }
	if (SlotButtons[3]) { SlotButtons[3]->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnSlot3Clicked); }
	if (SlotButtons[4]) { SlotButtons[4]->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnSlot4Clicked); }
	if (SlotButtons[5]) { SlotButtons[5]->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnSlot5Clicked); }
	if (SlotButtons[6]) { SlotButtons[6]->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnSlot6Clicked); }
	if (SlotButtons[7]) { SlotButtons[7]->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnSlot7Clicked); }
	if (SlotButtons[8]) { SlotButtons[8]->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnSlot8Clicked); }

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UCoMArmyStackWidget::OnCloseClicked);
	}
}

// =============================================================================
// Subsystem access
// =============================================================================

UCoMUnitSubsystem* UCoMArmyStackWidget::GetUnitSubsystem()
{
	if (CachedUnitSubsystem.IsValid())
	{
		return CachedUnitSubsystem.Get();
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI)
	{
		UCoMUnitSubsystem* Subsystem = GI->GetSubsystem<UCoMUnitSubsystem>();
		CachedUnitSubsystem = Subsystem;
		return Subsystem;
	}
	return nullptr;
}

// =============================================================================
// Public API
// =============================================================================

void UCoMArmyStackWidget::SetArmy(int32 ArmyId)
{
	CurrentArmyId = ArmyId;

	// Reset all slots.
	for (int32 i = 0; i < MAX_SLOTS; ++i)
	{
		SlotUnitIds[i] = -1;
		if (SlotPortraits[i]) { SlotPortraits[i]->SetBrushColor(AS::EmptySlot); }
		if (SlotLabels[i])    { SlotLabels[i]->SetText(FText::GetEmpty()); }
	}

	UCoMUnitSubsystem* UnitSub = GetUnitSubsystem();
	if (!UnitSub)
	{
		return;
	}

	const FCoMArmyGroup* Army = UnitSub->GetArmy(ArmyId);
	if (!Army)
	{
		return;
	}

	// Header
	if (HeaderText)
	{
		HeaderText->SetText(FText::FromString(
			FString::Printf(TEXT("Army - Group %d"), Army->ArmyGroupID)));
	}

	// Movement
	if (MovementText)
	{
		MovementText->SetText(FText::FromString(
			FString::Printf(TEXT("%d moves left"), Army->MovementRemaining)));
	}

	// Total strength and populate slots.
	int32 TotalHP = 0;
	const int32 UnitCount = FMath::Min(Army->UnitIDs.Num(), MAX_SLOTS);
	for (int32 i = 0; i < UnitCount; ++i)
	{
		const int32 UnitID = Army->UnitIDs[i];
		SlotUnitIds[i] = UnitID;

		const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitID);
		if (!Unit)
		{
			continue;
		}

		TotalHP += Unit->CurrentHP;

		// Portrait color by race hash.
		if (SlotPortraits[i])
		{
			const uint32 Hash = GetTypeHash(Unit->RaceTag.GetTagName());
			FLinearColor RaceColor(
				0.1f + FMath::Frac(Hash * 0.00137f) * 0.3f,
				0.1f + FMath::Frac(Hash * 0.00251f) * 0.3f,
				0.15f + FMath::Frac(Hash * 0.00419f) * 0.3f,
				1.0f);
			SlotPortraits[i]->SetBrushColor(RaceColor);
		}

		// Name label.
		if (SlotLabels[i])
		{
			SlotLabels[i]->SetText(FText::FromString(Unit->SpecID.ToString()));
		}
	}

	if (StrengthText)
	{
		StrengthText->SetText(FText::FromString(
			FString::Printf(TEXT("Strength: %d units, %d total HP"), UnitCount, TotalHP)));
	}
}

// =============================================================================
// Slot click handlers
// =============================================================================

void UCoMArmyStackWidget::OnSlot0Clicked() { HandleSlotClicked(0); }
void UCoMArmyStackWidget::OnSlot1Clicked() { HandleSlotClicked(1); }
void UCoMArmyStackWidget::OnSlot2Clicked() { HandleSlotClicked(2); }
void UCoMArmyStackWidget::OnSlot3Clicked() { HandleSlotClicked(3); }
void UCoMArmyStackWidget::OnSlot4Clicked() { HandleSlotClicked(4); }
void UCoMArmyStackWidget::OnSlot5Clicked() { HandleSlotClicked(5); }
void UCoMArmyStackWidget::OnSlot6Clicked() { HandleSlotClicked(6); }
void UCoMArmyStackWidget::OnSlot7Clicked() { HandleSlotClicked(7); }
void UCoMArmyStackWidget::OnSlot8Clicked() { HandleSlotClicked(8); }

void UCoMArmyStackWidget::HandleSlotClicked(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= MAX_SLOTS)
	{
		return;
	}

	const int32 UnitId = SlotUnitIds[SlotIndex];
	if (UnitId < 0)
	{
		return;
	}

	OnUnitSelected.Broadcast(UnitId);

	// Also show the unit card directly.
	if (auto* GI = GetGameInstance())
	{
		if (auto* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->ShowUnitCard(UnitId);
		}
	}
}

void UCoMArmyStackWidget::OnCloseClicked()
{
	if (auto* GI = GetGameInstance())
	{
		if (auto* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->HideArmyStack();
		}
	}
}

// =============================================================================
// Layout
// =============================================================================

void UCoMArmyStackWidget::BuildArmyLayout()
{
	// -- Panel size constraint -------------------------------------------------
	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(450.0f);
	PanelSize->SetHeightOverride(400.0f);
	WidgetTree->RootWidget = PanelSize;

	// -- Gold outer border -----------------------------------------------------
	OuterBorder = WidgetTree->ConstructWidget<UBorder>();
	OuterBorder->SetBrushColor(AS::Gold);
	OuterBorder->SetPadding(FMargin(2.0f));
	PanelSize->AddChild(OuterBorder);

	// -- Dark inner panel ------------------------------------------------------
	UBorder* InnerBorder = WidgetTree->ConstructWidget<UBorder>();
	InnerBorder->SetBrushColor(AS::BgDark);
	InnerBorder->SetPadding(FMargin(10.0f, 8.0f));
	OuterBorder->AddChild(InnerBorder);

	// -- Content column --------------------------------------------------------
	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	InnerBorder->AddChild(ContentBox);

	// -- Header: "Army - [Name]" -----------------------------------------------
	{
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("Army")));
		HeaderText->SetColorAndOpacity(FSlateColor(AS::Gold));
		HeaderText->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = HeaderText->GetFont();
		Font.Size = 18;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(Font);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 0, 0, 2)); }
	}

	// -- Strength display ------------------------------------------------------
	{
		StrengthText = WidgetTree->ConstructWidget<UTextBlock>();
		StrengthText->SetText(FText::FromString(TEXT("Strength: --")));
		StrengthText->SetColorAndOpacity(FSlateColor(AS::Silver));
		StrengthText->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = StrengthText->GetFont();
		Font.Size = 11;
		StrengthText->SetFont(Font);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(StrengthText);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 0, 0, 1)); }
	}

	// -- Movement remaining ----------------------------------------------------
	{
		MovementText = WidgetTree->ConstructWidget<UTextBlock>();
		MovementText->SetText(FText::FromString(TEXT("-- moves left")));
		MovementText->SetColorAndOpacity(FSlateColor(AS::Silver));
		MovementText->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = MovementText->GetFont();
		Font.Size = 11;
		MovementText->SetFont(Font);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(MovementText);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 0, 0, 6)); }
	}

	// -- 3x3 grid of unit slots ------------------------------------------------
	for (int32 Row = 0; Row < 3; ++Row)
	{
		UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>();

		for (int32 Col = 0; Col < 3; ++Col)
		{
			const int32 Idx = Row * 3 + Col;

			// Vertical container: portrait button + name text.
			UVerticalBox* SlotVBox = WidgetTree->ConstructWidget<UVerticalBox>();

			// Portrait button (120x120).
			USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>();
			PortraitSize->SetWidthOverride(120.0f);
			PortraitSize->SetHeightOverride(120.0f);

			SlotButtons[Idx] = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle Style = SlotButtons[Idx]->GetStyle();
			Style.Normal.DrawAs = ESlateBrushDrawType::Box;
			Style.Normal.TintColor = FSlateColor(AS::BtnNormal);
			Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
			Style.Hovered.TintColor = FSlateColor(AS::BtnHover);
			Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
			Style.Pressed.TintColor = FSlateColor(AS::BtnPressed);
			SlotButtons[Idx]->SetStyle(Style);

			// Portrait color placeholder inside the button.
			SlotPortraits[Idx] = WidgetTree->ConstructWidget<UBorder>();
			SlotPortraits[Idx]->SetBrushColor(AS::EmptySlot);
			SlotPortraits[Idx]->SetPadding(FMargin(0.0f));
			SlotButtons[Idx]->AddChild(SlotPortraits[Idx]);

			PortraitSize->AddChild(SlotButtons[Idx]);

			UVerticalBoxSlot* PortSlotRef = SlotVBox->AddChildToVerticalBox(PortraitSize);
			if (PortSlotRef) { PortSlotRef->SetHorizontalAlignment(HAlign_Center); }

			// Unit name label below portrait.
			SlotLabels[Idx] = WidgetTree->ConstructWidget<UTextBlock>();
			SlotLabels[Idx]->SetText(FText::GetEmpty());
			SlotLabels[Idx]->SetColorAndOpacity(FSlateColor(AS::Silver));
			SlotLabels[Idx]->SetJustification(ETextJustify::Center);
			FSlateFontInfo Font = SlotLabels[Idx]->GetFont();
			Font.Size = 10;
			SlotLabels[Idx]->SetFont(Font);

			UVerticalBoxSlot* LblSlotRef = SlotVBox->AddChildToVerticalBox(SlotLabels[Idx]);
			if (LblSlotRef) { LblSlotRef->SetHorizontalAlignment(HAlign_Center); LblSlotRef->SetPadding(FMargin(0, 1, 0, 0)); }

			UHorizontalBoxSlot* HSlotRef = RowBox->AddChildToHorizontalBox(SlotVBox);
			if (HSlotRef) { HSlotRef->SetPadding(FMargin(4.0f)); }
		}

		UVerticalBoxSlot* RowSlotRef = ContentBox->AddChildToVerticalBox(RowBox);
		if (RowSlotRef) { RowSlotRef->SetHorizontalAlignment(HAlign_Center); RowSlotRef->SetPadding(FMargin(0, 1)); }
	}

	// -- Close button ----------------------------------------------------------
	CloseButton = CreateStyledButton(TEXT("Close"), ContentBox);
}

// =============================================================================
// Helpers
// =============================================================================

UButton* UCoMArmyStackWidget::CreateStyledButton(const FString& Label, UVerticalBox* Parent)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(200.0f);
	SizeBox->SetHeightOverride(36.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(AS::BtnBorder);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();

	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(AS::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(AS::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(AS::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(AS::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = BtnLabel->GetFont();
	Font.Size = 14;
	BtnLabel->SetFont(Font);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UVerticalBoxSlot* SlotRef = Parent->AddChildToVerticalBox(SizeBox);
	if (SlotRef)
	{
		SlotRef->SetHorizontalAlignment(HAlign_Center);
		SlotRef->SetPadding(FMargin(0, 6, 0, 2));
	}

	return Button;
}
