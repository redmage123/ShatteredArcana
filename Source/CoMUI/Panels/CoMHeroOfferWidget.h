// Copyright Mythforge Studios. All Rights Reserved.
// CoMHeroOfferWidget.h -- Tavern offer popup. Lists pending hero offers for
// the local player and lets them Hire / Decline each one.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/Units/CoMHeroSubsystem.h"
#include "CoMHeroOfferWidget.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class UTextBlock;
class UCoMHeroOfferWidget;

/** Per-row helper carrying the offer ID into a UFunction. */
UCLASS()
class COMUI_API UCoMHeroOfferRow : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UCoMHeroOfferWidget> Owner;
	UPROPERTY() int32 OfferID = 0;
	UPROPERTY() bool bHire = true;

	UFUNCTION() void HandleClick();
};

/**
 * UCoMHeroOfferWidget
 *
 * Modal-ish list of tavern offers for wizard 0 (the local player).
 * Each row: hero name, tier, class, cost  [Hire] [Decline]
 */
UCLASS()
class COMUI_API UCoMHeroOfferWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void Configure(int32 InWizardIndex);

	void HandleRowClick(int32 OfferID, bool bHire);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION() void OnCloseClicked();

private:
	void BuildLayout();
	void RebuildList();

	int32 WizardIndex = 0;

	UPROPERTY() TObjectPtr<UBorder>    BackgroundBorder;
	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UScrollBox> ListScroll;
	UPROPERTY() TObjectPtr<UButton>    CloseButton;

	UPROPERTY() TArray<TObjectPtr<UCoMHeroOfferRow>> RowHelpers;
};
