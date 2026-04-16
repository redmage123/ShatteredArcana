// Copyright Mythforge Studios. All Rights Reserved.
// CoMMapOverviewWidget.cpp -- Full world map overview implementation.

#include "CoMMapOverviewWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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

// =============================================================================
// Colour palette
// =============================================================================

namespace MapColours
{
	static const FLinearColor BgDark     = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f); // #0e0e1a
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f); // #daa520
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.816f, 0.816f, 0.863f, 1.0f); // #d0d0dc
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPressed = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);

	// Plane-themed tab colours
	static const FLinearColor Aurelith   = FLinearColor(0.85f, 0.75f, 0.20f, 1.0f);
	static const FLinearColor Noctharion = FLinearColor(0.30f, 0.15f, 0.50f, 1.0f);
	static const FLinearColor Verdantis  = FLinearColor(0.15f, 0.60f, 0.15f, 1.0f);
	static const FLinearColor Infernyx   = FLinearColor(0.80f, 0.20f, 0.05f, 1.0f);
	static const FLinearColor Aethermist = FLinearColor(0.50f, 0.70f, 0.90f, 1.0f);
	static const FLinearColor Abyssal    = FLinearColor(0.55f, 0.10f, 0.10f, 1.0f);
	static const FLinearColor Ethereal   = FLinearColor(0.60f, 0.50f, 0.85f, 1.0f);
	static const FLinearColor Feywild    = FLinearColor(0.90f, 0.45f, 0.75f, 1.0f);

	// Map area placeholder colours by plane
	static const FLinearColor MapAurelith   = FLinearColor(0.12f, 0.10f, 0.06f, 1.0f);
	static const FLinearColor MapNoctharion = FLinearColor(0.06f, 0.03f, 0.10f, 1.0f);
	static const FLinearColor MapVerdantis  = FLinearColor(0.04f, 0.10f, 0.04f, 1.0f);
	static const FLinearColor MapInfernyx   = FLinearColor(0.12f, 0.04f, 0.02f, 1.0f);
	static const FLinearColor MapAethermist = FLinearColor(0.06f, 0.08f, 0.12f, 1.0f);
	static const FLinearColor MapAbyssal    = FLinearColor(0.10f, 0.02f, 0.02f, 1.0f);
	static const FLinearColor MapEthereal   = FLinearColor(0.08f, 0.06f, 0.12f, 1.0f);
	static const FLinearColor MapFeywild    = FLinearColor(0.10f, 0.06f, 0.10f, 1.0f);
}

// =============================================================================
// Helper: plane name string
// =============================================================================

static FString GetPlaneName(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return TEXT("Aurelith");
	case ECoMPlane::Noctharion: return TEXT("Noctharion");
	case ECoMPlane::Verdantis:  return TEXT("Verdantis");
	case ECoMPlane::Infernyx:   return TEXT("Infernyx");
	case ECoMPlane::Aethermist: return TEXT("Aethermist");
	case ECoMPlane::Abyssal:    return TEXT("Abyssal");
	case ECoMPlane::Ethereal:   return TEXT("Ethereal");
	case ECoMPlane::Feywild:    return TEXT("Feywild");
	default:                    return TEXT("Unknown");
	}
}

static FString GetLayerName(ECoMMapLayer Layer)
{
	switch (Layer)
	{
	case ECoMMapLayer::Surface:    return TEXT("Surface");
	case ECoMMapLayer::Underdark:  return TEXT("Underdark");
	case ECoMMapLayer::Underwater: return TEXT("Underwater");
	default:                       return TEXT("Unknown");
	}
}

static FLinearColor GetMapColor(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return MapColours::MapAurelith;
	case ECoMPlane::Noctharion: return MapColours::MapNoctharion;
	case ECoMPlane::Verdantis:  return MapColours::MapVerdantis;
	case ECoMPlane::Infernyx:   return MapColours::MapInfernyx;
	case ECoMPlane::Aethermist: return MapColours::MapAethermist;
	case ECoMPlane::Abyssal:    return MapColours::MapAbyssal;
	case ECoMPlane::Ethereal:   return MapColours::MapEthereal;
	case ECoMPlane::Feywild:    return MapColours::MapFeywild;
	default:                    return MapColours::BgDark;
	}
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMMapOverviewWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

// =============================================================================
// NativeConstruct
// =============================================================================

void UCoMMapOverviewWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton)            { CloseButton->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnCloseClicked); }
	if (AurelithTab)            { AurelithTab->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnAurelithTabClicked); }
	if (NoctharionTab)          { NoctharionTab->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnNoctharionTabClicked); }
	if (VerdantisTab)           { VerdantisTab->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnVerdantisTabClicked); }
	if (InfernyxTab)            { InfernyxTab->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnInfernyxTabClicked); }
	if (AethermistTab)          { AethermistTab->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnAethermistTabClicked); }
	if (AbyssalTab)             { AbyssalTab->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnAbyssalTabClicked); }
	if (EtherealTab)            { EtherealTab->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnEtherealTabClicked); }
	if (FeywildTab)             { FeywildTab->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnFeywildTabClicked); }
	if (SurfaceButton)          { SurfaceButton->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnSurfaceClicked); }
	if (UnderdarkButton)        { UnderdarkButton->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnUnderdarkClicked); }
	if (UnderwaterButton)       { UnderwaterButton->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnUnderwaterClicked); }
	if (CenterOnCapitalButton)  { CenterOnCapitalButton->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnCenterOnCapitalClicked); }
	if (ZoomInButton)           { ZoomInButton->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnZoomInClicked); }
	if (ZoomOutButton)          { ZoomOutButton->OnClicked.AddDynamic(this, &UCoMMapOverviewWidget::OnZoomOutClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMMapOverviewWidget::SelectPlane(ECoMPlane Plane)
{
	CurrentPlane = Plane;
	UpdateMapDisplay();
}

void UCoMMapOverviewWidget::SelectLayer(ECoMMapLayer Layer)
{
	CurrentLayer = Layer;
	UpdateMapDisplay();
}

void UCoMMapOverviewWidget::CenterOnPosition(FIntPoint Position)
{
	if (CoordinateText)
	{
		CoordinateText->SetText(FText::FromString(
			FString::Printf(TEXT("Position: (%d, %d)"), Position.X, Position.Y)));
	}
}

void UCoMMapOverviewWidget::UpdateMapDisplay()
{
	if (MapAreaBorder)
	{
		MapAreaBorder->SetBrushColor(GetMapColor(CurrentPlane));
	}
	if (MapPlaceholderText)
	{
		MapPlaceholderText->SetText(FText::FromString(
			FString::Printf(TEXT("Map: %s - %s"), *GetPlaneName(CurrentPlane), *GetLayerName(CurrentLayer))));
	}
}

// =============================================================================
// Button callbacks
// =============================================================================

void UCoMMapOverviewWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

void UCoMMapOverviewWidget::OnAurelithTabClicked()   { SelectPlane(ECoMPlane::Aurelith); }
void UCoMMapOverviewWidget::OnNoctharionTabClicked()  { SelectPlane(ECoMPlane::Noctharion); }
void UCoMMapOverviewWidget::OnVerdantisTabClicked()   { SelectPlane(ECoMPlane::Verdantis); }
void UCoMMapOverviewWidget::OnInfernyxTabClicked()    { SelectPlane(ECoMPlane::Infernyx); }
void UCoMMapOverviewWidget::OnAethermistTabClicked()  { SelectPlane(ECoMPlane::Aethermist); }
void UCoMMapOverviewWidget::OnAbyssalTabClicked()     { SelectPlane(ECoMPlane::Abyssal); }
void UCoMMapOverviewWidget::OnEtherealTabClicked()    { SelectPlane(ECoMPlane::Ethereal); }
void UCoMMapOverviewWidget::OnFeywildTabClicked()     { SelectPlane(ECoMPlane::Feywild); }

void UCoMMapOverviewWidget::OnSurfaceClicked()        { SelectLayer(ECoMMapLayer::Surface); }
void UCoMMapOverviewWidget::OnUnderdarkClicked()      { SelectLayer(ECoMMapLayer::Underdark); }
void UCoMMapOverviewWidget::OnUnderwaterClicked()     { SelectLayer(ECoMMapLayer::Underwater); }

void UCoMMapOverviewWidget::OnCenterOnCapitalClicked()
{
	CenterOnPosition(FIntPoint(80, 50)); // Default center
}

void UCoMMapOverviewWidget::OnZoomInClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MapOverview: Zoom In"));
}

void UCoMMapOverviewWidget::OnZoomOutClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MapOverview: Zoom Out"));
}

// =============================================================================
// Helpers
// =============================================================================

UButton* UCoMMapOverviewWidget::CreatePlaneTab(UHorizontalBox* Parent, const FString& Label, const FLinearColor& Color)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(110.0f);
	SizeBox->SetHeightOverride(36.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(Color * 0.6f);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(Color * 0.25f);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(Color * 0.40f);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(Color * 0.15f);
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
	if (SlotRef) { SlotRef->SetVerticalAlignment(VAlign_Center); SlotRef->SetPadding(FMargin(2.0f, 0.0f)); }

	return Button;
}

UButton* UCoMMapOverviewWidget::CreateLayerButton(UHorizontalBox* Parent, const FString& Label)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(120.0f);
	SizeBox->SetHeightOverride(32.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(MapColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(MapColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(MapColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(MapColours::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(MapColours::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 12;
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UHorizontalBoxSlot* SlotRef = Parent->AddChildToHorizontalBox(SizeBox);
	if (SlotRef) { SlotRef->SetVerticalAlignment(VAlign_Center); SlotRef->SetPadding(FMargin(4.0f, 0.0f)); }

	return Button;
}

UButton* UCoMMapOverviewWidget::CreateSidebarButton(UVerticalBox* Parent, const FString& Label, float Width)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(Width);
	SizeBox->SetHeightOverride(34.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(MapColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(MapColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(MapColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(MapColours::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(MapColours::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 12;
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UVerticalBoxSlot* SlotRef = Parent->AddChildToVerticalBox(SizeBox);
	if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Fill); SlotRef->SetPadding(FMargin(0, 3)); }

	return Button;
}

// =============================================================================
// Layout
// =============================================================================

void UCoMMapOverviewWidget::BuildLayout()
{
	// ── Full-screen dark background ──────────────────────────────────────
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(MapColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	UVerticalBox* RootVBox = WidgetTree->ConstructWidget<UVerticalBox>();
	BackgroundBorder->AddChild(RootVBox);

	// ══════════════════════════════════════════════════════════════════════
	// PLANE TABS (top row)
	// ══════════════════════════════════════════════════════════════════════
	{
		UBorder* TabBar = WidgetTree->ConstructWidget<UBorder>();
		TabBar->SetBrushColor(MapColours::PanelBg);
		TabBar->SetPadding(FMargin(10.0f, 6.0f));

		PlaneTabsBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		TabBar->AddChild(PlaneTabsBox);

		AurelithTab   = CreatePlaneTab(PlaneTabsBox, TEXT("Aurelith"),   MapColours::Aurelith);
		NoctharionTab = CreatePlaneTab(PlaneTabsBox, TEXT("Noctharion"), MapColours::Noctharion);
		VerdantisTab  = CreatePlaneTab(PlaneTabsBox, TEXT("Verdantis"),  MapColours::Verdantis);
		InfernyxTab   = CreatePlaneTab(PlaneTabsBox, TEXT("Infernyx"),   MapColours::Infernyx);
		AethermistTab = CreatePlaneTab(PlaneTabsBox, TEXT("Aethermist"), MapColours::Aethermist);
		AbyssalTab    = CreatePlaneTab(PlaneTabsBox, TEXT("Abyssal"),    MapColours::Abyssal);
		EtherealTab   = CreatePlaneTab(PlaneTabsBox, TEXT("Ethereal"),   MapColours::Ethereal);
		FeywildTab    = CreatePlaneTab(PlaneTabsBox, TEXT("Feywild"),    MapColours::Feywild);

		UVerticalBoxSlot* TabBarSlotRef = RootVBox->AddChildToVerticalBox(TabBar);
		if (TabBarSlotRef) { TabBarSlotRef->SetHorizontalAlignment(HAlign_Fill); }
	}

	// ── Layer toggles ────────────────────────────────────────────────────
	{
		UBorder* LayerBar = WidgetTree->ConstructWidget<UBorder>();
		LayerBar->SetBrushColor(FLinearColor(0.04f, 0.03f, 0.07f, 0.9f));
		LayerBar->SetPadding(FMargin(10.0f, 4.0f));

		LayerButtonsBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		LayerBar->AddChild(LayerButtonsBox);

		SurfaceButton    = CreateLayerButton(LayerButtonsBox, TEXT("Surface"));
		UnderdarkButton  = CreateLayerButton(LayerButtonsBox, TEXT("Underdark"));
		UnderwaterButton = CreateLayerButton(LayerButtonsBox, TEXT("Underwater"));

		UVerticalBoxSlot* LayerSlotRef = RootVBox->AddChildToVerticalBox(LayerBar);
		if (LayerSlotRef) { LayerSlotRef->SetHorizontalAlignment(HAlign_Fill); }
	}

	// ── Gold divider ─────────────────────────────────────────────────────
	{
		UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
		Div->SetBrushColor(MapColours::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(2.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* DivSlotRef = RootVBox->AddChildToVerticalBox(DivSize);
		if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); }
	}

	// ══════════════════════════════════════════════════════════════════════
	// CENTER: Map + Right Sidebar
	// ══════════════════════════════════════════════════════════════════════
	{
		UHorizontalBox* CenterRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		UVerticalBoxSlot* CenterSlotRef = RootVBox->AddChildToVerticalBox(CenterRow);
		if (CenterSlotRef)
		{
			CenterSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			CenterSlotRef->SetHorizontalAlignment(HAlign_Fill);
		}

		// ── Map area ─────────────────────────────────────────────────────
		{
			UBorder* MapOuterBorder = WidgetTree->ConstructWidget<UBorder>();
			MapOuterBorder->SetBrushColor(MapColours::GoldDim);
			MapOuterBorder->SetPadding(FMargin(1.5f));

			MapAreaBorder = WidgetTree->ConstructWidget<UBorder>();
			MapAreaBorder->SetBrushColor(GetMapColor(CurrentPlane));
			MapAreaBorder->SetPadding(FMargin(20.0f));
			MapOuterBorder->AddChild(MapAreaBorder);

			// Overlay for placeholder text and markers
			UOverlay* MapOverlay = WidgetTree->ConstructWidget<UOverlay>();
			MapAreaBorder->AddChild(MapOverlay);

			MapPlaceholderText = WidgetTree->ConstructWidget<UTextBlock>();
			MapPlaceholderText->SetText(FText::FromString(
				FString::Printf(TEXT("Map: %s - %s"), *GetPlaneName(CurrentPlane), *GetLayerName(CurrentLayer))));
			MapPlaceholderText->SetColorAndOpacity(FSlateColor(MapColours::Silver));
			MapPlaceholderText->SetJustification(ETextJustify::Center);
			FSlateFontInfo MapFont = MapPlaceholderText->GetFont();
			MapFont.Size = 22;
			MapPlaceholderText->SetFont(MapFont);
			UOverlaySlot* MapTextSlotRef = MapOverlay->AddChildToOverlay(MapPlaceholderText);
			if (MapTextSlotRef)
			{
				MapTextSlotRef->SetHorizontalAlignment(HAlign_Center);
				MapTextSlotRef->SetVerticalAlignment(VAlign_Center);
			}

			USizeBox* MapSizeBox = WidgetTree->ConstructWidget<USizeBox>();
			MapSizeBox->SetMinDesiredWidth(800.0f);
			MapSizeBox->SetMinDesiredHeight(500.0f);
			MapSizeBox->AddChild(MapOuterBorder);

			UHorizontalBoxSlot* MapSlotRef = CenterRow->AddChildToHorizontalBox(MapSizeBox);
			if (MapSlotRef)
			{
				MapSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				MapSlotRef->SetVerticalAlignment(VAlign_Fill);
				MapSlotRef->SetPadding(FMargin(10.0f));
			}
		}

		// ── Right sidebar ────────────────────────────────────────────────
		{
			USizeBox* SidebarSize = WidgetTree->ConstructWidget<USizeBox>();
			SidebarSize->SetWidthOverride(200.0f);

			UBorder* SidebarBorder = WidgetTree->ConstructWidget<UBorder>();
			SidebarBorder->SetBrushColor(MapColours::PanelBg);
			SidebarBorder->SetPadding(FMargin(10.0f, 8.0f));
			SidebarSize->AddChild(SidebarBorder);

			UVerticalBox* SidebarVBox = WidgetTree->ConstructWidget<UVerticalBox>();
			SidebarBorder->AddChild(SidebarVBox);

			// Legend header
			{
				UTextBlock* LegendHeader = WidgetTree->ConstructWidget<UTextBlock>();
				LegendHeader->SetText(FText::FromString(TEXT("Legend")));
				LegendHeader->SetColorAndOpacity(FSlateColor(MapColours::Gold));
				FSlateFontInfo HdrFont = LegendHeader->GetFont();
				HdrFont.Size = 15;
				HdrFont.TypefaceFontName = FName(TEXT("Bold"));
				LegendHeader->SetFont(HdrFont);
				UVerticalBoxSlot* HdrSlotRef = SidebarVBox->AddChildToVerticalBox(LegendHeader);
				if (HdrSlotRef) { HdrSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
			}

			// Terrain legend entries
			auto AddLegendEntry = [&](const FString& Label, const FLinearColor& Color)
			{
				UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

				UBorder* ColorSwatch = WidgetTree->ConstructWidget<UBorder>();
				ColorSwatch->SetBrushColor(Color);
				USizeBox* SwatchSize = WidgetTree->ConstructWidget<USizeBox>();
				SwatchSize->SetWidthOverride(14.0f);
				SwatchSize->SetHeightOverride(14.0f);
				SwatchSize->AddChild(ColorSwatch);
				UHorizontalBoxSlot* SwSlotRef = Row->AddChildToHorizontalBox(SwatchSize);
				if (SwSlotRef) { SwSlotRef->SetVerticalAlignment(VAlign_Center); SwSlotRef->SetPadding(FMargin(0, 0, 6, 0)); }

				UTextBlock* EntryText = WidgetTree->ConstructWidget<UTextBlock>();
				EntryText->SetText(FText::FromString(Label));
				EntryText->SetColorAndOpacity(FSlateColor(MapColours::Silver));
				FSlateFontInfo EFont = EntryText->GetFont();
				EFont.Size = 11;
				EntryText->SetFont(EFont);
				UHorizontalBoxSlot* ESlotRef = Row->AddChildToHorizontalBox(EntryText);
				if (ESlotRef) { ESlotRef->SetVerticalAlignment(VAlign_Center); }

				UVerticalBoxSlot* RowSlotRef = SidebarVBox->AddChildToVerticalBox(Row);
				if (RowSlotRef) { RowSlotRef->SetPadding(FMargin(0, 1)); }
			};

			AddLegendEntry(TEXT("City (gold dot)"), MapColours::Gold);
			AddLegendEntry(TEXT("Army (wizard color)"), MapColours::Silver);
			AddLegendEntry(TEXT("Grassland"), FLinearColor(0.15f, 0.45f, 0.15f));
			AddLegendEntry(TEXT("Hills"), FLinearColor(0.50f, 0.35f, 0.15f));
			AddLegendEntry(TEXT("Water"), FLinearColor(0.15f, 0.25f, 0.55f));
			AddLegendEntry(TEXT("Mountain"), FLinearColor(0.40f, 0.40f, 0.42f));

			// Divider
			{
				UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
				Div->SetBrushColor(MapColours::GoldDim);
				USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
				DivSize->SetHeightOverride(1.0f);
				DivSize->AddChild(Div);
				UVerticalBoxSlot* DivSlotRef = SidebarVBox->AddChildToVerticalBox(DivSize);
				if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); DivSlotRef->SetPadding(FMargin(0, 8)); }
			}

			// Navigation buttons
			CenterOnCapitalButton = CreateSidebarButton(SidebarVBox, TEXT("Center on Capital"));
			ZoomInButton          = CreateSidebarButton(SidebarVBox, TEXT("Zoom +"));
			ZoomOutButton         = CreateSidebarButton(SidebarVBox, TEXT("Zoom -"));

			// Spacer to push close to bottom
			{
				USpacer* Sp = WidgetTree->ConstructWidget<USpacer>();
				UVerticalBoxSlot* SpSlotRef = SidebarVBox->AddChildToVerticalBox(Sp);
				if (SpSlotRef) { SpSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
			}

			// Close button
			CloseButton = CreateSidebarButton(SidebarVBox, TEXT("Close"));

			UHorizontalBoxSlot* SideSlotRef = CenterRow->AddChildToHorizontalBox(SidebarSize);
			if (SideSlotRef) { SideSlotRef->SetVerticalAlignment(VAlign_Fill); SideSlotRef->SetPadding(FMargin(0, 0, 4, 0)); }
		}
	}

	// ── Gold divider above bottom bar ────────────────────────────────────
	{
		UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
		Div->SetBrushColor(MapColours::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(2.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* DivSlotRef = RootVBox->AddChildToVerticalBox(DivSize);
		if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); }
	}

	// ══════════════════════════════════════════════════════════════════════
	// BOTTOM BAR: Coordinates + Turn counter
	// ══════════════════════════════════════════════════════════════════════
	{
		UBorder* BottomBar = WidgetTree->ConstructWidget<UBorder>();
		BottomBar->SetBrushColor(MapColours::PanelBg);
		BottomBar->SetPadding(FMargin(16.0f, 6.0f));

		UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		BottomBar->AddChild(BottomRow);

		CoordinateText = WidgetTree->ConstructWidget<UTextBlock>();
		CoordinateText->SetText(FText::FromString(TEXT("Position: (0, 0)")));
		CoordinateText->SetColorAndOpacity(FSlateColor(MapColours::Grey));
		FSlateFontInfo CoordFont = CoordinateText->GetFont();
		CoordFont.Size = 12;
		CoordinateText->SetFont(CoordFont);
		UHorizontalBoxSlot* CoordSlotRef = BottomRow->AddChildToHorizontalBox(CoordinateText);
		if (CoordSlotRef) { CoordSlotRef->SetVerticalAlignment(VAlign_Center); }

		USpacer* BotSpacer = WidgetTree->ConstructWidget<USpacer>();
		UHorizontalBoxSlot* SpSlotRef = BottomRow->AddChildToHorizontalBox(BotSpacer);
		if (SpSlotRef) { SpSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

		TurnCounterText = WidgetTree->ConstructWidget<UTextBlock>();
		TurnCounterText->SetText(FText::FromString(TEXT("Turn: 1")));
		TurnCounterText->SetColorAndOpacity(FSlateColor(MapColours::Gold));
		FSlateFontInfo TurnFont = TurnCounterText->GetFont();
		TurnFont.Size = 13;
		TurnCounterText->SetFont(TurnFont);
		UHorizontalBoxSlot* TurnSlotRef = BottomRow->AddChildToHorizontalBox(TurnCounterText);
		if (TurnSlotRef) { TurnSlotRef->SetVerticalAlignment(VAlign_Center); }

		UVerticalBoxSlot* BotBarSlotRef = RootVBox->AddChildToVerticalBox(BottomBar);
		if (BotBarSlotRef) { BotBarSlotRef->SetHorizontalAlignment(HAlign_Fill); }
	}
}
