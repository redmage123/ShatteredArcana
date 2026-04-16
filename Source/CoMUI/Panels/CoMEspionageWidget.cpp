// Copyright Mythforge Studios. All Rights Reserved.
// CoMEspionageWidget.cpp -- Spy management implementation.

#include "CoMEspionageWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

#include "CoMUI/CoMUISubsystem.h"

namespace SpyColours
{
	static const FLinearColor BgDark     = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.816f, 0.816f, 0.863f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPressed = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);

	// Mission type colours
	static const FLinearColor Sabotage      = FLinearColor(0.9f, 0.4f, 0.1f, 1.0f);
	static const FLinearColor Theft         = FLinearColor(0.85f, 0.75f, 0.2f, 1.0f);
	static const FLinearColor Assassination = FLinearColor(0.8f, 0.1f, 0.1f, 1.0f);
	static const FLinearColor TurnAgent     = FLinearColor(0.4f, 0.6f, 0.9f, 1.0f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMEspionageWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMEspionageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton)          { CloseButton->OnClicked.AddDynamic(this, &UCoMEspionageWidget::OnCloseClicked); }
	if (DeployAgentButton)    { DeployAgentButton->OnClicked.AddDynamic(this, &UCoMEspionageWidget::OnDeployAgentClicked); }
	if (SabotageButton)       { SabotageButton->OnClicked.AddDynamic(this, &UCoMEspionageWidget::OnSabotageClicked); }
	if (TheftButton)          { TheftButton->OnClicked.AddDynamic(this, &UCoMEspionageWidget::OnTheftClicked); }
	if (AssassinationButton)  { AssassinationButton->OnClicked.AddDynamic(this, &UCoMEspionageWidget::OnAssassinationClicked); }
	if (TurnAgentButton)      { TurnAgentButton->OnClicked.AddDynamic(this, &UCoMEspionageWidget::OnTurnAgentClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMEspionageWidget::SetWizardId(int32 WizardId)
{
	CurrentWizardId = WizardId;

	if (!AgentListScrollBox) { return; }
	AgentListScrollBox->ClearChildren();

	// Placeholder agent entries
	struct FAgentEntry { FString Name; FString Location; FString Status; };
	TArray<FAgentEntry> Agents = {
		{ TEXT("Shadow Vex"),      TEXT("Noctharion - Wizard 2's Capital"), TEXT("Infiltrating") },
		{ TEXT("Silken Whisper"),   TEXT("Verdantis - Port Thornwall"),     TEXT("Active") },
		{ TEXT("The Grey Crow"),    TEXT("Infernyx - Iron Citadel"),        TEXT("Compromised") },
		{ TEXT("Mistwalker"),       TEXT("Aurelith - Your Capital"),        TEXT("Idle") },
	};

	// Column headers
	{
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		auto AddColHeader = [&](const FString& Text, float FillWeight)
		{
			UTextBlock* Col = WidgetTree->ConstructWidget<UTextBlock>();
			Col->SetText(FText::FromString(Text));
			Col->SetColorAndOpacity(FSlateColor(SpyColours::Gold));
			FSlateFontInfo CFont = Col->GetFont();
			CFont.Size = 12;
			CFont.TypefaceFontName = FName(TEXT("Bold"));
			Col->SetFont(CFont);
			UHorizontalBoxSlot* CSlotRef = HeaderRow->AddChildToHorizontalBox(Col);
			if (CSlotRef)
			{
				CSlotRef->SetVerticalAlignment(VAlign_Center);
				if (FillWeight > 0.f) { CSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
				CSlotRef->SetPadding(FMargin(4, 0));
			}
		};

		AddColHeader(TEXT("Agent"), 1.0f);
		AddColHeader(TEXT("Location"), 1.5f);
		AddColHeader(TEXT("Status"), 0.0f);

		UBorder* HdrBorder = WidgetTree->ConstructWidget<UBorder>();
		HdrBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		HdrBorder->SetPadding(FMargin(4.0f, 4.0f));
		HdrBorder->AddChild(HeaderRow);
		AgentListScrollBox->AddChild(HdrBorder);
	}

	for (const auto& Agent : Agents)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Name
		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>();
		NameText->SetText(FText::FromString(Agent.Name));
		NameText->SetColorAndOpacity(FSlateColor(SpyColours::Silver));
		FSlateFontInfo NFont = NameText->GetFont();
		NFont.Size = 13;
		NameText->SetFont(NFont);
		UHorizontalBoxSlot* NSlotRef = Row->AddChildToHorizontalBox(NameText);
		if (NSlotRef) { NSlotRef->SetVerticalAlignment(VAlign_Center); NSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); NSlotRef->SetPadding(FMargin(4, 0)); }

		// Location
		UTextBlock* LocText = WidgetTree->ConstructWidget<UTextBlock>();
		LocText->SetText(FText::FromString(Agent.Location));
		LocText->SetColorAndOpacity(FSlateColor(SpyColours::Grey));
		FSlateFontInfo LFont = LocText->GetFont();
		LFont.Size = 12;
		LocText->SetFont(LFont);
		UHorizontalBoxSlot* LSlotRef = Row->AddChildToHorizontalBox(LocText);
		if (LSlotRef) { LSlotRef->SetVerticalAlignment(VAlign_Center); LSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); LSlotRef->SetPadding(FMargin(4, 0)); }

		// Status (colored by state)
		UTextBlock* StatusText = WidgetTree->ConstructWidget<UTextBlock>();
		StatusText->SetText(FText::FromString(Agent.Status));
		FLinearColor StatusColor = SpyColours::Silver;
		if (Agent.Status == TEXT("Compromised")) { StatusColor = SpyColours::Assassination; }
		else if (Agent.Status == TEXT("Infiltrating")) { StatusColor = SpyColours::TurnAgent; }
		else if (Agent.Status == TEXT("Active")) { StatusColor = FLinearColor(0.2f, 0.7f, 0.3f, 1.0f); }
		StatusText->SetColorAndOpacity(FSlateColor(StatusColor));
		FSlateFontInfo SFont = StatusText->GetFont();
		SFont.Size = 12;
		StatusText->SetFont(SFont);
		UHorizontalBoxSlot* SSlotRef = Row->AddChildToHorizontalBox(StatusText);
		if (SSlotRef) { SSlotRef->SetVerticalAlignment(VAlign_Center); SSlotRef->SetPadding(FMargin(4, 0)); }

		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>();
		RowBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		RowBorder->SetPadding(FMargin(4.0f, 3.0f));
		RowBorder->AddChild(Row);
		AgentListScrollBox->AddChild(RowBorder);
	}
}

// =============================================================================
// Button callbacks
// =============================================================================

void UCoMEspionageWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

void UCoMEspionageWidget::OnDeployAgentClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Espionage: Deploy Agent"));
}

void UCoMEspionageWidget::OnSabotageClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Espionage: Sabotage mission selected"));
}

void UCoMEspionageWidget::OnTheftClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Espionage: Theft mission selected"));
}

void UCoMEspionageWidget::OnAssassinationClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Espionage: Assassination mission selected"));
}

void UCoMEspionageWidget::OnTurnAgentClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Espionage: Turn Agent mission selected"));
}

// =============================================================================
// Helpers
// =============================================================================

UButton* UCoMEspionageWidget::CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(Width);
	SizeBox->SetHeightOverride(36.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(SpyColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(SpyColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(SpyColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(SpyColours::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(SpyColours::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 13;
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UVerticalBoxSlot* SlotRef = Parent->AddChildToVerticalBox(SizeBox);
	if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 4)); }

	return Button;
}

UButton* UCoMEspionageWidget::CreateMissionButton(UHorizontalBox* Parent, const FString& Label, const FLinearColor& Color)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(120.0f);
	SizeBox->SetHeightOverride(32.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(Color * 0.5f);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(Color * 0.2f);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(Color * 0.35f);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(Color * 0.12f);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(Color));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 11;
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UHorizontalBoxSlot* SlotRef = Parent->AddChildToHorizontalBox(SizeBox);
	if (SlotRef) { SlotRef->SetVerticalAlignment(VAlign_Center); SlotRef->SetPadding(FMargin(3, 0)); }

	return Button;
}

// =============================================================================
// Layout
// =============================================================================

void UCoMEspionageWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(SpyColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(ScreenOverlay);

	// Center panel
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(SpyColours::GoldDim);
	PanelBorder->SetPadding(FMargin(1.5f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(SpyColours::PanelBg);
	PanelInner->SetPadding(FMargin(24.0f, 16.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(700.0f);
	PanelSize->SetHeightOverride(500.0f);
	PanelSize->AddChild(PanelBorder);

	UOverlaySlot* PanelSlotRef = ScreenOverlay->AddChildToOverlay(PanelSize);
	if (PanelSlotRef)
	{
		PanelSlotRef->SetHorizontalAlignment(HAlign_Center);
		PanelSlotRef->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelInner->AddChild(ContentBox);

	// Header
	{
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("Shadow Network")));
		HeaderText->SetColorAndOpacity(FSlateColor(SpyColours::Gold));
		HeaderText->SetJustification(ETextJustify::Center);
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 24;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);
		UVerticalBoxSlot* HdrSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HdrSlotRef) { HdrSlotRef->SetHorizontalAlignment(HAlign_Center); HdrSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// Divider
	{
		UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
		Div->SetBrushColor(SpyColours::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(1.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* DivSlotRef = ContentBox->AddChildToVerticalBox(DivSize);
		if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); DivSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	// Mission type buttons row
	{
		UTextBlock* MissionLabel = WidgetTree->ConstructWidget<UTextBlock>();
		MissionLabel->SetText(FText::FromString(TEXT("Mission Types:")));
		MissionLabel->SetColorAndOpacity(FSlateColor(SpyColours::Grey));
		FSlateFontInfo MLFont = MissionLabel->GetFont();
		MLFont.Size = 12;
		MissionLabel->SetFont(MLFont);
		UVerticalBoxSlot* MLSlotRef = ContentBox->AddChildToVerticalBox(MissionLabel);
		if (MLSlotRef) { MLSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

		MissionButtonsRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		SabotageButton      = CreateMissionButton(MissionButtonsRow, TEXT("Sabotage"),      SpyColours::Sabotage);
		TheftButton         = CreateMissionButton(MissionButtonsRow, TEXT("Theft"),          SpyColours::Theft);
		AssassinationButton = CreateMissionButton(MissionButtonsRow, TEXT("Assassination"),  SpyColours::Assassination);
		TurnAgentButton     = CreateMissionButton(MissionButtonsRow, TEXT("Turn Agent"),     SpyColours::TurnAgent);

		UVerticalBoxSlot* MissionRowSlotRef = ContentBox->AddChildToVerticalBox(MissionButtonsRow);
		if (MissionRowSlotRef) { MissionRowSlotRef->SetHorizontalAlignment(HAlign_Center); MissionRowSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// Scrollable agent list
	AgentListScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	UVerticalBoxSlot* ScrollSlotRef = ContentBox->AddChildToVerticalBox(AgentListScrollBox);
	if (ScrollSlotRef) { ScrollSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

	// Deploy Agent button
	DeployAgentButton = CreateStyledButton(ContentBox, TEXT("Deploy Agent"), 180.0f);

	// Close button
	CloseButton = CreateStyledButton(ContentBox, TEXT("Close"), 140.0f);
}
