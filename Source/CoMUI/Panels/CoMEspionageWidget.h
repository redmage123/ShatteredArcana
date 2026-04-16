// Copyright Mythforge Studios. All Rights Reserved.
// CoMEspionageWidget.h -- Spy management panel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMEspionageWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

/**
 * UCoMEspionageWidget
 *
 * Shadow Network panel for spy management. Shows active agents,
 * their locations and status, mission type selection, and deployment controls.
 */
UCLASS()
class COMUI_API UCoMEspionageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Set which wizard's spy network to display. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Espionage")
	void SetWizardId(int32 WizardId);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnDeployAgentClicked();
	UFUNCTION() void OnSabotageClicked();
	UFUNCTION() void OnTheftClicked();
	UFUNCTION() void OnAssassinationClicked();
	UFUNCTION() void OnTurnAgentClicked();

private:
	void BuildLayout();
	UButton* CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width = 160.f);
	UButton* CreateMissionButton(UHorizontalBox* Parent, const FString& Label, const FLinearColor& Color);

	UPROPERTY() TObjectPtr<UBorder> BackgroundBorder;
	UPROPERTY() TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY() TObjectPtr<UScrollBox> AgentListScrollBox;
	UPROPERTY() TObjectPtr<UHorizontalBox> MissionButtonsRow;
	UPROPERTY() TObjectPtr<UButton> SabotageButton;
	UPROPERTY() TObjectPtr<UButton> TheftButton;
	UPROPERTY() TObjectPtr<UButton> AssassinationButton;
	UPROPERTY() TObjectPtr<UButton> TurnAgentButton;
	UPROPERTY() TObjectPtr<UButton> DeployAgentButton;
	UPROPERTY() TObjectPtr<UButton> CloseButton;

	int32 CurrentWizardId = -1;
};
