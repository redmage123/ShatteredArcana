// Copyright Mythforge Studios. All Rights Reserved.
// CoMPlaneNexusWidget.cpp -- Planar connection map implementation.

#include "CoMPlaneNexusWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"

namespace NexusColors
{
	static const FLinearColor BgDark     = FLinearColor(0.015f, 0.010f, 0.040f, 0.95f);
	static const FLinearColor PanelBg    = FLinearColor(0.035f, 0.025f, 0.080f, 0.92f);
	static const FLinearColor Gold       = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim    = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver     = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor BtnNormal  = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover   = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor LineDim    = FLinearColor(0.3f, 0.3f, 0.4f, 0.5f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMPlaneNexusWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

// =============================================================================
// BuildLayout
// =============================================================================

void UCoMPlaneNexusWidget::BuildLayout()
{
	PlaneNodeBorders.Empty();
	PlaneNodeButtons.Empty();

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	Background->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = Background;

	UOverlay* ScreenOverlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->AddChild(ScreenOverlay);

	// Gold border panel
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(NexusColors::Gold);
	PanelBorder->SetPadding(FMargin(2.0f));

	UBorder* PanelInner = WidgetTree->ConstructWidget<UBorder>();
	PanelInner->SetBrushColor(NexusColors::PanelBg);
	PanelInner->SetPadding(FMargin(16.0f, 12.0f));
	PanelBorder->AddChild(PanelInner);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(700.0f);
	PanelSize->SetHeightOverride(600.0f);
	PanelSize->AddChild(PanelBorder);

	UOverlaySlot* PanelSlotRef = ScreenOverlay->AddChildToOverlay(PanelSize);
	if (PanelSlotRef)
	{
		PanelSlotRef->SetHorizontalAlignment(HAlign_Center);
		PanelSlotRef->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelInner->AddChild(ContentBox);

	// ── Header ───────────────────────────────────────────────────────────────
	{
		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(FText::FromString(TEXT("The Planar Nexus")));
		HeaderText->SetColorAndOpacity(FSlateColor(NexusColors::Gold));
		FSlateFontInfo TitleFont = HeaderText->GetFont();
		TitleFont.Size = 24;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		HeaderText->SetFont(TitleFont);

		UVerticalBoxSlot* HeaderSlotRef = ContentBox->AddChildToVerticalBox(HeaderText);
		if (HeaderSlotRef) { HeaderSlotRef->SetPadding(FMargin(0, 0, 0, 10)); HeaderSlotRef->SetHorizontalAlignment(HAlign_Center); }
	}

	// ── Main area: canvas (left) + info panel (right) ────────────────────────
	{
		UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Canvas area
		{
			NexusCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();

			USizeBox* CanvasSize = WidgetTree->ConstructWidget<USizeBox>();
			CanvasSize->SetWidthOverride(400.0f);
			CanvasSize->SetHeightOverride(400.0f);

			UBorder* CanvasBg = WidgetTree->ConstructWidget<UBorder>();
			CanvasBg->SetBrushColor(FLinearColor(0.01f, 0.01f, 0.03f, 1.0f));
			CanvasBg->AddChild(NexusCanvas);
			CanvasSize->AddChild(CanvasBg);

			UHorizontalBoxSlot* CanvasSlotRef = MainRow->AddChildToHorizontalBox(CanvasSize);
			if (CanvasSlotRef) { CanvasSlotRef->SetPadding(FMargin(0, 0, 12, 0)); }

			// Place 8 nodes in a circle (center ~200, 200; radius ~140)
			const float CX = 160.f, CY = 160.f, Radius = 130.f;
			struct FPlaneInfo { ECoMPlane Plane; FString Name; };
			static const FPlaneInfo Planes[] = {
				{ ECoMPlane::Aurelith,   TEXT("Aurelith") },
				{ ECoMPlane::Noctharion, TEXT("Noctharion") },
				{ ECoMPlane::Verdantis,  TEXT("Verdantis") },
				{ ECoMPlane::Infernyx,   TEXT("Infernyx") },
				{ ECoMPlane::Aethermist, TEXT("Aethermist") },
				{ ECoMPlane::Abyssal,    TEXT("Abyssal") },
				{ ECoMPlane::Ethereal,   TEXT("Ethereal") },
				{ ECoMPlane::Feywild,    TEXT("Feywild") },
			};

			TArray<FVector2D> NodePositions;
			for (int32 i = 0; i < 8; ++i)
			{
				float Angle = (static_cast<float>(i) / 8.f) * 2.f * PI;
				float X = CX + Radius * FMath::Cos(Angle) - 40.f; // offset by half node size
				float Y = CY + Radius * FMath::Sin(Angle) - 40.f;
				NodePositions.Add(FVector2D(X, Y));

				UButton* Node = CreatePlaneNode(Planes[i].Plane, GetPlaneColor(Planes[i].Plane),
					Planes[i].Name, X, Y);
				PlaneNodeButtons.Add(Node);
			}

			// Connection lines between adjacent planes
			for (int32 i = 0; i < 8; ++i)
			{
				int32 Next = (i + 1) % 8;
				CreateConnectionLine(
					NodePositions[i].X + 40.f, NodePositions[i].Y + 40.f,
					NodePositions[Next].X + 40.f, NodePositions[Next].Y + 40.f);
			}
			// Cross connections: Aurelith-Aethermist, Noctharion-Abyssal
			CreateConnectionLine(
				NodePositions[0].X + 40.f, NodePositions[0].Y + 40.f,
				NodePositions[4].X + 40.f, NodePositions[4].Y + 40.f);
			CreateConnectionLine(
				NodePositions[1].X + 40.f, NodePositions[1].Y + 40.f,
				NodePositions[5].X + 40.f, NodePositions[5].Y + 40.f);
		}

		// Right info panel
		{
			UVerticalBox* InfoBox = WidgetTree->ConstructWidget<UVerticalBox>();

			PlaneNameText = WidgetTree->ConstructWidget<UTextBlock>();
			PlaneNameText->SetText(FText::FromString(TEXT("Select a Plane")));
			PlaneNameText->SetColorAndOpacity(FSlateColor(NexusColors::Gold));
			FSlateFontInfo NameFont = PlaneNameText->GetFont();
			NameFont.Size = 18;
			NameFont.TypefaceFontName = FName(TEXT("Bold"));
			PlaneNameText->SetFont(NameFont);

			UVerticalBoxSlot* NameSlotRef = InfoBox->AddChildToVerticalBox(PlaneNameText);
			if (NameSlotRef) { NameSlotRef->SetPadding(FMargin(0, 0, 0, 6)); }

			PlaneDescText = WidgetTree->ConstructWidget<UTextBlock>();
			PlaneDescText->SetText(FText::FromString(TEXT("Click a plane node to view details.")));
			PlaneDescText->SetColorAndOpacity(FSlateColor(NexusColors::Silver));
			PlaneDescText->SetAutoWrapText(true);
			FSlateFontInfo DescFont = PlaneDescText->GetFont();
			DescFont.Size = 12;
			PlaneDescText->SetFont(DescFont);

			UVerticalBoxSlot* DescSlotRef = InfoBox->AddChildToVerticalBox(PlaneDescText);
			if (DescSlotRef) { DescSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }

			PlaneRealmText = WidgetTree->ConstructWidget<UTextBlock>();
			PlaneRealmText->SetText(FText::GetEmpty());
			PlaneRealmText->SetColorAndOpacity(FSlateColor(NexusColors::Silver));
			FSlateFontInfo RealmFont = PlaneRealmText->GetFont();
			RealmFont.Size = 12;
			PlaneRealmText->SetFont(RealmFont);

			UVerticalBoxSlot* RealmSlotRef = InfoBox->AddChildToVerticalBox(PlaneRealmText);
			if (RealmSlotRef) { RealmSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

			PlaneStatsText = WidgetTree->ConstructWidget<UTextBlock>();
			PlaneStatsText->SetText(FText::GetEmpty());
			PlaneStatsText->SetColorAndOpacity(FSlateColor(NexusColors::Grey));
			FSlateFontInfo StatsFont = PlaneStatsText->GetFont();
			StatsFont.Size = 11;
			PlaneStatsText->SetFont(StatsFont);

			UVerticalBoxSlot* StatsSlotRef = InfoBox->AddChildToVerticalBox(PlaneStatsText);
			if (StatsSlotRef) { StatsSlotRef->SetPadding(FMargin(0, 0, 0, 12)); }

			// Travel button
			TravelButton = CreateActionButton(TEXT("Travel Here"), 150.f);
			USizeBox* TravelSize = WidgetTree->ConstructWidget<USizeBox>();
			TravelSize->SetWidthOverride(150.0f);
			TravelSize->SetHeightOverride(36.0f);
			TravelSize->AddChild(TravelButton);

			UVerticalBoxSlot* TravelSlotRef = InfoBox->AddChildToVerticalBox(TravelSize);
			if (TravelSlotRef) { TravelSlotRef->SetPadding(FMargin(0, 0, 0, 0)); }

			USizeBox* InfoSize = WidgetTree->ConstructWidget<USizeBox>();
			InfoSize->SetWidthOverride(220.0f);
			InfoSize->AddChild(InfoBox);

			UHorizontalBoxSlot* InfoSlotRef = MainRow->AddChildToHorizontalBox(InfoSize);
			if (InfoSlotRef) { InfoSlotRef->SetVerticalAlignment(VAlign_Top); }
		}

		UVerticalBoxSlot* MainSlotRef = ContentBox->AddChildToVerticalBox(MainRow);
		if (MainSlotRef) { MainSlotRef->SetPadding(FMargin(0, 0, 0, 10)); }
	}

	// ── Close button ─────────────────────────────────────────────────────────
	{
		CloseButton = CreateActionButton(TEXT("Close"), 140.f);

		USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>();
		BtnSize->SetWidthOverride(140.0f);
		BtnSize->SetHeightOverride(36.0f);
		BtnSize->AddChild(CloseButton);

		UVerticalBoxSlot* BtnSlotRef = ContentBox->AddChildToVerticalBox(BtnSize);
		if (BtnSlotRef) { BtnSlotRef->SetHorizontalAlignment(HAlign_Right); }
	}
}

UButton* UCoMPlaneNexusWidget::CreatePlaneNode(ECoMPlane Plane, const FLinearColor& Color,
	const FString& Name, float X, float Y)
{
	if (!NexusCanvas) { return nullptr; }

	UButton* NodeBtn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = NodeBtn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(Color);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(FLinearColor(Color.R * 1.3f, Color.G * 1.3f, Color.B * 1.3f, 1.0f));
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(Color);
	NodeBtn->SetStyle(Style);

	UTextBlock* NameLabel = WidgetTree->ConstructWidget<UTextBlock>();
	NameLabel->SetText(FText::FromString(Name));
	NameLabel->SetColorAndOpacity(FSlateColor(NexusColors::Silver));
	NameLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo NFont = NameLabel->GetFont();
	NFont.Size = 9;
	NameLabel->SetFont(NFont);

	UVerticalBox* NodeVBox = WidgetTree->ConstructWidget<UVerticalBox>();
	NodeVBox->AddChildToVerticalBox(NodeBtn);

	UVerticalBoxSlot* LabelSlotRef = NodeVBox->AddChildToVerticalBox(NameLabel);
	if (LabelSlotRef) { LabelSlotRef->SetHorizontalAlignment(HAlign_Center); }

	USizeBox* BtnSize = WidgetTree->ConstructWidget<USizeBox>();
	BtnSize->SetWidthOverride(80.0f);
	BtnSize->SetHeightOverride(100.0f); // 80 for button + 20 for label
	BtnSize->AddChild(NodeVBox);

	UCanvasPanelSlot* CanvasSlotRef = NexusCanvas->AddChildToCanvas(BtnSize);
	if (CanvasSlotRef)
	{
		CanvasSlotRef->SetPosition(FVector2D(X, Y));
		CanvasSlotRef->SetSize(FVector2D(80.0f, 100.0f));
	}

	return NodeBtn;
}

void UCoMPlaneNexusWidget::CreateConnectionLine(float X1, float Y1, float X2, float Y2)
{
	if (!NexusCanvas) { return; }

	// Approximate a line with a thin border widget
	float DX = X2 - X1;
	float DY = Y2 - Y1;
	float Length = FMath::Sqrt(DX * DX + DY * DY);

	UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
	Line->SetBrushColor(NexusColors::LineDim);

	USizeBox* LineSize = WidgetTree->ConstructWidget<USizeBox>();
	LineSize->SetWidthOverride(FMath::Max(Length, 2.0f));
	LineSize->SetHeightOverride(2.0f);
	LineSize->AddChild(Line);

	float MidX = (X1 + X2) * 0.5f;
	float MidY = (Y1 + Y2) * 0.5f;

	UCanvasPanelSlot* LineSlotRef = NexusCanvas->AddChildToCanvas(LineSize);
	if (LineSlotRef)
	{
		LineSlotRef->SetPosition(FVector2D(FMath::Min(X1, X2), MidY - 1.0f));
		LineSlotRef->SetSize(FVector2D(FMath::Abs(DX), 2.0f));
	}
}

UButton* UCoMPlaneNexusWidget::CreateActionButton(const FString& Label, float Width)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(NexusColors::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(NexusColors::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(NexusColors::BtnNormal);
	Btn->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(NexusColors::Silver));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 13;
	Text->SetFont(Font);

	Btn->AddChild(Text);
	return Btn;
}

// =============================================================================
// NativeConstruct
// =============================================================================

void UCoMPlaneNexusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)  { CloseButton->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnCloseClicked); }
	if (TravelButton) { TravelButton->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnTravelClicked); }

	// Bind plane node buttons
	if (PlaneNodeButtons.Num() > 0 && PlaneNodeButtons[0]) { PlaneNodeButtons[0]->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnPlane0Clicked); }
	if (PlaneNodeButtons.Num() > 1 && PlaneNodeButtons[1]) { PlaneNodeButtons[1]->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnPlane1Clicked); }
	if (PlaneNodeButtons.Num() > 2 && PlaneNodeButtons[2]) { PlaneNodeButtons[2]->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnPlane2Clicked); }
	if (PlaneNodeButtons.Num() > 3 && PlaneNodeButtons[3]) { PlaneNodeButtons[3]->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnPlane3Clicked); }
	if (PlaneNodeButtons.Num() > 4 && PlaneNodeButtons[4]) { PlaneNodeButtons[4]->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnPlane4Clicked); }
	if (PlaneNodeButtons.Num() > 5 && PlaneNodeButtons[5]) { PlaneNodeButtons[5]->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnPlane5Clicked); }
	if (PlaneNodeButtons.Num() > 6 && PlaneNodeButtons[6]) { PlaneNodeButtons[6]->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnPlane6Clicked); }
	if (PlaneNodeButtons.Num() > 7 && PlaneNodeButtons[7]) { PlaneNodeButtons[7]->OnClicked.AddDynamic(this, &UCoMPlaneNexusWidget::OnPlane7Clicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMPlaneNexusWidget::SetDiscoveredPlanes(const TSet<ECoMPlane>& Discovered)
{
	DiscoveredPlanes = Discovered;

	// Dim undiscovered nodes
	static const ECoMPlane PlaneOrder[] = {
		ECoMPlane::Aurelith, ECoMPlane::Noctharion, ECoMPlane::Verdantis, ECoMPlane::Infernyx,
		ECoMPlane::Aethermist, ECoMPlane::Abyssal, ECoMPlane::Ethereal, ECoMPlane::Feywild
	};

	for (int32 i = 0; i < PlaneNodeBorders.Num() && i < 8; ++i)
	{
		if (PlaneNodeBorders[i])
		{
			float Opacity = Discovered.Contains(PlaneOrder[i]) ? 1.0f : 0.3f;
			FLinearColor Color = GetPlaneColor(PlaneOrder[i]);
			Color.A = Opacity;
			PlaneNodeBorders[i]->SetBrushColor(Color);
		}
	}
}

void UCoMPlaneNexusWidget::SelectPlane(ECoMPlane Plane)
{
	SelectedPlane = Plane;

	static const FString PlaneNames[] = {
		TEXT("Aurelith"), TEXT("Noctharion"), TEXT("Verdantis"), TEXT("Infernyx"),
		TEXT("Aethermist"), TEXT("Abyssal"), TEXT("Ethereal"), TEXT("Feywild")
	};

	int32 Idx = static_cast<int32>(Plane);
	FString PlaneName = (Idx >= 0 && Idx < 8) ? PlaneNames[Idx] : TEXT("Unknown");

	if (PlaneNameText)  { PlaneNameText->SetText(FText::FromString(PlaneName)); }
	if (PlaneDescText)  { PlaneDescText->SetText(FText::FromString(GetPlaneDescription(Plane))); }
	if (PlaneRealmText) { PlaneRealmText->SetText(FText::FromString(FString::Printf(TEXT("Aligned Realm: %s"), *GetPlaneRealm(Plane)))); }
	if (PlaneStatsText) { PlaneStatsText->SetText(FText::FromString(TEXT("Cities: -- | Armies: --"))); }

	if (TravelButton)
	{
		TravelButton->SetIsEnabled(DiscoveredPlanes.Contains(Plane));
	}
}

FLinearColor UCoMPlaneNexusWidget::GetPlaneColor(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return FLinearColor(0.290f, 0.549f, 0.247f, 1.0f); // #4a8c3f
	case ECoMPlane::Noctharion: return FLinearColor(0.353f, 0.227f, 0.478f, 1.0f); // #5a3a7a
	case ECoMPlane::Verdantis:  return FLinearColor(0.165f, 0.502f, 0.251f, 1.0f); // #2a8040
	case ECoMPlane::Infernyx:   return FLinearColor(0.541f, 0.188f, 0.063f, 1.0f); // #8a3010
	case ECoMPlane::Aethermist: return FLinearColor(0.314f, 0.439f, 0.627f, 1.0f); // #5070a0
	case ECoMPlane::Abyssal:    return FLinearColor(0.478f, 0.125f, 0.125f, 1.0f); // #7a2020
	case ECoMPlane::Ethereal:   return FLinearColor(0.251f, 0.376f, 0.627f, 1.0f); // #4060a0
	case ECoMPlane::Feywild:    return FLinearColor(0.478f, 0.251f, 0.502f, 1.0f); // #7a4080
	default:                    return NexusColors::Grey;
	}
}

FString UCoMPlaneNexusWidget::GetPlaneDescription(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return TEXT("Golden high-fantasy realm of light and order.");
	case ECoMPlane::Noctharion: return TEXT("Dark arcane shadow realm of death and secrets.");
	case ECoMPlane::Verdantis:  return TEXT("Primal nature realm of growth and beasts.");
	case ECoMPlane::Infernyx:   return TEXT("Fire and iron dual realm of volcanoes and devils.");
	case ECoMPlane::Aethermist: return TEXT("Celestial spirit realm of sorcery and illusion.");
	case ECoMPlane::Abyssal:    return TEXT("Chaotic demon realm of destruction and madness.");
	case ECoMPlane::Ethereal:   return TEXT("Alien spirit realm of dreams and thought.");
	case ECoMPlane::Feywild:    return TEXT("Ever-shifting fey realm of glamour and two courts.");
	default:                    return TEXT("Unknown plane.");
	}
}

FString UCoMPlaneNexusWidget::GetPlaneRealm(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return TEXT("Life");
	case ECoMPlane::Noctharion: return TEXT("Death");
	case ECoMPlane::Verdantis:  return TEXT("Nature");
	case ECoMPlane::Infernyx:   return TEXT("Chaos / Binding");
	case ECoMPlane::Aethermist: return TEXT("Sorcery");
	case ECoMPlane::Abyssal:    return TEXT("Chaos");
	case ECoMPlane::Ethereal:   return TEXT("Spirit");
	case ECoMPlane::Feywild:    return TEXT("Glamour");
	default:                    return TEXT("None");
	}
}

void UCoMPlaneNexusWidget::OnCloseClicked()   { SetVisibility(ESlateVisibility::Collapsed); }
void UCoMPlaneNexusWidget::OnTravelClicked()   { /* Route through game subsystem */ }

void UCoMPlaneNexusWidget::OnPlane0Clicked() { SelectPlane(ECoMPlane::Aurelith); }
void UCoMPlaneNexusWidget::OnPlane1Clicked() { SelectPlane(ECoMPlane::Noctharion); }
void UCoMPlaneNexusWidget::OnPlane2Clicked() { SelectPlane(ECoMPlane::Verdantis); }
void UCoMPlaneNexusWidget::OnPlane3Clicked() { SelectPlane(ECoMPlane::Infernyx); }
void UCoMPlaneNexusWidget::OnPlane4Clicked() { SelectPlane(ECoMPlane::Aethermist); }
void UCoMPlaneNexusWidget::OnPlane5Clicked() { SelectPlane(ECoMPlane::Abyssal); }
void UCoMPlaneNexusWidget::OnPlane6Clicked() { SelectPlane(ECoMPlane::Ethereal); }
void UCoMPlaneNexusWidget::OnPlane7Clicked() { SelectPlane(ECoMPlane::Feywild); }
