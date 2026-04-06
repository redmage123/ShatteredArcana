// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMArtifactDataAsset.h — Data asset for legendary unique artifacts with multi-step quest chains.
// Mirrors FCoMLegendaryArtifact + FCoMArtifactQuestStep from CoMStructs.h.
// COM-025: DataAsset types — Artifact

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMArtifactDataAsset.generated.h"

// Forward declarations
class UCoMItemTemplateDataAsset;
class UCoMSpellDataAsset;

/**
 * Data asset for a unique legendary artifact — one of 30+ named items with lore,
 * quest unlock chains spanning planes, and dual-phase power (base + unlocked).
 *
 * Relationship to runtime structs:
 *  - This asset defines the TEMPLATE (design-time authored properties).
 *  - FCoMLegendaryArtifact in CoMStructs.h holds runtime state (possession, quest progress).
 *  - The runtime reads this asset to seed the FCoMLegendaryArtifact instance at game start.
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMArtifactDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMArtifactDataAsset();

public:
	// ── Identity ──────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact")
	FName ArtifactID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact")
	FText LoreDescription;

	/** Short tagline shown in UI tooltips. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact")
	FText Tagline;

	// ── Physical Form ─────────────────────────────────────────────────────

	/** The item template this artifact is built upon (sword, staff, ring, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Form")
	TSoftObjectPtr<UCoMItemTemplateDataAsset> BaseItemTemplate;

	// ── Location & Discovery ──────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Discovery")
	ECoMArtifactLocation LocationType = ECoMArtifactLocation::InDungeon;

	/** If FragmentedAcrossPlanes: number of fragments required to forge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Discovery",
	          meta = (EditCondition = "LocationType == ECoMArtifactLocation::FragmentedAcrossPlanes"))
	int32 FragmentCount = 1;

	/** Planes where fragments / the artifact itself can be found. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Discovery")
	TArray<ECoMPlane> AppearOnPlanes;

	// ── Base Power (available immediately on possession) ──────────────────

	/** Stat modifiers active as soon as the artifact is carried. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Power|Base")
	TArray<FCoMStatModifier> BasePowerModifiers;

	/** Skills granted immediately on possession. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Power|Base")
	TArray<FCoMSkillEntry> BaseSkills;

	// ── Unlocked Power (after quest chain completion) ─────────────────────

	/** Additional stat modifiers unlocked after completing the quest chain. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Power|Unlocked")
	TArray<FCoMStatModifier> UnlockedPowerModifiers;

	/** Additional skills unlocked after quest chain. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Power|Unlocked")
	TArray<FCoMSkillEntry> UnlockedSkills;

	/** Spell granted to the possessor's spell book upon unlocking (FName = spell asset name). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Power|Unlocked")
	FName UnlockedSpellGrantID;

	/** Optional direct spell reference for the unlock reward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Power|Unlocked")
	TSoftObjectPtr<UCoMSpellDataAsset> UnlockedSpellGrant;

	// ── Quest Chain ───────────────────────────────────────────────────────

	/** Ordered steps required to unlock the artifact's full power.
	 *  These mirror FCoMArtifactQuestStep but carry design-time authored descriptions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Quest")
	TArray<FCoMArtifactQuestStep> QuestChain;

	// ── Gameplay Tags ─────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Tags")
	FGameplayTagContainer ArtifactTags;

	// ── Presentation ──────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Presentation")
	TSoftObjectPtr<UTexture2D> ArtifactIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Presentation")
	TSoftObjectPtr<UTexture2D> ArtifactFullArtwork;

	/** Soft path to mesh used in the world (resolved by CoMRendering). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Presentation")
	FSoftObjectPath WorldMeshPath;

	/** Soft path to ambient VFX for the artifact (resolved by CoMRendering). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Presentation")
	FSoftObjectPath AmbientVFXPath;

	/** Unique sound sting played when artifact is found (soft path). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Artifact|Presentation")
	FSoftObjectPath DiscoverySoundPath;

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("CoMArtifact"), GetFName());
	}
};
