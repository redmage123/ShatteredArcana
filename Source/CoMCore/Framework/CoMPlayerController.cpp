// Copyright Mythforge Studios. All Rights Reserved.
// CoMPlayerController.cpp — COM-032
#include "Framework/CoMPlayerController.h"
#include "Framework/CoMGameInstance.h"
#include "Camera/CoMCameraPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Save/CoMSaveSubsystem.h"
#include "Economy/CoMCitySubsystem.h"
#include "Units/CoMUnitSubsystem.h"
#include "World/CoMWorldMapSubsystem.h"
#include "CoreTypes/CoMConstants.h"
#include "Engine/World.h"

// ─── ACoMHumanPlayerController ────────────────────────────────────────────────

ACoMHumanPlayerController::ACoMHumanPlayerController()
{
	// Input asset refs are intentionally null — set in project assets (Sprint 2).
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ACoMHumanPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ACoMHumanPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Register the default mapping context so camera and hotkey bindings are live.
	if (DefaultMappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSS =
		        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			InputSS->AddMappingContext(DefaultMappingContext, StartupInputPriority);
		}
	}
}

void ACoMHumanPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		// Likely missing DefaultInputComponentClass=EnhancedInputComponent in DefaultEngine.ini.
		UE_LOG(LogTemp, Warning,
		       TEXT("ACoMHumanPlayerController: InputComponent is not UEnhancedInputComponent. "
		            "Set DefaultInputComponentClass=EnhancedInputComponent in DefaultEngine.ini."));
		return;
	}

	if (IA_EndTurn)
	{
		// Triggered on key-down only (Started); held/released not needed for a discrete command.
		EIC->BindAction(IA_EndTurn, ETriggerEvent::Started, this,
		                &ACoMHumanPlayerController::Input_EndTurn);
	}

	if (IA_QuickSave)
	{
		EIC->BindAction(IA_QuickSave, ETriggerEvent::Started, this,
		                &ACoMHumanPlayerController::Input_QuickSave);
	}

	if (IA_QuickLoad)
	{
		EIC->BindAction(IA_QuickLoad, ETriggerEvent::Started, this,
		                &ACoMHumanPlayerController::Input_QuickLoad);
	}

	// Left-click select: use legacy action binding (Select is defined in DefaultInput.ini).
	if (InputComponent)
	{
		InputComponent->BindAction(TEXT("Select"), IE_Pressed, this,
		                           &ACoMHumanPlayerController::OnSelectPressed);
	}
}

void ACoMHumanPlayerController::Input_EndTurn(const FInputActionValue& /*Value*/)
{
	// TODO (Sprint 2): Validate it is this wizard's turn, then call
	//   UCoMTurnSubsystem::CommitEndTurn() via GetGameInstance()->GetSubsystem.
	UE_LOG(LogTemp, Log, TEXT("ACoMHumanPlayerController: EndTurn triggered (stub)."));
}

FIntPoint ACoMHumanPlayerController::WorldToTile(const FVector& WorldPos)
{
	const int32 TX = FMath::FloorToInt32(WorldPos.X / CoM::TILE_WORLD_SIZE_CM);
	const int32 TY = FMath::FloorToInt32(WorldPos.Y / CoM::TILE_WORLD_SIZE_CM);
	return FIntPoint(TX, TY);
}

void ACoMHumanPlayerController::OnSelectPressed()
{
	// Line trace from mouse cursor to world.
	FHitResult HitResult;
	const bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	FVector WorldPos;
	if (bHit)
	{
		WorldPos = HitResult.ImpactPoint;
	}
	else
	{
		// Fallback: deproject mouse to world plane.
		FVector WorldLoc, WorldDir;
		if (!DeprojectMousePositionToWorld(WorldLoc, WorldDir)) { return; }
		// Intersect with Z=0 plane.
		if (FMath::IsNearlyZero(WorldDir.Z)) { return; }
		const float T = -WorldLoc.Z / WorldDir.Z;
		WorldPos = WorldLoc + WorldDir * T;
	}

	const FIntPoint TilePos = WorldToTile(WorldPos);
	UE_LOG(LogTemp, Log, TEXT("ACoMHumanPlayerController: Select at tile (%d, %d)"), TilePos.X, TilePos.Y);

	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }
	UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GI);
	if (!CoMGI) { return; }

	// Check for city at this tile.
	UCoMWorldMapSubsystem* MapSys = GI->GetSubsystem<UCoMWorldMapSubsystem>();
	if (MapSys)
	{
		const FCoMTileData* Tile = MapSys->GetTile(ECoMPlane::Aurelith, ECoMMapLayer::Surface, TilePos.X, TilePos.Y);
		if (Tile && Tile->CityID >= 0)
		{
			CoMGI->OnCityScreenRequested.Broadcast(Tile->CityID);
			return;
		}
	}

	// Check for army at this tile.
	UCoMUnitSubsystem* UnitSub = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (UnitSub)
	{
		TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesAtPosition(
			ECoMPlane::Aurelith, ECoMMapLayer::Surface, TilePos);
		if (Armies.Num() > 0)
		{
			const FCoMArmyGroup* FirstArmy = Armies[0];
			if (FirstArmy)
			{
				CoMGI->OnArmyStackRequested.Broadcast(FirstArmy->ArmyGroupID);
				return;
			}
		}
	}

	// No city or army — just log tile info for now.
	UE_LOG(LogTemp, Log, TEXT("ACoMHumanPlayerController: Empty tile at (%d, %d)"), TilePos.X, TilePos.Y);
}

void ACoMHumanPlayerController::Input_QuickSave(const FInputActionValue& /*Value*/)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMSaveSubsystem* SaveSub = GI->GetSubsystem<UCoMSaveSubsystem>())
		{
			SaveSub->QuickSave();
			UE_LOG(LogTemp, Log, TEXT("ACoMHumanPlayerController: QuickSave triggered."));
		}
	}
}

void ACoMHumanPlayerController::Input_QuickLoad(const FInputActionValue& /*Value*/)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMSaveSubsystem* SaveSub = GI->GetSubsystem<UCoMSaveSubsystem>())
		{
			SaveSub->QuickLoad();
			UE_LOG(LogTemp, Log, TEXT("ACoMHumanPlayerController: QuickLoad triggered."));
		}
	}
}

// ─── ACoMAIPlayerController ───────────────────────────────────────────────────

ACoMAIPlayerController::ACoMAIPlayerController()
{
}
