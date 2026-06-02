// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMItemSubsystem.h"

#define LOCTEXT_NAMESPACE "CoMItems"

// =====================================================================
// Lifecycle
// =====================================================================

void UCoMItemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SeedPowerCatalog();
}

void UCoMItemSubsystem::Deinitialize()
{
	AllItems.Empty();
	HeroEquipment.Empty();
	PowerCatalog.Empty();
	NextInstanceID = 1;
	Super::Deinitialize();
}

// =====================================================================
// Power catalog
// =====================================================================
//
// Each entry corresponds to a Master-of-Magic-style item enchantment.
// ManaCost values are tuned so a typical 11-pick wizard can afford a
// modest item (≈80 mana) by mid game and an artifact (≈300 mana) late.

namespace
{
	FCoMItemPower MakeStat(const TCHAR* ID, const TCHAR* Stat, int32 Mag, int32 Cost, FText Display)
	{
		FCoMItemPower P;
		P.PowerID     = FName(ID);
		P.Type        = ECoMItemPowerType::StatBonus;
		P.Key         = FName(Stat);
		P.Magnitude   = Mag;
		P.ManaCost    = Cost;
		P.DisplayName = Display;
		return P;
	}

	FCoMItemPower MakeSkill(const TCHAR* ID, const TCHAR* SkillKey, int32 Cost, FText Display)
	{
		FCoMItemPower P;
		P.PowerID     = FName(ID);
		P.Type        = ECoMItemPowerType::GrantSkill;
		P.Key         = FName(SkillKey);
		P.ManaCost    = Cost;
		P.DisplayName = Display;
		return P;
	}

	FCoMItemPower MakeCharges(const TCHAR* ID, const TCHAR* SpellKey, int32 Charges, int32 Cost, FText Display)
	{
		FCoMItemPower P;
		P.PowerID     = FName(ID);
		P.Type        = ECoMItemPowerType::SpellCharge;
		P.Key         = FName(SpellKey);
		P.Magnitude   = Charges;
		P.ManaCost    = Cost;
		P.DisplayName = Display;
		return P;
	}
}

void UCoMItemSubsystem::SeedPowerCatalog()
{
	PowerCatalog.Reset();

	// Stat bonuses ----------------------------------------------------
	PowerCatalog.Add(MakeStat(TEXT("attack_plus_1"),  TEXT("Attack"),     1, 25,  LOCTEXT("PowAtk1", "+1 Attack")));
	PowerCatalog.Add(MakeStat(TEXT("attack_plus_2"),  TEXT("Attack"),     2, 60,  LOCTEXT("PowAtk2", "+2 Attack")));
	PowerCatalog.Add(MakeStat(TEXT("attack_plus_3"),  TEXT("Attack"),     3, 120, LOCTEXT("PowAtk3", "+3 Attack")));
	PowerCatalog.Add(MakeStat(TEXT("defense_plus_1"), TEXT("Defense"),    1, 25,  LOCTEXT("PowDef1", "+1 Defense")));
	PowerCatalog.Add(MakeStat(TEXT("defense_plus_2"), TEXT("Defense"),    2, 60,  LOCTEXT("PowDef2", "+2 Defense")));
	PowerCatalog.Add(MakeStat(TEXT("defense_plus_3"), TEXT("Defense"),    3, 120, LOCTEXT("PowDef3", "+3 Defense")));
	PowerCatalog.Add(MakeStat(TEXT("resist_plus_1"),  TEXT("Resistance"), 1, 25,  LOCTEXT("PowRes1", "+1 Resistance")));
	PowerCatalog.Add(MakeStat(TEXT("resist_plus_2"),  TEXT("Resistance"), 2, 60,  LOCTEXT("PowRes2", "+2 Resistance")));
	PowerCatalog.Add(MakeStat(TEXT("resist_plus_3"),  TEXT("Resistance"), 3, 120, LOCTEXT("PowRes3", "+3 Resistance")));
	PowerCatalog.Add(MakeStat(TEXT("hp_plus_2"),      TEXT("HP"),         2, 30,  LOCTEXT("PowHP2",  "+2 Hit Points")));
	PowerCatalog.Add(MakeStat(TEXT("hp_plus_4"),      TEXT("HP"),         4, 70,  LOCTEXT("PowHP4",  "+4 Hit Points")));
	PowerCatalog.Add(MakeStat(TEXT("mana_plus_2"),    TEXT("Mana"),       2, 35,  LOCTEXT("PowMP2",  "+2 Spell Points")));
	PowerCatalog.Add(MakeStat(TEXT("move_plus_1"),    TEXT("Movement"),   1, 50,  LOCTEXT("PowMov1", "+1 Movement")));
	PowerCatalog.Add(MakeStat(TEXT("save_minus_1"),   TEXT("SpellSave"), -1, 75,  LOCTEXT("PowSav1", "-1 Spell Save")));
	PowerCatalog.Add(MakeStat(TEXT("save_minus_2"),   TEXT("SpellSave"), -2, 175, LOCTEXT("PowSav2", "-2 Spell Save")));

	// Granted combat skills ------------------------------------------
	PowerCatalog.Add(MakeSkill(TEXT("flame_blade"),       TEXT("flame_blade"),       80,  LOCTEXT("PowFlame",  "Flame Blade")));
	PowerCatalog.Add(MakeSkill(TEXT("frost_blade"),       TEXT("frost_blade"),       80,  LOCTEXT("PowFrost",  "Frost Blade")));
	PowerCatalog.Add(MakeSkill(TEXT("lightning_blade"),   TEXT("lightning_blade"),   90,  LOCTEXT("PowLight",  "Lightning Blade")));
	PowerCatalog.Add(MakeSkill(TEXT("vampiric"),          TEXT("vampiric"),          120, LOCTEXT("PowVamp",   "Vampiric")));
	PowerCatalog.Add(MakeSkill(TEXT("holy_avenger"),      TEXT("holy_avenger"),      150, LOCTEXT("PowHoly",   "Holy Avenger")));
	PowerCatalog.Add(MakeSkill(TEXT("stoning_touch"),     TEXT("stoning_touch"),     130, LOCTEXT("PowStone",  "Stoning Touch")));
	PowerCatalog.Add(MakeSkill(TEXT("destruction"),       TEXT("destruction"),       180, LOCTEXT("PowDestr",  "Destruction")));
	PowerCatalog.Add(MakeSkill(TEXT("flight"),            TEXT("flight"),            100, LOCTEXT("PowFly",    "Flight")));
	PowerCatalog.Add(MakeSkill(TEXT("invisibility"),      TEXT("invisibility"),      130, LOCTEXT("PowInvis",  "Invisibility")));
	PowerCatalog.Add(MakeSkill(TEXT("magic_immunity"),    TEXT("magic_immunity"),    220, LOCTEXT("PowMI",     "Magic Immunity")));
	PowerCatalog.Add(MakeSkill(TEXT("regeneration"),      TEXT("regeneration"),      150, LOCTEXT("PowRegen",  "Regeneration")));
	PowerCatalog.Add(MakeSkill(TEXT("true_sight"),        TEXT("true_sight"),        70,  LOCTEXT("PowTrue",   "True Sight")));
	PowerCatalog.Add(MakeSkill(TEXT("bless"),             TEXT("bless"),             40,  LOCTEXT("PowBless",  "Bless")));
	PowerCatalog.Add(MakeSkill(TEXT("haste"),             TEXT("haste"),             110, LOCTEXT("PowHaste",  "Haste")));

	// Spell charges --------------------------------------------------
	PowerCatalog.Add(MakeCharges(TEXT("charges_fireball"),    TEXT("fireball"),     4, 60,  LOCTEXT("PowChFire", "4× Fireball Charges")));
	PowerCatalog.Add(MakeCharges(TEXT("charges_heal"),        TEXT("healing"),      4, 50,  LOCTEXT("PowChHeal", "4× Healing Charges")));
	PowerCatalog.Add(MakeCharges(TEXT("charges_dispel"),      TEXT("dispel_magic"), 3, 45,  LOCTEXT("PowChDisp", "3× Dispel Charges")));
	PowerCatalog.Add(MakeCharges(TEXT("charges_teleport"),    TEXT("teleport"),     2, 90,  LOCTEXT("PowChTele", "2× Teleport Charges")));
}

// =====================================================================
// Cost / validation
// =====================================================================

int32 UCoMItemSubsystem::ComputeForgeCost(const TArray<FCoMItemPower>& Powers, bool bArtifact) const
{
	int32 Sum = 0;
	for (const FCoMItemPower& P : Powers) { Sum += P.ManaCost; }
	// Artifact tier = base 50 mana + 25% surcharge on power total.
	if (bArtifact)
	{
		Sum = 50 + static_cast<int32>(Sum * 1.25f);
	}
	return Sum;
}

int32 UCoMItemSubsystem::ComputeForgeTime(int32 ManaCost, int32 WizardCastingSkill) const
{
	// Floor of 1 turn; otherwise (cost / skill) rounded up. A wizard with
	// Casting Skill 25 forging a 200-mana item takes 8 turns.
	const int32 Skill = FMath::Max(1, WizardCastingSkill);
	return FMath::Max(1, FMath::DivideAndRoundUp(ManaCost, Skill));
}

ECoMSpellRealm UCoMItemSubsystem::ComputeDominantRealm(const TArray<FCoMItemPower>& Powers) const
{
	ECoMSpellRealm Best = ECoMSpellRealm::Arcane;
	int32 BestCost = -1;
	for (const FCoMItemPower& P : Powers)
	{
		if (P.ManaCost > BestCost)
		{
			BestCost = P.ManaCost;
			Best = P.Realm;
		}
	}
	return Best;
}

void UCoMItemSubsystem::ProcessForgeTurn(int32 WizardIndex)
{
	for (auto& Pair : AllItems)
	{
		FCoMItemInstance& I = Pair.Value;
		if (I.OwnerWizardIndex != WizardIndex) continue;
		if (I.ForgeTurnsRemaining <= 0)         continue;
		I.ForgeTurnsRemaining--;
		if (I.ForgeTurnsRemaining <= 0)
		{
			// Just finished -- fire the existing forged delegate so the
			// HUD toast + stats subsystem both pick it up.
			OnItemForged.Broadcast(I.InstanceID);
		}
	}
}

int32 UCoMItemSubsystem::DestroyItemForMana(int32 InstanceID)
{
	FCoMItemInstance* I = AllItems.Find(InstanceID);
	if (!I) return 0;
	const int32 Refund = FMath::Max(0, I->TotalManaCost / 2);
	// Unequip first so the hero's slot map doesn't keep a dangling ref.
	if (I->EquippedByHeroID != 0)
	{
		UnequipItem(I->EquippedByHeroID, I->Slot);
	}
	AllItems.Remove(InstanceID);
	UE_LOG(LogTemp, Log, TEXT("[Items] Destroyed instance %d, refunded %d mana to wizard %d"),
		InstanceID, Refund, I->OwnerWizardIndex);
	return Refund;
}

bool UCoMItemSubsystem::RenameItem(int32 InstanceID, const FText& NewName)
{
	FCoMItemInstance* I = AllItems.Find(InstanceID);
	if (!I || NewName.IsEmpty()) return false;
	I->DisplayName = NewName;
	return true;
}

int32 UCoMItemSubsystem::BeginForgeItem(int32 OwnerWizardIndex,
                                        ECoMItemSlot Slot,
                                        FName TemplateID,
                                        const FText& DisplayName,
                                        const TArray<FCoMItemPower>& Powers,
                                        bool bArtifact,
                                        FName ArtVariant,
                                        int32 WizardCastingSkill,
                                        int32 MaxEnchantments)
{
	if (!ArePowersValidForSlot(Slot, Powers)) return 0;
	const int32 Cap = FMath::Clamp(MaxEnchantments, 1, 8);
	if (Powers.Num() > Cap) return 0;

	FCoMItemInstance Item;
	Item.InstanceID          = NextInstanceID++;
	Item.TemplateID          = TemplateID;
	Item.DisplayName         = DisplayName.IsEmpty()
		? FText::Format(LOCTEXT("ForgedItemDefault", "Forged Item #{0}"), FText::AsNumber(Item.InstanceID))
		: DisplayName;
	Item.Slot                = Slot;
	Item.Powers              = Powers;
	Item.TotalManaCost       = ComputeForgeCost(Powers, bArtifact);
	Item.bArtifact           = bArtifact;
	Item.OwnerWizardIndex    = OwnerWizardIndex;
	Item.EquippedByHeroID    = 0;
	Item.ArtVariant          = ArtVariant;
	Item.DominantRealm       = ComputeDominantRealm(Powers);
	Item.MaxEnchantments     = Cap;
	Item.ForgeTurnsTotal     = ComputeForgeTime(Item.TotalManaCost, WizardCastingSkill);
	Item.ForgeTurnsRemaining = Item.ForgeTurnsTotal;

	AllItems.Add(Item.InstanceID, Item);
	// Don't fire OnItemForged yet -- it'll fire when ForgeTurnsRemaining
	// reaches 0 during ProcessForgeTurn. This separates "queued" from "ready"
	// so toasts and stats only fire on completion.
	return Item.InstanceID;
}

bool UCoMItemSubsystem::ArePowersValidForSlot(ECoMItemSlot Slot, const TArray<FCoMItemPower>& Powers) const
{
	if (Powers.Num() == 0) return false;

	// Weapons may carry attack-affecting blade skills; armor may not.
	const bool bIsWeapon  = (Slot == ECoMItemSlot::Weapon);
	const bool bIsArmor   = (Slot == ECoMItemSlot::Armor || Slot == ECoMItemSlot::Helm || Slot == ECoMItemSlot::Boots);
	const bool bIsTrinket = (Slot == ECoMItemSlot::Ring || Slot == ECoMItemSlot::Amulet || Slot == ECoMItemSlot::Relic);

	for (const FCoMItemPower& P : Powers)
	{
		const FString IDStr = P.PowerID.ToString();
		const bool bBladeSkill =
			IDStr == TEXT("flame_blade") ||
			IDStr == TEXT("frost_blade") ||
			IDStr == TEXT("lightning_blade") ||
			IDStr == TEXT("vampiric") ||
			IDStr == TEXT("holy_avenger") ||
			IDStr == TEXT("stoning_touch") ||
			IDStr == TEXT("destruction");

		if (bBladeSkill && !bIsWeapon)        return false;
		if (P.Key == TEXT("Defense") && !bIsArmor && !bIsTrinket && !bIsWeapon) return false;
	}
	return true;
}

// =====================================================================
// Forging
// =====================================================================

int32 UCoMItemSubsystem::ForgeItem(int32 OwnerWizardIndex,
                                   ECoMItemSlot Slot,
                                   FName TemplateID,
                                   const FText& DisplayName,
                                   const TArray<FCoMItemPower>& Powers,
                                   bool bArtifact)
{
	if (!ArePowersValidForSlot(Slot, Powers)) return 0;

	FCoMItemInstance Item;
	Item.InstanceID       = NextInstanceID++;
	Item.TemplateID       = TemplateID;
	Item.DisplayName      = DisplayName.IsEmpty()
		? FText::Format(LOCTEXT("ForgedItemDefault", "Forged Item #{0}"), FText::AsNumber(Item.InstanceID))
		: DisplayName;
	Item.Slot             = Slot;
	Item.Powers           = Powers;
	Item.TotalManaCost    = ComputeForgeCost(Powers, bArtifact);
	Item.bArtifact        = bArtifact;
	Item.OwnerWizardIndex = OwnerWizardIndex;
	Item.EquippedByHeroID = 0;

	AllItems.Add(Item.InstanceID, Item);
	OnItemForged.Broadcast(Item.InstanceID);
	return Item.InstanceID;
}

// =====================================================================
// Vault / lookup
// =====================================================================

TArray<FCoMItemInstance> UCoMItemSubsystem::GetItemsForWizard(int32 WizardIndex) const
{
	TArray<FCoMItemInstance> Out;
	for (const auto& Pair : AllItems)
	{
		if (Pair.Value.OwnerWizardIndex == WizardIndex) { Out.Add(Pair.Value); }
	}
	return Out;
}

TArray<FCoMItemInstance> UCoMItemSubsystem::GetVaultItems(int32 WizardIndex) const
{
	TArray<FCoMItemInstance> Out;
	for (const auto& Pair : AllItems)
	{
		if (Pair.Value.OwnerWizardIndex == WizardIndex && Pair.Value.EquippedByHeroID == 0)
		{
			Out.Add(Pair.Value);
		}
	}
	return Out;
}

bool UCoMItemSubsystem::GetItem(int32 InstanceID, FCoMItemInstance& OutItem) const
{
	if (const FCoMItemInstance* Found = AllItems.Find(InstanceID))
	{
		OutItem = *Found;
		return true;
	}
	return false;
}

// =====================================================================
// Equipment
// =====================================================================

TArray<FCoMItemInstance> UCoMItemSubsystem::GetHeroEquipment(int32 HeroUnitID) const
{
	TArray<FCoMItemInstance> Out;
	for (const auto& Pair : HeroEquipment)
	{
		if ((Pair.Key >> 8) != HeroUnitID) continue;
		if (const FCoMItemInstance* Item = AllItems.Find(Pair.Value)) { Out.Add(*Item); }
	}
	return Out;
}

bool UCoMItemSubsystem::GetHeroEquippedAt(int32 HeroUnitID, ECoMItemSlot Slot, FCoMItemInstance& OutItem) const
{
	const int32* InstanceID = HeroEquipment.Find(EquipKey(HeroUnitID, Slot));
	if (!InstanceID) return false;
	return GetItem(*InstanceID, OutItem);
}

bool UCoMItemSubsystem::EquipItem(int32 HeroUnitID, int32 InstanceID)
{
	FCoMItemInstance* Item = AllItems.Find(InstanceID);
	if (!Item || HeroUnitID == 0) return false;

	const ECoMItemSlot Slot = Item->Slot;
	if (Slot == ECoMItemSlot::None || Slot == ECoMItemSlot::MAX) return false;

	// Auto-unequip whatever was already in this slot.
	UnequipItem(HeroUnitID, Slot);

	// Auto-unequip this item from any other hero (move).
	if (Item->EquippedByHeroID != 0)
	{
		HeroEquipment.Remove(EquipKey(Item->EquippedByHeroID, Item->Slot));
	}

	HeroEquipment.Add(EquipKey(HeroUnitID, Slot), InstanceID);
	Item->EquippedByHeroID = HeroUnitID;
	OnItemEquipped.Broadcast(HeroUnitID, InstanceID);
	return true;
}

bool UCoMItemSubsystem::UnequipItem(int32 HeroUnitID, ECoMItemSlot Slot)
{
	const int64 Key = EquipKey(HeroUnitID, Slot);
	const int32* InstanceID = HeroEquipment.Find(Key);
	if (!InstanceID) return false;

	if (FCoMItemInstance* Item = AllItems.Find(*InstanceID))
	{
		Item->EquippedByHeroID = 0;
	}
	HeroEquipment.Remove(Key);
	return true;
}

// =====================================================================
// Save / Load
// =====================================================================

void UCoMItemSubsystem::ExportAll(TArray<FCoMItemInstance>& OutItems) const
{
	OutItems.Reset();
	for (const auto& Pair : AllItems) { OutItems.Add(Pair.Value); }
}

void UCoMItemSubsystem::ImportAll(const TArray<FCoMItemInstance>& InItems)
{
	AllItems.Empty();
	HeroEquipment.Empty();
	NextInstanceID = 1;
	for (const FCoMItemInstance& Item : InItems)
	{
		AllItems.Add(Item.InstanceID, Item);
		NextInstanceID = FMath::Max(NextInstanceID, Item.InstanceID + 1);
		if (Item.EquippedByHeroID != 0)
		{
			HeroEquipment.Add(EquipKey(Item.EquippedByHeroID, Item.Slot), Item.InstanceID);
		}
	}
}

#undef LOCTEXT_NAMESPACE
