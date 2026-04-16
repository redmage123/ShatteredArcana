// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameInstance.h — Cross-level persistence context. COM-032
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CoreTypes/CoMEnums.h"
#include "CoreTypes/CoMConstants.h"
#include "CoMGameInstance.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Context structs — GameInstance-local, not replicated
// ─────────────────────────────────────────────────────────────────────────────

/**
 * FCoMCombatContext
 *
 * Populated before a level transition into a tactical combat map.
 * ACoMCombatGameMode reads this on BeginPlay to reconstruct the battle state.
 * Clear via UCoMGameInstance::ClearCombatContext() after the level consumes it.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMCombatContext
{
	GENERATED_BODY()

	/** Army group IDs in this battle (attacker first, then defender and any allies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<int32> ParticipatingArmyGroupIDs;

	/** WizardIndex of the attacker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 AttackerWizardIndex = CoM::WIZARD_INDEX_NONE;

	/** WizardIndex of the defender; WIZARD_INDEX_NONE for neutral/monster stacks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 DefenderWizardIndex = CoM::WIZARD_INDEX_NONE;

	/** Overworld tile where the battle is occurring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FIntPoint OriginTile = FIntPoint::ZeroValue;

	/** Plane hosting the battle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ECoMPlane OriginPlane = ECoMPlane::Aurelith;

	/**
	 * Map to transition back to after combat resolves.
	 * Must be set before entering the combat level; empty = bug.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName ReturnMapName;

	/** Returns true when the context carries enough data to start a battle. */
	bool IsValid() const
	{
		return ParticipatingArmyGroupIDs.Num() > 0 && !ReturnMapName.IsNone();
	}

	/** Reset to default state (call after combat level finishes). */
	void Reset() { *this = FCoMCombatContext{}; }
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * FCoMExplorationContext
 *
 * Populated before a level transition into a dungeon / encounter map.
 * ACoMExplorationGameMode reads this on BeginPlay.
 * Clear via UCoMGameInstance::ClearExplorationContext() after the level consumes it.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMExplorationContext
{
	GENERATED_BODY()

	/** DataAsset row name identifying the dungeon / encounter template. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	FName DungeonID;

	/** Army group entering the dungeon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	int32 EntryArmyGroupID = -1;

	/** WizardIndex of the entering player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	int32 WizardIndex = CoM::WIZARD_INDEX_NONE;

	/** Overworld tile the dungeon entrance is on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	FIntPoint OriginTile = FIntPoint::ZeroValue;

	/** Plane hosting the dungeon entrance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	ECoMPlane OriginPlane = ECoMPlane::Aurelith;

	/** Map to return to after exploration ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	FName ReturnMapName;

	bool IsValid() const
	{
		return !DungeonID.IsNone() && EntryArmyGroupID >= 0 && !ReturnMapName.IsNone();
	}

	void Reset() { *this = FCoMExplorationContext{}; }
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * FCoMNewGameSettings
 *
 * Collects all wizard-creation choices made in the new-game UI.
 * Consumed once by ACoMOverworldGameMode::BeginPlay on the first overworld load.
 * After consumption the struct is reset; IsValid() returns false until the UI
 * repopulates it (never during normal play after session start).
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMNewGameSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard")
	FText WizardName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard")
	ECoMWizardClass WizardClass = ECoMWizardClass::None;

	/** Portrait asset (soft ref; resolved on first overworld load). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard")
	FSoftObjectPath WizardPortraitPath;

	/** Chosen retort DataAsset row names. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard")
	TArray<FName> ChosenRetorts;

	/** Starting spell row names selected in the wizard creation flow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard")
	TArray<FName> StartingSpells;

	/**
	 * Difficulty scalar applied to AI mana/production/research rates.
	 * 0 = easiest, 4 = hardest; 2 = Normal (default).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game",
	          meta = (ClampMin = 0, ClampMax = 4))
	int32 DifficultyLevel = 1; // 0=Easy, 1=Normal, 2=Hard, 3=Lunatic, 4=Impossible

	bool IsValid() const
	{
		return !WizardName.IsEmpty() && WizardClass != ECoMWizardClass::None;
	}

	void Reset() { *this = FCoMNewGameSettings{}; }
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * UCoMGameInstance
 *
 * Persists across all level transitions for a single play session.
 *
 * Holds three ephemeral contexts (populated before level transitions, consumed
 * on arrival, then cleared) plus the loaded save-slot name for the duration of
 * the session:
 *  - CombatContext      — which armies are fighting, entry map
 *  - ExplorationContext — which dungeon, army, entry position
 *  - NewGameSettings    — wizard creation choices (consumed on first overworld)
 *  - LoadedSaveSlotName — empty = new game; non-empty = loaded save
 */
UCLASS()
class COMCORE_API UCoMGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UCoMGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	// ─── Combat context ───────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadWrite, Category = "Context")
	FCoMCombatContext CombatContext;

	/** Call after the combat level reads and applies CombatContext. */
	UFUNCTION(BlueprintCallable, Category = "Context")
	void ClearCombatContext() { CombatContext.Reset(); }

	// ─── Exploration context ──────────────────────────────────────────────────

	UPROPERTY(BlueprintReadWrite, Category = "Context")
	FCoMExplorationContext ExplorationContext;

	/** Call after the exploration level reads and applies ExplorationContext. */
	UFUNCTION(BlueprintCallable, Category = "Context")
	void ClearExplorationContext() { ExplorationContext.Reset(); }

	// ─── New-game settings ────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadWrite, Category = "NewGame")
	FCoMNewGameSettings NewGameSettings;

	/**
	 * Returns a copy of NewGameSettings then resets the stored value.
	 * ACoMOverworldGameMode calls this once on the first overworld BeginPlay.
	 */
	UFUNCTION(BlueprintCallable, Category = "NewGame")
	FCoMNewGameSettings ConsumeNewGameSettings();

	// ─── Save slot ────────────────────────────────────────────────────────────

	/**
	 * Name of the save slot loaded at session start.
	 * Empty when this is a fresh new game.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString LoadedSaveSlotName;

	/** Returns true if the session was started by loading an existing save. */
	UFUNCTION(BlueprintPure, Category = "Save")
	bool IsLoadedGame() const { return !LoadedSaveSlotName.IsEmpty(); }

	// ─── UI delegates ─────────────────────────────────────────────────────────

	/** Fired when a game mode requests the main menu be shown (e.g. title screen). */
	DECLARE_MULTICAST_DELEGATE(FOnMainMenuRequested);
	FOnMainMenuRequested OnMainMenuRequested;

	/** Fired when a game mode requests the HUD be shown (game started). */
	DECLARE_MULTICAST_DELEGATE(FOnHUDRequested);
	FOnHUDRequested OnHUDRequested;

	/** Fired when a game mode requests all menus be hidden (transitioning to gameplay). */
	DECLARE_MULTICAST_DELEGATE(FOnHideMenusRequested);
	FOnHideMenusRequested OnHideMenusRequested;

	/** Fired when a new game is starting (after world gen, before turn loop). */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnNewGameStarting, int32 /*NumWizards*/);
	FOnNewGameStarting OnNewGameStarting;

	/** Fired when a city is selected (e.g. via overworld click). UI subsystem binds to this. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCityScreenRequested, int32 /*CityId*/);
	FOnCityScreenRequested OnCityScreenRequested;

	/** Fired when an army is selected (e.g. via overworld click). UI subsystem binds to this. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnArmyStackRequested, int32 /*ArmyId*/);
	FOnArmyStackRequested OnArmyStackRequested;
};
