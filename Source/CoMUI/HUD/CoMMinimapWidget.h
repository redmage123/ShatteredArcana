// Copyright Mythforge Studios. All Rights Reserved.
// CoMMinimapWidget.h -- 200x200 minimap widget for the overworld HUD.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMMinimapWidget.generated.h"

class FSlateWindowElementList;
class UImage;
class UTextBlock;
class UButton;
class UTexture2D;
class UBorder;
class USizeBox;
class UVerticalBox;
class UHorizontalBox;
class UOverlay;
class UCoMMinimapSubsystem;
class UCoMCitySubsystem;
class UCoMUnitSubsystem;
class ACoMCameraPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMinimapNavigate, FIntPoint, WorldPosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMinimapPlaneChanged, ECoMPlane, NewPlane);

/**
 * UCoMMinimapWidget
 *
 * A 200x200 pixel minimap showing the current plane's terrain, fog of war,
 * cities, and armies. Renders a 160x100 UTexture2D and scales it up with
 * nearest-neighbor filtering to fill the widget.
 *
 * Visual layers (bottom to top):
 *   1. Terrain colors (one pixel per tile)
 *   2. Fog of war overlay (black = unexplored, darkened = explored)
 *   3. City markers (2x2 colored dots)
 *   4. Army markers (1x1 colored dots)
 *   5. Camera viewport indicator (white rectangle outline, drawn in OnPaint)
 *
 * Integration:
 *   - Embedded inside CoMHUDWidget's MinimapFrame border
 *   - Click on minimap fires OnMinimapNavigate to move the camera
 *   - Plane indicator text and left/right arrows below the map
 */
UCLASS()
class COMUI_API UCoMMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// -- Initialization -------------------------------------------------------

	/**
	 * Set up the minimap for a given wizard and starting plane.
	 * Creates the texture, subscribes to subsystems, and does a full redraw.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void InitializeMinimap(int32 WizardId, ECoMPlane StartingPlane);

	// -- Refresh Methods ------------------------------------------------------

	/** Full redraw -- call on plane change, turn end, or fog update. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void RefreshMinimap();

	/** Dynamic update -- armies + camera rect only. Fast, safe to call every frame. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void UpdateDynamic();

	/** Switch to a different plane and full-redraw. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void SetPlane(ECoMPlane NewPlane);

	/** Cycle to the next plane (wraps around). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void CycleNextPlane();

	/** Cycle to the previous plane (wraps around). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Minimap")
	void CyclePrevPlane();

	// -- Delegates ------------------------------------------------------------

	/** Fired when the user clicks on the minimap. Carries the world tile position. */
	UPROPERTY(BlueprintAssignable, Category = "CoM|Minimap")
	FOnMinimapNavigate OnMinimapNavigate;

	/** Fired when the plane changes (via arrows or SetPlane). */
	UPROPERTY(BlueprintAssignable, Category = "CoM|Minimap")
	FOnMinimapPlaneChanged OnMinimapPlaneChanged;

protected:
	// -- UUserWidget overrides ------------------------------------------------

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                          const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                          int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// -- Widget bindings (optional -- can also be constructed in C++) ---------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> MinimapImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlaneNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PrevPlaneButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextPlaneButton;

private:
	// -- Internal helpers -----------------------------------------------------

	/** Build the entire minimap layout in C++. */
	void BuildLayout();

	/** Create the 160x100 dynamic texture. */
	void CreateMinimapTexture();

	/** Copy the subsystem's pixel buffer into the UTexture2D. */
	void UploadPixelBufferToTexture();

	/** Gather city positions from the city subsystem, split by ownership. */
	void GatherCityPositions(TArray<FIntPoint>& OutPlayer, TArray<FIntPoint>& OutEnemy,
	                         TArray<FIntPoint>& OutNeutral) const;

	/** Gather army positions from the unit subsystem, split by ownership. */
	void GatherArmyPositions(TArray<FIntPoint>& OutPlayer, TArray<FIntPoint>& OutEnemy) const;

	/** Compute camera viewport rect in tile coordinates. */
	void ComputeCameraRect(FIntPoint& OutTopLeft, FIntPoint& OutBottomRight) const;

	/** Convert a local widget-space position to minimap UV [0,1]. */
	FVector2D LocalPositionToMinimapUV(const FGeometry& Geo, FVector2D LocalPos) const;

	// -- Button callbacks -----------------------------------------------------

	UFUNCTION()
	void OnPrevPlaneClicked();

	UFUNCTION()
	void OnNextPlaneClicked();

	// -- State ----------------------------------------------------------------

	UPROPERTY()
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY()
	TObjectPtr<UTexture2D> MinimapTexture;

	UPROPERTY()
	TObjectPtr<UCoMMinimapSubsystem> MinimapSub;

	UPROPERTY()
	TObjectPtr<UCoMCitySubsystem> CitySub;

	UPROPERTY()
	TObjectPtr<UCoMUnitSubsystem> UnitSub;

	int32 WizardId = 0;
	ECoMPlane CurrentPlane = ECoMPlane::Aurelith;
	bool bInitialized = false;

	/** Accumulator for throttling dynamic updates (not every frame). */
	float DynamicUpdateAccumulator = 0.f;

	/** Seconds between dynamic updates (armies + camera). */
	static constexpr float DynamicUpdateInterval = 0.1f;

	/** Minimap texture dimensions (one pixel per tile). */
	static constexpr int32 TexWidth  = CoM::MAP_WIDTH;   // 160
	static constexpr int32 TexHeight = CoM::MAP_HEIGHT;   // 100

	/** Display size of the minimap widget in pixels. */
	static constexpr float DisplaySize = 200.f;

	/** Gold border color. */
	static const FLinearColor BorderColor;

	/** White camera rect color. */
	static const FLinearColor CameraRectColor;
};
