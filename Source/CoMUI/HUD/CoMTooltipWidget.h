// Copyright Mythforge Studios. All Rights Reserved.
// CoMTooltipWidget.h -- Floating context-sensitive tooltip for tiles, cities, armies, units.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMTooltipWidget.generated.h"

class UTextBlock;
class UBorder;
class UVerticalBox;
class UImage;
class UCoMWorldMapSubsystem;
class UCoMCitySubsystem;
class UCoMUnitSubsystem;

/**
 * UCoMTooltipWidget
 *
 * Compact floating panel that appears when hovering over tiles, units, cities,
 * or armies. Content adapts to the hovered context. Dark panel (#0f3460) with
 * gold border, max 200px wide.
 *
 * Position follows the mouse cursor with a (15, 15) pixel offset, clamped to
 * the viewport edges.
 */
UCLASS()
class COMUI_API UCoMTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// -- Show Methods ----------------------------------------------------------

	/**
	 * Show tooltip for a map tile: terrain name, plane, movement cost,
	 * resource yield, corruption status.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Tooltip")
	void ShowTileTooltip(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint TilePos, int32 WizardId);

	/**
	 * Show tooltip for a city: name, population, owner wizard, building count,
	 * garrison strength.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Tooltip")
	void ShowCityTooltip(int32 CityId);

	/**
	 * Show tooltip for an army: composition (unit count, total HP), owner wizard,
	 * hero name if present.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Tooltip")
	void ShowArmyTooltip(int32 ArmyId);

	/**
	 * Show tooltip for a single unit (e.g. in army panel): name, HP bar,
	 * attack/defense/movement stats.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Tooltip")
	void ShowUnitTooltip(int32 UnitId);

	/** Hide the tooltip. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Tooltip")
	void Hide();

	/**
	 * Set the tooltip screen position (follows mouse).
	 * Applies a (15, 15) offset and clamps to viewport bounds.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Tooltip")
	void SetScreenPosition(FVector2D ScreenPos);

protected:
	// -- Widget references (bound from UMG or created in C++) ------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TooltipBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailLine1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailLine2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailLine3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailLine4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TooltipIcon;

private:
	/** Build the tooltip layout in C++ when UMG bindings are not available. */
	void BuildDefaultLayout();

	/** Set detail lines. Empty strings hide that line. */
	void SetContent(const FString& Title, const FString& Line1, const FString& Line2 = TEXT(""),
	                const FString& Line3 = TEXT(""), const FString& Line4 = TEXT(""));

	/** Configure detail text block appearance (grey, 11pt). */
	void ConfigureDetailText(UTextBlock* Text);

	/** Subsystem accessors. */
	UCoMWorldMapSubsystem* GetWorldMap();
	UCoMCitySubsystem* GetCitySub();
	UCoMUnitSubsystem* GetUnitSub();

	/** Cached subsystem pointers. */
	TWeakObjectPtr<UCoMWorldMapSubsystem> CachedWorldMap;
	TWeakObjectPtr<UCoMCitySubsystem> CachedCitySub;
	TWeakObjectPtr<UCoMUnitSubsystem> CachedUnitSub;

	/** Whether the tooltip is currently visible. */
	bool bIsShowing = false;

	/** Tooltip offset from cursor in pixels. */
	static constexpr float CursorOffsetX = 15.f;
	static constexpr float CursorOffsetY = 15.f;

	/** Maximum tooltip width. */
	static constexpr float MaxTooltipWidth = 200.f;

	/** Gold color for title text. */
	static FLinearColor GoldColor() { return FLinearColor(0.85f, 0.65f, 0.13f, 1.f); }

	/** Grey color for detail text. */
	static FLinearColor GreyColor() { return FLinearColor(0.7f, 0.7f, 0.7f, 1.f); }

	/** Dark panel background color (#0f3460). */
	static FLinearColor PanelColor() { return FLinearColor(0.059f, 0.204f, 0.376f, 0.95f); }

	/** Gold border color. */
	static FLinearColor BorderColor() { return FLinearColor(0.85f, 0.65f, 0.13f, 1.f); }
};
