// Copyright Mythforge Studios. All Rights Reserved.
// CoMHUDWidget.cpp -- Main HUD overlay implementation.

#include "CoMHUDWidget.h"
#include "CoMMinimapWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"

// ─── RebuildWidget — constructs the full HUD layout in C++ ────────────────────
TSharedRef<SWidget> UCoMHUDWidget::RebuildWidget()
{
	TSharedRef<SWidget> Root = Super::RebuildWidget();
	BuildLayout();
	return Root;
}

void UCoMHUDWidget::BuildLayout()
{
	if (!WidgetTree) { return; }

	// Dark fantasy colors.
	const FLinearColor DarkBg(0.05f, 0.04f, 0.06f, 0.92f);
	const FLinearColor GoldAccent(0.85f, 0.7f, 0.2f, 1.0f);
	const FLinearColor ButtonBg(0.12f, 0.1f, 0.14f, 0.95f);
	const FLinearColor TextColor(0.9f, 0.85f, 0.7f, 1.0f);

	auto MakeFont = [](int32 Size) -> FSlateFontInfo {
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", Size);
		return Font;
	};

	auto MakeButtonStyle = [&](UButton* Btn) {
		if (!Btn) { return; }
		FButtonStyle Style = Btn->GetStyle();
		Style.Normal.TintColor = FSlateColor(ButtonBg);
		Style.Hovered.TintColor = FSlateColor(FLinearColor(0.2f, 0.16f, 0.22f, 1.0f));
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.08f, 0.06f, 0.1f, 1.0f));
		Btn->SetStyle(Style);
	};

	// Root canvas panel.
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDCanvas"));
	WidgetTree->RootWidget = Canvas;

	// ── TOP BAR ───────────────────────────────────────────────────────────────
	UBorder* TopBarBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TopBarBorder"));
	TopBarBorder->SetBrushColor(DarkBg);
	UCanvasPanelSlot* TopBarCanvasSlot = Canvas->AddChildToCanvas(TopBarBorder);
	if (TopBarCanvasSlot)
	{
		TopBarCanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		TopBarCanvasSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 40.f));
	}

	UHorizontalBox* TopBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopBar"));
	TopBarBorder->SetContent(TopBar);

	// Resource text blocks.
	auto AddResourceText = [&](const FString& Name, TObjectPtr<UTextBlock>& OutText) {
		OutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
		OutText->SetFont(MakeFont(14));
		OutText->SetColorAndOpacity(FSlateColor(GoldAccent));
		UHorizontalBoxSlot* HSlotRef = TopBar->AddChildToHorizontalBox(OutText);
		if (HSlotRef) { HSlotRef->SetPadding(FMargin(12.f, 8.f, 12.f, 8.f)); }
	};

	AddResourceText(TEXT("GoldText"), GoldText);
	AddResourceText(TEXT("ManaText"), ManaText);
	AddResourceText(TEXT("FoodText"), FoodText);
	AddResourceText(TEXT("ProductionText"), ProductionText);

	// Turn info text (right side of top bar).
	TurnInfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TurnInfoText"));
	TurnInfoText->SetFont(MakeFont(14));
	TurnInfoText->SetColorAndOpacity(FSlateColor(TextColor));
	UHorizontalBoxSlot* TurnSlotRef = TopBar->AddChildToHorizontalBox(TurnInfoText);
	if (TurnSlotRef)
	{
		TurnSlotRef->SetPadding(FMargin(20.f, 8.f, 12.f, 8.f));
		TurnSlotRef->SetHorizontalAlignment(HAlign_Right);
		TurnSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// ── RIGHT-SIDE BUTTON COLUMN ──────────────────────────────────────────────
	UBorder* RightPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanelBorder"));
	RightPanelBorder->SetBrushColor(DarkBg);
	UCanvasPanelSlot* RightCanvasSlot = Canvas->AddChildToCanvas(RightPanelBorder);
	if (RightCanvasSlot)
	{
		RightCanvasSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 1.f));
		RightCanvasSlot->SetOffsets(FMargin(-140.f, 50.f, 0.f, 0.f));
		RightCanvasSlot->SetAutoSize(true);
	}

	UVerticalBox* RightVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightVBox"));
	RightPanelBorder->SetContent(RightVBox);

	auto AddSideButton = [&](const FString& Label, const FString& Name, TObjectPtr<UButton>& OutBtn) {
		OutBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name);
		MakeButtonStyle(OutBtn);
		UVerticalBoxSlot* VSlotRef = RightVBox->AddChildToVerticalBox(OutBtn);
		if (VSlotRef) { VSlotRef->SetPadding(FMargin(4.f, 4.f, 4.f, 4.f)); }

		UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name + TEXT("Label")));
		BtnLabel->SetText(FText::FromString(Label));
		BtnLabel->SetFont(MakeFont(13));
		BtnLabel->SetColorAndOpacity(FSlateColor(GoldAccent));
		OutBtn->AddChild(BtnLabel);
	};

	AddSideButton(TEXT("End Turn"), TEXT("EndTurnButton"), EndTurnButton);
	AddSideButton(TEXT("Spell Book"), TEXT("SpellBookButton"), SpellBookButton);
	AddSideButton(TEXT("Cities"), TEXT("CityListButton"), CityListButton);
	AddSideButton(TEXT("Armies"), TEXT("ArmyManagerButton"), ArmyManagerButton);
	AddSideButton(TEXT("Diplomacy"), TEXT("DiplomacyButton"), DiplomacyButton);

	// Speed buttons.
	UHorizontalBox* SpeedRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SpeedRow"));
	UVerticalBoxSlot* SpeedRowSlotRef = RightVBox->AddChildToVerticalBox(SpeedRow);
	if (SpeedRowSlotRef) { SpeedRowSlotRef->SetPadding(FMargin(4.f, 8.f, 4.f, 4.f)); }

	auto AddSpeedButton = [&](const FString& Label, const FString& Name, TObjectPtr<UButton>& OutBtn) {
		OutBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name);
		MakeButtonStyle(OutBtn);
		UHorizontalBoxSlot* HSlotRef = SpeedRow->AddChildToHorizontalBox(OutBtn);
		if (HSlotRef) { HSlotRef->SetPadding(FMargin(2.f, 0.f, 2.f, 0.f)); }

		UTextBlock* SpeedLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name + TEXT("Label")));
		SpeedLabel->SetText(FText::FromString(Label));
		SpeedLabel->SetFont(MakeFont(11));
		SpeedLabel->SetColorAndOpacity(FSlateColor(TextColor));
		OutBtn->AddChild(SpeedLabel);
	};

	AddSpeedButton(TEXT("1x"), TEXT("Speed1xButton"), Speed1xButton);
	AddSpeedButton(TEXT("2x"), TEXT("Speed2xButton"), Speed2xButton);
	AddSpeedButton(TEXT("4x"), TEXT("Speed4xButton"), Speed4xButton);

	// Speed display text.
	SpeedDisplayText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeedDisplayText"));
	SpeedDisplayText->SetFont(MakeFont(11));
	SpeedDisplayText->SetColorAndOpacity(FSlateColor(TextColor));
	UHorizontalBoxSlot* SpeedDisplaySlotRef = SpeedRow->AddChildToHorizontalBox(SpeedDisplayText);
	if (SpeedDisplaySlotRef) { SpeedDisplaySlotRef->SetPadding(FMargin(6.f, 2.f, 2.f, 2.f)); }

	// ── BOTTOM-LEFT: MINIMAP PLACEHOLDER ──────────────────────────────────────
	MinimapFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MinimapFrame"));
	MinimapFrame->SetBrushColor(FLinearColor(0.08f, 0.07f, 0.1f, 0.95f));
	UCanvasPanelSlot* MinimapCanvasSlot = Canvas->AddChildToCanvas(MinimapFrame);
	if (MinimapCanvasSlot)
	{
		MinimapCanvasSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		MinimapCanvasSlot->SetOffsets(FMargin(8.f, -158.f, 208.f, 8.f));
		MinimapCanvasSlot->SetAutoSize(false);
	}

	// ── BOTTOM-CENTER: NOTIFICATION AREA ──────────────────────────────────────
	UBorder* NotifBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NotifBorder"));
	NotifBorder->SetBrushColor(FLinearColor(0.04f, 0.03f, 0.05f, 0.88f));
	UCanvasPanelSlot* NotifCanvasSlot = Canvas->AddChildToCanvas(NotifBorder);
	if (NotifCanvasSlot)
	{
		NotifCanvasSlot->SetAnchors(FAnchors(0.15f, 1.f, 0.85f, 1.f));
		NotifCanvasSlot->SetOffsets(FMargin(0.f, -120.f, 0.f, 8.f));
	}

	NotificationScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("NotificationScrollBox"));
	NotifBorder->SetContent(NotificationScrollBox);
}

void UCoMHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind button click events to our handler methods.
	if (EndTurnButton)
	{
		EndTurnButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnEndTurnClicked);
	}
	if (SpellBookButton)
	{
		SpellBookButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnSpellBookClicked);
	}
	if (CityListButton)
	{
		CityListButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnCityListClicked);
	}
	if (ArmyManagerButton)
	{
		ArmyManagerButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnArmyManagerClicked);
	}
	if (DiplomacyButton)
	{
		DiplomacyButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnDiplomacyClicked);
	}
	if (Speed1xButton) { Speed1xButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnSpeed1xClicked); }
	if (Speed2xButton) { Speed2xButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnSpeed2xClicked); }
	if (Speed4xButton) { Speed4xButton->OnClicked.AddDynamic(this, &UCoMHUDWidget::OnSpeed4xClicked); }

	// Initialize speed display.
	UpdateSpeedDisplay(1.0f);

	// Set initial display values.
	UpdateResources(0, 0, 0, 0);
	UpdateTurnInfo(1, TEXT("Unknown Wizard"));
}

void UCoMHUDWidget::UpdateResources(int32 Gold, int32 Mana, int32 Food, int32 Production)
{
	if (GoldText)
	{
		GoldText->SetText(FText::FromString(FString::Printf(TEXT("Gold: %d"), Gold)));
	}
	if (ManaText)
	{
		ManaText->SetText(FText::FromString(FString::Printf(TEXT("Mana: %d"), Mana)));
	}
	if (FoodText)
	{
		FoodText->SetText(FText::FromString(FString::Printf(TEXT("Food: %d"), Food)));
	}
	if (ProductionText)
	{
		ProductionText->SetText(FText::FromString(FString::Printf(TEXT("Prod: %d"), Production)));
	}
}

void UCoMHUDWidget::UpdateTurnInfo(int32 TurnNumber, const FString& WizardName)
{
	if (TurnInfoText)
	{
		TurnInfoText->SetText(FText::FromString(
			FString::Printf(TEXT("Turn %d - %s"), TurnNumber, *WizardName)));
	}
}

void UCoMHUDWidget::AddNotification(const FString& Message)
{
	if (!NotificationScrollBox)
	{
		return;
	}

	// Create a new text block for the notification.
	UTextBlock* NotifText = NewObject<UTextBlock>(this);
	if (NotifText)
	{
		NotifText->SetText(FText::FromString(Message));

		FSlateFontInfo FontInfo = NotifText->GetFont();
		FontInfo.Size = 12;
		NotifText->SetFont(FontInfo);

		NotificationScrollBox->AddChild(NotifText);

		// Trim oldest notifications if we exceed the cap.
		while (NotificationScrollBox->GetChildrenCount() > MaxNotifications)
		{
			UWidget* OldestChild = NotificationScrollBox->GetChildAt(0);
			if (OldestChild)
			{
				NotificationScrollBox->RemoveChild(OldestChild);
			}
		}

		// Scroll to the bottom to show the newest notification.
		NotificationScrollBox->ScrollToEnd();
	}
}

void UCoMHUDWidget::ClearNotifications()
{
	if (NotificationScrollBox)
	{
		NotificationScrollBox->ClearChildren();
	}
}

void UCoMHUDWidget::OnEndTurnClicked()
{
	OnEndTurnRequested.Broadcast();
}

void UCoMHUDWidget::OnSpellBookClicked()
{
	OnSpellBookRequested.Broadcast();
}

void UCoMHUDWidget::OnCityListClicked()
{
	OnCityListRequested.Broadcast();
}

void UCoMHUDWidget::OnArmyManagerClicked()
{
	OnArmyManagerRequested.Broadcast();
}

void UCoMHUDWidget::OnDiplomacyClicked()
{
	OnDiplomacyRequested.Broadcast();
}

void UCoMHUDWidget::InitializeMinimap(int32 WizardId, ECoMPlane StartingPlane)
{
	if (!MinimapFrame)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCoMHUDWidget::InitializeMinimap -- MinimapFrame is null, cannot embed minimap."));
		return;
	}

	// Create the minimap widget and add it to the MinimapFrame border.
	MinimapWidget = CreateWidget<UCoMMinimapWidget>(GetOwningPlayer(), UCoMMinimapWidget::StaticClass());
	if (MinimapWidget)
	{
		MinimapFrame->SetContent(MinimapWidget);
		MinimapWidget->InitializeMinimap(WizardId, StartingPlane);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UCoMHUDWidget::InitializeMinimap -- Failed to create UCoMMinimapWidget."));
	}
}

void UCoMHUDWidget::RefreshMinimap()
{
	if (MinimapWidget)
	{
		MinimapWidget->RefreshMinimap();
	}
}

// ─── Game Speed Control ─────────────────────────────────────────────────────

void UCoMHUDWidget::SetGameSpeed(float Speed)
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI) return;

	if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
	{
		TurnSub->SetGameSpeed(Speed);
	}

	UpdateSpeedDisplay(Speed);
}

void UCoMHUDWidget::UpdateSpeedDisplay(float CurrentSpeed)
{
	if (SpeedDisplayText)
	{
		SpeedDisplayText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0fx"), CurrentSpeed)));
	}
}

void UCoMHUDWidget::OnSpeed1xClicked() { SetGameSpeed(1.0f); }
void UCoMHUDWidget::OnSpeed2xClicked() { SetGameSpeed(2.0f); }
void UCoMHUDWidget::OnSpeed4xClicked() { SetGameSpeed(4.0f); }
