// Copyright Mythforge Studios. All Rights Reserved.
// CoMTacticalCombatWidget.h -- Grid-based tactical battle screen.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMTacticalCombatWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class UOverlay;
class UProgressBar;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;

/**
 * UCoMTacticalCombatWidget
 *
 * Full-screen grid-based tactical battle screen.
 * Shows a 20x20 combat grid with terrain-colored cells, unit tokens,
 * a right sidebar for unit stats and actions, and top/bottom bars.
 */
UCLASS()
class COMUI_API UCoMTacticalCombatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Initialize battle data for attacker vs defender on a specific plane. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Combat")
	void InitBattle(int32 AttackerArmyId, int32 DefenderArmyId, ECoMPlane Plane = ECoMPlane::Aurelith);

	/** Select a cell on the combat grid. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Combat")
	void SelectCell(int32 X, int32 Y);

	/** Highlight movement range for a unit. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Combat")
	void HighlightMovementRange(int32 UnitId);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnMoveClicked();

	UFUNCTION()
	void OnAttackClicked();

	UFUNCTION()
	void OnDefendClicked();

	UFUNCTION()
	void OnCastSpellClicked();

	UFUNCTION()
	void OnWaitClicked();

	UFUNCTION()
	void OnFleeClicked();

	UFUNCTION()
	void OnAutoResolveClicked();

	UFUNCTION()
	void OnRetreatClicked();

private:
	/** Build the entire combat layout in C++. */
	void BuildLayout();

	/** Create a styled action button and add it to the given VBox. */
	UButton* CreateActionButton(UVerticalBox* Parent, const FString& Label, float Width = 180.f);

	/** Create and place a grid cell at (X,Y) on the canvas. */
	UBorder* CreateGridCell(int32 X, int32 Y);

	// -- Layout elements -------------------------------------------------------

	UPROPERTY() TObjectPtr<UBorder> BackgroundBorder;
	UPROPERTY() TObjectPtr<UCanvasPanel> GridCanvas;
	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UTextBlock> AttackerNameText;
	UPROPERTY() TObjectPtr<UTextBlock> DefenderNameText;
	UPROPERTY() TObjectPtr<UTextBlock> TerrainTypeText;
	UPROPERTY() TObjectPtr<UTextBlock> RoundCounterText;
	UPROPERTY() TObjectPtr<UTextBlock> SelectedUnitNameText;
	UPROPERTY() TObjectPtr<UProgressBar> SelectedUnitHPBar;
	UPROPERTY() TObjectPtr<UTextBlock> SelectedUnitStatsText;
	UPROPERTY() TObjectPtr<UTextBlock> TurnIndicatorText;
	UPROPERTY() TObjectPtr<UScrollBox> InitiativeScrollBox;
	UPROPERTY() TObjectPtr<UButton> MoveButton;
	UPROPERTY() TObjectPtr<UButton> AttackButton;
	UPROPERTY() TObjectPtr<UButton> DefendButton;
	UPROPERTY() TObjectPtr<UButton> CastSpellButton;
	UPROPERTY() TObjectPtr<UButton> WaitButton;
	UPROPERTY() TObjectPtr<UButton> FleeButton;
	UPROPERTY() TObjectPtr<UButton> AutoResolveButton;
	UPROPERTY() TObjectPtr<UButton> RetreatButton;
	UPROPERTY() TObjectPtr<UButton> CloseButton;

	/** Grid cell borders (20x20 = 400 entries, row-major). */
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> GridCells;

	int32 CurrentAttackerArmyId = -1;
	int32 CurrentDefenderArmyId = -1;
	int32 SelectedX = -1;
	int32 SelectedY = -1;
	int32 CurrentRound = 1;
	ECoMPlane CurrentPlane = ECoMPlane::Aurelith;

	static constexpr int32 GridSize = 20;
	static constexpr float CellSize = 30.0f;

	// ── Plane-specific color palettes ────────────────────────────────────

	struct FPlaneColors
	{
		FLinearColor Background;   // Overall atmosphere
		FLinearColor Ground;       // Default grid cell
		FLinearColor GroundAlt;    // Alternating cell (checkerboard)
		FLinearColor Highlight;    // Selected cell
		FLinearColor Water;        // Water terrain
		FLinearColor Wall;         // Obstacle/wall cells
		FLinearColor Sky;          // Top bar tint
		FString PlaneName;
	};

	/** Get the color palette for a given plane. */
	static FPlaneColors GetPlaneColors(ECoMPlane Plane);

	/** Apply plane colors to all grid cells and background. */
	void ApplyPlaneTheme(ECoMPlane Plane);
};
