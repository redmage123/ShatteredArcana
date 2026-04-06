# Shattered Arcana — Testing Policy
**Effective:** 2026-04-04  
**Authority:** Project Owner  
**Owner:** QA Lead

---

## Overview

Every code deliverable in this project MUST satisfy all four testing tiers before it is considered complete. No exceptions. This policy applies to every sprint, every agent, and every handoff.

---

## Tier 1 — Unit Tests

- **Scope:** Every C++ class, subsystem, and DataAsset.
- **Framework:** UE5 Automation Framework (`FAutomationTestBase`).
- **Coverage:** Test individual methods, edge cases, and invalid inputs.
- **Minimum bar:** 1 test per public method.
- **Responsibility:** The engineer writing the code writes the tests.

---

## Tier 2 — Integration Tests

- **Scope:** Subsystem interactions and cross-module boundaries.
- **Required pairs (non-exhaustive):**
  - WorldMapSubsystem ↔ TurnSubsystem
  - CombatSubsystem ↔ MagicSubsystem
  - TradeSubsystem ↔ CitySubsystem
  - CoMCore ↔ CoMNetwork
  - CoMCore ↔ CoMRendering
- **Checks:** Correct joint initialization, event propagation across boundaries, no OOM or crash on startup.
- **Responsibility:** Gameplay Dev + Lead Engineer review integration coverage at every handoff.

---

## Tier 3 — Regression Tests

- **Scope:** Every bug fix.
- **Rule:** Each fix ships with a regression test that:
  1. Reproduces the original bug (must fail before the fix).
  2. Verifies the fix (must pass after).
- **Lifecycle:** Regression tests are permanent; they run in every subsequent BV pass.
- **Suite location:** `qa/regression/`
- **Responsibility:** QA Lead maintains the suite. Engineers write the test as part of the fix PR.

---

## Tier 4 — End-to-End (E2E) Tests

- **Scope:** Full game loop.
- **Canonical sequence:**  
  new game → wizard creation → first turn → city founding → unit production → army movement → combat → spell cast → end turn
- **Execution:** Headless via benchmark harness (`COM-PERF-005`).
- **Gate:** E2E suite must pass before any Gate review proceeds.
- **Responsibility:** QA Lead schedules and signs off on each E2E run.

---

## Sprint-End Code Walkthrough Policy

Before every sprint retrospective, the following walkthrough cycle must complete:

| Round | Participants | Outcome |
|-------|-------------|---------|
| 1st Walkthrough | Lead Engineer + QA Lead + Gameplay Dev | File bugs for every issue found |
| Fix Cycle | Engineers fix all bugs; QA verifies each | All bugs resolved |
| 2nd Walkthrough | Same group reviews fixes | If new issues found → repeat cycle |
| … | Repeat until clean | Zero open bugs, zero gaps |
| Retrospective | All team | Only after QA signs off |

**No sprint closes with known bugs or gaps. No exceptions.**

---

## QA Lead Responsibilities

- Own the test plan for every sprint.
- Ensure every agent writing code also writes tests.
- **Reject any handoff that lacks tests** — return it to the sender with required test coverage listed.
- Maintain and grow the regression suite (`qa/regression/`).
- Run the full four-tier test suite before every Gate review.
- Lead sprint-end code walkthroughs.
- Block the retrospective until walkthroughs are clean.

---

## BV Pass Checklist (test gate)

Before filing any BV results, confirm:

- [ ] All unit tests pass (zero failures)
- [ ] All integration tests pass
- [ ] Regression suite passes (no regressions)
- [ ] E2E headless run passes (`COM-PERF-005`)
- [ ] Zero new compiler warnings in CoMCore
- [ ] Gate-specific criteria met (see `qa/sprint1-qa-results.md` for Gate 1 definition)

---

## Enforcement

Any deliverable missing tests is **rejected at handoff** and returned for rework. The sprint clock does not stop; the delivering agent is responsible for the delay.

*Policy established by project owner, 2026-04-04.*
