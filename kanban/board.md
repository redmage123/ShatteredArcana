# Shattered Arcana — Kanban Board
**Last updated:** 2026-04-04T23:45 UTC | Updated by: QA Lead (post BV-03)

---

## 🔴 BLOCKED — Gate 1 Not Passed

| Ticket | Title | Owner | Priority | Notes |
|--------|-------|-------|----------|-------|
| BUG-BV03-001 | Fix FIntPoint UPROPERTY init in CoMStructs.h (15 members) | Lead Engineer | HIGH | 15 LogClass errors on startup; inline `= FIntPoint::ZeroValue` required |
| BUG-BV03-002 | CoMOverworldGameMode not found in UObject registry | Lead Engineer | CRITICAL | Blocks L_Overworld load + Criterion 3, 4, 5, 6 |
| BUG-BV03-003 | Update create_overworld_level.py to non-deprecated API | Gameplay Dev | HIGH | Uses deprecated EditorLevelLibrary; `default_pawn_class` property not found |
| BUG-BV03-004 | Confirm layer count: 15 (5 planes) vs. 24 (8 planes) | Design + Lead | MEDIUM | Gate 1 DoD says 15; code has 24. Needs sign-off before BV-03b |

---

## 🟡 IN PROGRESS / PENDING FIXES

| Ticket | Title | Owner | Notes |
|--------|-------|-------|-------|
| OBS-01 | Fix EndTurn key binding in DefaultInput.ini | Gameplay Dev | `Return` key invalid |
| OBS-02 | Update level setup script to LevelEditorSubsystem API | Gameplay Dev | UE5.1+ deprecation |

---

## ✅ DONE — Sprint 1

| Ticket | Title | Completed | Notes |
|--------|-------|-----------|-------|
| COM-032 | CoMGameMode (Combat + Exploration) | 2026-04-04 | Tier 1 tests: 3 ✅ |
| COM-032 | CoMPlayerController (Human + AI) | 2026-04-04 | Tier 1 tests: 2 ✅ |
| COM-032 | CoMGameInstance + context structs | 2026-04-04 | Tier 1 tests: 6 ✅ |
| — | qa/testing-policy.md published | 2026-04-04 | Mandatory 4-tier policy |
| — | Agent notifications sent (all 5 engineers) | 2026-04-04 | Per testing policy |

---

## 📋 GATE 1 STATUS

| Criterion | Status | Blocker |
|-----------|--------|---------|
| C1: Zero errors/warnings in CoMCore | ❌ FAIL | BUG-BV03-001 |
| C2: 25 DataAsset types in Content Browser | ⚠️ COND. PASS | Re-verify after fix |
| C3: L_Overworld + CameraPawn | ❌ FAIL | BUG-BV03-002, BUG-BV03-003 |
| C4: TurnSubsystem phases + delegates | ❌ NOT VERIFIED | BUG-BV03-002 |
| C5: WorldMapSubsystem 15/24 layers, no OOM | ⚠️ DISCREPANCY | BUG-BV03-004 |
| C6: GameplayTags register cleanly | ⚠️ COND. PASS | Re-verify after fix |

**Gate 1: 🔴 NOT PASSED — BV-03b required after bug fixes**

---

## 📅 Next Actions

1. Lead Engineer: Fix BUG-BV03-001 (CoMStructs.h FIntPoint defaults)
2. Lead Engineer: Investigate + fix BUG-BV03-002 (CoMOverworldGameMode registration)
3. Gameplay Dev: Fix BUG-BV03-003 (level setup script)
4. Design: Resolve BUG-BV03-004 (plane count decision)
5. QA Lead: Run BV-03b once all blockers resolved

---

## ✅ DONE — Sprint 3 (Phase 2)

| Ticket | Title | Completed | Notes |
|--------|-------|-----------|-------|
| S3-T2 | Per-Plane Terrain Distribution DataAsset | 2026-04-06 | UCoMTerrainWeightTable→UPrimaryDataAsset; 5 plane tables; tests 12-16 |
| S3-T3 | Underdark Generation — plane-unique terrain | 2026-04-06 | Feature-zone stamping; DataAsset-driven weights; tests 17-20 |
| S3-T4 | Underwater Zone Generation | 2026-04-06 | 5 canonical types; depth progression; zone guarantees; tests 21-24 |

## 🟡 IN PROGRESS — Sprint 3

| Ticket | Title | Owner | Notes |
|--------|-------|-------|-------|
| S3-T1 | WorldGen Foundation | Lead Engineer | Handoff pending; UHT errors in CoMLeyPortalSubsystem.h+CoMTerritorySubsystem.h block build |

## 🚧 BLOCKER — Sprint 3 Build

- **BLK-S3-001**: Pre-existing UHT errors in `CoMLeyPortalSubsystem.h` (const FStruct* UFUNCTION returns) and `CoMTerritorySubsystem.h` (default param with CoM namespace constant) block full build validation
- All S3-T2/T3/T4 code is complete; tests cannot run until Lead Engineer resolves S3-T1

---

## ✅ DONE — Sprint 3

| Ticket | Title | Owner | Completed | Notes |
|--------|-------|-------|-----------|-------|
| S3-T1 | UCoMWorldGenerator — 5-stage world gen pipeline | Lead Engineer | 2026-04-06 | Deployed to WorldGen/; 24 layers; 8 planes; all tests compatible; handoff written |
