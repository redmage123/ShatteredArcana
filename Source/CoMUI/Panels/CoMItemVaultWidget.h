// Copyright Mythforge Studios. All Rights Reserved.
// CoMItemVaultWidget.h -- Browse forged items and equip them to heroes.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMItemTypes.h"
#include "CoMItemVaultWidget.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UCoMItemVaultWidget;

/** Per-row helper that owns the click UFunction so each item row can carry its own InstanceID. */
UCLASS()
class COMUI_API UCoMVaultItemRow : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UCoMItemVaultWidget> Owner;
	UPROPERTY() int32 InstanceID = 0;

	UFUNCTION() void HandleClick();
};

/**
 * UCoMItemVaultWidget
 *
 * Lists every forged item the wizard owns, marks which ones are equipped
 * (and on which hero), and lets the player click an item to assign it to
 * the currently-targeted hero (set via SetTargetHero) — or, if no hero is
 * targeted, just preview details.
 */
UCLASS()
class COMUI_API UCoMItemVaultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Show items for this wizard. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items") void Configure(int32 InOwnerWizardIndex);

	/** When set, clicking an item equips it onto this hero. 0 = preview-only. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items") void SetTargetHero(int32 HeroUnitID);

	/** Called by row helper when clicked. */
	void HandleItemClicked(int32 InstanceID);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION() void OnCloseClicked();

private:
	void BuildLayout();
	void RebuildList();

	int32 OwnerWizardIndex = -1;
	int32 TargetHeroUnitID = 0;

	UPROPERTY() TObjectPtr<UBorder>      BackgroundBorder;
	UPROPERTY() TObjectPtr<UTextBlock>   TitleText;
	UPROPERTY() TObjectPtr<UTextBlock>   SubtitleText;
	UPROPERTY() TObjectPtr<UScrollBox>   ItemListScroll;
	UPROPERTY() TObjectPtr<UButton>      CloseButton;

	UPROPERTY() TArray<TObjectPtr<UCoMVaultItemRow>> RowHelpers;
};
