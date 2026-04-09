// Copyright Mythforge Studios. All Rights Reserved.
// CoMTacticalCombatSubsystem.cpp -- Grid-based tactical battle implementation

#include "CoMTacticalCombatSubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Framework/CoMGameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogTacticalCombat, Log, All);

// ═══════════════════════════════════════════════════════════════════════════
// Subsystem Lifecycle
// ═══════════════════════════════════════════════════════════════════════════

void UCoMTacticalCombatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCoMTacticalCombatSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// ═══════════════════════════════════════════════════════════════════════════
// Battle Initialization
// ═══════════════════════════════════════════════════════════════════════════

void UCoMTacticalCombatSubsystem::InitializeBattle(const FCoMCombatContext& Context)
{
	if (!Context.IsValid())
	{
		UE_LOG(LogTacticalCombat, Error, TEXT("InitializeBattle: Invalid CombatContext."));
		return;
	}

	StoredContext = Context;
	Result = ECoMCombatResult::Ongoing;
	TurnCount = 0;
	CurrentInitiativePos = 0;
	CurrentUnitIndex = -1;
	Units.Empty();
	InitiativeOrder.Empty();

	// Seed RNG from overworld tile coordinates for determinism.
	const uint64 Seed = static_cast<uint64>(Context.OriginTile.X * 7919 + Context.OriginTile.Y * 6271);
	Rng = FCoMDeterministicRandom(Seed, /*Sequence=*/ 4); // 4 = tactical combat stream

	// Look up overworld terrain for grid generation.
	// TODO: Query UCoMWorldMapSubsystem for the actual terrain at OriginTile.
	// For now, default to Grassland.
	ECoMTerrain OverworldTerrain = ECoMTerrain::Grassland;
	const bool bIsSiege = false; // TODO: detect from city presence at OriginTile.

	GenerateGrid(OverworldTerrain, bIsSiege);

	// Resolve army rosters via UCoMUnitSubsystem.
	UCoMUnitSubsystem* UnitSub = GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>();
	if (!UnitSub)
	{
		UE_LOG(LogTacticalCombat, Error, TEXT("InitializeBattle: UCoMUnitSubsystem not found."));
		return;
	}

	// Build attacker and defender unit ID lists from army groups.
	TArray<int32> AttackerUnitIDs;
	TArray<int32> DefenderUnitIDs;

	for (int32 ArmyID : Context.ParticipatingArmyGroupIDs)
	{
		const FCoMArmyGroup* Army = UnitSub->GetArmy(ArmyID);
		if (!Army) continue;

		if (Army->OwnerWizardIndex == Context.AttackerWizardIndex)
		{
			AttackerUnitIDs.Append(Army->UnitIDs);
		}
		else
		{
			DefenderUnitIDs.Append(Army->UnitIDs);
		}
	}

	PlaceUnits(AttackerUnitIDs, Context.AttackerWizardIndex,
	           DefenderUnitIDs, Context.DefenderWizardIndex);

	RollInitiative();

	bBattleActive = true;
	TurnCount = 1;

	UE_LOG(LogTacticalCombat, Log,
	       TEXT("Battle started: %d attackers vs %d defenders on %dx%d grid. Turn 1."),
	       AttackerUnitIDs.Num(), DefenderUnitIDs.Num(),
	       TACTICAL_GRID_WIDTH, TACTICAL_GRID_HEIGHT);

	OnBattleStarted.Broadcast(TurnCount);

	// Start the first unit's turn.
	if (InitiativeOrder.Num() > 0)
	{
		CurrentInitiativePos = 0;
		CurrentUnitIndex = InitiativeOrder[0];
		BeginUnitTurn(CurrentUnitIndex);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// Grid Generation
// ═══════════════════════════════════════════════════════════════════════════

void UCoMTacticalCombatSubsystem::GenerateGrid(ECoMTerrain OverworldTerrain, bool bIsSiege)
{
	// Allocate grid: Grid[Row][Column].
	Grid.SetNum(TACTICAL_GRID_HEIGHT);
	for (int32 Row = 0; Row < TACTICAL_GRID_HEIGHT; ++Row)
	{
		Grid[Row].SetNum(TACTICAL_GRID_WIDTH);
		for (int32 Col = 0; Col < TACTICAL_GRID_WIDTH; ++Col)
		{
			Grid[Row][Col] = FCoMTacticalTile();
		}
	}

	// Terrain distribution based on overworld tile.
	switch (OverworldTerrain)
	{
	case ECoMTerrain::Forest:
	case ECoMTerrain::EnchantedForest:
	case ECoMTerrain::Jungle:
	case ECoMTerrain::ShadowForest:
	case ECoMTerrain::FungalForest:
	case ECoMTerrain::ScorchedForest:
		// Forest: ~40% scattered forest tiles.
		for (int32 Row = 0; Row < TACTICAL_GRID_HEIGHT; ++Row)
		{
			for (int32 Col = 0; Col < TACTICAL_GRID_WIDTH; ++Col)
			{
				if (Rng.RollPercent(40))
				{
					Grid[Row][Col].Terrain = ECoMTerrain::Forest;
				}
			}
		}
		break;

	case ECoMTerrain::Mountains:
	case ECoMTerrain::DarkMountains:
	case ECoMTerrain::RootMountains:
	case ECoMTerrain::BasaltMountains:
		// Mountains: hills and rough terrain, high elevation.
		for (int32 Row = 0; Row < TACTICAL_GRID_HEIGHT; ++Row)
		{
			for (int32 Col = 0; Col < TACTICAL_GRID_WIDTH; ++Col)
			{
				if (Rng.RollPercent(35))
				{
					Grid[Row][Col].Terrain = ECoMTerrain::Hills;
					Grid[Row][Col].Elevation = Rng.RollPercent(40) ? 2 : 1;
				}
				else if (Rng.RollPercent(15))
				{
					// Impassable rock (treat as mountain tile).
					Grid[Row][Col].Terrain = ECoMTerrain::Mountains;
					Grid[Row][Col].Elevation = 2;
				}
			}
		}
		break;

	case ECoMTerrain::Hills:
	case ECoMTerrain::TwilightHills:
	case ECoMTerrain::VineHills:
	case ECoMTerrain::CinderHills:
		// Hilly: moderate elevation.
		for (int32 Row = 0; Row < TACTICAL_GRID_HEIGHT; ++Row)
		{
			for (int32 Col = 0; Col < TACTICAL_GRID_WIDTH; ++Col)
			{
				if (Rng.RollPercent(30))
				{
					Grid[Row][Col].Terrain = ECoMTerrain::Hills;
					Grid[Row][Col].Elevation = 1;
				}
			}
		}
		break;

	case ECoMTerrain::Swamp:
	case ECoMTerrain::CorruptedSwamp:
	case ECoMTerrain::LivingSwamp:
		// Swamp: difficult terrain, no elevation.
		for (int32 Row = 0; Row < TACTICAL_GRID_HEIGHT; ++Row)
		{
			for (int32 Col = 0; Col < TACTICAL_GRID_WIDTH; ++Col)
			{
				if (Rng.RollPercent(45))
				{
					Grid[Row][Col].Terrain = ECoMTerrain::Swamp;
				}
			}
		}
		break;

	default:
		// Flat terrain (Grassland, Plains, Desert, etc.) — add random hills ~20%.
		for (int32 Row = 0; Row < TACTICAL_GRID_HEIGHT; ++Row)
		{
			for (int32 Col = 0; Col < TACTICAL_GRID_WIDTH; ++Col)
			{
				if (Rng.RollPercent(20))
				{
					Grid[Row][Col].Elevation = 1;
				}
			}
		}
		break;
	}

	// Siege: add wall tiles on the defender side (columns 7-8).
	if (bIsSiege)
	{
		for (int32 Row = 1; Row < TACTICAL_GRID_HEIGHT - 1; ++Row)
		{
			Grid[Row][8].bIsWall = true;
			Grid[Row][8].WallHP = CoM::DEFAULT_WALL_HP;
			Grid[Row][8].OccupyingUnitIndex = -1;
		}
		// Gate opening at the center.
		const int32 GateRow = TACTICAL_GRID_HEIGHT / 2;
		Grid[GateRow][8].bIsWall = false;
		Grid[GateRow][8].WallHP = 0;
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// Unit Placement
// ═══════════════════════════════════════════════════════════════════════════

void UCoMTacticalCombatSubsystem::PlaceUnits(
	const TArray<int32>& AttackerUnitIDs, int32 AttackerWizardId,
	const TArray<int32>& DefenderUnitIDs, int32 DefenderWizardId)
{
	UCoMUnitSubsystem* UnitSub = GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>();
	if (!UnitSub) return;

	// Helper lambda to create a tactical unit from an overworld unit instance.
	auto CreateTacticalUnit = [&](int32 UnitID, int32 WizardId, bool bAttacker) -> FCoMTacticalUnit
	{
		FCoMTacticalUnit TacUnit;
		TacUnit.UnitInstanceId = UnitID;
		TacUnit.OwnerWizardId = WizardId;
		TacUnit.bIsAttacker = bAttacker;

		const FCoMUnitInstance* Inst = UnitSub->GetUnit(UnitID);
		if (Inst)
		{
			TacUnit.CurrentHP        = Inst->CurrentHP;
			TacUnit.MaxHP            = Inst->MaxHP;
			TacUnit.MeleeAttack      = 3;  // TODO: resolve from UCoMUnitSpecDataAsset
			TacUnit.RangedAttack     = 0;
			TacUnit.Defense          = 3;
			TacUnit.MovementPoints   = 2;
			TacUnit.MovementRemaining = 2;
			TacUnit.Initiative       = 5;
			TacUnit.bIsHero          = Inst->bIsHero;
			TacUnit.MovementType     = Inst->MovementType;

			// Flying units get more movement.
			if (Inst->MovementType == ECoMMovementType::Flying)
			{
				TacUnit.MovementPoints    = 4;
				TacUnit.MovementRemaining = 4;
				TacUnit.Initiative       += 2;
			}

			// Heroes get stat boosts.
			if (Inst->bIsHero)
			{
				TacUnit.MeleeAttack  += 2;
				TacUnit.Defense      += 1;
				TacUnit.Initiative   += 3;
				TacUnit.MovementPoints    += 1;
				TacUnit.MovementRemaining += 1;
			}
		}
		else
		{
			// Unit not found in overworld — use default stats.
			TacUnit.CurrentHP = 5;
			TacUnit.MaxHP     = 5;
			TacUnit.MeleeAttack = 3;
			TacUnit.Defense     = 3;
			TacUnit.MovementPoints   = 2;
			TacUnit.MovementRemaining = 2;
			TacUnit.Initiative = 5;
		}

		return TacUnit;
	};

	// Place attackers on the left side (columns 0-2).
	int32 PlaceRow = 0;
	int32 PlaceCol = 0;
	for (int32 UnitID : AttackerUnitIDs)
	{
		FCoMTacticalUnit TacUnit = CreateTacticalUnit(UnitID, AttackerWizardId, true);
		TacUnit.GridPosition = FIntPoint(PlaceCol, PlaceRow);

		const int32 Idx = Units.Add(TacUnit);
		if (IsInBounds(TacUnit.GridPosition))
		{
			Grid[PlaceRow][PlaceCol].OccupyingUnitIndex = Idx;
		}

		PlaceRow++;
		if (PlaceRow >= TACTICAL_GRID_HEIGHT)
		{
			PlaceRow = 0;
			PlaceCol++;
			if (PlaceCol > 2) PlaceCol = 2; // Clamp to column 2 max.
		}
	}

	// Place defenders on the right side (columns 9-11).
	PlaceRow = 0;
	PlaceCol = TACTICAL_GRID_WIDTH - 1; // Start at column 11.
	for (int32 UnitID : DefenderUnitIDs)
	{
		FCoMTacticalUnit TacUnit = CreateTacticalUnit(UnitID, DefenderWizardId, false);
		TacUnit.GridPosition = FIntPoint(PlaceCol, PlaceRow);

		const int32 Idx = Units.Add(TacUnit);
		if (IsInBounds(TacUnit.GridPosition))
		{
			Grid[PlaceRow][PlaceCol].OccupyingUnitIndex = Idx;
		}

		PlaceRow++;
		if (PlaceRow >= TACTICAL_GRID_HEIGHT)
		{
			PlaceRow = 0;
			PlaceCol--;
			if (PlaceCol < TACTICAL_GRID_WIDTH - 3) PlaceCol = TACTICAL_GRID_WIDTH - 3; // Clamp to column 9.
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// Initiative
// ═══════════════════════════════════════════════════════════════════════════

void UCoMTacticalCombatSubsystem::RollInitiative()
{
	InitiativeOrder.Empty();

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Units[i].IsAlive())
		{
			// Initiative = base + random(0..9).
			Units[i].Initiative += Rng.NextIntRange(0, 10);
			InitiativeOrder.Add(i);
		}
	}

	// Sort descending by initiative (highest acts first).
	InitiativeOrder.Sort([this](int32 A, int32 B)
	{
		return Units[A].Initiative > Units[B].Initiative;
	});
}

// ═══════════════════════════════════════════════════════════════════════════
// Turn Flow
// ═══════════════════════════════════════════════════════════════════════════

void UCoMTacticalCombatSubsystem::BeginUnitTurn(int32 UnitIndex)
{
	if (!bBattleActive) return;
	if (!Units.IsValidIndex(UnitIndex)) return;

	FCoMTacticalUnit& Unit = Units[UnitIndex];
	if (!Unit.IsAlive()) return;

	Unit.bHasActed = false;
	Unit.bIsDefending = false;
	Unit.MovementRemaining = Unit.MovementPoints;

	OnUnitTurnStarted.Broadcast(UnitIndex);
}

void UCoMTacticalCombatSubsystem::EndUnitTurn()
{
	if (!bBattleActive) return;
	if (!Units.IsValidIndex(CurrentUnitIndex)) return;

	Units[CurrentUnitIndex].bHasActed = true;
	CheckBattleEnd();

	if (bBattleActive)
	{
		NextUnit();
	}
}

void UCoMTacticalCombatSubsystem::NextUnit()
{
	CurrentInitiativePos++;

	// If we've gone through all units, start a new round.
	if (CurrentInitiativePos >= InitiativeOrder.Num())
	{
		TurnCount++;

		// Check turn limit.
		if (TurnCount > TACTICAL_MAX_TURNS)
		{
			Result = ECoMCombatResult::Draw;
			bBattleActive = false;
			OnBattleEnded.Broadcast(Result);
			return;
		}

		// Re-roll initiative for the new round (keeps things dynamic).
		RollInitiative();
		CurrentInitiativePos = 0;
	}

	// Skip dead/fled units.
	while (CurrentInitiativePos < InitiativeOrder.Num())
	{
		const int32 Idx = InitiativeOrder[CurrentInitiativePos];
		if (Units.IsValidIndex(Idx) && Units[Idx].IsAlive())
		{
			CurrentUnitIndex = Idx;
			BeginUnitTurn(CurrentUnitIndex);
			return;
		}
		CurrentInitiativePos++;
	}

	// All remaining units are dead — should have been caught by CheckBattleEnd.
	CheckBattleEnd();
}

// ═══════════════════════════════════════════════════════════════════════════
// Movement (A* Pathfinding)
// ═══════════════════════════════════════════════════════════════════════════

bool UCoMTacticalCombatSubsystem::MoveUnit(int32 UnitIndex, FIntPoint TargetTile)
{
	if (!bBattleActive) return false;
	if (!Units.IsValidIndex(UnitIndex)) return false;

	FCoMTacticalUnit& Unit = Units[UnitIndex];
	if (!Unit.IsAlive() || Unit.bHasActed) return false;
	if (!IsInBounds(TargetTile)) return false;

	// Target must be unoccupied.
	if (Grid[TargetTile.Y][TargetTile.X].OccupyingUnitIndex >= 0)
	{
		return false;
	}

	// Find path via A*.
	TArray<FIntPoint> Path = FindPath(Unit.GridPosition, TargetTile, Unit.MovementType);
	if (Path.Num() == 0) return false;

	// Calculate total path cost.
	int32 TotalCost = 0;
	for (const FIntPoint& Step : Path)
	{
		const int32 StepCost = GetTileCost(Step, Unit.MovementType);
		if (StepCost < 0) return false; // Impassable tile in path — shouldn't happen with A*.
		TotalCost += StepCost;
	}

	if (TotalCost > Unit.MovementRemaining) return false;

	// Execute the move.
	const FIntPoint OldPos = Unit.GridPosition;
	Grid[OldPos.Y][OldPos.X].OccupyingUnitIndex = -1;
	Unit.GridPosition = TargetTile;
	Grid[TargetTile.Y][TargetTile.X].OccupyingUnitIndex = UnitIndex;
	Unit.MovementRemaining -= TotalCost;

	OnUnitMoved.Broadcast(UnitIndex, TargetTile);
	return true;
}

TArray<FIntPoint> UCoMTacticalCombatSubsystem::FindPath(
	FIntPoint Start, FIntPoint End, ECoMMovementType MoveType) const
{
	TArray<FIntPoint> ResultPath;
	if (!IsInBounds(Start) || !IsInBounds(End)) return ResultPath;
	if (Start == End) return ResultPath;

	// A* on the small 12x8 grid.
	TMap<int32, FTacticalPathNode> OpenSet;
	TSet<int32> ClosedSet;

	auto PosToKey = [](FIntPoint P) -> int32 { return P.Y * TACTICAL_GRID_WIDTH + P.X; };

	FTacticalPathNode StartNode;
	StartNode.Position = Start;
	StartNode.GCost = 0;
	StartNode.HCost = GetManhattanDistance(Start, End);
	StartNode.Parent = FIntPoint(-1, -1);
	OpenSet.Add(PosToKey(Start), StartNode);

	// 8-directional neighbors.
	static const FIntPoint Dirs[] = {
		{0, -1}, {0, 1}, {-1, 0}, {1, 0},
		{-1, -1}, {-1, 1}, {1, -1}, {1, 1}
	};

	TMap<int32, FTacticalPathNode> AllNodes;
	AllNodes.Add(PosToKey(Start), StartNode);

	while (OpenSet.Num() > 0)
	{
		// Find node with lowest F cost in the open set.
		int32 BestKey = -1;
		int32 BestF = TNumericLimits<int32>::Max();
		for (auto& Pair : OpenSet)
		{
			const int32 F = Pair.Value.FCost();
			if (F < BestF)
			{
				BestF = F;
				BestKey = Pair.Key;
			}
		}

		FTacticalPathNode Current = OpenSet[BestKey];
		OpenSet.Remove(BestKey);
		ClosedSet.Add(BestKey);

		// Reached the goal?
		if (Current.Position == End)
		{
			// Reconstruct path.
			FIntPoint Trace = End;
			while (Trace != Start)
			{
				ResultPath.Add(Trace);
				const int32 Key = PosToKey(Trace);
				if (AllNodes.Contains(Key))
				{
					Trace = AllNodes[Key].Parent;
				}
				else
				{
					break;
				}
			}
			Algo::Reverse(ResultPath);
			return ResultPath;
		}

		// Expand neighbors.
		for (const FIntPoint& Dir : Dirs)
		{
			const FIntPoint Neighbor = Current.Position + Dir;
			if (!IsInBounds(Neighbor)) continue;

			const int32 NKey = PosToKey(Neighbor);
			if (ClosedSet.Contains(NKey)) continue;

			const int32 TileCost = GetTileCost(Neighbor, MoveType);
			if (TileCost < 0) continue; // Impassable.

			// Check occupied tiles.
			const int32 OccIdx = Grid[Neighbor.Y][Neighbor.X].OccupyingUnitIndex;
			if (OccIdx >= 0 && Neighbor != End)
			{
				// Flying and PhaseWalk units can pass through friendly-occupied tiles.
				const bool bCanFlyOver = (MoveType == ECoMMovementType::Flying || MoveType == ECoMMovementType::PhaseWalk);
				const int32 MoverIdx = Grid[Start.Y][Start.X].OccupyingUnitIndex;
				const bool bIsFriendly = Units.IsValidIndex(OccIdx) && MoverIdx >= 0 &&
					(Units[OccIdx].bIsAttacker == Units[MoverIdx].bIsAttacker);
				if (!bCanFlyOver || !bIsFriendly)
				{
					continue;
				}
			}

			const int32 NewG = Current.GCost + TileCost;

			if (!OpenSet.Contains(NKey) || NewG < AllNodes[NKey].GCost)
			{
				FTacticalPathNode NeighborNode;
				NeighborNode.Position = Neighbor;
				NeighborNode.GCost = NewG;
				NeighborNode.HCost = GetManhattanDistance(Neighbor, End);
				NeighborNode.Parent = Current.Position;

				OpenSet.Add(NKey, NeighborNode);
				AllNodes.Add(NKey, NeighborNode);
			}
		}
	}

	return ResultPath; // No path found.
}

int32 UCoMTacticalCombatSubsystem::GetTileCost(FIntPoint Pos, ECoMMovementType MoveType) const
{
	if (!IsInBounds(Pos)) return -1;

	const FCoMTacticalTile& Tile = Grid[Pos.Y][Pos.X];

	// Flying units ignore terrain costs entirely.
	if (MoveType == ECoMMovementType::Flying)
	{
		return Tile.bIsWall ? 1 : 1; // Flying passes over walls too.
	}

	// Phase Walk ignores walls and terrain.
	if (MoveType == ECoMMovementType::PhaseWalk)
	{
		return 1;
	}

	// Walls are impassable for walking units.
	if (Tile.bIsWall && Tile.WallHP > 0)
	{
		return -1;
	}

	// Mountain tiles are impassable.
	if (Tile.Terrain == ECoMTerrain::Mountains)
	{
		return -1;
	}

	// Rough terrain costs more.
	switch (Tile.Terrain)
	{
	case ECoMTerrain::Forest:
	case ECoMTerrain::EnchantedForest:
	case ECoMTerrain::Jungle:
	case ECoMTerrain::ShadowForest:
	case ECoMTerrain::FungalForest:
		return 2;

	case ECoMTerrain::Swamp:
	case ECoMTerrain::CorruptedSwamp:
	case ECoMTerrain::LivingSwamp:
		return 2;

	case ECoMTerrain::Hills:
	case ECoMTerrain::TwilightHills:
	case ECoMTerrain::VineHills:
	case ECoMTerrain::CinderHills:
		return 2;

	default:
		return 1; // Flat terrain.
	}
}

int32 UCoMTacticalCombatSubsystem::GetElevationDefenseBonus(FIntPoint Pos) const
{
	if (!IsInBounds(Pos)) return 0;
	switch (Grid[Pos.Y][Pos.X].Elevation)
	{
	case 1: return 2;  // Hill: +2 defense.
	case 2: return 3;  // High ground: +3 defense.
	default: return 0;
	}
}

bool UCoMTacticalCombatSubsystem::IsInBounds(FIntPoint Pos)
{
	return Pos.X >= 0 && Pos.X < TACTICAL_GRID_WIDTH &&
	       Pos.Y >= 0 && Pos.Y < TACTICAL_GRID_HEIGHT;
}

// ═══════════════════════════════════════════════════════════════════════════
// Queries
// ═══════════════════════════════════════════════════════════════════════════

TArray<FIntPoint> UCoMTacticalCombatSubsystem::GetValidMoves(int32 UnitIndex) const
{
	TArray<FIntPoint> ValidTiles;
	if (!Units.IsValidIndex(UnitIndex)) return ValidTiles;

	const FCoMTacticalUnit& Unit = Units[UnitIndex];
	if (!Unit.IsAlive()) return ValidTiles;

	// BFS flood-fill from the unit's position within movement range.
	struct FFloodNode
	{
		FIntPoint Pos;
		int32 CostSoFar;
	};

	TQueue<FFloodNode> Queue;
	TSet<int32> Visited;

	auto PosToKey = [](FIntPoint P) -> int32 { return P.Y * TACTICAL_GRID_WIDTH + P.X; };

	FFloodNode Start;
	Start.Pos = Unit.GridPosition;
	Start.CostSoFar = 0;
	Queue.Enqueue(Start);
	Visited.Add(PosToKey(Start.Pos));

	static const FIntPoint Dirs[] = {
		{0, -1}, {0, 1}, {-1, 0}, {1, 0},
		{-1, -1}, {-1, 1}, {1, -1}, {1, 1}
	};

	while (!Queue.IsEmpty())
	{
		FFloodNode Current;
		Queue.Dequeue(Current);

		for (const FIntPoint& Dir : Dirs)
		{
			const FIntPoint Next = Current.Pos + Dir;
			if (!IsInBounds(Next)) continue;

			const int32 Key = PosToKey(Next);
			if (Visited.Contains(Key)) continue;

			const int32 Cost = GetTileCost(Next, Unit.MovementType);
			if (Cost < 0) continue; // Impassable.

			const int32 NewCost = Current.CostSoFar + Cost;
			if (NewCost > Unit.MovementRemaining) continue;

			Visited.Add(Key);

			// Only add as a valid destination if unoccupied.
			if (Grid[Next.Y][Next.X].OccupyingUnitIndex < 0)
			{
				ValidTiles.Add(Next);
			}

			// Continue expanding even through occupied tiles (allies).
			FFloodNode NextNode;
			NextNode.Pos = Next;
			NextNode.CostSoFar = NewCost;
			Queue.Enqueue(NextNode);
		}
	}

	return ValidTiles;
}

TArray<int32> UCoMTacticalCombatSubsystem::GetValidMeleeTargets(int32 UnitIndex) const
{
	TArray<int32> Targets;
	if (!Units.IsValidIndex(UnitIndex)) return Targets;

	const FCoMTacticalUnit& Unit = Units[UnitIndex];
	if (!Unit.IsAlive()) return Targets;

	// Check all 8 adjacent tiles.
	static const FIntPoint Dirs[] = {
		{0, -1}, {0, 1}, {-1, 0}, {1, 0},
		{-1, -1}, {-1, 1}, {1, -1}, {1, 1}
	};

	for (const FIntPoint& Dir : Dirs)
	{
		const FIntPoint Adj = Unit.GridPosition + Dir;
		if (!IsInBounds(Adj)) continue;

		const int32 OccIdx = Grid[Adj.Y][Adj.X].OccupyingUnitIndex;
		if (OccIdx >= 0 && Units.IsValidIndex(OccIdx) && Units[OccIdx].IsAlive())
		{
			// Enemy check: different side.
			if (Units[OccIdx].bIsAttacker != Unit.bIsAttacker)
			{
				Targets.Add(OccIdx);
			}
		}
	}

	return Targets;
}

TArray<int32> UCoMTacticalCombatSubsystem::GetValidRangedTargets(int32 UnitIndex) const
{
	TArray<int32> Targets;
	if (!Units.IsValidIndex(UnitIndex)) return Targets;

	const FCoMTacticalUnit& Unit = Units[UnitIndex];
	if (!Unit.IsAlive() || Unit.RangedAttack <= 0) return Targets;

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (i == UnitIndex) continue;
		if (!Units[i].IsAlive()) continue;
		if (Units[i].bIsAttacker == Unit.bIsAttacker) continue; // Same side.

		const int32 Dist = GetManhattanDistance(Unit.GridPosition, Units[i].GridPosition);

		// Ranged can't shoot point-blank (adjacent).
		if (Dist <= 1) continue;

		// Must be within range.
		if (Dist <= TACTICAL_DEFAULT_RANGE)
		{
			Targets.Add(i);
		}
	}

	return Targets;
}

bool UCoMTacticalCombatSubsystem::IsUnitAdjacent(int32 A, int32 B) const
{
	if (!Units.IsValidIndex(A) || !Units.IsValidIndex(B)) return false;

	const FIntPoint Diff = Units[A].GridPosition - Units[B].GridPosition;
	return FMath::Abs(Diff.X) <= 1 && FMath::Abs(Diff.Y) <= 1 &&
	       (Diff.X != 0 || Diff.Y != 0);
}

int32 UCoMTacticalCombatSubsystem::GetManhattanDistance(FIntPoint A, FIntPoint B)
{
	return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}

const FCoMTacticalUnit* UCoMTacticalCombatSubsystem::GetUnitAt(FIntPoint Pos) const
{
	if (!IsInBounds(Pos)) return nullptr;

	const int32 Idx = Grid[Pos.Y][Pos.X].OccupyingUnitIndex;
	if (Idx >= 0 && Units.IsValidIndex(Idx))
	{
		return &Units[Idx];
	}
	return nullptr;
}

bool UCoMTacticalCombatSubsystem::IsTilePassable(FIntPoint Pos, ECoMMovementType MoveType) const
{
	return GetTileCost(Pos, MoveType) >= 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Combat Actions
// ═══════════════════════════════════════════════════════════════════════════

bool UCoMTacticalCombatSubsystem::MeleeAttack(int32 AttackerIndex, int32 DefenderIndex)
{
	if (!bBattleActive) return false;
	if (!Units.IsValidIndex(AttackerIndex) || !Units.IsValidIndex(DefenderIndex)) return false;

	FCoMTacticalUnit& Attacker = Units[AttackerIndex];
	FCoMTacticalUnit& Defender = Units[DefenderIndex];

	if (!Attacker.IsAlive() || Attacker.bHasActed) return false;
	if (!Defender.IsAlive()) return false;

	// Must be adjacent.
	if (!IsUnitAdjacent(AttackerIndex, DefenderIndex)) return false;

	// Must be an enemy.
	if (Attacker.bIsAttacker == Defender.bIsAttacker) return false;

	ResolveMeleeHit(Attacker, Defender);
	Attacker.bHasActed = true;

	CheckBattleEnd();
	return true;
}

bool UCoMTacticalCombatSubsystem::RangedAttack(int32 AttackerIndex, int32 DefenderIndex)
{
	if (!bBattleActive) return false;
	if (!Units.IsValidIndex(AttackerIndex) || !Units.IsValidIndex(DefenderIndex)) return false;

	FCoMTacticalUnit& Attacker = Units[AttackerIndex];
	FCoMTacticalUnit& Defender = Units[DefenderIndex];

	if (!Attacker.IsAlive() || Attacker.bHasActed) return false;
	if (!Defender.IsAlive()) return false;
	if (Attacker.RangedAttack <= 0) return false;

	// Must be an enemy.
	if (Attacker.bIsAttacker == Defender.bIsAttacker) return false;

	const int32 Dist = GetManhattanDistance(Attacker.GridPosition, Defender.GridPosition);

	// No point-blank ranged.
	if (Dist <= 1) return false;

	// Must be within range.
	if (Dist > TACTICAL_DEFAULT_RANGE) return false;

	ResolveRangedHit(Attacker, Defender);
	Attacker.bHasActed = true;

	CheckBattleEnd();
	return true;
}

bool UCoMTacticalCombatSubsystem::CastCombatSpell(int32 CasterIndex, FName SpellId, FIntPoint Target)
{
	if (!bBattleActive) return false;
	if (!Units.IsValidIndex(CasterIndex)) return false;

	FCoMTacticalUnit& Caster = Units[CasterIndex];
	if (!Caster.IsAlive() || Caster.bHasActed) return false;

	// TODO: Integrate with UCoMMagicSubsystem for spell resolution.
	// For now, log and mark the action as taken.
	UE_LOG(LogTacticalCombat, Log,
	       TEXT("Unit %d casts spell '%s' at (%d,%d) — spell system not yet integrated."),
	       CasterIndex, *SpellId.ToString(), Target.X, Target.Y);

	Caster.bHasActed = true;
	return true;
}

bool UCoMTacticalCombatSubsystem::Defend(int32 UnitIndex)
{
	if (!bBattleActive) return false;
	if (!Units.IsValidIndex(UnitIndex)) return false;

	FCoMTacticalUnit& Unit = Units[UnitIndex];
	if (!Unit.IsAlive() || Unit.bHasActed) return false;

	Unit.bIsDefending = true;
	Unit.bHasActed = true;

	UE_LOG(LogTacticalCombat, Verbose, TEXT("Unit %d assumes defensive stance."), UnitIndex);
	return true;
}

bool UCoMTacticalCombatSubsystem::Wait(int32 UnitIndex)
{
	if (!bBattleActive) return false;
	if (!Units.IsValidIndex(UnitIndex)) return false;

	FCoMTacticalUnit& Unit = Units[UnitIndex];
	if (!Unit.IsAlive() || Unit.bHasActed) return false;

	// Push this unit to the end of the current initiative order.
	// Remove from current position and append at end.
	InitiativeOrder.Remove(UnitIndex);
	InitiativeOrder.Add(UnitIndex);

	// Adjust CurrentInitiativePos since we removed an element before it.
	if (CurrentInitiativePos > 0)
	{
		CurrentInitiativePos--;
	}

	UE_LOG(LogTacticalCombat, Verbose, TEXT("Unit %d waits (moved to end of initiative)."), UnitIndex);

	// Don't mark as acted — the unit will get another chance this round.
	return true;
}

bool UCoMTacticalCombatSubsystem::AttemptFlee(int32 UnitIndex)
{
	if (!bBattleActive) return false;
	if (!Units.IsValidIndex(UnitIndex)) return false;

	FCoMTacticalUnit& Unit = Units[UnitIndex];
	if (!Unit.IsAlive() || Unit.bHasActed) return false;

	// 50% base chance + 10% per movement speed advantage over average enemy.
	int32 FleeChance = 50;

	// Calculate average enemy movement.
	int32 TotalEnemyMove = 0;
	int32 EnemyCount = 0;
	for (const FCoMTacticalUnit& Other : Units)
	{
		if (Other.IsAlive() && Other.bIsAttacker != Unit.bIsAttacker)
		{
			TotalEnemyMove += Other.MovementPoints;
			EnemyCount++;
		}
	}

	if (EnemyCount > 0)
	{
		const int32 AvgEnemyMove = TotalEnemyMove / EnemyCount;
		const int32 SpeedAdvantage = Unit.MovementPoints - AvgEnemyMove;
		FleeChance += SpeedAdvantage * 10;
	}

	FleeChance = FMath::Clamp(FleeChance, 10, 90);

	if (Rng.RollPercent(FleeChance))
	{
		// Flee successful.
		Unit.bHasFled = true;
		Grid[Unit.GridPosition.Y][Unit.GridPosition.X].OccupyingUnitIndex = -1;
		Unit.GridPosition = FIntPoint(-1, -1);

		UE_LOG(LogTacticalCombat, Log, TEXT("Unit %d fled successfully (chance was %d%%)."), UnitIndex, FleeChance);

		CheckBattleEnd();
		return true;
	}
	else
	{
		// Flee failed — loses turn.
		Unit.bHasActed = true;
		UE_LOG(LogTacticalCombat, Log, TEXT("Unit %d failed to flee (chance was %d%%)."), UnitIndex, FleeChance);
		return false;
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// Combat Resolution
// ═══════════════════════════════════════════════════════════════════════════

void UCoMTacticalCombatSubsystem::ResolveMeleeHit(FCoMTacticalUnit& Attacker, FCoMTacticalUnit& Defender)
{
	// Hit chance: 30 + MeleeAttack - Defense, clamped [5, 95].
	int32 EffectiveDefense = Defender.Defense;

	// Defending stance bonus: +50% defense.
	if (Defender.bIsDefending)
	{
		EffectiveDefense = EffectiveDefense + (EffectiveDefense / 2);
	}

	// Terrain elevation bonus.
	EffectiveDefense += GetElevationDefenseBonus(Defender.GridPosition);

	const int32 HitChance = FMath::Clamp(30 + Attacker.MeleeAttack - EffectiveDefense, 5, 95);

	if (Rng.RollPercent(HitChance))
	{
		// Hit! Damage = MeleeAttack (simplified; later: * FiguresDamageMultiplier).
		const int32 Damage = FMath::Max(1, Attacker.MeleeAttack);

		UE_LOG(LogTacticalCombat, Verbose,
		       TEXT("Melee hit! Unit %d -> Unit %d for %d damage (chance %d%%)."),
		       Attacker.UnitInstanceId, Defender.UnitInstanceId, Damage, HitChance);

		ApplyDamage(Defender, Damage);
	}
	else
	{
		UE_LOG(LogTacticalCombat, Verbose,
		       TEXT("Melee miss. Unit %d -> Unit %d (chance was %d%%)."),
		       Attacker.UnitInstanceId, Defender.UnitInstanceId, HitChance);
	}
}

void UCoMTacticalCombatSubsystem::ResolveRangedHit(FCoMTacticalUnit& Attacker, FCoMTacticalUnit& Defender)
{
	// Damage: RangedAttack - (Defense / 2), minimum 1.
	int32 EffectiveDefense = Defender.Defense / 2;

	// Defending stance bonus.
	if (Defender.bIsDefending)
	{
		EffectiveDefense += Defender.Defense / 4; // +50% of the halved defense.
	}

	// Hit chance for ranged: 30 + RangedAttack - EffectiveDefense, clamped.
	const int32 HitChance = FMath::Clamp(30 + Attacker.RangedAttack - EffectiveDefense, 5, 95);

	// TODO: Apply weather penalty from UCoMWeatherSubsystem.

	if (Rng.RollPercent(HitChance))
	{
		const int32 Damage = FMath::Max(1, Attacker.RangedAttack - EffectiveDefense);

		UE_LOG(LogTacticalCombat, Verbose,
		       TEXT("Ranged hit! Unit %d -> Unit %d for %d damage (chance %d%%)."),
		       Attacker.UnitInstanceId, Defender.UnitInstanceId, Damage, HitChance);

		ApplyDamage(Defender, Damage);
	}
	else
	{
		UE_LOG(LogTacticalCombat, Verbose,
		       TEXT("Ranged miss. Unit %d -> Unit %d (chance was %d%%)."),
		       Attacker.UnitInstanceId, Defender.UnitInstanceId, HitChance);
	}
}

void UCoMTacticalCombatSubsystem::ApplyDamage(FCoMTacticalUnit& Target, int32 Damage)
{
	if (Damage <= 0) return;

	Target.CurrentHP -= Damage;

	// Find the index of this unit for broadcasting.
	const int32 TargetIndex = Units.IndexOfByPredicate([&](const FCoMTacticalUnit& U)
	{
		return &U == &Target;
	});

	OnUnitDamaged.Broadcast(TargetIndex, Damage);

	if (Target.CurrentHP <= 0)
	{
		KillUnit(Target);

		if (TargetIndex >= 0)
		{
			OnUnitKilled.Broadcast(TargetIndex);
		}
	}
}

void UCoMTacticalCombatSubsystem::KillUnit(FCoMTacticalUnit& Unit)
{
	Unit.bIsDead = true;
	Unit.CurrentHP = 0;

	// Free the tile.
	if (IsInBounds(Unit.GridPosition))
	{
		Grid[Unit.GridPosition.Y][Unit.GridPosition.X].OccupyingUnitIndex = -1;
	}

	UE_LOG(LogTacticalCombat, Log, TEXT("Unit %d (Wizard %d) killed."),
	       Unit.UnitInstanceId, Unit.OwnerWizardId);
}

// ═══════════════════════════════════════════════════════════════════════════
// Battle End Check
// ═══════════════════════════════════════════════════════════════════════════

void UCoMTacticalCombatSubsystem::CheckBattleEnd()
{
	if (!bBattleActive) return;

	bool bAnyAttackerAlive = false;
	bool bAnyDefenderAlive = false;

	for (const FCoMTacticalUnit& Unit : Units)
	{
		if (Unit.IsAlive())
		{
			if (Unit.bIsAttacker)
				bAnyAttackerAlive = true;
			else
				bAnyDefenderAlive = true;
		}
	}

	if (!bAnyAttackerAlive && !bAnyDefenderAlive)
	{
		Result = ECoMCombatResult::Draw;
	}
	else if (!bAnyAttackerAlive)
	{
		// Check if attackers fled rather than died.
		bool bAnyAttackerFled = false;
		for (const FCoMTacticalUnit& Unit : Units)
		{
			if (Unit.bIsAttacker && Unit.bHasFled)
			{
				bAnyAttackerFled = true;
				break;
			}
		}
		Result = bAnyAttackerFled ? ECoMCombatResult::AttackerFlee : ECoMCombatResult::DefenderVictory;
	}
	else if (!bAnyDefenderAlive)
	{
		bool bAnyDefenderFled = false;
		for (const FCoMTacticalUnit& Unit : Units)
		{
			if (!Unit.bIsAttacker && Unit.bHasFled)
			{
				bAnyDefenderFled = true;
				break;
			}
		}
		Result = bAnyDefenderFled ? ECoMCombatResult::DefenderFlee : ECoMCombatResult::AttackerVictory;
	}
	else
	{
		return; // Battle continues.
	}

	bBattleActive = false;
	UE_LOG(LogTacticalCombat, Log, TEXT("Battle ended: result=%d after %d turns."),
	       static_cast<int32>(Result), TurnCount);

	OnBattleEnded.Broadcast(Result);
}

// ═══════════════════════════════════════════════════════════════════════════
// Finalize — Write Back to Overworld
// ═══════════════════════════════════════════════════════════════════════════

void UCoMTacticalCombatSubsystem::FinalizeBattle()
{
	UCoMUnitSubsystem* UnitSub = GetGameInstance()->GetSubsystem<UCoMUnitSubsystem>();
	if (!UnitSub)
	{
		UE_LOG(LogTacticalCombat, Error, TEXT("FinalizeBattle: UCoMUnitSubsystem not found."));
		return;
	}

	int32 AttackerKills = 0;
	int32 DefenderKills = 0;

	// Apply casualties to overworld units.
	for (const FCoMTacticalUnit& TacUnit : Units)
	{
		if (TacUnit.bIsDead)
		{
			// Despawn the unit from the overworld.
			UnitSub->DespawnUnit(TacUnit.UnitInstanceId);

			if (TacUnit.bIsAttacker)
				AttackerKills++;  // Defender killed an attacker.
			else
				DefenderKills++;  // Attacker killed a defender.
		}
		// Fled units survive on the overworld — their HP is preserved.
		// Wounded survivors: update HP on the overworld instance.
		// TODO: UCoMUnitSubsystem needs a SetUnitHP method; for now we rely on
		// the overworld reading the CombatResult.
	}

	// Distribute XP: 2 XP per enemy killed, awarded to all survivors on the winning side.
	// TODO: Call UCoMHeroSubsystem::AwardHeroXP for hero units.
	const int32 XPPerKill = 2;
	for (const FCoMTacticalUnit& TacUnit : Units)
	{
		if (TacUnit.IsAlive() && TacUnit.bIsHero)
		{
			const int32 EnemyKills = TacUnit.bIsAttacker ? DefenderKills : AttackerKills;
			const int32 XPEarned = XPPerKill * EnemyKills;
			UE_LOG(LogTacticalCombat, Log, TEXT("Hero unit %d earns %d XP."),
			       TacUnit.UnitInstanceId, XPEarned);
			// TODO: UnitSub->AddUnitXP(TacUnit.UnitInstanceId, XPEarned);
		}
	}

	UE_LOG(LogTacticalCombat, Log,
	       TEXT("Battle finalized. Attacker casualties: %d, Defender casualties: %d."),
	       AttackerKills, DefenderKills);
}

// ═══════════════════════════════════════════════════════════════════════════
// AI Combat Decision
// ═══════════════════════════════════════════════════════════════════════════

ECoMTacticalAction UCoMTacticalCombatSubsystem::DecideAIAction(int32 UnitIndex)
{
	if (!Units.IsValidIndex(UnitIndex)) return ECoMTacticalAction::Defend;

	const FCoMTacticalUnit& Unit = Units[UnitIndex];
	if (!Unit.IsAlive()) return ECoMTacticalAction::Defend;

	// Priority 1: If adjacent to an enemy, melee attack.
	TArray<int32> MeleeTargets = GetValidMeleeTargets(UnitIndex);
	if (MeleeTargets.Num() > 0)
	{
		return ECoMTacticalAction::MeleeAttack;
	}

	// Priority 2: If ranged and enemy in range, ranged attack.
	if (Unit.RangedAttack > 0)
	{
		TArray<int32> RangedTargets = GetValidRangedTargets(UnitIndex);
		if (RangedTargets.Num() > 0)
		{
			return ECoMTacticalAction::RangedAttack;
		}
	}

	// Priority 3: Move toward the nearest enemy.
	TArray<FIntPoint> ValidMoves = GetValidMoves(UnitIndex);
	if (ValidMoves.Num() > 0)
	{
		const int32 NearestEnemy = FindNearestEnemy(UnitIndex);
		if (NearestEnemy >= 0)
		{
			return ECoMTacticalAction::Move;
		}
	}

	// Priority 4: Defend.
	return ECoMTacticalAction::Defend;
}

void UCoMTacticalCombatSubsystem::ExecuteAITurn(int32 UnitIndex)
{
	if (!Units.IsValidIndex(UnitIndex)) return;

	const ECoMTacticalAction Action = DecideAIAction(UnitIndex);

	switch (Action)
	{
	case ECoMTacticalAction::MeleeAttack:
	{
		TArray<int32> Targets = GetValidMeleeTargets(UnitIndex);
		if (Targets.Num() > 0)
		{
			// Pick the weakest target (lowest HP).
			int32 BestTarget = Targets[0];
			int32 LowestHP = Units[Targets[0]].CurrentHP;
			for (int32 T : Targets)
			{
				if (Units[T].CurrentHP < LowestHP)
				{
					LowestHP = Units[T].CurrentHP;
					BestTarget = T;
				}
			}
			MeleeAttack(UnitIndex, BestTarget);
		}
		break;
	}

	case ECoMTacticalAction::RangedAttack:
	{
		TArray<int32> Targets = GetValidRangedTargets(UnitIndex);
		if (Targets.Num() > 0)
		{
			// Pick the weakest target.
			int32 BestTarget = Targets[0];
			int32 LowestHP = Units[Targets[0]].CurrentHP;
			for (int32 T : Targets)
			{
				if (Units[T].CurrentHP < LowestHP)
				{
					LowestHP = Units[T].CurrentHP;
					BestTarget = T;
				}
			}
			RangedAttack(UnitIndex, BestTarget);
		}
		break;
	}

	case ECoMTacticalAction::Move:
	{
		const int32 NearestEnemy = FindNearestEnemy(UnitIndex);
		if (NearestEnemy >= 0)
		{
			const FIntPoint EnemyPos = Units[NearestEnemy].GridPosition;
			TArray<FIntPoint> ValidMoves = GetValidMoves(UnitIndex);

			// Find the valid move tile closest to the enemy.
			FIntPoint BestTile = Units[UnitIndex].GridPosition;
			int32 BestDist = GetManhattanDistance(BestTile, EnemyPos);

			for (const FIntPoint& Tile : ValidMoves)
			{
				const int32 Dist = GetManhattanDistance(Tile, EnemyPos);
				if (Dist < BestDist)
				{
					BestDist = Dist;
					BestTile = Tile;
				}
			}

			if (BestTile != Units[UnitIndex].GridPosition)
			{
				MoveUnit(UnitIndex, BestTile);
			}

			// After moving, check if we can now attack.
			TArray<int32> MeleeTargets = GetValidMeleeTargets(UnitIndex);
			if (MeleeTargets.Num() > 0 && !Units[UnitIndex].bHasActed)
			{
				int32 BestTarget = MeleeTargets[0];
				int32 LowestHP = Units[MeleeTargets[0]].CurrentHP;
				for (int32 T : MeleeTargets)
				{
					if (Units[T].CurrentHP < LowestHP)
					{
						LowestHP = Units[T].CurrentHP;
						BestTarget = T;
					}
				}
				MeleeAttack(UnitIndex, BestTarget);
			}
		}
		break;
	}

	case ECoMTacticalAction::Defend:
	default:
		Defend(UnitIndex);
		break;
	}
}

int32 UCoMTacticalCombatSubsystem::FindNearestEnemy(int32 UnitIndex) const
{
	if (!Units.IsValidIndex(UnitIndex)) return -1;

	const FCoMTacticalUnit& Unit = Units[UnitIndex];
	int32 NearestIndex = -1;
	int32 NearestDist = TNumericLimits<int32>::Max();

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (i == UnitIndex) continue;
		if (!Units[i].IsAlive()) continue;
		if (Units[i].bIsAttacker == Unit.bIsAttacker) continue;

		const int32 Dist = GetManhattanDistance(Unit.GridPosition, Units[i].GridPosition);
		if (Dist < NearestDist)
		{
			NearestDist = Dist;
			NearestIndex = i;
		}
	}

	return NearestIndex;
}
