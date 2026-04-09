// Copyright Mythforge Studios. All Rights Reserved.
// CoMLoadScreenWidget.h -- Save slot browser for loading/deleting saved games.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMLoadScreenWidget.generated.h"

class UBorder;
class UVerticalBox;
class UButton;
class UScrollBox;
class UTextBlock;

struct FCoMSaveSlotInfo;

/**
 * UCoMSlotButtonHelper
 *
 * Tiny helper UObject that captures a slot name for UButton::OnClicked binding.
 * Required because UE5's FOnButtonClickedEvent (dynamic multicast delegate)
 * cannot capture variables in lambdas — it needs a UFUNCTION on a UObject.
 */
UCLASS()
class COMUI_API UCoMSlotButtonHelper : public UObject
{
    GENERATED_BODY()

public:
    /** The slot name this helper is bound to. */
    UPROPERTY()
    FString SlotName;

    /** Back-pointer to the load screen (weak to avoid prevent GC). */
    UPROPERTY()
    TWeakObjectPtr<UUserWidget> OwnerWidget;

    /** Whether this is a Load or Delete action. */
    UPROPERTY()
    bool bIsDeleteAction = false;

    UFUNCTION()
    void OnClicked();
};

/**
 * UCoMLoadScreenWidget
 *
 * Full-screen widget listing all save slots. Each slot has Load and Delete buttons.
 * Back button returns to the main menu.
 */
UCLASS()
class COMUI_API UCoMLoadScreenWidget : public UUserWidget
{
    GENERATED_BODY()

    friend class UCoMSlotButtonHelper;

protected:
    virtual void NativeConstruct() override;

private:
    void BuildLayout();
    void RefreshSlotList();
    void CreateSlotRow(const FCoMSaveSlotInfo& Info);
    UButton* CreateActionButton(const FString& Label, const FLinearColor& NormalColor,
                                const FLinearColor& HoverColor);

    void HandleLoadSlot(const FString& SlotName);
    void HandleDeleteSlot(const FString& SlotName);

    UFUNCTION()
    void OnBackClicked();

    UPROPERTY() TObjectPtr<UBorder> BackgroundBorder;
    UPROPERTY() TObjectPtr<UVerticalBox> ContentBox;
    UPROPERTY() TObjectPtr<UScrollBox> SlotScrollBox;
    UPROPERTY() TObjectPtr<UButton> BackButton;

    /** Helpers are stored here to prevent GC. */
    UPROPERTY()
    TArray<TObjectPtr<UCoMSlotButtonHelper>> ButtonHelpers;
};
