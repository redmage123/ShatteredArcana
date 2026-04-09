#!/usr/bin/env python3
"""
Shattered Arcana — Automated Playtest
Runs a full game simulation via UE5 commandlet, exercising all subsystems.
Reports what works, what breaks, and what feels off.

Usage:
    # From UE5 command line (Editor mode):
    /opt/UnrealEngine/Engine/Binaries/Linux/UnrealEditor-Cmd \
        /home/bbrelin/ShatteredArcana/ShatteredArcana.uproject \
        -ExecCmds="com.playtest 20" -unattended -NullRHI -NoSound -log

    # Or just validate the C++ subsystem logic directly:
    python3 Scripts/playtest.py
"""

import subprocess
import sys
import os
import json
import time

PROJECT = "/home/bbrelin/ShatteredArcana/ShatteredArcana.uproject"
UE_CMD = "/opt/UnrealEngine/Engine/Binaries/Linux/UnrealEditor-Cmd"

def run_ue5_playtest(num_turns=20):
    """Run UE5 in commandlet mode with the playtest console command."""
    cmd = [
        UE_CMD,
        PROJECT,
        f'-ExecCmds=com.playtest {num_turns}',
        '-unattended',
        '-NullRHI',      # No GPU needed
        '-NoSound',      # No audio device needed
        '-nosplash',
        '-log',
        '-NOSAVECONFIG',
    ]

    print(f"=== Running UE5 Playtest ({num_turns} turns) ===")
    print(f"Command: {' '.join(cmd[:4])}...")
    print()

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=300,  # 5 minutes max
        )

        # Parse output for our log lines
        lines = result.stdout.split('\n') + result.stderr.split('\n')
        playtest_lines = [l for l in lines if '[Playtest]' in l or '[CoM' in l]

        print("=== Playtest Output ===")
        for line in playtest_lines:
            print(f"  {line.strip()}")

        errors = [l for l in lines if 'Error' in l or 'error' in l.lower()]
        warnings = [l for l in lines if 'Warning' in l and 'CoM' in l]

        print(f"\n=== Summary ===")
        print(f"  Playtest lines: {len(playtest_lines)}")
        print(f"  Errors: {len(errors)}")
        print(f"  CoM Warnings: {len(warnings)}")

        if errors:
            print("\n=== Errors ===")
            for e in errors[:20]:
                print(f"  {e.strip()}")

        if warnings:
            print("\n=== Warnings ===")
            for w in warnings[:20]:
                print(f"  {w.strip()}")

        return result.returncode

    except subprocess.TimeoutExpired:
        print("ERROR: Playtest timed out after 5 minutes")
        return 1
    except FileNotFoundError:
        print(f"ERROR: UE5 Editor not found at {UE_CMD}")
        print("Falling back to subsystem-level validation...")
        return run_subsystem_validation()


def run_subsystem_validation():
    """Validate subsystem logic without UE5 — check the code for obvious issues."""
    print("\n=== Subsystem Validation (no UE5 required) ===\n")

    issues = []
    root = "/home/bbrelin/ShatteredArcana/Source/CoMCore"

    # Check 1: All subsystems have Export/Import for save
    print("Check 1: Save/Load coverage...")
    subsystems_needing_save = [
        "World/CoMWorldMapSubsystem",
        "Economy/CoMCitySubsystem",
        "Units/CoMUnitSubsystem",
        "Units/CoMHeroSubsystem",
        "Units/CoMDragonSubsystem",
        "Diplomacy/CoMDiplomacySubsystem",
        "Espionage/CoMEspionageSubsystem",
        "Magic/CoMMagicSubsystem",
        "Quests/CoMQuestSubsystem",
        "Combat/CoMNavalSubsystem",
        "Turn/CoMTurnSubsystem",
        "Events/CoMWorldEventSubsystem",
        "Victory/CoMVictorySubsystem",
    ]
    for sub in subsystems_needing_save:
        h_path = os.path.join(root, sub + ".h")
        if os.path.exists(h_path):
            content = open(h_path).read()
            has_export = "ExportAll" in content or "Export" in content
            has_import = "ImportAll" in content or "Import" in content
            if not has_export or not has_import:
                issues.append(f"  MISSING SAVE: {sub} lacks Export/Import")
            else:
                print(f"  OK: {sub}")
        else:
            issues.append(f"  MISSING FILE: {h_path}")

    # Check 2: TurnSubsystem wires all subsystems
    print("\nCheck 2: Turn processing coverage...")
    turn_cpp = os.path.join(root, "Turn/CoMTurnSubsystem.cpp")
    if os.path.exists(turn_cpp):
        content = open(turn_cpp).read()
        expected_calls = [
            "SeasonSubsystem", "WeatherSubsystem", "WorldEvents",
            "UnitSubsystem", "CombatSubsystem", "NavalSubsystem",
            "ResourceSubsystem", "CitySubsystem", "MagicSubsystem",
            "HeroSubsystem", "DragonSubsystem", "DiplomacySubsystem",
            "EspionageSubsystem", "QuestSubsystem", "FogOfWarSubsystem",
            "VictorySubsystem",
        ]
        for call in expected_calls:
            if call in content:
                print(f"  OK: {call} wired")
            else:
                issues.append(f"  MISSING WIRE: {call} not in TurnSubsystem")

    # Check 3: All UI widgets exist
    print("\nCheck 3: UI widget coverage...")
    ui_root = "/home/bbrelin/ShatteredArcana/Source/CoMUI"
    expected_widgets = [
        "HUD/CoMHUDWidget",
        "HUD/CoMMainMenuWidget",
        "HUD/CoMMinimapWidget",
        "HUD/CoMTooltipWidget",
        "HUD/CoMTurnNotificationWidget",
        "HUD/CoMSpellTargetingWidget",
        "HUD/CoMLoadScreenWidget",
        "Panels/CoMCityScreenWidget",
        "Panels/CoMSpellBookWidget",
        "Panels/CoMDiplomacyWidget",
        "Panels/CoMArmyPanelWidget",
        "Panels/CoMCreditsWidget",
        "Panels/CoMVictoryScreenWidget",
        "Panels/CoMSettingsWidget",
        "Panels/CoMWizardCreationWidget",
    ]
    for widget in expected_widgets:
        h_path = os.path.join(ui_root, widget + ".h")
        cpp_path = os.path.join(ui_root, widget + ".cpp")
        if os.path.exists(h_path) and os.path.exists(cpp_path):
            print(f"  OK: {widget}")
        else:
            issues.append(f"  MISSING WIDGET: {widget}")

    # Check 4: Art assets coverage
    print("\nCheck 4: Art asset coverage...")
    art_root = "/home/bbrelin/ShatteredArcana/Art/GameAssets/processed"
    categories = {
        "terrain": 196,  # ECoMTerrain enum values
        "units": 126,    # 42 races x 3 types
        "buildings": 24, # minimum building types
        "heroes": 26,    # 13 archetypes x 2 genders
        "spells": 600,   # 600 spell catalogue
        "map_features": 23,
        "ui": 50,        # minimum UI elements
    }
    for cat, minimum in categories.items():
        cat_dir = os.path.join(art_root, cat)
        if os.path.isdir(cat_dir):
            count = sum(1 for f in os.listdir(cat_dir) if f.endswith('.png'))
            # Check subdirs too
            for sub in os.listdir(cat_dir):
                subpath = os.path.join(cat_dir, sub)
                if os.path.isdir(subpath):
                    count += sum(1 for f in os.listdir(subpath) if f.endswith('.png'))
            if count >= minimum:
                print(f"  OK: {cat} ({count} >= {minimum})")
            else:
                issues.append(f"  LOW ART: {cat} has {count}, needs {minimum}")
        else:
            issues.append(f"  MISSING DIR: {cat_dir}")

    # Check 5: Audio coverage
    print("\nCheck 5: Audio coverage...")
    audio_root = "/home/bbrelin/ShatteredArcana/Content/Audio"
    audio_categories = {
        "Music": 10,
        "SFX/UI": 50,
        "SFX/Combat": 20,
        "SFX/Magic": 10,
        "SFX/Creatures": 10,
    }
    for cat, minimum in audio_categories.items():
        cat_dir = os.path.join(audio_root, cat)
        if os.path.isdir(cat_dir):
            count = sum(1 for f in os.listdir(cat_dir)
                       if f.endswith(('.ogg', '.wav', '.mp3')))
            if count >= minimum:
                print(f"  OK: {cat} ({count} >= {minimum})")
            else:
                issues.append(f"  LOW AUDIO: {cat} has {count}, needs {minimum}")
        else:
            issues.append(f"  MISSING DIR: {cat_dir}")

    # Check 6: Key gameplay values not still placeholder
    print("\nCheck 6: Placeholder values...")
    city_cpp = os.path.join(root, "Economy/CoMCitySubsystem.cpp")
    if os.path.exists(city_cpp):
        content = open(city_cpp).read()
        if "BuildingCost = 50" in content:
            issues.append("  PLACEHOLDER: Building cost still hardcoded at 50 (CoMCitySubsystem.cpp)")
        if "TODO" in content:
            todo_count = content.count("TODO")
            if todo_count > 5:
                issues.append(f"  TODOs: CoMCitySubsystem.cpp has {todo_count} TODO comments")

    # Check 7: Determinism — all subsystems use deterministic seeds
    print("\nCheck 7: Determinism...")
    for sub_dir in ["Economy", "Espionage", "Magic", "Quests", "Units"]:
        for fname in os.listdir(os.path.join(root, sub_dir)):
            if fname.endswith('.cpp'):
                fpath = os.path.join(root, sub_dir, fname)
                content = open(fpath).read()
                if "FPlatformTime::Cycles()" in content:
                    issues.append(f"  NON-DETERMINISTIC: {sub_dir}/{fname} uses FPlatformTime::Cycles()")
                elif "FRandomStream" in content and "0x" in content:
                    print(f"  OK: {sub_dir}/{fname} uses deterministic seed")

    # Report
    print(f"\n{'='*60}")
    print(f"PLAYTEST VALIDATION COMPLETE")
    print(f"{'='*60}")
    if issues:
        print(f"\n{len(issues)} ISSUES FOUND:\n")
        for issue in issues:
            print(issue)
    else:
        print("\nALL CHECKS PASSED — no issues found.")

    return 0 if not issues else 1


if __name__ == '__main__':
    # Try UE5 commandlet first, fall back to validation
    if os.path.exists(UE_CMD):
        num_turns = int(sys.argv[1]) if len(sys.argv) > 1 else 20
        exit(run_ue5_playtest(num_turns))
    else:
        exit(run_subsystem_validation())
