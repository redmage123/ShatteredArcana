// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMConsoleCommands.h"
#include "CoMCore/Turn/CoMTurnManager.h"
#include "CoMCore/World/CoMFogOfWarSubsystem.h"
#include "HAL/IConsoleManager.h"

// =====================================================================
// Subsystem lifecycle
// =====================================================================


void UCoMConsoleCommands::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IConsoleManager& Console = IConsoleManager::Get();

	// Capture `this` — safe because we unregister in Deinitialize().
	auto Reg = [&](const TCHAR* Name, const TCHAR* Help, void (UCoMConsoleCommands::*Handler)(const TArray<FString>&))
	{
		IConsoleObject* Obj = Console.RegisterConsoleCommand(
			Name,
			Help,
			FConsoleCommandWithArgsDelegate::CreateUObject(this, Handler),
			ECVF_Default
		);
		if (Obj)
		{
			RegisteredCommands.Add(Obj);
		}
	};

	Reg(TEXT("com.newgame"),
		TEXT("Start a new game. Usage: com.newgame <numHumans> <numAI>"),
		&UCoMConsoleCommands::Cmd_NewGame);

	Reg(TEXT("com.endturn"),
		TEXT("End the current wizard's input phase."),
		&UCoMConsoleCommands::Cmd_EndTurn);

	Reg(TEXT("com.status"),
		TEXT("Print turn number, current wizard, army/city counts."),
		&UCoMConsoleCommands::Cmd_Status);

	Reg(TEXT("com.movearmyto"),
		TEXT("Queue army movement. Usage: com.movearmyto <ArmyID> <X> <Y>"),
		&UCoMConsoleCommands::Cmd_MoveArmyTo);

	Reg(TEXT("com.armies"),
		TEXT("List all armies for the current wizard."),
		&UCoMConsoleCommands::Cmd_Armies);

	Reg(TEXT("com.cities"),
		TEXT("List all cities for the current wizard."),
		&UCoMConsoleCommands::Cmd_Cities);

	Reg(TEXT("com.mapinfo"),
		TEXT("Print tile info. Usage: com.mapinfo <X> <Y>"),
		&UCoMConsoleCommands::Cmd_MapInfo);

	Reg(TEXT("com.spawnunit"),
		TEXT("Debug: spawn a unit. Usage: com.spawnunit <SpecID> <X> <Y>"),
		&UCoMConsoleCommands::Cmd_SpawnUnit);

	Reg(TEXT("com.spawnarmyat"),
		TEXT("Debug: create army with 3 basic units. Usage: com.spawnarmyat <X> <Y>"),
		&UCoMConsoleCommands::Cmd_SpawnArmyAt);

	Reg(TEXT("com.declarewar"),
		TEXT("Declare war on another wizard. Usage: com.declarewar <TargetWizardID>"),
		&UCoMConsoleCommands::Cmd_DeclareWar);

	UE_LOG(LogTemp, Log, TEXT("CoMConsoleCommands: %d commands registered."), RegisteredCommands.Num());
}

void UCoMConsoleCommands::Deinitialize()
{
	IConsoleManager& Console = IConsoleManager::Get();
	for (IConsoleObject* Obj : RegisteredCommands)
	{
		Console.UnregisterConsoleObject(Obj);
	}
	RegisteredCommands.Empty();

	Super::Deinitialize();
}

// =====================================================================
// Command handlers
// =====================================================================

void UCoMConsoleCommands::Cmd_NewGame(const TArray<FString>& Args)
{
	if (Args.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Usage: com.newgame <numHumans> <numAI>"));
		return;
	}

	const int32 NumHumans = FCString::Atoi(*Args[0]);
	const int32 NumAI     = FCString::Atoi(*Args[1]);

	if (NumHumans < 0 || NumAI < 0 || (NumHumans + NumAI) < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Need at least 2 total wizards."));
		return;
	}

	UCoMTurnManager* TM = GetGameInstance()->GetSubsystem<UCoMTurnManager>();
	if (!TM)
	{
		UE_LOG(LogTemp, Error, TEXT("TurnManager subsystem not found!"));
		return;
	}

	TM->InitializeGame(NumHumans, NumAI);
	UE_LOG(LogTemp, Log, TEXT("New game started: %d humans, %d AI."), NumHumans, NumAI);
}

void UCoMConsoleCommands::Cmd_EndTurn(const TArray<FString>& Args)
{
	UCoMTurnManager* TM = GetGameInstance()->GetSubsystem<UCoMTurnManager>();
	if (!TM)
	{
		UE_LOG(LogTemp, Error, TEXT("TurnManager subsystem not found!"));
		return;
	}

	if (TM->IsGameOver())
	{
		UE_LOG(LogTemp, Warning, TEXT("Game is already over."));
		return;
	}

	const int32 WizardId = TM->GetCurrentWizard();
	TM->EndPlayerTurn();
	UE_LOG(LogTemp, Log, TEXT("Wizard %d ended turn. Now wizard %d's turn (turn %d)."),
		WizardId, TM->GetCurrentWizard(), TM->GetTurnNumber());
}

void UCoMConsoleCommands::Cmd_Status(const TArray<FString>& Args)
{
	UCoMTurnManager* TM = GetGameInstance()->GetSubsystem<UCoMTurnManager>();
	if (!TM)
	{
		UE_LOG(LogTemp, Error, TEXT("TurnManager subsystem not found!"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════════"));
	UE_LOG(LogTemp, Log, TEXT("  Turn:           %d"), TM->GetTurnNumber());
	UE_LOG(LogTemp, Log, TEXT("  Current Wizard: %d (%s)"),
		TM->GetCurrentWizard(),
		TM->IsHumanTurn() ? TEXT("Human") : TEXT("AI"));
	UE_LOG(LogTemp, Log, TEXT("  Active Wizards: %d"), TM->GetActiveWizardCount());
	UE_LOG(LogTemp, Log, TEXT("  Game Over:      %s"), TM->IsGameOver() ? TEXT("YES") : TEXT("No"));

	// TODO: query army/city subsystems for counts per wizard.
	UE_LOG(LogTemp, Log, TEXT("  Armies:         (subsystem not wired)"));
	UE_LOG(LogTemp, Log, TEXT("  Cities:         (subsystem not wired)"));
	UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════════"));
}

void UCoMConsoleCommands::Cmd_MoveArmyTo(const TArray<FString>& Args)
{
	if (Args.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Usage: com.movearmyto <ArmyID> <X> <Y>"));
		return;
	}

	const int32 ArmyID = FCString::Atoi(*Args[0]);
	const int32 X      = FCString::Atoi(*Args[1]);
	const int32 Y      = FCString::Atoi(*Args[2]);

	// TODO: Queue movement command in army/command subsystem.
	UE_LOG(LogTemp, Log, TEXT("Movement queued: Army %d -> (%d, %d)  [placeholder — no army subsystem yet]"),
		ArmyID, X, Y);
}

void UCoMConsoleCommands::Cmd_Armies(const TArray<FString>& Args)
{
	UCoMTurnManager* TM = GetGameInstance()->GetSubsystem<UCoMTurnManager>();
	if (!TM)
	{
		UE_LOG(LogTemp, Error, TEXT("TurnManager subsystem not found!"));
		return;
	}

	const int32 WizardId = TM->GetCurrentWizard();

	// TODO: query army subsystem for all armies owned by WizardId.
	UE_LOG(LogTemp, Log, TEXT("Armies for Wizard %d:  [placeholder — no army subsystem yet]"), WizardId);
}

void UCoMConsoleCommands::Cmd_Cities(const TArray<FString>& Args)
{
	UCoMTurnManager* TM = GetGameInstance()->GetSubsystem<UCoMTurnManager>();
	if (!TM)
	{
		UE_LOG(LogTemp, Error, TEXT("TurnManager subsystem not found!"));
		return;
	}

	const int32 WizardId = TM->GetCurrentWizard();

	// TODO: query city subsystem for all cities owned by WizardId.
	UE_LOG(LogTemp, Log, TEXT("Cities for Wizard %d:  [placeholder — no city subsystem yet]"), WizardId);
}

void UCoMConsoleCommands::Cmd_MapInfo(const TArray<FString>& Args)
{
	if (Args.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Usage: com.mapinfo <X> <Y>"));
		return;
	}

	const int32 X = FCString::Atoi(*Args[0]);
	const int32 Y = FCString::Atoi(*Args[1]);

	// TODO: query world map subsystem for tile data.
	UE_LOG(LogTemp, Log, TEXT("Tile (%d, %d):  [placeholder — no world map subsystem wired to console]"), X, Y);

	// Check fog of war visibility for the current wizard.
	UCoMTurnManager* TM = GetGameInstance()->GetSubsystem<UCoMTurnManager>();
	UCoMFogOfWarSubsystem* FoW = GetGameInstance()->GetSubsystem<UCoMFogOfWarSubsystem>();

	if (TM && FoW)
	{
		const int32 WizardId = TM->GetCurrentWizard();
		const FIntPoint Tile(X, Y);

		// Check visibility on Aurelith (plane 0) as default.
		const bool bVisible  = FoW->IsTileVisible(WizardId, ECoMPlane::Aurelith, Tile);
		const bool bExplored = FoW->IsTileExplored(WizardId, ECoMPlane::Aurelith, Tile);

		UE_LOG(LogTemp, Log, TEXT("  Wizard %d — Visible: %s, Explored: %s"),
			WizardId,
			bVisible  ? TEXT("Yes") : TEXT("No"),
			bExplored ? TEXT("Yes") : TEXT("No"));
	}
}

void UCoMConsoleCommands::Cmd_SpawnUnit(const TArray<FString>& Args)
{
	if (Args.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Usage: com.spawnunit <SpecID> <X> <Y>"));
		return;
	}

	const int32 SpecID = FCString::Atoi(*Args[0]);
	const int32 X      = FCString::Atoi(*Args[1]);
	const int32 Y      = FCString::Atoi(*Args[2]);

	// TODO: create unit via unit subsystem.
	UE_LOG(LogTemp, Log, TEXT("SpawnUnit: Spec %d at (%d, %d)  [placeholder — no unit subsystem yet]"),
		SpecID, X, Y);
}

void UCoMConsoleCommands::Cmd_SpawnArmyAt(const TArray<FString>& Args)
{
	if (Args.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Usage: com.spawnarmyat <X> <Y>"));
		return;
	}

	const int32 X = FCString::Atoi(*Args[0]);
	const int32 Y = FCString::Atoi(*Args[1]);

	UCoMTurnManager* TM = GetGameInstance()->GetSubsystem<UCoMTurnManager>();
	const int32 WizardId = TM ? TM->GetCurrentWizard() : 0;

	// TODO: create army with 3 basic units via army/unit subsystem.
	UE_LOG(LogTemp, Log,
		TEXT("SpawnArmyAt: (%d, %d) for Wizard %d with 3 basic units  [placeholder — no army subsystem yet]"),
		X, Y, WizardId);

	// Reveal the area around the spawned army for immediate feedback.
	UCoMFogOfWarSubsystem* FoW = GetGameInstance()->GetSubsystem<UCoMFogOfWarSubsystem>();
	if (FoW)
	{
		FoW->RevealArea(WizardId, ECoMPlane::Aurelith, FIntPoint(X, Y), 2);
		UE_LOG(LogTemp, Log, TEXT("  Vision revealed in radius 2 around (%d, %d)."), X, Y);
	}
}

void UCoMConsoleCommands::Cmd_DeclareWar(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Usage: com.declarewar <TargetWizardID>"));
		return;
	}

	const int32 TargetId = FCString::Atoi(*Args[0]);

	UCoMTurnManager* TM = GetGameInstance()->GetSubsystem<UCoMTurnManager>();
	const int32 WizardId = TM ? TM->GetCurrentWizard() : 0;

	if (WizardId == TargetId)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot declare war on yourself."));
		return;
	}

	// TODO: set diplomatic state via diplomacy subsystem.
	UE_LOG(LogTemp, Log,
		TEXT("Wizard %d declares war on Wizard %d!  [placeholder — no diplomacy subsystem yet]"),
		WizardId, TargetId);
}

