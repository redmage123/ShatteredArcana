// Copyright Mythforge Studios. All Rights Reserved.
// CoMTurnNotificationWidget.cpp -- Turn notification system implementation.

#include "CoMTurnNotificationWidget.h"
#include "CoMHUDWidget.h"

#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Sound/SoundBase.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"
#include "CoMCore/Events/CoMWorldEventSubsystem.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"
#include "CoMCore/Espionage/CoMEspionageSubsystem.h"
#include "CoMCore/TacticalCombat/CoMTacticalCombatSubsystem.h"
#include "CoMCore/World/CoMSiteEncounterSubsystem.h"
#include "CoMCore/Items/CoMItemSubsystem.h"
#include "CoMUI/CoMUISubsystem.h"

// =============================================================================
// Construction & Tick
// =============================================================================

void UCoMTurnNotificationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Start all visual elements hidden.
	if (TurnBannerBorder)  { TurnBannerBorder->SetVisibility(ESlateVisibility::Collapsed); }
	if (EventPopupBorder)  { EventPopupBorder->SetVisibility(ESlateVisibility::Collapsed); }
	if (CombatResultBorder){ CombatResultBorder->SetVisibility(ESlateVisibility::Collapsed); }
}

void UCoMTurnNotificationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateBanner(InDeltaTime);
	UpdatePopup(InDeltaTime);
	UpdateCombatResult(InDeltaTime);
}

// =============================================================================
// Turn Banner
// =============================================================================

void UCoMTurnNotificationWidget::ShowTurnBanner(int32 TurnNumber, const FString& WizardName)
{
	if (TurnBannerText)
	{
		TurnBannerText->SetText(FText::FromString(
			FString::Printf(TEXT("TURN %d \u2014 %s's Move"), TurnNumber, *WizardName)));

		FSlateFontInfo FontInfo = TurnBannerText->GetFont();
		FontInfo.Size = 28;
		TurnBannerText->SetFont(FontInfo);
		TurnBannerText->SetColorAndOpacity(FSlateColor(GoldColor()));
	}

	if (TurnBannerBorder)
	{
		TurnBannerBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		TurnBannerBorder->SetRenderOpacity(0.f);
	}

	BannerState = EBannerState::FadingIn;
	BannerTimer = 0.f;

	// Also add to the notification feed.
	AddNotificationMessage(
		FString::Printf(TEXT("Turn %d started - %s's turn"), TurnNumber, *WizardName),
		GoldColor(), ECoMNotificationPriority::Normal);
}

void UCoMTurnNotificationWidget::UpdateBanner(float DeltaTime)
{
	if (BannerState == EBannerState::Hidden)
	{
		return;
	}

	BannerTimer += DeltaTime;

	switch (BannerState)
	{
	case EBannerState::FadingIn:
		if (TurnBannerBorder)
		{
			float Alpha = FMath::Clamp(BannerTimer / BannerFadeInDuration, 0.f, 1.f);
			TurnBannerBorder->SetRenderOpacity(Alpha);
		}
		if (BannerTimer >= BannerFadeInDuration)
		{
			BannerState = EBannerState::Showing;
			BannerTimer = 0.f;
		}
		break;

	case EBannerState::Showing:
		if (BannerTimer >= BannerShowDuration)
		{
			BannerState = EBannerState::FadingOut;
			BannerTimer = 0.f;
		}
		break;

	case EBannerState::FadingOut:
		if (TurnBannerBorder)
		{
			float Alpha = 1.f - FMath::Clamp(BannerTimer / BannerFadeOutDuration, 0.f, 1.f);
			TurnBannerBorder->SetRenderOpacity(Alpha);
		}
		if (BannerTimer >= BannerFadeOutDuration)
		{
			BannerState = EBannerState::Hidden;
			if (TurnBannerBorder) { TurnBannerBorder->SetVisibility(ESlateVisibility::Collapsed); }
		}
		break;

	default:
		break;
	}
}

// =============================================================================
// Event Popup
// =============================================================================

void UCoMTurnNotificationWidget::ShowEventNotification(const FCoMWorldEvent& Event)
{
	// Build title from event type.
	const UEnum* EventEnum = StaticEnum<ECoMWorldEventType>();
	FString Title = EventEnum
		? EventEnum->GetDisplayNameTextByValue(static_cast<int64>(Event.Type)).ToString()
		: TEXT("World Event");

	// Build body: affected planes and intensity.
	FString AffectedStr;
	const UEnum* PlaneEnum = StaticEnum<ECoMPlane>();
	for (ECoMPlane Plane : Event.AffectedPlanes)
	{
		if (!AffectedStr.IsEmpty()) { AffectedStr += TEXT(", "); }
		AffectedStr += PlaneEnum
			? PlaneEnum->GetDisplayNameTextByValue(static_cast<int64>(Plane)).ToString()
			: TEXT("Unknown");
	}

	FString Body = FString::Printf(TEXT("Affecting: %s\nDuration: %d turns"),
		AffectedStr.IsEmpty() ? TEXT("All Planes") : *AffectedStr,
		Event.Duration);

	QueueNotification(Title, Body, ECoMNotificationPriority::Important);

	// Also push to notification feed.
	AddNotificationMessage(FString::Printf(TEXT("Event: %s"), *Title), GoldColor(),
		ECoMNotificationPriority::Important);
}

void UCoMTurnNotificationWidget::UpdatePopup(float DeltaTime)
{
	if (PopupState == EPopupState::Hidden)
	{
		// Try to show next queued notification.
		if (NotificationQueue.Num() > 0)
		{
			ShowNextQueuedNotification();
		}
		return;
	}

	PopupTimer += DeltaTime;

	switch (PopupState)
	{
	case EPopupState::SlidingIn:
		if (EventPopupBorder)
		{
			// Slide from right: translate X from 300 to 0.
			float Progress = FMath::Clamp(PopupTimer / PopupSlideDuration, 0.f, 1.f);
			float OffsetX = FMath::Lerp(300.f, 0.f, Progress);
			EventPopupBorder->SetRenderTranslation(FVector2D(OffsetX, 0.f));
			EventPopupBorder->SetRenderOpacity(Progress);
		}
		if (PopupTimer >= PopupSlideDuration)
		{
			PopupState = EPopupState::Showing;
			PopupTimer = 0.f;
		}
		break;

	case EPopupState::Showing:
		if (PopupTimer >= PopupShowDuration)
		{
			PopupState = EPopupState::SlidingOut;
			PopupTimer = 0.f;
		}
		break;

	case EPopupState::SlidingOut:
		if (EventPopupBorder)
		{
			float Progress = FMath::Clamp(PopupTimer / PopupSlideDuration, 0.f, 1.f);
			float OffsetX = FMath::Lerp(0.f, 300.f, Progress);
			EventPopupBorder->SetRenderTranslation(FVector2D(OffsetX, 0.f));
			EventPopupBorder->SetRenderOpacity(1.f - Progress);
		}
		if (PopupTimer >= PopupSlideDuration)
		{
			PopupState = EPopupState::Hidden;
			if (EventPopupBorder) { EventPopupBorder->SetVisibility(ESlateVisibility::Collapsed); }
		}
		break;

	default:
		break;
	}
}

void UCoMTurnNotificationWidget::ShowNextQueuedNotification()
{
	if (NotificationQueue.Num() == 0) { return; }

	FCoMQueuedNotification Entry = NotificationQueue[0];
	NotificationQueue.RemoveAt(0);

	if (EventTitleText)
	{
		EventTitleText->SetText(FText::FromString(Entry.Title));
		FSlateFontInfo FontInfo = EventTitleText->GetFont();
		FontInfo.Size = 16;
		EventTitleText->SetFont(FontInfo);
		EventTitleText->SetColorAndOpacity(FSlateColor(GoldColor()));
	}

	if (EventBodyText)
	{
		EventBodyText->SetText(FText::FromString(Entry.Body));
		FSlateFontInfo FontInfo = EventBodyText->GetFont();
		FontInfo.Size = 12;
		EventBodyText->SetFont(FontInfo);
		EventBodyText->SetColorAndOpacity(FSlateColor(WhiteColor()));
	}

	if (EventPopupBorder)
	{
		EventPopupBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		EventPopupBorder->SetRenderTranslation(FVector2D(300.f, 0.f));
		EventPopupBorder->SetRenderOpacity(0.f);
	}

	PopupState = EPopupState::SlidingIn;
	PopupTimer = 0.f;
}

void UCoMTurnNotificationWidget::QueueNotification(const FString& Title, const FString& Body,
	ECoMNotificationPriority InPriority)
{
	// Event popups: only show for Critical and Important.
	if (InPriority > ECoMNotificationPriority::Important)
	{
		return;
	}

	// Also filter against MinimumPriority.
	if (InPriority > MinimumPriority)
	{
		return;
	}

	FCoMQueuedNotification Entry;
	Entry.Title = Title;
	Entry.Body = Body;
	Entry.Priority = InPriority;
	NotificationQueue.Add(Entry);
}

// =============================================================================
// Combat Result
// =============================================================================

void UCoMTurnNotificationWidget::ShowCombatResult(ECoMCombatResult Result,
                                                   int32 AttackerLosses,
                                                   int32 DefenderLosses)
{
	FString ResultStr;
	FLinearColor ResultColor;

	switch (Result)
	{
	case ECoMCombatResult::AttackerVictory:
		ResultStr = TEXT("Victory!");
		ResultColor = GreenColor();
		break;
	case ECoMCombatResult::DefenderVictory:
		ResultStr = TEXT("Defeat!");
		ResultColor = RedColor();
		break;
	case ECoMCombatResult::Draw:
		ResultStr = TEXT("Draw");
		ResultColor = GoldColor();
		break;
	case ECoMCombatResult::AttackerFlee:
		ResultStr = TEXT("Attacker Retreated");
		ResultColor = GoldColor();
		break;
	case ECoMCombatResult::DefenderFlee:
		ResultStr = TEXT("Defender Retreated");
		ResultColor = GoldColor();
		break;
	default:
		ResultStr = TEXT("Battle Ended");
		ResultColor = WhiteColor();
		break;
	}

	if (CombatResultText)
	{
		CombatResultText->SetText(FText::FromString(ResultStr));
		FSlateFontInfo FontInfo = CombatResultText->GetFont();
		FontInfo.Size = 24;
		CombatResultText->SetFont(FontInfo);
		CombatResultText->SetColorAndOpacity(FSlateColor(ResultColor));
	}

	if (CombatDetailText)
	{
		CombatDetailText->SetText(FText::FromString(
			FString::Printf(TEXT("Attacker lost %d units | Defender lost %d units"),
				AttackerLosses, DefenderLosses)));
		FSlateFontInfo FontInfo = CombatDetailText->GetFont();
		FontInfo.Size = 14;
		CombatDetailText->SetFont(FontInfo);
		CombatDetailText->SetColorAndOpacity(FSlateColor(WhiteColor()));
	}

	if (CombatResultBorder)
	{
		CombatResultBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		CombatResultBorder->SetRenderOpacity(1.f);
	}

	CombatState = ECombatState::Showing;
	CombatTimer = 0.f;

	// Also push to notification feed.
	AddNotificationMessage(
		FString::Printf(TEXT("Battle: %s (Lost %d / Enemy lost %d)"), *ResultStr, AttackerLosses, DefenderLosses),
		ResultColor, ECoMNotificationPriority::Important);
}

void UCoMTurnNotificationWidget::UpdateCombatResult(float DeltaTime)
{
	if (CombatState == ECombatState::Hidden) { return; }

	CombatTimer += DeltaTime;

	switch (CombatState)
	{
	case ECombatState::Showing:
		if (CombatTimer >= CombatShowDuration)
		{
			CombatState = ECombatState::FadingOut;
			CombatTimer = 0.f;
		}
		break;

	case ECombatState::FadingOut:
		if (CombatResultBorder)
		{
			float Alpha = 1.f - FMath::Clamp(CombatTimer / CombatFadeOutDuration, 0.f, 1.f);
			CombatResultBorder->SetRenderOpacity(Alpha);
		}
		if (CombatTimer >= CombatFadeOutDuration)
		{
			CombatState = ECombatState::Hidden;
			if (CombatResultBorder) { CombatResultBorder->SetVisibility(ESlateVisibility::Collapsed); }
		}
		break;

	default:
		break;
	}
}

// =============================================================================
// Notification Feed
// =============================================================================

void UCoMTurnNotificationWidget::AddNotificationMessage(const FString& Message, FLinearColor Color,
	ECoMNotificationPriority InPriority)
{
	// Filter: skip messages below the minimum priority threshold.
	if (InPriority > MinimumPriority)
	{
		return;
	}

	UCoMHUDWidget* HUD = GetHUDWidget();
	if (!HUD)
	{
		return;
	}

	// The HUD AddNotification uses plain text. We use it as the feed destination
	// and format the message with a color prefix hint.
	// For full color support, we would need to extend the HUD's scroll box.
	// For now, prepend a severity marker.
	FString Prefix;
	if (Color == RedColor())         { Prefix = TEXT("[!] "); }
	else if (Color == GreenColor())  { Prefix = TEXT("[+] "); }
	else if (Color == GoldColor())   { Prefix = TEXT("[*] "); }
	else                             { Prefix = TEXT("    "); }

	HUD->AddNotification(Prefix + Message);

	// Critical notifications also play a sound.
	if (InPriority == ECoMNotificationPriority::Critical && CriticalNotificationSound)
	{
		UGameplayStatics::PlaySound2D(this, CriticalNotificationSound);
	}
}

void UCoMTurnNotificationWidget::SetMinimumPriority(ECoMNotificationPriority InPriority)
{
	MinimumPriority = InPriority;
}

UCoMHUDWidget* UCoMTurnNotificationWidget::GetHUDWidget()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI) { return nullptr; }

	UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>();
	return UISS ? UISS->GetHUDWidget() : nullptr;
}

// =============================================================================
// Delegate Binding
// =============================================================================

void UCoMTurnNotificationWidget::BindToSubsystems()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI) { return; }

	// Turn subsystem: OnTurnStarted -> ShowTurnBanner, OnIdleWarning -> HandleIdleWarning
	if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
	{
		TurnSub->OnTurnStarted.AddDynamic(this, &UCoMTurnNotificationWidget::HandleTurnStarted);
		TurnSub->OnIdleWarning.AddDynamic(this, &UCoMTurnNotificationWidget::HandleIdleWarning);
	}

	// World event subsystem: OnWorldEventTriggered -> ShowEventNotification
	if (UCoMWorldEventSubsystem* EventSub = GI->GetSubsystem<UCoMWorldEventSubsystem>())
	{
		EventSub->OnWorldEventTriggered.AddDynamic(this, &UCoMTurnNotificationWidget::HandleWorldEvent);
	}

	// Tactical combat subsystem: OnBattleEnded -> HandleBattleEnded
	if (UCoMTacticalCombatSubsystem* CombatSub = GI->GetSubsystem<UCoMTacticalCombatSubsystem>())
	{
		CombatSub->OnBattleEnded.AddDynamic(this, &UCoMTurnNotificationWidget::HandleBattleEnded);
	}

	// Site encounter subsystem: cleared sites and freed nodes.
	if (UCoMSiteEncounterSubsystem* Sites = GI->GetSubsystem<UCoMSiteEncounterSubsystem>())
	{
		Sites->OnSiteCleared.AddDynamic(this, &UCoMTurnNotificationWidget::HandleSiteCleared);
		Sites->OnNodeGuardDefeated.AddDynamic(this, &UCoMTurnNotificationWidget::HandleNodeGuardDefeated);
	}

	// Item subsystem: any forge fires this (player or AI).
	if (UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>())
	{
		Items->OnItemForged.AddDynamic(this, &UCoMTurnNotificationWidget::HandleItemForged);
	}

	// Diplomacy: war declared / treaty resolved / gift sent.
	if (UCoMDiplomacySubsystem* Dip = GI->GetSubsystem<UCoMDiplomacySubsystem>())
	{
		Dip->OnWarDeclared.AddDynamic(this,    &UCoMTurnNotificationWidget::HandleWarDeclared);
		Dip->OnTreatyResolved.AddDynamic(this, &UCoMTurnNotificationWidget::HandleTreatyResolved);
		Dip->OnGiftSent.AddDynamic(this,       &UCoMTurnNotificationWidget::HandleGiftSent);
	}

	// Espionage: mission resolved.
	if (UCoMEspionageSubsystem* Esp = GI->GetSubsystem<UCoMEspionageSubsystem>())
	{
		Esp->OnMissionResolved.AddDynamic(this, &UCoMTurnNotificationWidget::HandleMissionResolved);
	}
}

void UCoMTurnNotificationWidget::UnbindFromSubsystems()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI) { return; }

	if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
	{
		TurnSub->OnTurnStarted.RemoveDynamic(this, &UCoMTurnNotificationWidget::HandleTurnStarted);
		TurnSub->OnIdleWarning.RemoveDynamic(this, &UCoMTurnNotificationWidget::HandleIdleWarning);
	}

	if (UCoMWorldEventSubsystem* EventSub = GI->GetSubsystem<UCoMWorldEventSubsystem>())
	{
		EventSub->OnWorldEventTriggered.RemoveDynamic(this, &UCoMTurnNotificationWidget::HandleWorldEvent);
	}

	if (UCoMTacticalCombatSubsystem* CombatSub = GI->GetSubsystem<UCoMTacticalCombatSubsystem>())
	{
		CombatSub->OnBattleEnded.RemoveDynamic(this, &UCoMTurnNotificationWidget::HandleBattleEnded);
	}

	if (UCoMSiteEncounterSubsystem* Sites = GI->GetSubsystem<UCoMSiteEncounterSubsystem>())
	{
		Sites->OnSiteCleared.RemoveDynamic(this, &UCoMTurnNotificationWidget::HandleSiteCleared);
		Sites->OnNodeGuardDefeated.RemoveDynamic(this, &UCoMTurnNotificationWidget::HandleNodeGuardDefeated);
	}

	if (UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>())
	{
		Items->OnItemForged.RemoveDynamic(this, &UCoMTurnNotificationWidget::HandleItemForged);
	}
}

// =============================================================================
// Delegate Callbacks
// =============================================================================

void UCoMTurnNotificationWidget::HandleTurnStarted(int32 TurnNumber)
{
	// Determine wizard name. For now use a placeholder until wizard name
	// lookup is wired via GameState.
	ShowTurnBanner(TurnNumber, TEXT("Your"));
}

void UCoMTurnNotificationWidget::HandleWorldEvent(const FCoMWorldEvent& Event)
{
	ShowEventNotification(Event);
}

void UCoMTurnNotificationWidget::HandleBattleEnded(ECoMCombatResult Result)
{
	// Losses would ideally come from the combat subsystem's finalized context.
	// For now show placeholder values; the combat subsystem can broadcast
	// detailed results in a future sprint.
	ShowCombatResult(Result, 0, 0);
}

void UCoMTurnNotificationWidget::HandleIdleWarning(int32 IdleArmyCount, int32 IdleCityCount)
{
	ShowIdleWarning(IdleArmyCount, IdleCityCount);
}

void UCoMTurnNotificationWidget::HandleSiteCleared(int32 SiteID, int32 WizardIndex,
                                                     int32 GoldReward, int32 ManaReward)
{
	// Player gets the success ("you cleared X"); AI clears get an "intel" notice.
	const bool bIsPlayer = (WizardIndex == 0); // local player is wizard 0 by convention
	const FString Title = bIsPlayer
		? TEXT("Site Cleared")
		: FString::Printf(TEXT("Wizard %d cleared a site"), WizardIndex);
	const FString Body = bIsPlayer
		? FString::Printf(TEXT("Your army cleared a site. +%d gold, +%d mana."),
			GoldReward, ManaReward)
		: FString::Printf(TEXT("Reward: %d gold, %d mana."), GoldReward, ManaReward);

	QueueNotification(Title, Body,
		bIsPlayer ? ECoMNotificationPriority::Important : ECoMNotificationPriority::Normal);

	const FLinearColor C = bIsPlayer ? GreenColor() : WhiteColor();
	AddNotificationMessage(FString::Printf(TEXT("%s — %s"), *Title, *Body), C,
		bIsPlayer ? ECoMNotificationPriority::Important : ECoMNotificationPriority::Normal);
}

void UCoMTurnNotificationWidget::HandleNodeGuardDefeated(FIntPoint NodePosition, int32 WizardIndex)
{
	const bool bIsPlayer = (WizardIndex == 0);
	const FString Body = FString::Printf(
		TEXT("%s defeated the guardian of a mana node at (%d,%d)."),
		bIsPlayer ? TEXT("You") : *FString::Printf(TEXT("Wizard %d"), WizardIndex),
		NodePosition.X, NodePosition.Y);
	QueueNotification(TEXT("Mana Node Freed"), Body,
		bIsPlayer ? ECoMNotificationPriority::Important : ECoMNotificationPriority::Normal);
	AddNotificationMessage(Body, bIsPlayer ? GreenColor() : WhiteColor(),
		bIsPlayer ? ECoMNotificationPriority::Important : ECoMNotificationPriority::Normal);
}

void UCoMTurnNotificationWidget::HandleItemForged(int32 InstanceID)
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI) return;
	UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>();
	if (!Items) return;

	FCoMItemInstance Inst;
	if (!Items->GetItem(InstanceID, Inst)) return;

	// Player triggered their own forge — they already see the result. Only
	// surface a notification when an AI wizard forges something.
	if (Inst.OwnerWizardIndex == 0) return;

	const FString Body = FString::Printf(
		TEXT("Wizard %d forged %s (%d mana)."),
		Inst.OwnerWizardIndex, *Inst.DisplayName.ToString(), Inst.TotalManaCost);
	QueueNotification(TEXT("Rival Forged Item"), Body, ECoMNotificationPriority::Normal);
	AddNotificationMessage(Body, WhiteColor(), ECoMNotificationPriority::Normal);
}

// =============================================================================
// Diplomacy + espionage toasts
// =============================================================================

namespace
{
	const TCHAR* WizName(int32 Idx)
	{
		switch (Idx)
		{
		case 0:  return TEXT("Merlin");
		case 1:  return TEXT("Nekros");
		case 2:  return TEXT("Pyraxis");
		case 3:  return TEXT("Gaia");
		case 4:  return TEXT("Rjak");
		case 5:  return TEXT("Ariel");
		case 6:  return TEXT("Tlaloc");
		case 7:  return TEXT("Sss'ra");
		case 8:  return TEXT("Kali");
		case 9:  return TEXT("Lo Pan");
		case 10: return TEXT("Horus");
		case 11: return TEXT("Freya");
		case 12: return TEXT("Tauron");
		case 13: return TEXT("Oberon");
		default: return TEXT("A rival");
		}
	}

	const TCHAR* TreatyShort(uint8 T)
	{
		switch (static_cast<ECoMTreatyType>(T))
		{
		case ECoMTreatyType::MilitaryAlliance: return TEXT("Alliance");
		case ECoMTreatyType::DefensivePact:    return TEXT("Defensive Pact");
		case ECoMTreatyType::NonAggression:    return TEXT("Non-Aggression Pact");
		case ECoMTreatyType::TradeAgreement:   return TEXT("Trade Agreement");
		case ECoMTreatyType::OpenBorders:      return TEXT("Open Borders");
		case ECoMTreatyType::WizardsPact:      return TEXT("Wizard's Pact");
		default:                                return TEXT("Treaty");
		}
	}
}

void UCoMTurnNotificationWidget::HandleWarDeclared(int32 AttackerWizard, int32 DefenderWizard)
{
	const bool bAgainstUs = (DefenderWizard == LocalPlayerWizardIdx);
	const bool bByUs      = (AttackerWizard == LocalPlayerWizardIdx);
	if (!bAgainstUs && !bByUs) return; // rival-on-rival, keep the HUD quiet

	const FString Title = bAgainstUs ? TEXT("War Declared!") : TEXT("War Declared");
	const FString Body = bAgainstUs
		? FString::Printf(TEXT("%s declares war on you!"), WizName(AttackerWizard))
		: FString::Printf(TEXT("You declare war on %s."), WizName(DefenderWizard));
	const ECoMNotificationPriority Pri = bAgainstUs
		? ECoMNotificationPriority::Critical : ECoMNotificationPriority::Important;
	QueueNotification(Title, Body, Pri);
	AddNotificationMessage(Body, RedColor(), Pri);
}

void UCoMTurnNotificationWidget::HandleTreatyResolved(int32 ProposerWizard, int32 TargetWizard,
	uint8 TreatyType, bool bAccepted)
{
	const bool bWeProposed = (ProposerWizard == LocalPlayerWizardIdx);
	const bool bWeTargeted = (TargetWizard == LocalPlayerWizardIdx);
	if (!bWeProposed && !bWeTargeted) return;

	FString Title;
	FString Body;
	if (bWeProposed)
	{
		Title = bAccepted ? TEXT("Treaty Accepted") : TEXT("Treaty Rejected");
		Body  = FString::Printf(TEXT("%s %s your %s offer."),
			WizName(TargetWizard),
			bAccepted ? TEXT("accepted") : TEXT("rejected"),
			TreatyShort(TreatyType));
	}
	else
	{
		Title = TEXT("Treaty Settled");
		Body  = FString::Printf(TEXT("You %s %s's %s proposal."),
			bAccepted ? TEXT("accepted") : TEXT("rejected"),
			WizName(ProposerWizard),
			TreatyShort(TreatyType));
	}
	QueueNotification(Title, Body, ECoMNotificationPriority::Important);
	AddNotificationMessage(Body, bAccepted ? GreenColor() : GoldColor(),
		ECoMNotificationPriority::Important);
}

void UCoMTurnNotificationWidget::HandleGiftSent(int32 SenderWizard, int32 ReceiverWizard, int32 ManaValue)
{
	const bool bToUs   = (ReceiverWizard == LocalPlayerWizardIdx);
	const bool bFromUs = (SenderWizard == LocalPlayerWizardIdx);
	if (!bToUs && !bFromUs) return;
	const FString Body = bToUs
		? FString::Printf(TEXT("%s sent you a gift worth %d mana."), WizName(SenderWizard), ManaValue)
		: FString::Printf(TEXT("You gifted %s %d mana."), WizName(ReceiverWizard), ManaValue);
	QueueNotification(bToUs ? TEXT("Gift Received") : TEXT("Gift Sent"), Body,
		ECoMNotificationPriority::Normal);
	AddNotificationMessage(Body, GreenColor(), ECoMNotificationPriority::Normal);
}

void UCoMTurnNotificationWidget::HandleMissionResolved(int32 OwnerWizardId, const FCoMMissionResult& Result)
{
	const bool bOurs = (OwnerWizardId == LocalPlayerWizardIdx);
	if (!bOurs) return; // rival-on-rival spy ops stay invisible (as in MoM)

	const TCHAR* Verb = TEXT("act");
	switch (Result.Mission)
	{
	case ECoMAgentMission::Spy:                Verb = TEXT("intel"); break;
	case ECoMAgentMission::Sabotage:           Verb = TEXT("sabotage"); break;
	case ECoMAgentMission::Assassinate:        Verb = TEXT("assassination"); break;
	case ECoMAgentMission::Steal:              Verb = TEXT("theft"); break;
	case ECoMAgentMission::Recruit:            Verb = TEXT("recruitment"); break;
	case ECoMAgentMission::InfiltrateCity:     Verb = TEXT("infiltration"); break;
	case ECoMAgentMission::CorruptOfficial:    Verb = TEXT("corruption"); break;
	case ECoMAgentMission::PropagandaCampaign: Verb = TEXT("propaganda"); break;
	default: break;
	}
	FString Title;
	FString Body;
	if (Result.bKilled)
	{
		Title = TEXT("Agent Killed");
		Body  = FString::Printf(TEXT("Our %s agent was killed during the mission."), Verb);
	}
	else if (Result.bCaptured)
	{
		Title = TEXT("Agent Captured");
		Body  = FString::Printf(TEXT("Our %s agent was captured by the enemy."), Verb);
	}
	else if (Result.bSuccess)
	{
		Title = TEXT("Mission Success");
		Body  = Result.ResultDescription.IsEmpty()
			? FString::Printf(TEXT("Our %s mission succeeded."), Verb)
			: Result.ResultDescription;
	}
	else
	{
		Title = TEXT("Mission Failed");
		Body  = Result.ResultDescription.IsEmpty()
			? FString::Printf(TEXT("Our %s mission failed."), Verb)
			: Result.ResultDescription;
	}
	const ECoMNotificationPriority Pri =
		(Result.bKilled || Result.bCaptured)
		? ECoMNotificationPriority::Critical
		: ECoMNotificationPriority::Important;
	QueueNotification(Title, Body, Pri);
	AddNotificationMessage(Body, Result.bSuccess ? GreenColor() : RedColor(), Pri);
}

// =============================================================================
// Idle Warning
// =============================================================================

void UCoMTurnNotificationWidget::ShowIdleWarning(int32 IdleArmyCount, int32 IdleCityCount)
{
	FString Title = TEXT("Idle Units Warning");
	FString Body;

	if (IdleArmyCount > 0 && IdleCityCount > 0)
	{
		Body = FString::Printf(
			TEXT("You have %d idle %s and %d %s with no production.\nEnd turn anyway?"),
			IdleArmyCount, IdleArmyCount == 1 ? TEXT("army") : TEXT("armies"),
			IdleCityCount, IdleCityCount == 1 ? TEXT("city") : TEXT("cities"));
	}
	else if (IdleArmyCount > 0)
	{
		Body = FString::Printf(
			TEXT("You have %d idle %s with movement remaining.\nEnd turn anyway?"),
			IdleArmyCount, IdleArmyCount == 1 ? TEXT("army") : TEXT("armies"));
	}
	else
	{
		Body = FString::Printf(
			TEXT("You have %d %s with no production queued.\nEnd turn anyway?"),
			IdleCityCount, IdleCityCount == 1 ? TEXT("city") : TEXT("cities"));
	}

	QueueNotification(Title, Body, ECoMNotificationPriority::Important);

	// Also add to notification feed.
	AddNotificationMessage(
		FString::Printf(TEXT("Warning: %d idle armies, %d idle cities"), IdleArmyCount, IdleCityCount),
		GoldColor(), ECoMNotificationPriority::Important);
}
