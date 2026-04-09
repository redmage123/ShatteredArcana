# Shattered Arcana — Kanban Board
**Last updated:** 2026-04-09T08:30 UTC | Updated by: Claude Code (animation pipeline + MoMask integration)

---

## 🔴 BLOCKED — Gate 1 Not Passed

| Ticket | Title | Owner | Priority | Notes |
|--------|-------|-------|----------|-------|
| BUG-BV03-002 | CoMOverworldGameMode not found in UObject registry | Lead Engineer | CRITICAL | Blocks L_Overworld load + Criterion 3, 4, 5, 6 |
| BUG-BV03-003 | Update create_overworld_level.py to non-deprecated API | Gameplay Dev | HIGH | Uses deprecated EditorLevelLibrary; needs LevelEditorSubsystem API |
| OBS-01 | Fix EndTurn key binding in DefaultInput.ini | Gameplay Dev | MEDIUM | `Return` key invalid |

---

## ✅ RESOLVED THIS SESSION (2026-04-09)

| Ticket | Title | Resolution |
|--------|-------|------------|
| S5-ANIM | Aurelith knight animation v4 | **Complete**: 15s WebM, steel plate + gold inlay texture, 5-beat sequence, quaternion-space keyframes, quality gate passes |
| ANIM-001 | Three.js mixer/Euler quaternion sync bug | **Fixed**: persistent action cache in clip-loader.js, quaternion-space overrides in api.js |
| ANIM-002 | Animation jitter (freeze, alternating diffs, hard cuts) | **Fixed**: range-bounded keyframes, auto crossfade, getClipTime negative speed, quality gate in animator.py |
| ANIM-003 | Knight texture (red tabard → steel plate) | **Complete**: Runtime texture repaint in materials.js, no skin visible, gold inlay filigree |
| ANIM-004 | MoMask motion diffusion integration | **Complete**: text-to-motion pipeline, SMPL→Mixamo retargeting, fine-tuned on knight data |
| ANIM-005 | Denizen batch pipeline | **Complete**: denizen_pipeline.py with 8 plane configs, --motion-diffusion flag, quality gate |

---

## ✅ DONE — Animation Pipeline (2026-04-09)

**Full denizen animation toolchain built:**

| Component | Status | Details |
|-----------|--------|---------|
| Pose editor (Playwright) | ✅ | 14 JS modules, 50+ Python methods, auto-center camera |
| Clip sequencer | ✅ | Negative speed, auto crossfade, persistent action cache |
| Keyframe engine | ✅ | Quaternion-space overrides, range-bounded interpolation |
| Quality gate | ✅ | Freeze/jitter/static detection, runs before WebM encode |
| Steel plate textures | ✅ | Runtime repaint: red→steel, skin→helmet, chain→plate, gold inlays |
| Background presets | ✅ | All 8 planes configured (aurelith, noctharion, verdantis, infernyx, aethermist, abyssal, ethereal, feywild) |
| MoMask integration | ✅ | Text→motion→Mixamo retarget, ~2s/beat on RTX 4000 |
| Beat crossfade blending | ✅ | 6-frame slerp at beat boundaries |
| Fine-tuned model | ✅ | VQ-VAE + Transformer on 25 knight combat samples |
| Batch pipeline | ✅ | `denizen_pipeline.py --all --motion-diffusion` |

---

## ✅ DONE — Codebase Audit & Fix Pass (2026-04-08)

**~125 fixes across ~45 files in 193-file C++ codebase** (unchanged from prior session)

---

## 📋 GATE 1 STATUS (updated 2026-04-08)

| Criterion | Status | Notes |
|-----------|--------|-------|
| C1: Zero errors/warnings in CoMCore | ⚠️ LIKELY PASS | ~125 fixes applied; needs build verification |
| C2: 25 DataAsset types in Content Browser | ⚠️ COND. PASS | Re-verify |
| C3: L_Overworld + CameraPawn | ❌ BLOCKED | BUG-BV03-002 still open |
| C4: TurnSubsystem phases + delegates | ⚠️ LIKELY PASS | All ProcessTurn calls wired |
| C5: WorldMapSubsystem 24 layers, no OOM | ✅ RESOLVED | Unified to 24 |
| C6: GameplayTags register cleanly | ⚠️ LIKELY PASS | Glamour + Feywild tags added |

**Gate 1: 🟡 NEARLY PASSED — needs BUG-BV03-002 fix + build verification**

---

## 📅 SPRINT 4 — Gate 1 Pass + Integration

| Ticket | Title | Priority | Owner |
|--------|-------|----------|-------|
| S4-T1 | Fix BUG-BV03-002 (CoMOverworldGameMode registration) | CRITICAL | Lead Engineer |
| S4-T2 | Fix BUG-BV03-003 (level setup script) | HIGH | Gameplay Dev |
| S4-T3 | Fix OBS-01 (EndTurn key binding) | MEDIUM | Gameplay Dev |
| S4-T4 | Run Gate 1 BV-03b | HIGH | QA Lead |
| S4-T5 | Enable test suite (remove #if 0) | MEDIUM | Lead Engineer |
| S4-T6 | Consolidate dual TurnManager/TurnSubsystem | MEDIUM | Lead Engineer |

## 📅 SPRINT 5 (After Gate 1) — Candidates

| Ticket | Title | Priority | Notes |
|--------|-------|----------|-------|
| S5-AI | CoMAI module — Strategic/Tactical AI | HIGH | Module exists but empty |
| S5-UI | CoMUI module — UMG widgets | HIGH | 68 UI PNGs ready |
| S5-SAVE | Save/Load system | MEDIUM | CoMCore/Save/ empty |
| ~~S5-ANIM~~ | ~~Aurelith knight 15s animation~~ | ~~MEDIUM~~ | **DONE** — v4 rendered + pipeline built |
| S5-DENIZENS | Render remaining 7 plane denizen animations | MEDIUM | Pipeline ready, one command per plane |
| S5-EVENTS | Phase 8: World Events | MEDIUM | Enum + struct defined, no subsystem |
| S5-VICTORY | Victory Conditions | MEDIUM | Enum defined |

---

## 📊 PROJECT METRICS (2026-04-09)

| Metric | Value |
|--------|-------|
| C++ source files | 193 (+3 network stubs) |
| C++ lines (est.) | ~30,000 |
| Test files | 21 |
| Game subsystems | 16 (all wired into TurnManager) |
| Data asset types | 26 |
| Gameplay tags | ~200 |
| Art assets (gameplay) | 1,333 PNGs |
| Art assets (website) | 845+ |
| Spell catalogue | 600 |
| Races (gameplay tags) | 60+ |
| Planes | 8 |
| Terrain types | 196 (all with art) |
| Godot prototype | 3,427 lines GDScript, playable |
| **Animation JS modules** | **14** |
| **Animation Python tools** | **6 (animator, retarget, motion_generator, denizen_pipeline, build_finetune_data, finetune_momask)** |
| **Mixamo FBX clips** | **51** |
| **MoMask fine-tune samples** | **25 (5 beats × 5 captions)** |
| **Denizen WebMs rendered** | **1 (Aurelith v4) + 6 early versions** |
| **Background presets** | **10 (8 planes + dark + neutral)** |
