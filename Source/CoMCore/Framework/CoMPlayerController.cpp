// Copyright Mythforge Studios. All Rights Reserved.
// CoMPlayerController.cpp — COM-032
#include "Framework/CoMPlayerController.h"
#include "Camera/CoMCameraPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Save/CoMSaveSubsystem.h"

// ─── ACoMHumanPlayerController ────────────────────────────────────────────────

ACoMHumanPlayerController::ACoMHumanPlayerController()
{
	// Input asset refs are intentionally null — set in project assets (Sprint 2).
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
}

void ACoMHumanPlayerController::Input_EndTurn(const FInputActionValue& /*Value*/)
{
	// TODO (Sprint 2): Validate it is this wizard's turn, then call
	//   UCoMTurnSubsystem::CommitEndTurn() via GetGameInstance()->GetSubsystem.
	UE_LOG(LogTemp, Log, TEXT("ACoMHumanPlayerController: EndTurn triggered (stub)."));
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
