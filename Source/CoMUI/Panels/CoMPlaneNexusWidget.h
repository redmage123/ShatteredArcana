// Copyright Mythforge Studios. All Rights Reserved.
// CoMPlaneNexusWidget.h -- Planar connection map showing plane nodes and links.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMPlaneNexusWidget.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UOverlay;
class UCanvasPanel;
class UCanvasPanelSlot;

/**
 * UCoMPlaneNexusWidget
 *
 * Planar nexus map showing 8 plane nodes arranged in a circle with connection
 * lines between linked planes. Undiscovered planes are dimmed. Right panel
 * shows selected plane details and a "Travel Here" button.
 */
UCLASS()
class COMUI_API UCoMPlaneNexusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Set which planes have been discovered (undiscovered are dimmed). */
	UFUNCTION(BlueprintCallable, Category = "CoM|PlaneNexus")
	void SetDiscoveredPlanes(const TSet<ECoMPlane>& Discovered);

	/** Select a plane to show its details. */
	UFUNCTION(BlueprintCallable, Category = "CoM|PlaneNexus")
	void SelectPlane(ECoMPlane Plane);

protected:
	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnTravelClicked();

	// Plane click callbacks
	UFUNCTION() void OnPlane0Clicked();
	UFUNCTION() void OnPlane1Clicked();
	UFUNCTION() void OnPlane2Clicked();
	UFUNCTION() void OnPlane3Clicked();
	UFUNCTION() void OnPlane4Clicked();
	UFUNCTION() void OnPlane5Clicked();
	UFUNCTION() void OnPlane6Clicked();
	UFUNCTION() void OnPlane7Clicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlaneNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlaneDescText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlaneRealmText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlaneStatsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TravelButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> NexusCanvas;

private:
	void BuildLayout();
	UButton* CreateActionButton(const FString& Label, float Width = 140.f);

	/** Create a plane node on the canvas at the given position. */
	UButton* CreatePlaneNode(ECoMPlane Plane, const FLinearColor& Color, const FString& Name,
		float X, float Y);

	/** Create a connection line between two points on the canvas. */
	void CreateConnectionLine(float X1, float Y1, float X2, float Y2);

	/** Get the theme color for a plane. */
	static FLinearColor GetPlaneColor(ECoMPlane Plane);

	/** Get a description for a plane. */
	static FString GetPlaneDescription(ECoMPlane Plane);

	/** Get the aligned realm name for a plane. */
	static FString GetPlaneRealm(ECoMPlane Plane);

	TSet<ECoMPlane> DiscoveredPlanes;
	ECoMPlane SelectedPlane = ECoMPlane::Aurelith;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> PlaneNodeBorders;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> PlaneNodeButtons;
};
