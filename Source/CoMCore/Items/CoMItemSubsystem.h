// Copyright Shattered Arcana. All Rights Reserved.
// CoMItemSubsystem.h -- Owns the per-wizard magical-item vault and the
// hero equipment graph. Knows which powers can be forged, what they cost,
// and exposes a single Forge() entry point used by Enchant Item /
// Create Artifact spell handlers.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMItemTypes.h"
#include "CoMItemSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemForged,    int32, InstanceID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquipped, int32, HeroUnitID, int32, InstanceID);

/**
 * Manages the player's magical-item economy: forge → vault → equip.
 *
 * Storage model:
 *   AllItems        : map of every instance the player has forged or found.
 *   HeroEquipment   : per-hero map<slot, instanceID>.
 *
 * Forging:
 *   GetPowerCatalog() returns the seedable list of available powers.
 *   ComputeForgeCost(...) sums the mana cost given a chosen power list.
 *   ForgeItem(...) creates the instance, drops it in the wizard's vault,
 *   and returns its InstanceID. Mana spending is the caller's job.
 */
UCLASS()
class COMCORE_API UCoMItemSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -- Power catalog --------------------------------------------------------

	/** All powers a wizard may forge onto an item. Static for now; could be data-driven later. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Items")
	const TArray<FCoMItemPower>& GetPowerCatalog() const { return PowerCatalog; }

	/** Sum the mana cost of forging an item with the given power list. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Items")
	int32 ComputeForgeCost(const TArray<FCoMItemPower>& Powers, bool bArtifact) const;

	/** Are these powers compatible with the chosen slot? (No GrantSkill on rings, etc.) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Items")
	bool ArePowersValidForSlot(ECoMItemSlot Slot, const TArray<FCoMItemPower>& Powers) const;

	// -- Forging --------------------------------------------------------------

	/**
	 * Create a new item instance with the given powers and put it in the
	 * wizard's vault. Returns the new InstanceID, or 0 if Powers were
	 * empty or invalid for the slot.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	int32 ForgeItem(int32 OwnerWizardIndex,
	                ECoMItemSlot Slot,
	                FName TemplateID,
	                const FText& DisplayName,
	                const TArray<FCoMItemPower>& Powers,
	                bool bArtifact);

	/** Extended forge entry that lets the caller pick the art variant and
	 *  the dominant realm, and start a multi-turn forging job rather than
	 *  instant-creating the item. Returns the new InstanceID. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	int32 BeginForgeItem(int32 OwnerWizardIndex,
	                     ECoMItemSlot Slot,
	                     FName TemplateID,
	                     const FText& DisplayName,
	                     const TArray<FCoMItemPower>& Powers,
	                     bool bArtifact,
	                     FName ArtVariant,
	                     int32 WizardCastingSkill,
	                     int32 MaxEnchantments);

	/** Sum (Powers[i].ManaCost) + artifact surcharge. Mirrors ComputeForgeCost. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Items")
	int32 ComputeTotalManaCost(const TArray<FCoMItemPower>& Powers, bool bArtifact) const
	{ return ComputeForgeCost(Powers, bArtifact); }

	/** Turns required to fully forge a job that costs ManaCost mana for a
	 *  wizard with the given CastingSkill (mana/turn). Floor of 1 turn. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Items")
	int32 ComputeForgeTime(int32 ManaCost, int32 WizardCastingSkill) const;

	/** Determine the dominant realm of a power list (the realm of the
	 *  highest-mana-cost power, with ties broken by first-seen). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Items")
	ECoMSpellRealm ComputeDominantRealm(const TArray<FCoMItemPower>& Powers) const;

	/** Tick forging jobs for a given wizard: decrement each in-progress
	 *  item's ForgeTurnsRemaining. Fires OnItemForged when one finishes. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	void ProcessForgeTurn(int32 WizardIndex);

	/** Destroy an item and refund 50% of its TotalManaCost to the owner.
	 *  Returns the mana refunded (0 on failure). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	int32 DestroyItemForMana(int32 InstanceID);

	/** Player rename: update the DisplayName on an existing instance. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	bool RenameItem(int32 InstanceID, const FText& NewName);

	// -- Vault / lookup -------------------------------------------------------

	/** All items owned by a wizard (equipped or in vault). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Items")
	TArray<FCoMItemInstance> GetItemsForWizard(int32 WizardIndex) const;

	/** Items in the vault (not currently equipped) for a wizard. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Items")
	TArray<FCoMItemInstance> GetVaultItems(int32 WizardIndex) const;

	/** Lookup a single item by ID. Returns true on hit. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	bool GetItem(int32 InstanceID, FCoMItemInstance& OutItem) const;

	// -- Equipment ------------------------------------------------------------

	/** All items currently equipped on a hero, keyed by slot. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Items")
	TArray<FCoMItemInstance> GetHeroEquipment(int32 HeroUnitID) const;

	/** What's in a single equipment slot? Returns false if empty. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	bool GetHeroEquippedAt(int32 HeroUnitID, ECoMItemSlot Slot, FCoMItemInstance& OutItem) const;

	/**
	 * Equip an item on a hero. Auto-unequips whatever was already in that slot.
	 * Returns false if InstanceID is invalid or the item's slot doesn't match.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	bool EquipItem(int32 HeroUnitID, int32 InstanceID);

	/** Remove an item from a hero. Item returns to the owning wizard's vault. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	bool UnequipItem(int32 HeroUnitID, ECoMItemSlot Slot);

	// -- Save / Load ---------------------------------------------------------

	void ExportAll(TArray<FCoMItemInstance>& OutItems) const;
	void ImportAll(const TArray<FCoMItemInstance>& InItems);

	// -- Delegates -----------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Items") FOnItemForged   OnItemForged;
	UPROPERTY(BlueprintAssignable, Category = "CoM|Items") FOnItemEquipped OnItemEquipped;

private:
	/** Build the static power catalog. Called from Initialize(). */
	void SeedPowerCatalog();

	/** All forged items, keyed by InstanceID. */
	UPROPERTY() TMap<int32, FCoMItemInstance> AllItems;

	/** Hero equipment graph: (HeroUnitID << 8) | Slot -> InstanceID. */
	TMap<int64, int32> HeroEquipment;

	/** Allocates next instance ID. */
	int32 NextInstanceID = 1;

	/** Static power catalog. */
	UPROPERTY() TArray<FCoMItemPower> PowerCatalog;

	static int64 EquipKey(int32 HeroID, ECoMItemSlot Slot)
	{
		return (static_cast<int64>(HeroID) << 8) | static_cast<int64>(Slot);
	}
};
