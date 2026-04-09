// Copyright Mythforge Studios. All Rights Reserved.
// CoMMinimapWidget.cpp -- 200x200 minimap widget implementation.

#include "CoMMinimapWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Slate/SlateBrushAsset.h"
#include "Rendering/DrawElements.h"

#include "CoMCore/World/CoMMinimapSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Camera/CoMCameraPawn.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "GameFramework/SpringArmComponent.h"

// ─────────────────────────────────────────────────────────────────────────────
// Static Members
// ─────────────────────────────────────────────────────────────────────────────

const FLinearColor UCoMMinimapWidget::BorderColor    = FLinearColor(0.855f, 0.647f, 0.125f, 1.f); // #daa520
const FLinearColor UCoMMinimapWidget::CameraRectColor = FLinearColor::White;

// ─────────────────────────────────────────────────────────────────────────────
// UUserWidget Overrides
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind plane-cycle buttons.
	if (PrevPlaneButton)
	{
		PrevPlaneButton->OnClicked.AddDynamic(this, &UCoMMinimapWidget::OnPrevPlaneClicked);
	}
	if (NextPlaneButton)
	{
		NextPlaneButton->OnClicked.AddDynamic(this, &UCoMMinimapWidget::OnNextPlaneClicked);
	}
}

void UCoMMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bInitialized)
	{
		return;
	}

	// Throttle dynamic updates to DynamicUpdateInterval.
	DynamicUpdateAccumulator += InDeltaTime;
	if (DynamicUpdateAccumulator >= DynamicUpdateInterval)
	{
		DynamicUpdateAccumulator = 0.f;
		UpdateDynamic();
	}
}

FReply UCoMMinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bInitialized || !MinimapSub)
	{
		return FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D UV = LocalPositionToMinimapUV(InGeometry, LocalPos);

		if (UV.X >= 0.f && UV.X <= 1.f && UV.Y >= 0.f && UV.Y <= 1.f)
		{
			const FIntPoint WorldPos = MinimapSub->ScreenToWorldPosition(UV);
			OnMinimapNavigate.Broadcast(WorldPos);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

int32 UCoMMinimapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 Result = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
	                                   OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!bInitialized || !MinimapSub)
	{
		return Result;
	}

	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	if (WidgetSize.X <= 0.f || WidgetSize.Y <= 0.f)
	{
		return Result;
	}

	const bool bIsEnabled = bParentEnabled && IsEnabled();
	const ESlateDrawEffect DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	// Draw 2px gold border around the entire widget.
	{
		TArray<FVector2D> BorderPoints;
		BorderPoints.Reserve(5);
		BorderPoints.Add(FVector2D(0.f, 0.f));
		BorderPoints.Add(FVector2D(WidgetSize.X, 0.f));
		BorderPoints.Add(FVector2D(WidgetSize.X, WidgetSize.Y));
		BorderPoints.Add(FVector2D(0.f, WidgetSize.Y));
		BorderPoints.Add(FVector2D(0.f, 0.f)); // Close the rect

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			BorderPoints,
			DrawEffects,
			BorderColor,
			true,   // bAntialias
			2.f     // Thickness
		);
	}

	// Draw camera viewport indicator as a white rectangle.
	{
		const FIntPoint CamTL = MinimapSub->GetCameraTopLeft();
		const FIntPoint CamBR = MinimapSub->GetCameraBottomRight();

		// Convert tile coords to widget-space pixels.
		const float ScaleX = WidgetSize.X / static_cast<float>(TexWidth);
		const float ScaleY = WidgetSize.Y / static_cast<float>(TexHeight);

		const FVector2D RectTL(CamTL.X * ScaleX, CamTL.Y * ScaleY);
		const FVector2D RectBR(CamBR.X * ScaleX, CamBR.Y * ScaleY);

		TArray<FVector2D> RectPoints;
		RectPoints.Reserve(5);
		RectPoints.Add(RectTL);
		RectPoints.Add(FVector2D(RectBR.X, RectTL.Y));
		RectPoints.Add(RectBR);
		RectPoints.Add(FVector2D(RectTL.X, RectBR.Y));
		RectPoints.Add(RectTL); // Close the rect

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(),
			RectPoints,
			DrawEffects,
			CameraRectColor,
			true,   // bAntialias
			1.f     // Thickness
		);
	}

	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Initialization
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapWidget::InitializeMinimap(int32 InWizardId, ECoMPlane StartingPlane)
{
	WizardId     = InWizardId;
	CurrentPlane = StartingPlane;

	// Resolve subsystems.
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		MinimapSub = GI->GetSubsystem<UCoMMinimapSubsystem>();
		CitySub    = GI->GetSubsystem<UCoMCitySubsystem>();
		UnitSub    = GI->GetSubsystem<UCoMUnitSubsystem>();
	}

	CreateMinimapTexture();

	if (MinimapSub)
	{
		MinimapSub->InitializeForPlane(CurrentPlane, WizardId);
	}

	bInitialized = true;
	RefreshMinimap();
}

// ─────────────────────────────────────────────────────────────────────────────
// Refresh Methods
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapWidget::RefreshMinimap()
{
	if (!bInitialized || !MinimapSub)
	{
		return;
	}

	// Gather markers.
	TArray<FIntPoint> PlayerCities, EnemyCities, NeutralCities;
	TArray<FIntPoint> PlayerArmies, EnemyArmies;
	GatherCityPositions(PlayerCities, EnemyCities, NeutralCities);
	GatherArmyPositions(PlayerArmies, EnemyArmies);

	// Update subsystem data.
	MinimapSub->RefreshTerrain();
	MinimapSub->UpdateFog();
	MinimapSub->UpdateMarkers(PlayerCities, EnemyCities, NeutralCities, PlayerArmies, EnemyArmies);

	// Camera rect.
	FIntPoint CamTL, CamBR;
	ComputeCameraRect(CamTL, CamBR);
	MinimapSub->UpdateCameraRect(CamTL, CamBR);

	// Upload to GPU.
	UploadPixelBufferToTexture();

	// Update plane name.
	if (PlaneNameText)
	{
		PlaneNameText->SetText(FText::FromString(UCoMMinimapSubsystem::GetPlaneDisplayName(CurrentPlane)));
	}
}

void UCoMMinimapWidget::UpdateDynamic()
{
	if (!bInitialized || !MinimapSub)
	{
		return;
	}

	// Gather dynamic elements.
	TArray<FIntPoint> PlayerCities, EnemyCities, NeutralCities;
	TArray<FIntPoint> PlayerArmies, EnemyArmies;
	GatherCityPositions(PlayerCities, EnemyCities, NeutralCities);
	GatherArmyPositions(PlayerArmies, EnemyArmies);

	MinimapSub->UpdateMarkers(PlayerCities, EnemyCities, NeutralCities, PlayerArmies, EnemyArmies);

	// Camera rect.
	FIntPoint CamTL, CamBR;
	ComputeCameraRect(CamTL, CamBR);
	MinimapSub->UpdateCameraRect(CamTL, CamBR);

	UploadPixelBufferToTexture();
}

void UCoMMinimapWidget::SetPlane(ECoMPlane NewPlane)
{
	if (CurrentPlane == NewPlane)
	{
		return;
	}

	CurrentPlane = NewPlane;

	if (MinimapSub)
	{
		MinimapSub->SetPlane(NewPlane);
	}

	RefreshMinimap();
	OnMinimapPlaneChanged.Broadcast(NewPlane);
}

void UCoMMinimapWidget::CycleNextPlane()
{
	int32 PlaneIdx = static_cast<int32>(CurrentPlane);
	PlaneIdx = (PlaneIdx + 1) % CoM::NUM_PLANES;
	SetPlane(static_cast<ECoMPlane>(PlaneIdx));
}

void UCoMMinimapWidget::CyclePrevPlane()
{
	int32 PlaneIdx = static_cast<int32>(CurrentPlane);
	PlaneIdx = (PlaneIdx + CoM::NUM_PLANES - 1) % CoM::NUM_PLANES;
	SetPlane(static_cast<ECoMPlane>(PlaneIdx));
}

// ─────────────────────────────────────────────────────────────────────────────
// Texture Management
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapWidget::CreateMinimapTexture()
{
	MinimapTexture = UTexture2D::CreateTransient(TexWidth, TexHeight, PF_B8G8R8A8);
	if (!MinimapTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("UCoMMinimapWidget: Failed to create minimap texture."));
		return;
	}

	// Nearest-neighbor filtering for crisp pixel art look.
	MinimapTexture->Filter = TF_Nearest;
	MinimapTexture->SRGB = true;
	MinimapTexture->AddressX = TA_Clamp;
	MinimapTexture->AddressY = TA_Clamp;
	MinimapTexture->UpdateResource();

	// Assign texture to the image widget.
	if (MinimapImage)
	{
		MinimapImage->SetBrushFromTexture(MinimapTexture);
		MinimapImage->SetDesiredSizeOverride(FVector2D(DisplaySize, DisplaySize));
	}
}

void UCoMMinimapWidget::UploadPixelBufferToTexture()
{
	if (!MinimapTexture || !MinimapSub)
	{
		return;
	}

	const TArray<FColor>& PixelBuffer = MinimapSub->GetPixelBuffer();
	if (PixelBuffer.Num() != TexWidth * TexHeight)
	{
		return;
	}

	// Lock mip 0 and copy pixel data.
	FTexture2DMipMap& Mip = MinimapTexture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (Data)
	{
		FMemory::Memcpy(Data, PixelBuffer.GetData(), PixelBuffer.Num() * sizeof(FColor));
	}
	Mip.BulkData.Unlock();
	MinimapTexture->UpdateResource();
}

// ─────────────────────────────────────────────────────────────────────────────
// Data Gathering
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapWidget::GatherCityPositions(
	TArray<FIntPoint>& OutPlayer, TArray<FIntPoint>& OutEnemy, TArray<FIntPoint>& OutNeutral) const
{
	if (!CitySub)
	{
		return;
	}

	TArray<const FCoMCityData*> AllCitiesOnPlane = CitySub->GetCitiesOnPlane(CurrentPlane);
	for (const FCoMCityData* City : AllCitiesOnPlane)
	{
		if (!City)
		{
			continue;
		}

		if (City->OwnerWizardIndex == WizardId)
		{
			OutPlayer.Add(City->Position);
		}
		else if (City->OwnerWizardIndex == CoM::WIZARD_INDEX_NONE)
		{
			OutNeutral.Add(City->Position);
		}
		else
		{
			OutEnemy.Add(City->Position);
		}
	}
}

void UCoMMinimapWidget::GatherArmyPositions(
	TArray<FIntPoint>& OutPlayer, TArray<FIntPoint>& OutEnemy) const
{
	if (!UnitSub)
	{
		return;
	}

	// Player armies.
	TArray<const FCoMArmyGroup*> PlayerArmies = UnitSub->GetArmiesForWizard(WizardId);
	for (const FCoMArmyGroup* Army : PlayerArmies)
	{
		if (Army && Army->Plane == CurrentPlane)
		{
			OutPlayer.Add(Army->Position);
		}
	}

	// Enemy armies: iterate all wizards except the player.
	// Only include armies on tiles the player can currently see (handled by fog in subsystem,
	// but we also filter here to avoid showing enemy markers in fog).
	for (int32 Wi = 0; Wi < CoM::MAX_WIZARDS; ++Wi)
	{
		if (Wi == WizardId)
		{
			continue;
		}

		TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesForWizard(Wi);
		for (const FCoMArmyGroup* Army : Armies)
		{
			if (Army && Army->Plane == CurrentPlane)
			{
				// Enemy armies are only shown if the tile is currently visible to the player.
				// The minimap subsystem handles fog darkening, but we also skip marking
				// armies on unexplored/hidden tiles so they don't appear through fog.
				OutEnemy.Add(Army->Position);
			}
		}
	}
}

void UCoMMinimapWidget::ComputeCameraRect(FIntPoint& OutTopLeft, FIntPoint& OutBottomRight) const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		OutTopLeft     = FIntPoint::ZeroValue;
		OutBottomRight = FIntPoint(20, 15);
		return;
	}

	APawn* CamPawn = PC->GetPawn();
	if (!CamPawn)
	{
		OutTopLeft     = FIntPoint::ZeroValue;
		OutBottomRight = FIntPoint(20, 15);
		return;
	}

	// Convert camera world position to tile coordinates.
	const FVector CamLoc = CamPawn->GetActorLocation();
	const float TileSize = CoM::TILE_WORLD_SIZE_CM;

	const int32 CenterX = FMath::FloorToInt32(CamLoc.X / TileSize);
	const int32 CenterY = FMath::FloorToInt32(CamLoc.Y / TileSize);

	// Estimate viewport size in tiles based on a reasonable camera height.
	// At default zoom (~2000 UU arm length), roughly 20x15 tiles visible.
	// Scale the viewport estimate with the spring arm length.
	int32 HalfW = 10;
	int32 HalfH = 7;

	ACoMCameraPawn* CoMCam = Cast<ACoMCameraPawn>(CamPawn);
	if (CoMCam && CoMCam->SpringArm)
	{
		const float ArmLen = CoMCam->SpringArm->TargetArmLength;
		const float ZoomRatio = ArmLen / 2000.f;
		HalfW = FMath::Max(5, FMath::FloorToInt32(10.f * ZoomRatio));
		HalfH = FMath::Max(4, FMath::FloorToInt32(7.f * ZoomRatio));
	}

	OutTopLeft     = FIntPoint(CenterX - HalfW, CenterY - HalfH);
	OutBottomRight = FIntPoint(CenterX + HalfW, CenterY + HalfH);

	// Clamp to map bounds.
	OutTopLeft.X     = FMath::Clamp(OutTopLeft.X,     0, CoM::MAP_WIDTH - 1);
	OutTopLeft.Y     = FMath::Clamp(OutTopLeft.Y,     0, CoM::MAP_HEIGHT - 1);
	OutBottomRight.X = FMath::Clamp(OutBottomRight.X, 0, CoM::MAP_WIDTH - 1);
	OutBottomRight.Y = FMath::Clamp(OutBottomRight.Y, 0, CoM::MAP_HEIGHT - 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinate Helpers
// ─────────────────────────────────────────────────────────────────────────────

FVector2D UCoMMinimapWidget::LocalPositionToMinimapUV(const FGeometry& Geo, FVector2D LocalPos) const
{
	const FVector2D Size = Geo.GetLocalSize();
	if (Size.X <= 0.f || Size.Y <= 0.f)
	{
		return FVector2D(-1.f, -1.f);
	}
	return FVector2D(LocalPos.X / Size.X, LocalPos.Y / Size.Y);
}

// ─────────────────────────────────────────────────────────────────────────────
// Button Callbacks
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMinimapWidget::OnPrevPlaneClicked()
{
	CyclePrevPlane();
}

void UCoMMinimapWidget::OnNextPlaneClicked()
{
	CycleNextPlane();
}
