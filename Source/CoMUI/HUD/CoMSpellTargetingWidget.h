// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellTargetingWidget.h -- Spell targeting mode UI for overworld casting.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMSpellTargetingWidget.generated.h"

class UTextBlock;
class UBorder;
class UButton;
class UImage;
class UCoMMagicSubsystem;
class UCoMWorldMapSubsystem;
class UCoMUnitSubsystem;
class UCoMCitySubsystem;

/**
 * UCoMSpellTargetingWidget
 *
 * When a player casts a spell from the spell book, this widget handles target
 * selection on the overworld map. Supports multiple targeting modes based on
 * spell data: NoTarget, TileTarget, UnitTarget, CityTarget, ArmyTarget.
 *
 * Visual feedback:
 * - Targeting mode indicator text
 * - Range indicator (spell range from caster position)
 * - "Cancel" hint text
 * - Valid/invalid target feedback
 *
 * Flow:
 * 1. SpellBook "Cast" button -> BeginTargeting(SpellId, CasterWizardId)
 * 2. Widget shows targeting UI
 * 3. Player clicks valid target -> ConfirmTarget / ConfirmUnitTarget / ConfirmCityTarget
 * 4. Calls MagicSubsystem->CastSpell()
 * 5. Clean up targeting UI
 *
 * Cancel with CancelTargeting() (right-click or Escape from input controller).
 */
UCLASS()
class COMUI_API UCoMSpellTargetingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// -- Targeting Lifecycle ---------------------------------------------------

	/**
	 * Enter targeting mode for a spell. Determines target type from spell data
	 * and shows the appropriate targeting UI.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellTargeting")
	void BeginTargeting(FName SpellId, int32 CasterWizardId);

	/**
	 * Cancel targeting mode. Cleans up UI, returns to normal input.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellTargeting")
	void CancelTargeting();

	/**
	 * Confirm a tile target. Validates and casts the spell.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellTargeting")
	void ConfirmTarget(FIntPoint TileTarget);

	/**
	 * Confirm a unit target. Validates and casts the spell.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellTargeting")
	void ConfirmUnitTarget(int32 UnitId);

	/**
	 * Confirm a city target. Validates and casts the spell.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellTargeting")
	void ConfirmCityTarget(int32 CityId);

	// -- State Queries ---------------------------------------------------------

	/** Returns true if currently in targeting mode. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellTargeting")
	bool IsTargeting() const { return bIsTargeting; }

	/** Returns the current target type. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellTargeting")
	ECoMSpellTargetType GetTargetType() const { return CurrentTargetType; }

	/** Returns the active spell being targeted. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellTargeting")
	FName GetActiveSpellId() const { return ActiveSpellId; }

	/** Returns the caster wizard ID. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellTargeting")
	int32 GetCasterWizardId() const { return CasterWizardId; }

	// -- Target Validation -----------------------------------------------------

	/** Check if a tile position is a valid target for the current spell. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellTargeting")
	bool IsValidTarget(FIntPoint TilePos) const;

	/** Check if a unit is a valid target for the current spell. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellTargeting")
	bool IsValidUnitTarget(int32 UnitId) const;

	/** Check if a city is a valid target for the current spell. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellTargeting")
	bool IsValidCityTarget(int32 CityId) const;

	/** Get the range of the active spell (in tiles). */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellTargeting")
	int32 GetSpellRange() const { return SpellRange; }

	// -- Delegate for external listeners ---------------------------------------

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetingStarted);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetingEnded);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetingCancelled);

	UPROPERTY(BlueprintAssignable, Category = "CoM|SpellTargeting")
	FOnTargetingStarted OnTargetingStarted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|SpellTargeting")
	FOnTargetingEnded OnTargetingEnded;

	UPROPERTY(BlueprintAssignable, Category = "CoM|SpellTargeting")
	FOnTargetingCancelled OnTargetingCancelled;

protected:
	// -- Widget References -----------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TargetingOverlayBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpellNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TargetTypeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CancelHintText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RangeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelButton;

private:
	UFUNCTION()
	void OnCancelButtonClicked();

	/** Update the targeting UI text based on current state. */
	void UpdateTargetingDisplay();

	/** Execute the actual spell cast through the magic subsystem. */
	void ExecuteCast(FName SpellId, int32 WizardId, FIntPoint TargetTile, int32 TargetUnitId = -1);

	/** Clean up targeting state and hide UI. */
	void CleanupTargeting();

	/** Determine the target type for a spell. */
	ECoMSpellTargetType DetermineTargetType(FName SpellId) const;

	/** Subsystem accessors. */
	UCoMMagicSubsystem* GetMagicSubsystem();
	UCoMWorldMapSubsystem* GetWorldMapSubsystem();
	UCoMUnitSubsystem* GetUnitSubsystem();
	UCoMCitySubsystem* GetCitySubsystem();

	// -- State -----------------------------------------------------------------

	bool bIsTargeting = false;
	FName ActiveSpellId = NAME_None;
	int32 CasterWizardId = -1;
	ECoMSpellTargetType CurrentTargetType = ECoMSpellTargetType::NoTarget;
	int32 SpellRange = 0;

	/** Cached subsystem pointers. */
	TWeakObjectPtr<UCoMMagicSubsystem> CachedMagicSub;
	TWeakObjectPtr<UCoMWorldMapSubsystem> CachedWorldMapSub;
	TWeakObjectPtr<UCoMUnitSubsystem> CachedUnitSub;
	TWeakObjectPtr<UCoMCitySubsystem> CachedCitySub;

	// -- Color Constants -------------------------------------------------------

	static FLinearColor GoldColor()  { return FLinearColor(0.85f, 0.65f, 0.13f, 1.f); }
	static FLinearColor WhiteColor() { return FLinearColor::White; }
};
