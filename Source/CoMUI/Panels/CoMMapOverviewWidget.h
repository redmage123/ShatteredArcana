// Copyright Mythforge Studios. All Rights Reserved.
// CoMMapOverviewWidget.h -- Full world map overview with plane selection.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMMapOverviewWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UOverlay;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;

/**
 * UCoMMapOverviewWidget
 *
 * Full-screen world map overview with plane tabs, layer toggles,
 * a large map area placeholder, city/army markers, and navigation controls.
 */
UCLASS()
class COMUI_API UCoMMapOverviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Switch to a specific plane. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Map")
	void SelectPlane(ECoMPlane Plane);

	/** Switch map layer. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Map")
	void SelectLayer(ECoMMapLayer Layer);

	/** Center the map on a world position. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Map")
	void CenterOnPosition(FIntPoint Position);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION() void OnCloseClicked();

	// Plane tab callbacks
	UFUNCTION() void OnAurelithTabClicked();
	UFUNCTION() void OnNoctharionTabClicked();
	UFUNCTION() void OnVerdantisTabClicked();
	UFUNCTION() void OnInfernyxTabClicked();
	UFUNCTION() void OnAethermistTabClicked();
	UFUNCTION() void OnAbyssalTabClicked();
	UFUNCTION() void OnEtherealTabClicked();
	UFUNCTION() void OnFeywildTabClicked();

	// Layer toggle callbacks
	UFUNCTION() void OnSurfaceClicked();
	UFUNCTION() void OnUnderdarkClicked();
	UFUNCTION() void OnUnderwaterClicked();

	// Navigation callbacks
	UFUNCTION() void OnCenterOnCapitalClicked();
	UFUNCTION() void OnZoomInClicked();
	UFUNCTION() void OnZoomOutClicked();

private:
	void BuildLayout();

	UButton* CreatePlaneTab(UHorizontalBox* Parent, const FString& Label, const FLinearColor& Color);
	UButton* CreateLayerButton(UHorizontalBox* Parent, const FString& Label);
	UButton* CreateSidebarButton(UVerticalBox* Parent, const FString& Label, float Width = 160.f);

	void UpdateMapDisplay();

	// -- Layout elements -------------------------------------------------------

	UPROPERTY() TObjectPtr<UBorder> BackgroundBorder;
	UPROPERTY() TObjectPtr<UHorizontalBox> PlaneTabsBox;
	UPROPERTY() TObjectPtr<UHorizontalBox> LayerButtonsBox;
	UPROPERTY() TObjectPtr<UBorder> MapAreaBorder;
	UPROPERTY() TObjectPtr<UTextBlock> MapPlaceholderText;
	UPROPERTY() TObjectPtr<UTextBlock> CoordinateText;
	UPROPERTY() TObjectPtr<UTextBlock> TurnCounterText;
	UPROPERTY() TObjectPtr<UButton> CloseButton;
	UPROPERTY() TObjectPtr<UButton> CenterOnCapitalButton;
	UPROPERTY() TObjectPtr<UButton> ZoomInButton;
	UPROPERTY() TObjectPtr<UButton> ZoomOutButton;

	// Plane tab buttons
	UPROPERTY() TObjectPtr<UButton> AurelithTab;
	UPROPERTY() TObjectPtr<UButton> NoctharionTab;
	UPROPERTY() TObjectPtr<UButton> VerdantisTab;
	UPROPERTY() TObjectPtr<UButton> InfernyxTab;
	UPROPERTY() TObjectPtr<UButton> AethermistTab;
	UPROPERTY() TObjectPtr<UButton> AbyssalTab;
	UPROPERTY() TObjectPtr<UButton> EtherealTab;
	UPROPERTY() TObjectPtr<UButton> FeywildTab;

	// Layer buttons
	UPROPERTY() TObjectPtr<UButton> SurfaceButton;
	UPROPERTY() TObjectPtr<UButton> UnderdarkButton;
	UPROPERTY() TObjectPtr<UButton> UnderwaterButton;

	ECoMPlane CurrentPlane = ECoMPlane::Aurelith;
	ECoMMapLayer CurrentLayer = ECoMMapLayer::Surface;
};
