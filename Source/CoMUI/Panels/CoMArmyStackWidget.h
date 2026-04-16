// Copyright Mythforge Studios. All Rights Reserved.
// CoMArmyStackWidget.h -- Army stack grid showing clickable unit portraits.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMArmyStackWidget.generated.h"

class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UButton;
class USizeBox;
class UUniformGridPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitSelected, int32, UnitId);

/**
 * UCoMArmyStackWidget
 *
 * Displays all units in an army stack as a 3x3 grid of clickable portrait slots.
 * Clicking a slot fires FOnUnitSelected and can open the Unit Card widget.
 * Built entirely in C++ via RebuildWidget().
 */
UCLASS()
class COMUI_API UCoMArmyStackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Load and display data for the specified army. */
	UFUNCTION(BlueprintCallable, Category = "CoM|ArmyStack")
	void SetArmy(int32 ArmyId);

	/** Fired when a unit slot is clicked. */
	UPROPERTY(BlueprintAssignable, Category = "CoM|ArmyStack")
	FOnUnitSelected OnUnitSelected;

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	// -- Layout construction ------------------------------------------------

	void BuildArmyLayout();

	/** Helper: create a styled button matching dark fantasy theme. */
	UButton* CreateStyledButton(const FString& Label, UVerticalBox* Parent);

	// -- Slot click handlers ------------------------------------------------

	UFUNCTION() void OnSlot0Clicked();
	UFUNCTION() void OnSlot1Clicked();
	UFUNCTION() void OnSlot2Clicked();
	UFUNCTION() void OnSlot3Clicked();
	UFUNCTION() void OnSlot4Clicked();
	UFUNCTION() void OnSlot5Clicked();
	UFUNCTION() void OnSlot6Clicked();
	UFUNCTION() void OnSlot7Clicked();
	UFUNCTION() void OnSlot8Clicked();

	void HandleSlotClicked(int32 SlotIndex);

	UFUNCTION()
	void OnCloseClicked();

	// -- Widget references (created in C++) --------------------------------

	UPROPERTY() TObjectPtr<UBorder> OuterBorder;
	UPROPERTY() TObjectPtr<UVerticalBox> ContentBox;
	UPROPERTY() TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY() TObjectPtr<UTextBlock> StrengthText;
	UPROPERTY() TObjectPtr<UTextBlock> MovementText;
	UPROPERTY() TObjectPtr<UButton> CloseButton;

	// 3x3 grid of unit slots (buttons + labels)
	static constexpr int32 MAX_SLOTS = 9;
	UPROPERTY() TObjectPtr<UButton> SlotButtons[MAX_SLOTS];
	UPROPERTY() TObjectPtr<UBorder> SlotPortraits[MAX_SLOTS];
	UPROPERTY() TObjectPtr<UTextBlock> SlotLabels[MAX_SLOTS];

	/** UnitID stored per slot for click callbacks. */
	int32 SlotUnitIds[MAX_SLOTS] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };

	int32 CurrentArmyId = -1;

	TWeakObjectPtr<UCoMUnitSubsystem> CachedUnitSubsystem;
	UCoMUnitSubsystem* GetUnitSubsystem();
};
