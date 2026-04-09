// Copyright Mythforge Studios. All Rights Reserved.
// CoMTooltipWidget.cpp -- Floating context-sensitive tooltip implementation.

#include "CoMTooltipWidget.h"

#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"

// =============================================================================
// Construction & Tick
// =============================================================================

void UCoMTooltipWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildDefaultLayout();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCoMTooltipWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsShowing)
	{
		// Follow mouse position each frame.
		APlayerController* PC = GetOwningPlayer();
		if (PC)
		{
			float MouseX, MouseY;
			if (PC->GetMousePosition(MouseX, MouseY))
			{
				SetScreenPosition(FVector2D(MouseX, MouseY));
			}
		}
	}
}

// =============================================================================
// Show Methods
// =============================================================================

void UCoMTooltipWidget::ShowTileTooltip(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint TilePos, int32 WizardId)
{
	UCoMWorldMapSubsystem* WorldMap = GetWorldMap();
	if (!WorldMap)
	{
		return;
	}

	const FCoMTileData* Tile = WorldMap->GetTileAtPos(Plane, Layer, TilePos);
	if (!Tile)
	{
		Hide();
		return;
	}

	// Terrain name.
	const UEnum* TerrainEnum = StaticEnum<ECoMTerrain>();
	FString TerrainName = TerrainEnum
		? TerrainEnum->GetDisplayNameTextByValue(static_cast<int64>(Tile->Terrain)).ToString()
		: TEXT("Unknown");

	// Plane name.
	const UEnum* PlaneEnum = StaticEnum<ECoMPlane>();
	FString PlaneName = PlaneEnum
		? PlaneEnum->GetDisplayNameTextByValue(static_cast<int64>(Plane)).ToString()
		: TEXT("Unknown");

	// Movement cost.
	FString MoveCost = FString::Printf(TEXT("Move Cost: %d"), Tile->MoveCostModifier.IsZero() ? 1 : static_cast<int32>(Tile->MoveCostModifier.ToDouble()));

	// Resource.
	FString ResourceStr;
	if (Tile->Resource != ECoMResource::None)
	{
		const UEnum* ResEnum = StaticEnum<ECoMResource>();
		ResourceStr = ResEnum
			? FString::Printf(TEXT("Resource: %s"), *ResEnum->GetDisplayNameTextByValue(static_cast<int64>(Tile->Resource)).ToString())
			: TEXT("Resource: Yes");
	}

	// Corruption.
	FString CorruptStr;
	if (Tile->Corruption != ECoMCorruptionType::None)
	{
		const UEnum* CorruptEnum = StaticEnum<ECoMCorruptionType>();
		CorruptStr = CorruptEnum
			? FString::Printf(TEXT("Corrupted: %s"), *CorruptEnum->GetDisplayNameTextByValue(static_cast<int64>(Tile->Corruption)).ToString())
			: TEXT("Corrupted");
	}

	FString Line2 = MoveCost;
	FString Line3 = ResourceStr;
	FString Line4 = CorruptStr;

	SetContent(TerrainName, FString::Printf(TEXT("Plane: %s"), *PlaneName), Line2, Line3, Line4);

	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsShowing = true;
}

void UCoMTooltipWidget::ShowCityTooltip(int32 CityId)
{
	UCoMCitySubsystem* CitySub = GetCitySub();
	if (!CitySub)
	{
		return;
	}

	const FCoMCityData* City = CitySub->GetCity(CityId);
	if (!City)
	{
		Hide();
		return;
	}

	FString Title = City->CityName.ToString();
	FString Line1 = FString::Printf(TEXT("Pop: %d  Owner: Wizard %d"), City->Population, City->OwnerWizardIndex);
	FString Line2 = FString::Printf(TEXT("Buildings: %d"), City->BuildingIDs.Num());

	// Garrison strength: count units in the garrison army.
	FString Line3;
	if (City->GarrisonArmyID >= 0)
	{
		UCoMUnitSubsystem* UnitSub = GetUnitSub();
		if (UnitSub)
		{
			const FCoMArmyGroup* Garrison = UnitSub->GetArmy(City->GarrisonArmyID);
			if (Garrison)
			{
				Line3 = FString::Printf(TEXT("Garrison: %d units"), Garrison->UnitIDs.Num());
			}
		}
	}
	if (Line3.IsEmpty())
	{
		Line3 = TEXT("Garrison: None");
	}

	SetContent(Title, Line1, Line2, Line3);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsShowing = true;
}

void UCoMTooltipWidget::ShowArmyTooltip(int32 ArmyId)
{
	UCoMUnitSubsystem* UnitSub = GetUnitSub();
	if (!UnitSub)
	{
		return;
	}

	const FCoMArmyGroup* Army = UnitSub->GetArmy(ArmyId);
	if (!Army)
	{
		Hide();
		return;
	}

	// Sum HP and find hero.
	int32 TotalHP = 0;
	FString HeroName;
	for (int32 UnitID : Army->UnitIDs)
	{
		const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitID);
		if (Unit)
		{
			TotalHP += Unit->CurrentHP;
			if (Unit->bIsHero && HeroName.IsEmpty())
			{
				HeroName = Unit->SpecID.ToString();
			}
		}
	}

	FString Title = FString::Printf(TEXT("Army (Wizard %d)"), Army->OwnerWizardIndex);
	FString Line1 = FString::Printf(TEXT("Units: %d  Total HP: %d"), Army->UnitIDs.Num(), TotalHP);
	FString Line2 = HeroName.IsEmpty() ? TEXT("No Hero") : FString::Printf(TEXT("Hero: %s"), *HeroName);

	SetContent(Title, Line1, Line2);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsShowing = true;
}

void UCoMTooltipWidget::ShowUnitTooltip(int32 UnitId)
{
	UCoMUnitSubsystem* UnitSub = GetUnitSub();
	if (!UnitSub)
	{
		return;
	}

	const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitId);
	if (!Unit)
	{
		Hide();
		return;
	}

	FString Title = Unit->SpecID.ToString();
	FString Line1 = FString::Printf(TEXT("HP: %d / %d"), Unit->CurrentHP, Unit->MaxHP);
	FString Line2 = FString::Printf(TEXT("Level: %d  XP: %d"), Unit->Level, Unit->Experience);
	FString Line3 = Unit->bFlying ? TEXT("Flying") : TEXT("Ground");

	SetContent(Title, Line1, Line2, Line3);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsShowing = true;
}

void UCoMTooltipWidget::Hide()
{
	bIsShowing = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

// =============================================================================
// Positioning
// =============================================================================

void UCoMTooltipWidget::SetScreenPosition(FVector2D ScreenPos)
{
	// Apply offset.
	FVector2D Pos = ScreenPos + FVector2D(CursorOffsetX, CursorOffsetY);

	// Clamp to viewport.
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);

		// Estimate tooltip size for clamping (use max width and approximate height).
		const float EstHeight = 100.f;
		if (Pos.X + MaxTooltipWidth > ViewportSize.X)
		{
			Pos.X = ScreenPos.X - MaxTooltipWidth - CursorOffsetX;
		}
		if (Pos.Y + EstHeight > ViewportSize.Y)
		{
			Pos.Y = ScreenPos.Y - EstHeight - CursorOffsetY;
		}
		Pos.X = FMath::Max(Pos.X, 0.0);
		Pos.Y = FMath::Max(Pos.Y, 0.0);
	}

	SetPositionInViewport(Pos, false);
}

// =============================================================================
// Internal Layout
// =============================================================================

void UCoMTooltipWidget::BuildDefaultLayout()
{
	// If widgets are bound from UMG, configure their appearance.
	if (TitleText)
	{
		FSlateFontInfo FontInfo = TitleText->GetFont();
		FontInfo.Size = 13;
		TitleText->SetFont(FontInfo);
		TitleText->SetColorAndOpacity(FSlateColor(GoldColor()));
	}

	ConfigureDetailText(DetailLine1);
	ConfigureDetailText(DetailLine2);
	ConfigureDetailText(DetailLine3);
	ConfigureDetailText(DetailLine4);
}

void UCoMTooltipWidget::SetContent(const FString& Title, const FString& Line1,
                                   const FString& Line2, const FString& Line3,
                                   const FString& Line4)
{
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(Title));
	}

	auto SetLine = [](UTextBlock* Block, const FString& Text)
	{
		if (Block)
		{
			if (Text.IsEmpty())
			{
				Block->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				Block->SetText(FText::FromString(Text));
				Block->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
	};

	SetLine(DetailLine1, Line1);
	SetLine(DetailLine2, Line2);
	SetLine(DetailLine3, Line3);
	SetLine(DetailLine4, Line4);
}

void UCoMTooltipWidget::ConfigureDetailText(UTextBlock* Text)
{
	if (Text)
	{
		FSlateFontInfo FontInfo = Text->GetFont();
		FontInfo.Size = 11;
		Text->SetFont(FontInfo);
		Text->SetColorAndOpacity(FSlateColor(GreyColor()));
	}
}

// =============================================================================
// Subsystem Accessors
// =============================================================================

UCoMWorldMapSubsystem* UCoMTooltipWidget::GetWorldMap()
{
	if (CachedWorldMap.IsValid()) { return CachedWorldMap.Get(); }
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI) { CachedWorldMap = GI->GetSubsystem<UCoMWorldMapSubsystem>(); return CachedWorldMap.Get(); }
	return nullptr;
}

UCoMCitySubsystem* UCoMTooltipWidget::GetCitySub()
{
	if (CachedCitySub.IsValid()) { return CachedCitySub.Get(); }
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI) { CachedCitySub = GI->GetSubsystem<UCoMCitySubsystem>(); return CachedCitySub.Get(); }
	return nullptr;
}

UCoMUnitSubsystem* UCoMTooltipWidget::GetUnitSub()
{
	if (CachedUnitSub.IsValid()) { return CachedUnitSub.Get(); }
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI) { CachedUnitSub = GI->GetSubsystem<UCoMUnitSubsystem>(); return CachedUnitSub.Get(); }
	return nullptr;
}
