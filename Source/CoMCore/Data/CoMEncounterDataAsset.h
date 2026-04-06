// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMEncounterDataAsset.h — Template for random overworld encounter events.
// Triggered when armies explore tiles, enter specific terrain, or discover sites.
// COM-025: DataAsset types — Encounter

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMEncounterDataAsset.generated.h"

// Forward declarations
class UCoMUnitSpecDataAsset;
class UCoMSpellDataAsset;
class UCoMItemTemplateDataAsset;

// ─────────────────────────────────────────────────────────────────────────────

/** What triggers this encounter to fire. */
UENUM(BlueprintType)
enum class ECoMEncounterTrigger : uint8
{
	/** Army enters a specific terrain type. */
	TerrainEntry       UMETA(DisplayName = "Terrain Entry"),
	/** Army investigates an explorable site. */
	SiteExploration    UMETA(DisplayName = "Site Exploration"),
	/** Army moves through a ley-line intersection. */
	LeyLineIntersection UMETA(DisplayName = "Ley Line Intersection"),
	/** Triggered randomly each turn with a given probability. */
	RandomRoaming      UMETA(DisplayName = "Random Roaming"),
	/** Triggered when a wizard casts a specific type of spell. */
	SpellCast          UMETA(DisplayName = "Spell Cast"),
	/** Army crosses a plane border via a portal. */
	PortalTransit      UMETA(DisplayName = "Portal Transit"),

	MAX UMETA(Hidden)
};

/** The category of encounter — determines how the runtime resolves it. */
UENUM(BlueprintType)
enum class ECoMEncounterCategory : uint8
{
	/** Guards that must be defeated or evaded. */
	CombatAmbush     UMETA(DisplayName = "Combat Ambush"),
	/** Treasure or resources found without combat. */
	TreasureFind     UMETA(DisplayName = "Treasure Find"),
	/** A roaming warband opens dialogue / offers a deal. */
	WarbandParley    UMETA(DisplayName = "Warband Parley"),
	/** A strange magical event with narrative choices. */
	MagicEvent       UMETA(DisplayName = "Magic Event"),
	/** Hero or unit gains XP / a new skill from the encounter. */
	LearningMoment   UMETA(DisplayName = "Learning Moment"),
	/** Curse, plague, or negative effect placed on army/wizard. */
	Curse            UMETA(DisplayName = "Curse"),

	MAX UMETA(Hidden)
};

/** One possible reward entry from this encounter. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMEncounterReward
{
	GENERATED_BODY()

	/** Type of reward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMRewardType RewardType = ECoMRewardType::Gold;

	/** Minimum value (gold amount, mana, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ValueMin = 0;

	/** Maximum value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ValueMax = 0;

	/** Specific spell rewarded (if RewardType == Spell). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UCoMSpellDataAsset> SpellReward;

	/** Specific item rewarded (if RewardType == Item). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UCoMItemTemplateDataAsset> ItemReward;

	/** Probability weight relative to sibling rewards (higher = more likely). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FFixed64 Weight = FFixed64(1);
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Data asset defining a random overworld encounter template.
 * The runtime picks from eligible encounters via weighted random selection
 * based on trigger, terrain, plane, and turn/season constraints.
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMEncounterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMEncounterDataAsset();

public:
	// ── Identity ──────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FName EncounterID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FText NarrativeText;

	// ── Classification ────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Classification")
	ECoMEncounterTrigger Trigger = ECoMEncounterTrigger::RandomRoaming;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Classification")
	ECoMEncounterCategory Category = ECoMEncounterCategory::TreasureFind;

	/** Tags that further classify this encounter (e.g., "Underdark.Spider", "Magic.Chaos"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Classification")
	FGameplayTagContainer EncounterTags;

	// ── Trigger Conditions ────────────────────────────────────────────────

	/** Terrain types that can trigger this encounter (empty = any terrain). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Conditions")
	TArray<ECoMTerrain> ValidTerrains;

	/** Planes this encounter can appear on (empty = all planes). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Conditions")
	TArray<ECoMPlane> ValidPlanes;

	/** Map layers this encounter can appear on (empty = all layers). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Conditions")
	TArray<ECoMMapLayer> ValidLayers;

	/** Minimum turn number for this encounter to be eligible (0 = always). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Conditions")
	int32 MinTurn = 0;

	/** Base chance per eligible tile-enter event (0.0–1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Conditions",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FFixed64 TriggerProbability = FFixed64(0.05f);

	/** If true, only heroes can trigger this encounter (not regular armies). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Conditions")
	bool bHeroOnly = false;

	/** Maximum times this encounter can fire per game (-1 = unlimited). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Conditions")
	int32 MaxOccurrences = -1;

	// ── Combat (if CombatAmbush) ──────────────────────────────────────────

	/** Unit types that make up the ambushing force. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Combat")
	TArray<TSoftObjectPtr<UCoMUnitSpecDataAsset>> AmbushUnitTypes;

	/** Min number of ambush units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Combat")
	int32 AmbushCountMin = 0;

	/** Max number of ambush units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Combat")
	int32 AmbushCountMax = 0;

	/** If true, player can attempt to evade (Scouting roll). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Combat")
	bool bCanEvade = true;

	// ── Rewards ──────────────────────────────────────────────────────────

	/** Weighted pool of possible rewards (one is selected randomly). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Rewards")
	TArray<FCoMEncounterReward> RewardPool;

	/** If true, reward only granted after winning combat (if CombatAmbush). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Rewards")
	bool bRewardRequiresCombatVictory = true;

	// ── Stat Modifiers (curses / buffs) ──────────────────────────────────

	/** Modifiers applied to the player's army from this encounter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Effects")
	TArray<FCoMStatModifier> ArmyModifiers;

	/** Duration in turns for temporary modifiers (0 = permanent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Effects")
	int32 ModifierDurationTurns = 0;

	// ── Presentation ──────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Presentation")
	TSoftObjectPtr<UTexture2D> EncounterArtwork;

	/** Ambient sound played during the encounter event (soft path). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Presentation")
	FSoftObjectPath EncounterSoundPath;

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("CoMEncounter"), GetFName());
	}
};
