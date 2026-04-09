// Copyright Shattered Arcana. All Rights Reserved.
// CoMPreBattleWidget.cpp -- Pre-battle popup implementation.

#include "CoMPreBattleWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/Combat/CoMCombatSubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMUI/CoMUISubsystem.h"

void UCoMPreBattleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AutoResolveButton)
	{
		AutoResolveButton->OnClicked.AddDynamic(this, &UCoMPreBattleWidget::OnAutoResolveClicked);
	}
	if (FightButton)
	{
		FightButton->OnClicked.AddDynamic(this, &UCoMPreBattleWidget::OnFightClicked);
	}
}

UCoMCombatSubsystem* UCoMPreBattleWidget::GetCombatSubsystem()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	return GI ? GI->GetSubsystem<UCoMCombatSubsystem>() : nullptr;
}

void UCoMPreBattleWidget::SetBattle(int32 AttackerArmyId, int32 DefenderArmyId)
{
	CachedAttackerArmyId = AttackerArmyId;
	CachedDefenderArmyId = DefenderArmyId;

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI) return;

	UCoMUnitSubsystem* UnitSub = GI->GetSubsystem<UCoMUnitSubsystem>();
	UCoMCombatSubsystem* CombatSub = GI->GetSubsystem<UCoMCombatSubsystem>();
	if (!UnitSub || !CombatSub) return;

	// Attacker info.
	const FCoMArmyGroup* AtkArmy = UnitSub->GetArmy(AttackerArmyId);
	const FCoMArmyGroup* DefArmy = UnitSub->GetArmy(DefenderArmyId);

	const FFixed64 AtkPower = CombatSub->CalculateArmyPower(AttackerArmyId);
	const FFixed64 DefPower = CombatSub->CalculateArmyPower(DefenderArmyId);

	if (AttackerInfoText && AtkArmy)
	{
		AttackerInfoText->SetText(FText::FromString(
			FString::Printf(TEXT("Attacker: Army #%d\nUnits: %d  |  Power: %d"),
				AttackerArmyId, AtkArmy->UnitIDs.Num(),
				static_cast<int32>(AtkPower.ToInt32()))));
	}

	if (DefenderInfoText && DefArmy)
	{
		DefenderInfoText->SetText(FText::FromString(
			FString::Printf(TEXT("Defender: Army #%d\nUnits: %d  |  Power: %d"),
				DefenderArmyId, DefArmy->UnitIDs.Num(),
				static_cast<int32>(DefPower.ToInt32()))));
	}

	// Odds prediction.
	if (OddsText)
	{
		const int32 AtkInt = FMath::Max(1, static_cast<int32>(AtkPower.ToInt32()));
		const int32 DefInt = FMath::Max(1, static_cast<int32>(DefPower.ToInt32()));
		const int32 Total = AtkInt + DefInt;
		const int32 AtkPct = (AtkInt * 100) / Total;

		FString OddsStr;
		if (AtkPct > 65)      { OddsStr = TEXT("Predicted: Decisive Advantage"); }
		else if (AtkPct > 50) { OddsStr = TEXT("Predicted: Slight Advantage"); }
		else if (AtkPct > 35) { OddsStr = TEXT("Predicted: Even Odds"); }
		else                  { OddsStr = TEXT("Predicted: Disadvantage"); }

		OddsText->SetText(FText::FromString(
			FString::Printf(TEXT("%s (%d%% vs %d%%)"), *OddsStr, AtkPct, 100 - AtkPct)));
	}
}

void UCoMPreBattleWidget::OnAutoResolveClicked()
{
	if (UCoMCombatSubsystem* CombatSub = GetCombatSubsystem())
	{
		CombatSub->ResolvePendingBattle(/*bAutoResolve=*/ true);
	}
	RemoveFromParent();
}

void UCoMPreBattleWidget::OnFightClicked()
{
	if (UCoMCombatSubsystem* CombatSub = GetCombatSubsystem())
	{
		CombatSub->ResolvePendingBattle(/*bAutoResolve=*/ false);
	}
	RemoveFromParent();
}
