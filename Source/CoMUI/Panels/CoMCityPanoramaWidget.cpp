// Copyright Mythforge Studios. All Rights Reserved.
// CoMCityPanoramaWidget.cpp -- Visual city panorama implementation.

#include "CoMCityPanoramaWidget.h"

#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

// =============================================================================
// Building layout — positions in the panorama (960×480 canvas)
// =============================================================================

// Row 0 = back (y ~50-150), Row 1 = middle (y ~150-280), Row 2 = front (y ~280-400)
// Buildings are placed left-to-right within each row

static const TMap<FName, UCoMCityPanoramaWidget::FBuildingSlot>& GetBuildingLayoutMap()
{
	static TMap<FName, UCoMCityPanoramaWidget::FBuildingSlot> Layout;
	if (Layout.Num() == 0)
	{
		// Back row — large civic structures
		Layout.Add(FName("palace"),         {420, 30,  120, 0});
		Layout.Add(FName("cathedral"),      {280, 40,  110, 0});
		Layout.Add(FName("war_college"),    {580, 45,  105, 0});
		Layout.Add(FName("wizard_guild"),   {160, 50,  100, 0});
		Layout.Add(FName("mage_tower"),     {720, 25,  115, 0});
		Layout.Add(FName("observatory"),    {80,  55,  90,  0});

		// Middle row — standard buildings
		Layout.Add(FName("barracks"),       {60,  170, 85, 1});
		Layout.Add(FName("marketplace"),    {170, 160, 90, 1});
		Layout.Add(FName("library"),        {290, 165, 85, 1});
		Layout.Add(FName("temple"),         {400, 155, 90, 1});
		Layout.Add(FName("smithy"),         {510, 170, 80, 1});
		Layout.Add(FName("stable"),         {620, 175, 80, 1});
		Layout.Add(FName("granary"),        {730, 165, 80, 1});
		Layout.Add(FName("tavern"),         {840, 170, 80, 1});

		// Middle row — additional
		Layout.Add(FName("fighters_guild"), {120, 240, 80, 1});
		Layout.Add(FName("thieves_guild"),  {230, 245, 75, 1});
		Layout.Add(FName("armory"),         {340, 235, 80, 1});
		Layout.Add(FName("enchanter_workshop"), {450, 240, 80, 1});
		Layout.Add(FName("alchemist_lab"),  {560, 245, 75, 1});
		Layout.Add(FName("summoning_circle"), {670, 235, 85, 1});
		Layout.Add(FName("oracle"),         {780, 240, 75, 1});

		// Front row — infrastructure and small
		Layout.Add(FName("aqueduct"),       {100, 330, 80, 2});
		Layout.Add(FName("bank"),           {220, 335, 75, 2});
		Layout.Add(FName("colosseum"),      {350, 320, 90, 2});
		Layout.Add(FName("shrine"),         {480, 335, 70, 2});
		Layout.Add(FName("memorial"),       {590, 340, 65, 2});
		Layout.Add(FName("lighthouse"),     {700, 325, 80, 2});
		Layout.Add(FName("planar_beacon"),  {810, 330, 75, 2});

		// Docks/shipyard — far right, partially off-screen for coastal feel
		Layout.Add(FName("docks"),          {860, 380, 90, 2});
		Layout.Add(FName("shipyard"),       {870, 280, 85, 2});
		Layout.Add(FName("dragon_roost"),   {30,  100, 110, 0});

		// Walls — span the bottom/edges
		Layout.Add(FName("walls_wood"),     {0,   390, 960, 2});
		Layout.Add(FName("walls_stone"),    {0,   390, 960, 2});
		Layout.Add(FName("walls_iron"),     {0,   390, 960, 2});
	}
	return Layout;
}

UCoMCityPanoramaWidget::FBuildingSlot UCoMCityPanoramaWidget::GetSlotForBuilding(FName BuildingID)
{
	const auto& Layout = GetBuildingLayoutMap();
	if (const FBuildingSlot* Found = Layout.Find(BuildingID))
	{
		return *Found;
	}
	// Default: place unknown buildings in middle area
	static int32 UnknownOffset = 0;
	FBuildingSlot Default = {100.0f + (UnknownOffset++ * 90) % 700, 200.0f, 75.0f, 1};
	return Default;
}

// =============================================================================
// Widget lifecycle
// =============================================================================

TSharedRef<SWidget> UCoMCityPanoramaWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		// Create the panorama canvas structure
		RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = RootOverlay;

		// Background image (plane-specific)
		BackgroundImage = WidgetTree->ConstructWidget<UImage>();
		UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(BackgroundImage);
		if (BgSlot)
		{
			BgSlot->SetHorizontalAlignment(HAlign_Fill);
			BgSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// Canvas for building placement
		BuildingCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		UOverlaySlot* CanvasSlot = RootOverlay->AddChildToOverlay(BuildingCanvas);
		if (CanvasSlot)
		{
			CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
			CanvasSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	return Super::RebuildWidget();
}

void UCoMCityPanoramaWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

// =============================================================================
// Public API
// =============================================================================

void UCoMCityPanoramaWidget::UpdatePanorama(
	const TArray<FName>& BuildingIDs,
	FName InProductionID,
	float ProductionProgress,
	ECoMPlane Plane,
	int32 WallLevel,
	int32 Population)
{
	ClearBuildings();

	// Set background based on plane
	UTexture2D* BgTex = LoadBackgroundTexture(Plane);
	if (BgTex && BackgroundImage)
	{
		FSlateBrush BgBrush;
		BgBrush.SetResourceObject(BgTex);
		BgBrush.ImageSize = FVector2D(960.0f, 480.0f);
		BgBrush.DrawAs = ESlateBrushDrawType::Image;
		BgBrush.Tiling = ESlateBrushTileType::NoTile;
		BackgroundImage->SetBrush(BgBrush);
	}

	if (!BuildingCanvas) { return; }

	// Place completed buildings
	for (const FName& BuildingID : BuildingIDs)
	{
		FBuildingSlot SlotInfo = GetSlotForBuilding(BuildingID);

		// Special handling for walls — only show highest level
		if (BuildingID == FName("walls_wood") && WallLevel != 1) continue;
		if (BuildingID == FName("walls_stone") && WallLevel != 2) continue;
		if (BuildingID == FName("walls_iron") && WallLevel != 3) continue;

		UTexture2D* Tex = LoadBuildingTexture(BuildingID.ToString(), true);
		if (Tex)
		{
			PlaceBuildingSprite(BuildingID.ToString() + "_complete", SlotInfo.X, SlotInfo.Y, SlotInfo.Size);
		}
	}

	// Place building under construction (semi-transparent)
	if (!InProductionID.IsNone())
	{
		FBuildingSlot SlotInfo = GetSlotForBuilding(InProductionID);
		PlaceBuildingSprite(InProductionID.ToString() + "_construction", SlotInfo.X, SlotInfo.Y, SlotInfo.Size);
	}
}

// =============================================================================
// Internal helpers
// =============================================================================

void UCoMCityPanoramaWidget::ClearBuildings()
{
	for (UImage* Img : PlacedBuildingImages)
	{
		if (Img && Img->GetParent())
		{
			Img->RemoveFromParent();
		}
	}
	PlacedBuildingImages.Empty();
}

void UCoMCityPanoramaWidget::PlaceBuildingSprite(const FString& TextureName, float X, float Y, float Size)
{
	if (!BuildingCanvas || !WidgetTree) { return; }

	// Try to load the cityview texture
	FString AssetPath = FString::Printf(
		TEXT("/Game/Textures/CityView/%s.%s"), *TextureName, *TextureName);
	UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *AssetPath);

	// Fallback to the processed buildings directory
	if (!Tex)
	{
		// Try the original building sprite as fallback
		FString BaseName = TextureName;
		BaseName.ReplaceInline(TEXT("_complete"), TEXT(""));
		BaseName.ReplaceInline(TEXT("_construction"), TEXT(""));
		AssetPath = FString::Printf(
			TEXT("/Game/Textures/Buildings/%s.%s"), *BaseName, *BaseName);
		Tex = LoadObject<UTexture2D>(nullptr, *AssetPath);
	}

	if (!Tex) { return; }

	UImage* BuildingImg = WidgetTree->ConstructWidget<UImage>();

	FSlateBrush Brush;
	Brush.SetResourceObject(Tex);
	Brush.ImageSize = FVector2D(Size, Size);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.Tiling = ESlateBrushTileType::NoTile;
	BuildingImg->SetBrush(Brush);

	// If it's a construction sprite, make it semi-transparent
	if (TextureName.Contains(TEXT("construction")))
	{
		BuildingImg->SetRenderOpacity(0.7f);
	}

	UCanvasPanelSlot* CanvasSlot = BuildingCanvas->AddChildToCanvas(BuildingImg);
	if (CanvasSlot)
	{
		CanvasSlot->SetPosition(FVector2D(X, Y));
		CanvasSlot->SetSize(FVector2D(Size, Size));
	}

	PlacedBuildingImages.Add(BuildingImg);
}

UTexture2D* UCoMCityPanoramaWidget::LoadBuildingTexture(const FString& BuildingName, bool bComplete)
{
	FString State = bComplete ? TEXT("complete") : TEXT("construction");
	FString AssetPath = FString::Printf(
		TEXT("/Game/Textures/CityView/%s_%s.%s_%s"),
		*BuildingName, *State, *BuildingName, *State);
	return LoadObject<UTexture2D>(nullptr, *AssetPath);
}

UTexture2D* UCoMCityPanoramaWidget::LoadBackgroundTexture(ECoMPlane Plane)
{
	FString PlaneName;
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   PlaneName = TEXT("aurelith"); break;
	case ECoMPlane::Noctharion: PlaneName = TEXT("noctharion"); break;
	case ECoMPlane::Verdantis:  PlaneName = TEXT("verdantis"); break;
	case ECoMPlane::Infernyx:   PlaneName = TEXT("infernyx"); break;
	case ECoMPlane::Aethermist: PlaneName = TEXT("aethermist"); break;
	case ECoMPlane::Abyssal:    PlaneName = TEXT("abyssal"); break;
	case ECoMPlane::Ethereal:   PlaneName = TEXT("ethereal"); break;
	case ECoMPlane::Feywild:    PlaneName = TEXT("feywild"); break;
	default:                    PlaneName = TEXT("aurelith"); break;
	}

	FString AssetPath = FString::Printf(
		TEXT("/Game/Textures/CityView/city_bg_%s.city_bg_%s"),
		*PlaneName, *PlaneName);
	return LoadObject<UTexture2D>(nullptr, *AssetPath);
}
