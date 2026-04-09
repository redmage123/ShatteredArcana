#include "Framework/CoMGameMode.h"
#include "Framework/CoMPlayerController.h"
#include "Framework/CoMGameInstance.h"
#include "CoMCore/TacticalCombat/CoMTacticalCombatSubsystem.h"
#include "Kismet/GameplayStatics.h"


// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameMode.cpp — COM-032

// ─── ACoMCombatGameMode ───────────────────────────────────────────────────────

ACoMCombatGameMode::ACoMCombatGameMode()
{
	PlayerControllerClass = ACoMHumanPlayerController::StaticClass();
	// No free-roaming camera in combat; the combat subsystem assigns pawns directly.
	DefaultPawnClass = nullptr;
}

void ACoMCombatGameMode::BeginPlay()
{
	Super::BeginPlay();

	UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance());
	if (!CoMGI)
	{
		UE_LOG(LogTemp, Error, TEXT("ACoMCombatGameMode: No UCoMGameInstance found."));
		return;
	}

	if (!CoMGI->CombatContext.IsValid())
	{
		UE_LOG(LogTemp, Error,
		       TEXT("ACoMCombatGameMode: CombatContext is invalid — level entered "
		            "without a valid context. Returning to overworld."));
		// Attempt to return if we have a map name, otherwise just log.
		if (!CoMGI->CombatContext.ReturnMapName.IsNone())
		{
			UGameplayStatics::OpenLevel(this, CoMGI->CombatContext.ReturnMapName);
		}
		return;
	}

	// Get the tactical combat subsystem and initialize the battle.
	TacticalCombatSubsystem = CoMGI->GetSubsystem<UCoMTacticalCombatSubsystem>();
	if (!TacticalCombatSubsystem)
	{
		UE_LOG(LogTemp, Error,
		       TEXT("ACoMCombatGameMode: UCoMTacticalCombatSubsystem not found."));
		return;
	}

	// Bind to the battle-end delegate.
	TacticalCombatSubsystem->OnBattleEnded.AddDynamic(this, &ACoMCombatGameMode::OnBattleEnded);

	// Start the battle.
	TacticalCombatSubsystem->InitializeBattle(CoMGI->CombatContext);

	UE_LOG(LogTemp, Log, TEXT("ACoMCombatGameMode: Tactical battle initialized."));
}

void ACoMCombatGameMode::OnBattleEnded(ECoMCombatResult Result)
{
	UE_LOG(LogTemp, Log, TEXT("ACoMCombatGameMode: Battle ended with result %d."),
	       static_cast<int32>(Result));

	// Finalize the battle — apply casualties back to overworld armies.
	if (TacticalCombatSubsystem)
	{
		TacticalCombatSubsystem->FinalizeBattle();
	}

	// Transition back to the overworld.
	UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance());
	if (CoMGI && !CoMGI->CombatContext.ReturnMapName.IsNone())
	{
		const FName ReturnMap = CoMGI->CombatContext.ReturnMapName;
		CoMGI->ClearCombatContext();
		UGameplayStatics::OpenLevel(this, ReturnMap);
	}
	else
	{
		UE_LOG(LogTemp, Error,
		       TEXT("ACoMCombatGameMode: No ReturnMapName set — cannot transition back to overworld."));
	}
}

// ─── ACoMExplorationGameMode ──────────────────────────────────────────────────

ACoMExplorationGameMode::ACoMExplorationGameMode()
{
	PlayerControllerClass = ACoMHumanPlayerController::StaticClass();
	DefaultPawnClass      = nullptr;
}

void ACoMExplorationGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (const UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance()))
	{
		if (CoMGI->ExplorationContext.EntryArmyGroupID < 0)
		{
			UE_LOG(LogTemp, Error,
			       TEXT("ACoMExplorationGameMode: ExplorationContext is invalid — level "
			            "entered without a valid context."));
		}
		// TODO (Phase 7): Pass ExplorationContext to UCoMExplorationSubsystem.
	}
}

