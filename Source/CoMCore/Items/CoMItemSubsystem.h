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
