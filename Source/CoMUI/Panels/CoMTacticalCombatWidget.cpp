// Copyright Mythforge Studios. All Rights Reserved.
// CoMTacticalCombatWidget.cpp -- Grid-based tactical battle screen implementation.

#include "CoMTacticalCombatWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

#include "CoMUI/CoMUISubsystem.h"
#include "CoMCore/Framework/CoMGameInstance.h"
#include "CoMCore/TacticalCombat/CoMTacticalCombatSubsystem.h"
#include "CoMCore/TacticalCombat/TacticalTypes.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Audio/CoMAudioSubsystem.h"
#include "Engine/Texture2D.h"

// =============================================================================
// Colour palette
// =============================================================================

namespace TacticalColours
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

	// Terrain cell colours
	static const FLinearColor Grass      = FLinearColor(0.15f, 0.45f, 0.15f, 1.0f);
	static const FLinearColor Hills      = FLinearColor(0.50f, 0.35f, 0.15f, 1.0f);
	static const FLinearColor Stone      = FLinearColor(0.40f, 0.40f, 0.42f, 1.0f);
	static const FLinearColor Water      = FLinearColor(0.15f, 0.25f, 0.55f, 1.0f);
	static const FLinearColor CellBorder = FLinearColor(0.20f, 0.20f, 0.25f, 0.6f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMTacticalCombatWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

// =============================================================================
// NativeConstruct — bind delegates
// =============================================================================

void UCoMTacticalCombatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton)       { CloseButton->OnClicked.AddDynamic(this, &UCoMTacticalCombatWidget::OnCloseClicked); }
	if (MoveButton)        { MoveButton->OnClicked.AddDynamic(this, &UCoMTacticalCombatWidget::OnMoveClicked); }
	if (AttackButton)      { AttackButton->OnClicked.AddDynamic(this, &UCoMTacticalCombatWidget::OnAttackClicked); }
	if (DefendButton)      { DefendButton->OnClicked.AddDynamic(this, &UCoMTacticalCombatWidget::OnDefendClicked); }
	if (CastSpellButton)   { CastSpellButton->OnClicked.AddDynamic(this, &UCoMTacticalCombatWidget::OnCastSpellClicked); }
	if (WaitButton)        { WaitButton->OnClicked.AddDynamic(this, &UCoMTacticalCombatWidget::OnWaitClicked); }
	if (FleeButton)        { FleeButton->OnClicked.AddDynamic(this, &UCoMTacticalCombatWidget::OnFleeClicked); }
	if (AutoResolveButton) { AutoResolveButton->OnClicked.AddDynamic(this, &UCoMTacticalCombatWidget::OnAutoResolveClicked); }
	if (RetreatButton)     { RetreatButton->OnClicked.AddDynamic(this, &UCoMTacticalCombatWidget::OnRetreatClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMTacticalCombatWidget::InitBattle(int32 AttackerArmyId, int32 DefenderArmyId, ECoMPlane Plane)
{
	CurrentAttackerArmyId = AttackerArmyId;
	CurrentDefenderArmyId = DefenderArmyId;
	CurrentPlane = Plane;
	CurrentRound = 1;

	// Apply the plane's visual theme to the grid and background
	ApplyPlaneTheme(Plane);

	if (AttackerNameText)
	{
		AttackerNameText->SetText(FText::FromString(FString::Printf(TEXT("Army %d"), AttackerArmyId)));
	}
	if (DefenderNameText)
	{
		DefenderNameText->SetText(FText::FromString(FString::Printf(TEXT("Army %d"), DefenderArmyId)));
	}
	if (RoundCounterText)
	{
		RoundCounterText->SetText(FText::FromString(FString::Printf(TEXT("Round: %d"), CurrentRound)));
	}
}

void UCoMTacticalCombatWidget::SelectCell(int32 X, int32 Y)
{
	// Deselect previous cell
	if (SelectedX >= 0 && SelectedY >= 0)
	{
		int32 PrevIdx = SelectedY * GridSize + SelectedX;
		if (GridCells.IsValidIndex(PrevIdx) && GridCells[PrevIdx])
		{
			GridCells[PrevIdx]->SetBrushColor(TacticalColours::CellBorder);
		}
	}

	SelectedX = X;
	SelectedY = Y;

	// Highlight new cell with plane-specific highlight color
	if (X >= 0 && Y >= 0)
	{
		int32 Idx = Y * GridSize + X;
		if (GridCells.IsValidIndex(Idx) && GridCells[Idx])
		{
			const FPlaneColors PC = GetPlaneColors(CurrentPlane);
			GridCells[Idx]->SetBrushColor(PC.Highlight);
		}
	}
}

void UCoMTacticalCombatWidget::HighlightMovementRange(int32 UnitId)
{
	// Placeholder: in a full implementation, query the combat subsystem
	// for reachable cells and highlight them. For now, log the request.
	UE_LOG(LogTemp, Log, TEXT("HighlightMovementRange requested for unit %d"), UnitId);
}

// =============================================================================
// Button callbacks
// =============================================================================

void UCoMTacticalCombatWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

// Helper: convert engine-grid coords (0..GRID_W-1, 0..GRID_H-1) to widget cell index.
static int32 EngineToCellIdx(FIntPoint P, int32 GridSizeW)
{
	const int32 X = FMath::Clamp(P.X, 0, GridSizeW - 1);
	const int32 Y = FMath::Clamp(P.Y, 0, GridSizeW - 1);
	return Y * GridSizeW + X;
}

void UCoMTacticalCombatWidget::OnMoveClicked()
{
	if (!TacSub.IsValid()) return;
	if (SelectedX < 0 || SelectedY < 0) return;
	const int32 Curr = TacSub->GetCurrentUnitIndex();
	if (Curr < 0) return;
	TacSub->MoveUnit(Curr, FIntPoint(SelectedX, SelectedY));
	RefreshFromSubsystem();
}

void UCoMTacticalCombatWidget::OnAttackClicked()
{
	// Greedy: attack the nearest valid target. Melee preferred over ranged
	// when both are available, so the player keeps initiative on the same
	// turn slot. If there's a selected tile with an enemy, prefer that.
	if (!TacSub.IsValid()) return;
	const int32 Curr = TacSub->GetCurrentUnitIndex();
	if (Curr < 0) return;
	TArray<int32> Melee = TacSub->GetValidMeleeTargets(Curr);
	TArray<int32> Ranged = TacSub->GetValidRangedTargets(Curr);
	const TArray<int32>& Pick = Melee.Num() > 0 ? Melee : Ranged;
	if (Pick.Num() == 0) return;
	const int32 Target = TacSub->PickBestTarget(Pick);
	if (Melee.Num() > 0) TacSub->MeleeAttack(Curr, Target);
	else                 TacSub->RangedAttack(Curr, Target);
	RefreshFromSubsystem();
	DrainAIUntilPlayerOrEnd();
}

void UCoMTacticalCombatWidget::OnDefendClicked()
{
	if (!TacSub.IsValid()) return;
	const int32 Curr = TacSub->GetCurrentUnitIndex();
	if (Curr < 0) return;
	TacSub->Defend(Curr);
	TacSub->EndUnitTurn();
	PlaySFXByName(FName(TEXT("shield_block")));
	RefreshFromSubsystem();
	DrainAIUntilPlayerOrEnd();
}

void UCoMTacticalCombatWidget::OnCastSpellClicked()
{
	// Combat-spell casting from heroes is a separate vertical slice; for now
	// log + treat as Wait so the turn doesn't hang.
	UE_LOG(LogTemp, Log, TEXT("TacticalCombat: cast-spell stub (no spell selected)"));
	OnWaitClicked();
}

void UCoMTacticalCombatWidget::OnWaitClicked()
{
	if (!TacSub.IsValid()) return;
	const int32 Curr = TacSub->GetCurrentUnitIndex();
	if (Curr < 0) return;
	TacSub->Wait(Curr);
	RefreshFromSubsystem();
	DrainAIUntilPlayerOrEnd();
}

void UCoMTacticalCombatWidget::OnFleeClicked()
{
	if (!TacSub.IsValid()) return;
	const int32 Curr = TacSub->GetCurrentUnitIndex();
	if (Curr < 0) return;
	TacSub->AttemptFlee(Curr);
	RefreshFromSubsystem();
	DrainAIUntilPlayerOrEnd();
}

void UCoMTacticalCombatWidget::OnAutoResolveClicked()
{
	// Surrender player control: drain every remaining unit through AI.
	PlayerWizardIdx = -1;
	DrainAIUntilPlayerOrEnd();
}

void UCoMTacticalCombatWidget::OnRetreatClicked()
{
	// Retreat = flee + end widget. Same as Flee under the current model.
	OnFleeClicked();
}

// ─── Live wiring to the subsystem ────────────────────────────────────────────

void UCoMTacticalCombatWidget::StartLiveBattle(const FCoMCombatContext& Context, int32 InPlayerWizardIndex)
{
	PlayerWizardIdx = InPlayerWizardIndex;
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMTacticalCombatSubsystem* Sub = GI->GetSubsystem<UCoMTacticalCombatSubsystem>();
	if (!Sub) return;
	TacSub = Sub;

	// Subscribe (idempotent: clear first to avoid duplicate binds on re-entry).
	Sub->OnBattleStarted.RemoveAll(this);
	Sub->OnUnitTurnStarted.RemoveAll(this);
	Sub->OnUnitMoved.RemoveAll(this);
	Sub->OnUnitDamaged.RemoveAll(this);
	Sub->OnUnitKilled.RemoveAll(this);
	Sub->OnBattleEnded.RemoveAll(this);
	Sub->OnBattleStarted.AddDynamic(this, &UCoMTacticalCombatWidget::OnLiveBattleStarted);
	Sub->OnUnitTurnStarted.AddDynamic(this, &UCoMTacticalCombatWidget::OnLiveUnitTurnStarted);
	Sub->OnUnitMoved.AddDynamic(this, &UCoMTacticalCombatWidget::OnLiveUnitMoved);
	Sub->OnUnitDamaged.AddDynamic(this, &UCoMTacticalCombatWidget::OnLiveUnitDamaged);
	Sub->OnUnitKilled.AddDynamic(this, &UCoMTacticalCombatWidget::OnLiveUnitKilled);
	Sub->OnBattleEnded.AddDynamic(this, &UCoMTacticalCombatWidget::OnLiveBattleEnded);

	Sub->InitializeBattle(Context);
	CurrentPlane = Context.OriginPlane;
	ApplyPlaneTheme(CurrentPlane);
	RefreshFromSubsystem();
	DrainAIUntilPlayerOrEnd();
}

void UCoMTacticalCombatWidget::RefreshFromSubsystem()
{
	if (!TacSub.IsValid()) return;
	UCoMTacticalCombatSubsystem* Sub = TacSub.Get();

	// Round counter.
	CurrentRound = Sub->GetTurnCount();
	if (RoundCounterText)
	{
		RoundCounterText->SetText(FText::FromString(
			FString::Printf(TEXT("Round: %d"), CurrentRound)));
	}

	// Reset grid colours, then paint unit tokens on top.
	const FPlaneColors PC = GetPlaneColors(CurrentPlane);
	for (int32 i = 0; i < GridCells.Num(); ++i)
	{
		if (!GridCells[i]) continue;
		const int32 GX = i % GridSize, GY = i / GridSize;
		const bool bAlt = ((GX + GY) % 2) == 0;
		GridCells[i]->SetBrushColor(bAlt ? PC.Ground : PC.GroundAlt);
	}

	// Highlight the active actor's tile underneath the token.
	const int32 CurrIdxEarly = Sub->GetCurrentUnitIndex();
	if (CurrIdxEarly >= 0 && Sub->GetUnits().IsValidIndex(CurrIdxEarly))
	{
		const FCoMTacticalUnit& U0 = Sub->GetUnits()[CurrIdxEarly];
		const int32 HIdx = EngineToCellIdx(U0.GridPosition, GridSize);
		if (GridCells.IsValidIndex(HIdx) && GridCells[HIdx])
		{
			GridCells[HIdx]->SetBrushColor(PC.Highlight);
		}
	}

	// Create / update unit tokens (portrait sprite per unit). Tokens live
	// on GridCanvas as siblings of the cells and tween toward target
	// positions each tick. Dying tokens stay around with a fade timer.
	TSet<int32> SeenIds;
	UCoMUnitSubsystem* UnitSub = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>() : nullptr;
	for (const FCoMTacticalUnit& U : Sub->GetUnits())
	{
		if (!U.IsAlive()) continue;
		SeenIds.Add(U.UnitInstanceId);
		TObjectPtr<UImage>& Tok = UnitTokens.FindOrAdd(U.UnitInstanceId);
		if (!Tok)
		{
			Tok = WidgetTree->ConstructWidget<UImage>();
			if (GridCanvas)
			{
				UCanvasPanelSlot* TSlot = GridCanvas->AddChildToCanvas(Tok);
				if (TSlot)
				{
					TSlot->SetSize(FVector2D(CellSize, CellSize));
					TSlot->SetZOrder(10);
					TSlot->SetPosition(FVector2D(
						U.GridPosition.X * CellSize, U.GridPosition.Y * CellSize));
				}
			}
			// Resolve and apply the unit portrait if we have one.
			if (UnitSub)
			{
				if (const FCoMUnitInstance* Inst = UnitSub->GetUnit(U.UnitInstanceId))
				{
					const FString Path = FString::Printf(
						TEXT("/Game/UI/Units/%s.%s"),
						*Inst->SpecID.ToString(), *Inst->SpecID.ToString());
					if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path))
					{
						Tok->SetBrushFromTexture(Tex);
					}
				}
			}
		}
		const bool bFriendly = (U.OwnerWizardId == PlayerWizardIdx);
		Tok->SetColorAndOpacity(bFriendly
			? FLinearColor(1.0f, 0.92f, 0.55f, 1.0f)
			: FLinearColor(1.0f, 0.55f, 0.55f, 1.0f));
		TokenTargetPos.FindOrAdd(U.UnitInstanceId) =
			FVector2D(U.GridPosition.X * CellSize, U.GridPosition.Y * CellSize);
	}
	// Tokens for units that vanished from the engine (corpses past their
	// fade window) get removed; deaths are scheduled separately by
	// OnLiveUnitKilled which seeds a fade timer.
	for (auto It = UnitTokens.CreateIterator(); It; ++It)
	{
		if (!SeenIds.Contains(It->Key) && !TokenFadeTimer.Contains(It->Key))
		{
			if (It->Value) { It->Value->RemoveFromParent(); }
			TokenTargetPos.Remove(It->Key);
			It.RemoveCurrent();
		}
	}

	const int32 Curr = Sub->GetCurrentUnitIndex();
	if (Curr >= 0 && Sub->GetUnits().IsValidIndex(Curr))
	{
		const FCoMTacticalUnit& U = Sub->GetUnits()[Curr];
		// Stats panel
		if (SelectedUnitNameText)
		{
			SelectedUnitNameText->SetText(FText::FromString(
				FString::Printf(TEXT("Unit #%d (wiz %d)"), U.UnitInstanceId, U.OwnerWizardId)));
		}
		if (SelectedUnitStatsText)
		{
			SelectedUnitStatsText->SetText(FText::FromString(FString::Printf(
				TEXT("HP %d/%d  M%d R%d D%d  MP %d/%d"),
				U.CurrentHP, U.MaxHP,
				U.MeleeAttack, U.RangedAttack, U.Defense,
				U.MovementRemaining, U.MovementPoints)));
		}
		if (SelectedUnitHPBar)
		{
			SelectedUnitHPBar->SetPercent(U.MaxHP > 0 ? (float)U.CurrentHP / U.MaxHP : 0.f);
		}
		if (TurnIndicatorText)
		{
			const bool bPlayerTurn = (U.OwnerWizardId == PlayerWizardIdx);
			TurnIndicatorText->SetText(FText::FromString(
				bPlayerTurn ? TEXT("Your turn") : TEXT("Enemy turn")));
		}
	}
}

void UCoMTacticalCombatWidget::DrainAIUntilPlayerOrEnd()
{
	if (!TacSub.IsValid()) return;
	UCoMTacticalCombatSubsystem* Sub = TacSub.Get();

	// Hard cap to avoid infinite loops if the AI gets stuck (defensive).
	int32 Steps = 0;
	while (Sub->IsBattleActive() && Steps++ < 512)
	{
		const int32 Curr = Sub->GetCurrentUnitIndex();
		if (Curr < 0) break;
		const FCoMTacticalUnit& U = Sub->GetUnits()[Curr];
		const bool bAIControlled = (U.OwnerWizardId != PlayerWizardIdx);
		if (!bAIControlled) break; // hand back to the player
		Sub->ExecuteAITurn(Curr);
		Sub->EndUnitTurn();
	}
	RefreshFromSubsystem();
}

// Subsystem delegate handlers — keep the UI in sync without polling.
void UCoMTacticalCombatWidget::OnLiveBattleStarted(int32 /*InTurnCount*/)   { RefreshFromSubsystem(); }
void UCoMTacticalCombatWidget::OnLiveUnitTurnStarted(int32 /*UnitIndex*/)  { RefreshFromSubsystem(); }

void UCoMTacticalCombatWidget::OnLiveUnitMoved(int32 UnitIndex, FIntPoint /*NewPos*/)
{
	if (TacSub.IsValid() && TacSub->GetUnits().IsValidIndex(UnitIndex))
	{
		PlayMoveSFX(TacSub->GetUnits()[UnitIndex].UnitInstanceId);
	}
	RefreshFromSubsystem();
}

void UCoMTacticalCombatWidget::OnLiveUnitDamaged(int32 UnitIndex, int32 Damage)
{
	if (TacSub.IsValid() && TacSub->GetUnits().IsValidIndex(UnitIndex))
	{
		const FCoMTacticalUnit& U = TacSub->GetUnits()[UnitIndex];
		const FVector2D Pos(U.GridPosition.X * CellSize + CellSize * 0.4f,
		                    U.GridPosition.Y * CellSize - 4.0f);
		SpawnFloatText(FString::Printf(TEXT("-%d"), Damage), Pos,
			FLinearColor(1.0f, 0.35f, 0.35f, 1.0f));
		// Pick the SFX based on who was attacking. We don't have that here, so
		// default to sword_clash; ranged hits get arrow_thud heuristically by
		// checking if the current actor has a ranged stat.
		FName Sfx = FName(TEXT("sword_clash"));
		const int32 Actor = TacSub->GetCurrentUnitIndex();
		if (TacSub->GetUnits().IsValidIndex(Actor)
			&& TacSub->GetUnits()[Actor].RangedAttack > TacSub->GetUnits()[Actor].MeleeAttack)
		{
			Sfx = FName(TEXT("arrow_thud"));
		}
		PlaySFXByName(Sfx);
	}
	RefreshFromSubsystem();
}

void UCoMTacticalCombatWidget::OnLiveUnitKilled(int32 UnitIndex)
{
	if (TacSub.IsValid() && TacSub->GetUnits().IsValidIndex(UnitIndex))
	{
		const FCoMTacticalUnit& U = TacSub->GetUnits()[UnitIndex];
		// Schedule a 0.6s fade-out for the dying token before cleanup.
		TokenFadeTimer.Add(U.UnitInstanceId, 0.6f);
	}
	PlaySFXByName(FName(TEXT("death_grunt")));
	RefreshFromSubsystem();
}
void UCoMTacticalCombatWidget::OnLiveBattleEnded(ECoMCombatResult Result)
{
	UE_LOG(LogTemp, Log, TEXT("TacticalCombat: battle ended with result %d"), (int32)Result);
	RefreshFromSubsystem();
	// Auto-close after a short delay so the player sees the final state.
	if (UWorld* W = GetWorld())
	{
		FTimerHandle H;
		W->GetTimerManager().SetTimer(H, [WP = TWeakObjectPtr<UCoMTacticalCombatWidget>(this)]()
		{
			if (UCoMTacticalCombatWidget* Self = WP.Get())
			{
				Self->RemoveFromParent();
			}
		}, 2.5f, false);
	}
}

// =============================================================================
// Helpers
// =============================================================================

UButton* UCoMTacticalCombatWidget::CreateActionButton(UVerticalBox* Parent, const FString& Label, float Width)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(Width);
	SizeBox->SetHeightOverride(36.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(TacticalColours::GoldDim);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs  = ESlateBrushDrawType::Box;
	Style.Normal.TintColor  = FSlateColor(TacticalColours::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(TacticalColours::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(TacticalColours::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(TacticalColours::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo BtnFont = BtnLabel->GetFont();
	BtnFont.Size = 13;
	BtnLabel->SetFont(BtnFont);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UVerticalBoxSlot* SlotRef = Parent->AddChildToVerticalBox(SizeBox);
	if (SlotRef)
	{
		SlotRef->SetHorizontalAlignment(HAlign_Fill);
		SlotRef->SetPadding(FMargin(0.0f, 2.0f));
	}

	return Button;
}

UBorder* UCoMTacticalCombatWidget::CreateGridCell(int32 X, int32 Y)
{
	// Use plane-specific colors for terrain
	const FPlaneColors PC = GetPlaneColors(CurrentPlane);

	// Terrain variety via hash — use plane ground color with variation
	int32 Hash = (X * 7 + Y * 13) % 10;
	FLinearColor CellColor;
	if (Hash < 6)
	{
		// Majority: ground color (with subtle checkerboard)
		CellColor = ((X + Y) % 2 == 0) ? PC.Ground : PC.GroundAlt;
	}
	else if (Hash < 8)
	{
		// Obstacles/walls
		CellColor = PC.Wall;
	}
	else
	{
		// Water/special
		CellColor = PC.Water;
	}

	// Outer border for selection highlight
	UBorder* CellBorder = WidgetTree->ConstructWidget<UBorder>();
	CellBorder->SetBrushColor(TacticalColours::CellBorder);
	CellBorder->SetPadding(FMargin(1.0f));

	// Inner fill with terrain color
	UBorder* CellFill = WidgetTree->ConstructWidget<UBorder>();
	CellFill->SetBrushColor(CellColor);
	CellFill->SetPadding(FMargin(0.0f));
	CellBorder->AddChild(CellFill);

	// Position on canvas
	UCanvasPanelSlot* CanvasSlotRef = GridCanvas->AddChildToCanvas(CellBorder);
	if (CanvasSlotRef)
	{
		CanvasSlotRef->SetPosition(FVector2D(X * CellSize, Y * CellSize));
		CanvasSlotRef->SetSize(FVector2D(CellSize, CellSize));
	}

	return CellBorder;
}

// =============================================================================
// Layout
// =============================================================================

void UCoMTacticalCombatWidget::BuildLayout()
{
	// ── Full-screen dark background ──────────────────────────────────────
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(TacticalColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = BackgroundBorder;

	// ── Main vertical layout ─────────────────────────────────────────────
	UVerticalBox* RootVBox = WidgetTree->ConstructWidget<UVerticalBox>();
	BackgroundBorder->AddChild(RootVBox);

	// ══════════════════════════════════════════════════════════════════════
	// TOP BAR
	// ══════════════════════════════════════════════════════════════════════
	{
		UBorder* TopBar = WidgetTree->ConstructWidget<UBorder>();
		TopBar->SetBrushColor(TacticalColours::PanelBg);
		TopBar->SetPadding(FMargin(16.0f, 8.0f));

		UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		TopBar->AddChild(TopRow);

		// Title
		TitleText = WidgetTree->ConstructWidget<UTextBlock>();
		TitleText->SetText(FText::FromString(TEXT("BATTLE")));
		TitleText->SetColorAndOpacity(FSlateColor(TacticalColours::Gold));
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 26;
		TitleFont.TypefaceFontName = FName(TEXT("Bold"));
		TitleText->SetFont(TitleFont);
		UHorizontalBoxSlot* TitleSlotRef = TopRow->AddChildToHorizontalBox(TitleText);
		if (TitleSlotRef) { TitleSlotRef->SetVerticalAlignment(VAlign_Center); TitleSlotRef->SetPadding(FMargin(0, 0, 20, 0)); }

		// Attacker name
		AttackerNameText = WidgetTree->ConstructWidget<UTextBlock>();
		AttackerNameText->SetText(FText::FromString(TEXT("Attacker")));
		AttackerNameText->SetColorAndOpacity(FSlateColor(TacticalColours::Silver));
		FSlateFontInfo NameFont = AttackerNameText->GetFont();
		NameFont.Size = 16;
		AttackerNameText->SetFont(NameFont);
		UHorizontalBoxSlot* AtkSlotRef = TopRow->AddChildToHorizontalBox(AttackerNameText);
		if (AtkSlotRef) { AtkSlotRef->SetVerticalAlignment(VAlign_Center); }

		// "vs"
		UTextBlock* VsText = WidgetTree->ConstructWidget<UTextBlock>();
		VsText->SetText(FText::FromString(TEXT("  vs  ")));
		VsText->SetColorAndOpacity(FSlateColor(TacticalColours::Gold));
		FSlateFontInfo VsFont = VsText->GetFont();
		VsFont.Size = 14;
		VsText->SetFont(VsFont);
		UHorizontalBoxSlot* VsSlotRef = TopRow->AddChildToHorizontalBox(VsText);
		if (VsSlotRef) { VsSlotRef->SetVerticalAlignment(VAlign_Center); }

		// Defender name
		DefenderNameText = WidgetTree->ConstructWidget<UTextBlock>();
		DefenderNameText->SetText(FText::FromString(TEXT("Defender")));
		DefenderNameText->SetColorAndOpacity(FSlateColor(TacticalColours::Silver));
		DefenderNameText->SetFont(NameFont);
		UHorizontalBoxSlot* DefSlotRef = TopRow->AddChildToHorizontalBox(DefenderNameText);
		if (DefSlotRef) { DefSlotRef->SetVerticalAlignment(VAlign_Center); }

		// Spacer
		USpacer* TopSpacer = WidgetTree->ConstructWidget<USpacer>();
		UHorizontalBoxSlot* SpSlotRef = TopRow->AddChildToHorizontalBox(TopSpacer);
		if (SpSlotRef) { SpSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

		// Terrain type
		TerrainTypeText = WidgetTree->ConstructWidget<UTextBlock>();
		TerrainTypeText->SetText(FText::FromString(TEXT("Terrain: Plains")));
		TerrainTypeText->SetColorAndOpacity(FSlateColor(TacticalColours::Grey));
		FSlateFontInfo SmallFont = TerrainTypeText->GetFont();
		SmallFont.Size = 13;
		TerrainTypeText->SetFont(SmallFont);
		UHorizontalBoxSlot* TerrSlotRef = TopRow->AddChildToHorizontalBox(TerrainTypeText);
		if (TerrSlotRef) { TerrSlotRef->SetVerticalAlignment(VAlign_Center); TerrSlotRef->SetPadding(FMargin(0, 0, 20, 0)); }

		// Round counter
		RoundCounterText = WidgetTree->ConstructWidget<UTextBlock>();
		RoundCounterText->SetText(FText::FromString(TEXT("Round: 1")));
		RoundCounterText->SetColorAndOpacity(FSlateColor(TacticalColours::Gold));
		RoundCounterText->SetFont(SmallFont);
		UHorizontalBoxSlot* RndSlotRef = TopRow->AddChildToHorizontalBox(RoundCounterText);
		if (RndSlotRef) { RndSlotRef->SetVerticalAlignment(VAlign_Center); }

		UVerticalBoxSlot* TopBarSlotRef = RootVBox->AddChildToVerticalBox(TopBar);
		if (TopBarSlotRef) { TopBarSlotRef->SetHorizontalAlignment(HAlign_Fill); }
	}

	// ── Gold divider below top bar ───────────────────────────────────────
	{
		UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
		Div->SetBrushColor(TacticalColours::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(2.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* DivSlotRef = RootVBox->AddChildToVerticalBox(DivSize);
		if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); }
	}

	// ══════════════════════════════════════════════════════════════════════
	// CENTER AREA: Grid + Right Sidebar
	// ══════════════════════════════════════════════════════════════════════
	{
		UHorizontalBox* CenterRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		UVerticalBoxSlot* CenterSlotRef = RootVBox->AddChildToVerticalBox(CenterRow);
		if (CenterSlotRef)
		{
			CenterSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			CenterSlotRef->SetHorizontalAlignment(HAlign_Fill);
		}

		// ── Grid area ────────────────────────────────────────────────────
		{
			UBorder* GridBorder = WidgetTree->ConstructWidget<UBorder>();
			GridBorder->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.06f, 1.0f));
			GridBorder->SetPadding(FMargin(10.0f));

			GridCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
			GridBorder->AddChild(GridCanvas);

			// Build 20x20 grid cells
			GridCells.SetNum(GridSize * GridSize);
			for (int32 Y = 0; Y < GridSize; ++Y)
			{
				for (int32 X = 0; X < GridSize; ++X)
				{
					GridCells[Y * GridSize + X] = CreateGridCell(X, Y);
				}
			}

			USizeBox* GridSizeBox = WidgetTree->ConstructWidget<USizeBox>();
			GridSizeBox->SetWidthOverride(GridSize * CellSize + 20.0f);
			GridSizeBox->SetHeightOverride(GridSize * CellSize + 20.0f);
			GridSizeBox->AddChild(GridBorder);

			UHorizontalBoxSlot* GridSlotRef = CenterRow->AddChildToHorizontalBox(GridSizeBox);
			if (GridSlotRef)
			{
				GridSlotRef->SetVerticalAlignment(VAlign_Center);
				GridSlotRef->SetHorizontalAlignment(HAlign_Center);
				GridSlotRef->SetPadding(FMargin(10.0f));
			}
		}

		// ── Spacer between grid and sidebar ──────────────────────────────
		{
			USpacer* MidSpacer = WidgetTree->ConstructWidget<USpacer>();
			UHorizontalBoxSlot* SpSlotRef = CenterRow->AddChildToHorizontalBox(MidSpacer);
			if (SpSlotRef) { SpSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
		}

		// ── Right Sidebar ────────────────────────────────────────────────
		{
			USizeBox* SidebarSize = WidgetTree->ConstructWidget<USizeBox>();
			SidebarSize->SetWidthOverride(200.0f);

			UBorder* SidebarBorder = WidgetTree->ConstructWidget<UBorder>();
			SidebarBorder->SetBrushColor(TacticalColours::PanelBg);
			SidebarBorder->SetPadding(FMargin(10.0f, 8.0f));
			SidebarSize->AddChild(SidebarBorder);

			UVerticalBox* SidebarVBox = WidgetTree->ConstructWidget<UVerticalBox>();
			SidebarBorder->AddChild(SidebarVBox);

			// -- Selected unit section --
			{
				UTextBlock* UnitHeader = WidgetTree->ConstructWidget<UTextBlock>();
				UnitHeader->SetText(FText::FromString(TEXT("Selected Unit")));
				UnitHeader->SetColorAndOpacity(FSlateColor(TacticalColours::Gold));
				FSlateFontInfo HdrFont = UnitHeader->GetFont();
				HdrFont.Size = 15;
				HdrFont.TypefaceFontName = FName(TEXT("Bold"));
				UnitHeader->SetFont(HdrFont);
				UVerticalBoxSlot* HdrSlotRef = SidebarVBox->AddChildToVerticalBox(UnitHeader);
				if (HdrSlotRef) { HdrSlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

				SelectedUnitNameText = WidgetTree->ConstructWidget<UTextBlock>();
				SelectedUnitNameText->SetText(FText::FromString(TEXT("(none)")));
				SelectedUnitNameText->SetColorAndOpacity(FSlateColor(TacticalColours::Silver));
				FSlateFontInfo NameFont = SelectedUnitNameText->GetFont();
				NameFont.Size = 13;
				SelectedUnitNameText->SetFont(NameFont);
				SidebarVBox->AddChildToVerticalBox(SelectedUnitNameText);

				// HP bar
				SelectedUnitHPBar = WidgetTree->ConstructWidget<UProgressBar>();
				SelectedUnitHPBar->SetPercent(1.0f);
				SelectedUnitHPBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f, 1.0f));
				USizeBox* HPSizeBox = WidgetTree->ConstructWidget<USizeBox>();
				HPSizeBox->SetHeightOverride(12.0f);
				HPSizeBox->AddChild(SelectedUnitHPBar);
				UVerticalBoxSlot* HPSlotRef = SidebarVBox->AddChildToVerticalBox(HPSizeBox);
				if (HPSlotRef) { HPSlotRef->SetPadding(FMargin(0, 4, 0, 4)); HPSlotRef->SetHorizontalAlignment(HAlign_Fill); }

				// Stats text
				SelectedUnitStatsText = WidgetTree->ConstructWidget<UTextBlock>();
				SelectedUnitStatsText->SetText(FText::FromString(TEXT("ATK: --  DEF: --")));
				SelectedUnitStatsText->SetColorAndOpacity(FSlateColor(TacticalColours::Grey));
				FSlateFontInfo StatsFont = SelectedUnitStatsText->GetFont();
				StatsFont.Size = 12;
				SelectedUnitStatsText->SetFont(StatsFont);
				UVerticalBoxSlot* StatsSlotRef = SidebarVBox->AddChildToVerticalBox(SelectedUnitStatsText);
				if (StatsSlotRef) { StatsSlotRef->SetPadding(FMargin(0, 0, 0, 8)); }
			}

			// -- Divider --
			{
				UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
				Div->SetBrushColor(TacticalColours::GoldDim);
				USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
				DivSize->SetHeightOverride(1.0f);
				DivSize->AddChild(Div);
				UVerticalBoxSlot* DivSlotRef = SidebarVBox->AddChildToVerticalBox(DivSize);
				if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); DivSlotRef->SetPadding(FMargin(0, 4, 0, 8)); }
			}

			// -- Action buttons --
			{
				UTextBlock* ActionsHeader = WidgetTree->ConstructWidget<UTextBlock>();
				ActionsHeader->SetText(FText::FromString(TEXT("Actions")));
				ActionsHeader->SetColorAndOpacity(FSlateColor(TacticalColours::Gold));
				FSlateFontInfo AFont = ActionsHeader->GetFont();
				AFont.Size = 14;
				AFont.TypefaceFontName = FName(TEXT("Bold"));
				ActionsHeader->SetFont(AFont);
				UVerticalBoxSlot* ASlotRef = SidebarVBox->AddChildToVerticalBox(ActionsHeader);
				if (ASlotRef) { ASlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

				MoveButton      = CreateActionButton(SidebarVBox, TEXT("Move"));
				AttackButton    = CreateActionButton(SidebarVBox, TEXT("Attack"));
				DefendButton    = CreateActionButton(SidebarVBox, TEXT("Defend"));
				CastSpellButton = CreateActionButton(SidebarVBox, TEXT("Cast Spell"));
				WaitButton      = CreateActionButton(SidebarVBox, TEXT("Wait"));
				FleeButton      = CreateActionButton(SidebarVBox, TEXT("Flee"));
			}

			// -- Divider --
			{
				UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
				Div->SetBrushColor(TacticalColours::GoldDim);
				USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
				DivSize->SetHeightOverride(1.0f);
				DivSize->AddChild(Div);
				UVerticalBoxSlot* DivSlotRef = SidebarVBox->AddChildToVerticalBox(DivSize);
				if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); DivSlotRef->SetPadding(FMargin(0, 8, 0, 8)); }
			}

			// -- Initiative order --
			{
				UTextBlock* InitHeader = WidgetTree->ConstructWidget<UTextBlock>();
				InitHeader->SetText(FText::FromString(TEXT("Initiative Order")));
				InitHeader->SetColorAndOpacity(FSlateColor(TacticalColours::Gold));
				FSlateFontInfo IFont = InitHeader->GetFont();
				IFont.Size = 13;
				IFont.TypefaceFontName = FName(TEXT("Bold"));
				InitHeader->SetFont(IFont);
				UVerticalBoxSlot* ISlotRef = SidebarVBox->AddChildToVerticalBox(InitHeader);
				if (ISlotRef) { ISlotRef->SetPadding(FMargin(0, 0, 0, 4)); }

				InitiativeScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
				UVerticalBoxSlot* ScrollSlotRef = SidebarVBox->AddChildToVerticalBox(InitiativeScrollBox);
				if (ScrollSlotRef)
				{
					ScrollSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				}

				// Placeholder initiative entries
				for (int32 i = 1; i <= 6; ++i)
				{
					UTextBlock* Entry = WidgetTree->ConstructWidget<UTextBlock>();
					Entry->SetText(FText::FromString(FString::Printf(TEXT("%d. Unit %d"), i, i)));
					Entry->SetColorAndOpacity(FSlateColor(TacticalColours::Silver));
					FSlateFontInfo EFont = Entry->GetFont();
					EFont.Size = 11;
					Entry->SetFont(EFont);
					InitiativeScrollBox->AddChild(Entry);
				}
			}

			UHorizontalBoxSlot* SideSlotRef = CenterRow->AddChildToHorizontalBox(SidebarSize);
			if (SideSlotRef)
			{
				SideSlotRef->SetVerticalAlignment(VAlign_Fill);
				SideSlotRef->SetPadding(FMargin(0, 0, 4, 0));
			}
		}
	}

	// ── Gold divider above bottom bar ────────────────────────────────────
	{
		UBorder* Div = WidgetTree->ConstructWidget<UBorder>();
		Div->SetBrushColor(TacticalColours::GoldDim);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(2.0f);
		DivSize->AddChild(Div);
		UVerticalBoxSlot* DivSlotRef = RootVBox->AddChildToVerticalBox(DivSize);
		if (DivSlotRef) { DivSlotRef->SetHorizontalAlignment(HAlign_Fill); }
	}

	// ══════════════════════════════════════════════════════════════════════
	// BOTTOM BAR
	// ══════════════════════════════════════════════════════════════════════
	{
		UBorder* BottomBar = WidgetTree->ConstructWidget<UBorder>();
		BottomBar->SetBrushColor(TacticalColours::PanelBg);
		BottomBar->SetPadding(FMargin(16.0f, 8.0f));

		UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		BottomBar->AddChild(BottomRow);

		// Auto-Resolve button
		{
			USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
			SizeBox->SetWidthOverride(140.0f);
			SizeBox->SetHeightOverride(36.0f);

			UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
			BtnBorder->SetBrushColor(TacticalColours::GoldDim);
			BtnBorder->SetPadding(FMargin(1.0f));

			AutoResolveButton = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle Style = AutoResolveButton->GetStyle();
			Style.Normal.DrawAs  = ESlateBrushDrawType::Box;
			Style.Normal.TintColor  = FSlateColor(TacticalColours::BtnNormal);
			Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
			Style.Hovered.TintColor = FSlateColor(TacticalColours::BtnHover);
			Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
			Style.Pressed.TintColor = FSlateColor(TacticalColours::BtnPressed);
			AutoResolveButton->SetStyle(Style);

			UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
			BtnLabel->SetText(FText::FromString(TEXT("Auto-Resolve")));
			BtnLabel->SetColorAndOpacity(FSlateColor(TacticalColours::Silver));
			BtnLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo BtnFont = BtnLabel->GetFont();
			BtnFont.Size = 13;
			BtnLabel->SetFont(BtnFont);

			AutoResolveButton->AddChild(BtnLabel);
			BtnBorder->AddChild(AutoResolveButton);
			SizeBox->AddChild(BtnBorder);

			UHorizontalBoxSlot* ARSlotRef = BottomRow->AddChildToHorizontalBox(SizeBox);
			if (ARSlotRef) { ARSlotRef->SetVerticalAlignment(VAlign_Center); ARSlotRef->SetPadding(FMargin(0, 0, 10, 0)); }
		}

		// Retreat button
		{
			USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
			SizeBox->SetWidthOverride(120.0f);
			SizeBox->SetHeightOverride(36.0f);

			UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
			BtnBorder->SetBrushColor(TacticalColours::GoldDim);
			BtnBorder->SetPadding(FMargin(1.0f));

			RetreatButton = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle Style = RetreatButton->GetStyle();
			Style.Normal.DrawAs  = ESlateBrushDrawType::Box;
			Style.Normal.TintColor  = FSlateColor(TacticalColours::BtnNormal);
			Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
			Style.Hovered.TintColor = FSlateColor(TacticalColours::BtnHover);
			Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
			Style.Pressed.TintColor = FSlateColor(TacticalColours::BtnPressed);
			RetreatButton->SetStyle(Style);

			UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
			BtnLabel->SetText(FText::FromString(TEXT("Retreat")));
			BtnLabel->SetColorAndOpacity(FSlateColor(TacticalColours::Silver));
			BtnLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo BtnFont = BtnLabel->GetFont();
			BtnFont.Size = 13;
			BtnLabel->SetFont(BtnFont);

			RetreatButton->AddChild(BtnLabel);
			BtnBorder->AddChild(RetreatButton);
			SizeBox->AddChild(BtnBorder);

			UHorizontalBoxSlot* RtSlotRef = BottomRow->AddChildToHorizontalBox(SizeBox);
			if (RtSlotRef) { RtSlotRef->SetVerticalAlignment(VAlign_Center); }
		}

		// Spacer
		{
			USpacer* BotSpacer = WidgetTree->ConstructWidget<USpacer>();
			UHorizontalBoxSlot* SpSlotRef = BottomRow->AddChildToHorizontalBox(BotSpacer);
			if (SpSlotRef) { SpSlotRef->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
		}

		// Turn indicator
		TurnIndicatorText = WidgetTree->ConstructWidget<UTextBlock>();
		TurnIndicatorText->SetText(FText::FromString(TEXT("Your Turn")));
		TurnIndicatorText->SetColorAndOpacity(FSlateColor(TacticalColours::Gold));
		FSlateFontInfo TurnFont = TurnIndicatorText->GetFont();
		TurnFont.Size = 14;
		TurnIndicatorText->SetFont(TurnFont);
		UHorizontalBoxSlot* TurnSlotRef = BottomRow->AddChildToHorizontalBox(TurnIndicatorText);
		if (TurnSlotRef) { TurnSlotRef->SetVerticalAlignment(VAlign_Center); TurnSlotRef->SetPadding(FMargin(0, 0, 16, 0)); }

		// Close button
		{
			USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
			SizeBox->SetWidthOverride(80.0f);
			SizeBox->SetHeightOverride(36.0f);

			UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
			BtnBorder->SetBrushColor(TacticalColours::GoldDim);
			BtnBorder->SetPadding(FMargin(1.0f));

			CloseButton = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle Style = CloseButton->GetStyle();
			Style.Normal.DrawAs  = ESlateBrushDrawType::Box;
			Style.Normal.TintColor  = FSlateColor(TacticalColours::BtnNormal);
			Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
			Style.Hovered.TintColor = FSlateColor(TacticalColours::BtnHover);
			Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
			Style.Pressed.TintColor = FSlateColor(TacticalColours::BtnPressed);
			CloseButton->SetStyle(Style);

			UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
			BtnLabel->SetText(FText::FromString(TEXT("Close")));
			BtnLabel->SetColorAndOpacity(FSlateColor(TacticalColours::Silver));
			BtnLabel->SetJustification(ETextJustify::Center);
			FSlateFontInfo BtnFont = BtnLabel->GetFont();
			BtnFont.Size = 13;
			BtnLabel->SetFont(BtnFont);

			CloseButton->AddChild(BtnLabel);
			BtnBorder->AddChild(CloseButton);
			SizeBox->AddChild(BtnBorder);

			UHorizontalBoxSlot* ClsSlotRef = BottomRow->AddChildToHorizontalBox(SizeBox);
			if (ClsSlotRef) { ClsSlotRef->SetVerticalAlignment(VAlign_Center); }
		}

		UVerticalBoxSlot* BotBarSlotRef = RootVBox->AddChildToVerticalBox(BottomBar);
		if (BotBarSlotRef) { BotBarSlotRef->SetHorizontalAlignment(HAlign_Fill); }
	}
}

// =============================================================================
// Plane-specific color palettes
// =============================================================================

UCoMTacticalCombatWidget::FPlaneColors UCoMTacticalCombatWidget::GetPlaneColors(ECoMPlane Plane)
{
	FPlaneColors C;

	switch (Plane)
	{
	case ECoMPlane::Aurelith:
		C.Background = FLinearColor(0.08f, 0.15f, 0.25f, 1.0f);  // Warm blue sky
		C.Ground     = FLinearColor(0.18f, 0.42f, 0.15f, 1.0f);  // Green grass
		C.GroundAlt  = FLinearColor(0.22f, 0.48f, 0.18f, 1.0f);  // Lighter green
		C.Highlight  = FLinearColor(0.85f, 0.65f, 0.12f, 1.0f);  // Gold
		C.Water      = FLinearColor(0.12f, 0.30f, 0.55f, 1.0f);  // Blue water
		C.Wall       = FLinearColor(0.40f, 0.35f, 0.25f, 1.0f);  // Brown hills
		C.Sky        = FLinearColor(0.15f, 0.25f, 0.45f, 1.0f);  // Blue
		C.PlaneName  = TEXT("Aurelith");
		break;

	case ECoMPlane::Noctharion:
		C.Background = FLinearColor(0.06f, 0.04f, 0.12f, 1.0f);  // Deep purple night
		C.Ground     = FLinearColor(0.15f, 0.10f, 0.20f, 1.0f);  // Dark soil
		C.GroundAlt  = FLinearColor(0.18f, 0.12f, 0.24f, 1.0f);  // Slightly lighter
		C.Highlight  = FLinearColor(0.70f, 0.70f, 0.80f, 1.0f);  // Silver
		C.Water      = FLinearColor(0.10f, 0.08f, 0.25f, 1.0f);  // Dark purple water
		C.Wall       = FLinearColor(0.25f, 0.22f, 0.30f, 1.0f);  // Grey stone
		C.Sky        = FLinearColor(0.08f, 0.05f, 0.18f, 1.0f);  // Purple
		C.PlaneName  = TEXT("Noctharion");
		break;

	case ECoMPlane::Verdantis:
		C.Background = FLinearColor(0.05f, 0.15f, 0.08f, 1.0f);  // Deep jungle
		C.Ground     = FLinearColor(0.10f, 0.35f, 0.12f, 1.0f);  // Dense green
		C.GroundAlt  = FLinearColor(0.12f, 0.40f, 0.15f, 1.0f);  // Lush green
		C.Highlight  = FLinearColor(0.85f, 0.65f, 0.12f, 1.0f);  // Gold
		C.Water      = FLinearColor(0.08f, 0.25f, 0.20f, 1.0f);  // Jungle stream
		C.Wall       = FLinearColor(0.20f, 0.30f, 0.15f, 1.0f);  // Mossy wood
		C.Sky        = FLinearColor(0.06f, 0.18f, 0.10f, 1.0f);  // Emerald canopy
		C.PlaneName  = TEXT("Verdantis");
		break;

	case ECoMPlane::Infernyx:
		C.Background = FLinearColor(0.12f, 0.04f, 0.02f, 1.0f);  // Red haze
		C.Ground     = FLinearColor(0.10f, 0.06f, 0.05f, 1.0f);  // Volcanic black
		C.GroundAlt  = FLinearColor(0.14f, 0.08f, 0.06f, 1.0f);  // Charred earth
		C.Highlight  = FLinearColor(1.00f, 0.50f, 0.10f, 1.0f);  // Molten orange
		C.Water      = FLinearColor(0.60f, 0.15f, 0.02f, 1.0f);  // Lava
		C.Wall       = FLinearColor(0.20f, 0.12f, 0.10f, 1.0f);  // Obsidian
		C.Sky        = FLinearColor(0.18f, 0.06f, 0.02f, 1.0f);  // Smoke red
		C.PlaneName  = TEXT("Infernyx");
		break;

	case ECoMPlane::Aethermist:
		C.Background = FLinearColor(0.12f, 0.18f, 0.28f, 1.0f);  // Pale blue mist
		C.Ground     = FLinearColor(0.35f, 0.40f, 0.50f, 1.0f);  // Crystal stone
		C.GroundAlt  = FLinearColor(0.40f, 0.45f, 0.55f, 1.0f);  // Light crystal
		C.Highlight  = FLinearColor(0.90f, 0.90f, 1.00f, 1.0f);  // White
		C.Water      = FLinearColor(0.25f, 0.35f, 0.55f, 1.0f);  // Ethereal blue
		C.Wall       = FLinearColor(0.50f, 0.50f, 0.60f, 1.0f);  // Pale stone
		C.Sky        = FLinearColor(0.15f, 0.22f, 0.35f, 1.0f);  // Misty blue
		C.PlaneName  = TEXT("Aethermist");
		break;

	case ECoMPlane::Abyssal:
		C.Background = FLinearColor(0.10f, 0.02f, 0.02f, 1.0f);  // Blood dark
		C.Ground     = FLinearColor(0.18f, 0.08f, 0.06f, 1.0f);  // Bone/flesh
		C.GroundAlt  = FLinearColor(0.22f, 0.10f, 0.08f, 1.0f);  // Dark flesh
		C.Highlight  = FLinearColor(0.80f, 0.15f, 0.10f, 1.0f);  // Blood red
		C.Water      = FLinearColor(0.30f, 0.05f, 0.05f, 1.0f);  // Blood pool
		C.Wall       = FLinearColor(0.30f, 0.25f, 0.20f, 1.0f);  // Bone
		C.Sky        = FLinearColor(0.12f, 0.03f, 0.03f, 1.0f);  // Crimson sky
		C.PlaneName  = TEXT("Abyssal");
		break;

	case ECoMPlane::Ethereal:
		C.Background = FLinearColor(0.05f, 0.08f, 0.15f, 1.0f);  // Void
		C.Ground     = FLinearColor(0.18f, 0.22f, 0.35f, 1.0f);  // Phase-shifted blue
		C.GroundAlt  = FLinearColor(0.22f, 0.26f, 0.40f, 1.0f);  // Translucent blue
		C.Highlight  = FLinearColor(0.40f, 0.60f, 1.00f, 1.0f);  // Bright blue
		C.Water      = FLinearColor(0.12f, 0.18f, 0.40f, 1.0f);  // Shimmer void
		C.Wall       = FLinearColor(0.25f, 0.30f, 0.45f, 1.0f);  // Echo stone
		C.Sky        = FLinearColor(0.06f, 0.10f, 0.20f, 1.0f);  // Starlight
		C.PlaneName  = TEXT("Ethereal");
		break;

	case ECoMPlane::Feywild:
		C.Background = FLinearColor(0.10f, 0.06f, 0.15f, 1.0f);  // Prismatic twilight
		C.Ground     = FLinearColor(0.25f, 0.15f, 0.30f, 1.0f);  // Mushroom purple
		C.GroundAlt  = FLinearColor(0.30f, 0.18f, 0.35f, 1.0f);  // Flower pink
		C.Highlight  = FLinearColor(1.00f, 0.40f, 0.80f, 1.0f);  // Fey pink
		C.Water      = FLinearColor(0.15f, 0.20f, 0.40f, 1.0f);  // Crystal spring
		C.Wall       = FLinearColor(0.30f, 0.20f, 0.25f, 1.0f);  // Living wood
		C.Sky        = FLinearColor(0.12f, 0.08f, 0.20f, 1.0f);  // Aurora
		C.PlaneName  = TEXT("Feywild");
		break;

	default:
		C = GetPlaneColors(ECoMPlane::Aurelith);
		break;
	}

	return C;
}

void UCoMTacticalCombatWidget::ApplyPlaneTheme(ECoMPlane Plane)
{
	const FPlaneColors PC = GetPlaneColors(Plane);

	// Update background
	if (BackgroundBorder)
	{
		BackgroundBorder->SetBrushColor(PC.Background);
	}

	// Update terrain type display
	if (TerrainTypeText)
	{
		TerrainTypeText->SetText(FText::FromString(
			FString::Printf(TEXT("Plane: %s"), *PC.PlaneName)));
	}

	// Recolor all grid cells
	for (int32 Idx = 0; Idx < GridCells.Num(); ++Idx)
	{
		if (!GridCells[Idx]) continue;

		int32 X = Idx % GridSize;
		int32 Y = Idx / GridSize;
		int32 Hash = (X * 7 + Y * 13) % 10;

		FLinearColor CellColor;
		if (Hash < 6)
		{
			CellColor = ((X + Y) % 2 == 0) ? PC.Ground : PC.GroundAlt;
		}
		else if (Hash < 8)
		{
			CellColor = PC.Wall;
		}
		else
		{
			CellColor = PC.Water;
		}

		// The grid cell is a border with a child border (fill)
		// Update the child's color
		if (UBorder* CellFill = Cast<UBorder>(GridCells[Idx]->GetChildAt(0)))
		{
			CellFill->SetBrushColor(CellColor);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TacticalCombat] Applied plane theme: %s"), *PC.PlaneName);
}

// =============================================================================
// Tween + animation tick
// =============================================================================

void UCoMTacticalCombatWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	// Movement tween for each live token toward its target canvas pixel.
	for (auto& Kvp : UnitTokens)
	{
		UImage* Tok = Kvp.Value;
		if (!Tok) continue;
		UCanvasPanelSlot* CS = Cast<UCanvasPanelSlot>(Tok->Slot);
		if (!CS) continue;
		const FVector2D* Tgt = TokenTargetPos.Find(Kvp.Key);
		if (!Tgt) continue;
		const FVector2D Cur = CS->GetPosition();
		const FVector2D New = FMath::Vector2DInterpTo(Cur, *Tgt, DeltaTime, /*Speed*/ 12.0f);
		CS->SetPosition(New);
	}

	// Death fade — tick down each timer; remove token when it reaches 0.
	for (auto It = TokenFadeTimer.CreateIterator(); It; ++It)
	{
		It->Value -= DeltaTime;
		if (UImage* Tok = UnitTokens.FindRef(It->Key))
		{
			const float Alpha = FMath::Clamp(It->Value / 0.6f, 0.0f, 1.0f);
			Tok->SetRenderOpacity(Alpha);
		}
		if (It->Value <= 0.0f)
		{
			if (UImage* Tok = UnitTokens.FindRef(It->Key))
			{
				Tok->RemoveFromParent();
			}
			UnitTokens.Remove(It->Key);
			TokenTargetPos.Remove(It->Key);
			It.RemoveCurrent();
		}
	}

	// Floating damage numbers — drift up, fade, then remove.
	for (int32 i = FloatTexts.Num() - 1; i >= 0; --i)
	{
		FFloatingNumber& F = FloatTexts[i];
		F.Time += DeltaTime;
		const float t = FMath::Clamp(F.Time / F.Duration, 0.0f, 1.0f);
		if (!F.Text.IsValid() || t >= 1.0f)
		{
			if (F.Text.IsValid()) F.Text->RemoveFromParent();
			FloatTexts.RemoveAt(i);
			continue;
		}
		if (UCanvasPanelSlot* FS = Cast<UCanvasPanelSlot>(F.Text->Slot))
		{
			FS->SetPosition(FVector2D(F.Start.X, F.Start.Y - 36.0f * t));
		}
		F.Text->SetRenderOpacity(1.0f - t);
	}
}

void UCoMTacticalCombatWidget::SpawnFloatText(const FString& Text,
	FVector2D CanvasPos, FLinearColor Tint)
{
	if (!GridCanvas || !WidgetTree) return;
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetText(FText::FromString(Text));
	T->SetColorAndOpacity(FSlateColor(Tint));
	{ FSlateFontInfo F = T->GetFont(); F.Size = 16; T->SetFont(F); }
	if (UCanvasPanelSlot* FS = GridCanvas->AddChildToCanvas(T))
	{
		FS->SetSize(FVector2D(40, 20));
		FS->SetZOrder(20);
		FS->SetPosition(CanvasPos);
	}
	FFloatingNumber FN;
	FN.Text = T;
	FN.Start = CanvasPos;
	FloatTexts.Add(FN);
}

void UCoMTacticalCombatWidget::PlaySFXByName(FName SoundID)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMAudioSubsystem* A = GI->GetSubsystem<UCoMAudioSubsystem>())
		{
			A->PlayUISound(SoundID); // dynamic-loads /Game/Audio/SFX/UI/<id>
		}
	}
}

void UCoMTacticalCombatWidget::PlayMoveSFX(int32 UnitInstanceId)
{
	if (!GetGameInstance()) return;
	UCoMUnitSubsystem* US = GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>();
	if (!US) return;
	const FCoMUnitInstance* Inst = US->GetUnit(UnitInstanceId);
	if (!Inst) return;

	// Pick the sound by mobility type. Flying takes priority; cavalry next;
	// everything else uses the marching footsteps loop.
	FName Sfx(TEXT("footsteps_march"));
	if (Inst->bFlying || Inst->MovementType == ECoMMovementType::Flying)
	{
		Sfx = FName(TEXT("wing_flap"));
	}
	else if (Inst->SpecID.ToString().Contains(TEXT("Cavalry")))
	{
		Sfx = FName(TEXT("horse_gallop"));
	}
	PlaySFXByName(Sfx);
}

// =============================================================================
// Console: open the widget on a synthetic 2-army battle for smoke testing.
//   com.test_tactical_ui
// Plays as wizard 0 vs wizard 1. Useful for verifying buttons + delegates
// without needing the overworld pre-battle popup wired up.
// =============================================================================
#include "CoMCore/Units/CoMUnitSubsystem.h"

static FAutoConsoleCommandWithWorldAndArgs GTestTacticalUICmd(
	TEXT("com.test_tactical_ui"),
	TEXT("Open the tactical battle widget on a synthetic encounter for testing."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			UGameInstance* GI = World->GetGameInstance();
			if (!GI) return;
			UCoMUnitSubsystem* US = GI->GetSubsystem<UCoMUnitSubsystem>();
			if (!US) return;
			// Look up the first two armies in the world, treat the lower
			// ArmyID as attacker. If there are not two, bail.
			TArray<int32> ArmyIDs;
			for (const auto& P : US->GetAllArmies()) { ArmyIDs.Add(P.Key); }
			if (ArmyIDs.Num() < 2)
			{
				UE_LOG(LogTemp, Warning, TEXT("com.test_tactical_ui: need >= 2 armies in world (have %d)"), ArmyIDs.Num());
				return;
			}
			ArmyIDs.Sort();
			const FCoMArmyGroup* A = US->GetArmy(ArmyIDs[0]);
			const FCoMArmyGroup* B = US->GetArmy(ArmyIDs[1]);
			if (!A || !B) return;
			FCoMCombatContext Ctx;
			Ctx.ParticipatingArmyGroupIDs.Add(ArmyIDs[0]);
			Ctx.ParticipatingArmyGroupIDs.Add(ArmyIDs[1]);
			Ctx.AttackerWizardIndex = A->OwnerWizardIndex;
			Ctx.DefenderWizardIndex = B->OwnerWizardIndex;
			Ctx.OriginTile  = A->Position;
			Ctx.OriginPlane = A->Plane;
			Ctx.ReturnMapName = FName(TEXT("InteractiveTest"));

			UCoMTacticalCombatWidget* W = CreateWidget<UCoMTacticalCombatWidget>(
				World, UCoMTacticalCombatWidget::StaticClass());
			if (W)
			{
				W->AddToViewport(120);
				W->StartLiveBattle(Ctx, A->OwnerWizardIndex);
			}
		}));
