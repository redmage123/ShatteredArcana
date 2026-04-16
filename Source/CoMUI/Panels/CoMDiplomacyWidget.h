// Copyright Mythforge Studios. All Rights Reserved.
// CoMDiplomacyWidget.h -- Diplomacy screen.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMDiplomacyWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UProgressBar;
class UBorder;
class USizeBox;
class UHorizontalBox;
class UOverlay;
class UCoMDiplomacySubsystem;

/**
 * UCoMDiplomacyWidget
 *
 * Diplomacy screen showing known wizards, treaty status, reputation bars,
 * and diplomatic action buttons (propose peace, declare war, offer alliance, trade).
 */
UCLASS()
class COMUI_API UCoMDiplomacyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// -- Data Loading ----------------------------------------------------------

	/** Set the player wizard whose perspective we are viewing from. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Diplomacy")
	void SetPlayerWizardId(int32 WizardId);

	/** Refresh the list of known wizards and their statuses. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Diplomacy")
	void RefreshWizardList();

	/** Show detailed panel for a selected wizard. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Diplomacy")
	void OnWizardSelected(int32 WizardId);

	// -- Diplomatic Actions ----------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "CoM|Diplomacy")
	void OnDeclareWarClicked(int32 TargetWizardId);

	UFUNCTION(BlueprintCallable, Category = "CoM|Diplomacy")
	void OnProposePeaceClicked(int32 TargetWizardId);

	UFUNCTION(BlueprintCallable, Category = "CoM|Diplomacy")
	void OnProposeAllianceClicked(int32 TargetWizardId);

	UFUNCTION(BlueprintCallable, Category = "CoM|Diplomacy")
	void OnTradeClicked(int32 TargetWizardId);

protected:
	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnDeclareWarButtonClicked();

	UFUNCTION()
	void OnProposePeaceButtonClicked();

	UFUNCTION()
	void OnProposeAllianceButtonClicked();

	UFUNCTION()
	void OnTradeButtonClicked();

	UFUNCTION()
	void OnProposeTreatyButtonClicked();

	UFUNCTION()
	void OnSendGiftButtonClicked();

	// -- Widget references -----------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> WizardListScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedWizardNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TreatyStatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReputationValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ReputationBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> DetailPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeclareWarButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ProposePeaceButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ProposeAllianceButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TradeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ProposeTreatyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SendGiftButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TreatyTypeLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY()
	TObjectPtr<UBorder> BackgroundBorder;

private:
	/** Build the entire panel layout in C++. */
	void BuildLayout();

	/** Create a styled action button and add it to a parent vertical box. */
	UButton* CreateStyledButton(const FString& Label, UVerticalBox* Parent);

	/** Helper: add a centered text block to a vertical box. */
	UTextBlock* AddLabelToBox(const FString& Text, const FLinearColor& Color, int32 FontSize, UVerticalBox* Parent, const FMargin& Pad = FMargin(0));

	UCoMDiplomacySubsystem* GetDiplomacySubsystem();

	/** Convert reputation (-1000..+1000) to a normalized 0-1 bar value. */
	static float ReputationToBarPercent(int32 Reputation);

	/** Get a color for the reputation value. */
	static FLinearColor ReputationToColor(int32 Reputation);

	/** Get display string for a treaty type. */
	static FString TreatyToString(int32 TreatyValue);

	int32 PlayerWizardId = -1;
	int32 SelectedTargetWizardId = -1;

	TWeakObjectPtr<UCoMDiplomacySubsystem> CachedDiplomacySubsystem;
};
