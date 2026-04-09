// Copyright Mythforge Studios. All Rights Reserved.
// CoMTurnNotificationWidget.h -- Turn-related visual feedback: banners, events, combat results.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/TacticalCombat/TacticalTypes.h"
#include "CoMTurnNotificationWidget.generated.h"

class UTextBlock;
class UBorder;
class UVerticalBox;
class UScrollBox;
class UCanvasPanel;
class UCoMHUDWidget;
class UAudioComponent;
class USoundBase;

/**
 * Notification priority levels for filtering late-game spam.
 */
UENUM(BlueprintType)
enum class ECoMNotificationPriority : uint8
{
	Critical  UMETA(DisplayName = "Critical"),   // Always show: victory, defeat, city lost, hero died
	Important UMETA(DisplayName = "Important"),   // Show by default: combat results, events, city complete
	Normal    UMETA(DisplayName = "Normal"),      // Show by default: turn start, research done, trade complete
	Minor     UMETA(DisplayName = "Minor"),       // Show on verbose: army spotted, fog revealed, minor events
	Debug     UMETA(DisplayName = "Debug"),       // Never show in gameplay: subsystem logs
	MAX UMETA(Hidden)
};

/**
 * Queued notification entry for the slide-in popup system.
 */
USTRUCT()
struct FCoMQueuedNotification
{
	GENERATED_BODY()

	UPROPERTY() FString Title;
	UPROPERTY() FString Body;
	UPROPERTY() float DisplayTime = 4.f;
	UPROPERTY() ECoMNotificationPriority Priority = ECoMNotificationPriority::Normal;
};

/**
 * UCoMTurnNotificationWidget
 *
 * Handles all turn-related visual feedback:
 * - "Your Turn" banner (centered, fades in/out over 2 seconds)
 * - World event popups (slide in from right, stay 4 seconds, slide out)
 * - Combat result popups
 * - Notification feed integration with HUD scroll box
 *
 * Manages a queue of notifications to display sequentially.
 */
UCLASS()
class COMUI_API UCoMTurnNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// -- Turn Banner -----------------------------------------------------------

	/**
	 * Show the "TURN X -- Your Move" banner. Large centered gold text,
	 * fades in for 2 seconds then auto-fades out.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notification")
	void ShowTurnBanner(int32 TurnNumber, const FString& WizardName);

	// -- Event Notification ----------------------------------------------------

	/**
	 * Show a notification card for a world event: event name (gold),
	 * description (white), affected plane icon. Slides in from right,
	 * stays 4 seconds, slides out. Queues if another is showing.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notification")
	void ShowEventNotification(const FCoMWorldEvent& Event);

	// -- Combat Result ---------------------------------------------------------

	/**
	 * Show "Victory!" or "Defeat!" with casualty summary after tactical combat.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notification")
	void ShowCombatResult(ECoMCombatResult Result, int32 AttackerLosses, int32 DefenderLosses);

	// -- Idle Warning ----------------------------------------------------------

	/**
	 * Show idle army/city warning popup before end turn.
	 * "You have X idle armies and Y cities with no production. End turn anyway?"
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notification")
	void ShowIdleWarning(int32 IdleArmyCount, int32 IdleCityCount);

	// -- Notification Feed -----------------------------------------------------

	/**
	 * Add a color-coded message to the HUD notification scroll box.
	 * Colors: gold (important), white (info), red (danger), green (success).
	 * Priority determines whether the message is shown based on MinimumPriority filter.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notification")
	void AddNotificationMessage(const FString& Message, FLinearColor Color,
		ECoMNotificationPriority InPriority = ECoMNotificationPriority::Normal);

	/**
	 * Queue a generic notification popup (title + body).
	 * Only Critical and Important popups are shown as event popups.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notification")
	void QueueNotification(const FString& Title, const FString& Body,
		ECoMNotificationPriority InPriority = ECoMNotificationPriority::Normal);

	/**
	 * Set the minimum notification priority to display. Messages below this
	 * threshold are silently discarded.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notification")
	void SetMinimumPriority(ECoMNotificationPriority InPriority);

	/** Get the current minimum notification priority. */
	UFUNCTION(BlueprintPure, Category = "CoM|Notification")
	ECoMNotificationPriority GetMinimumPriority() const { return MinimumPriority; }

	// -- Delegate Binding Helpers ----------------------------------------------

	/** Bind to subsystem delegates. Called by CoMUISubsystem after construction. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notification")
	void BindToSubsystems();

	/** Unbind all delegate connections. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Notification")
	void UnbindFromSubsystems();

protected:
	// -- Turn Banner Widgets ---------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TurnBannerBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TurnBannerText;

	// -- Event Popup Widgets ---------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> EventPopupBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EventTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EventBodyText;

	// -- Combat Result Widgets -------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CombatResultBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CombatResultText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CombatDetailText;

private:
	// -- Delegate Callbacks (must be UFUNCTION for dynamic delegates) -----------

	UFUNCTION()
	void HandleTurnStarted(int32 TurnNumber);

	UFUNCTION()
	void HandleWorldEvent(const FCoMWorldEvent& Event);

	UFUNCTION()
	void HandleBattleEnded(ECoMCombatResult Result);

	UFUNCTION()
	void HandleIdleWarning(int32 IdleArmyCount, int32 IdleCityCount);

	// -- Banner Animation State ------------------------------------------------

	enum class EBannerState : uint8 { Hidden, FadingIn, Showing, FadingOut };
	EBannerState BannerState = EBannerState::Hidden;
	float BannerTimer = 0.f;

	static constexpr float BannerFadeInDuration = 0.5f;
	static constexpr float BannerShowDuration = 1.5f;
	static constexpr float BannerFadeOutDuration = 0.5f;

	void UpdateBanner(float DeltaTime);

	// -- Event Popup Animation State -------------------------------------------

	enum class EPopupState : uint8 { Hidden, SlidingIn, Showing, SlidingOut };
	EPopupState PopupState = EPopupState::Hidden;
	float PopupTimer = 0.f;

	static constexpr float PopupSlideDuration = 0.3f;
	static constexpr float PopupShowDuration = 4.f;

	void UpdatePopup(float DeltaTime);
	void ShowNextQueuedNotification();

	TArray<FCoMQueuedNotification> NotificationQueue;

	// -- Combat Result Animation State -----------------------------------------

	enum class ECombatState : uint8 { Hidden, Showing, FadingOut };
	ECombatState CombatState = ECombatState::Hidden;
	float CombatTimer = 0.f;

	static constexpr float CombatShowDuration = 3.f;
	static constexpr float CombatFadeOutDuration = 0.5f;

	void UpdateCombatResult(float DeltaTime);

	// -- HUD Integration -------------------------------------------------------

	/** Find the HUD widget to push notification messages into its scroll box. */
	UCoMHUDWidget* GetHUDWidget();

	// -- Priority Filtering ----------------------------------------------------

	/** Minimum priority level to display. Default: Normal (shows Critical, Important, Normal). */
	ECoMNotificationPriority MinimumPriority = ECoMNotificationPriority::Normal;

	/** Sound to play for critical notifications. */
	UPROPERTY(EditDefaultsOnly, Category = "CoM|Notification")
	TObjectPtr<USoundBase> CriticalNotificationSound;

	// -- Color Constants -------------------------------------------------------

	static FLinearColor GoldColor()  { return FLinearColor(0.85f, 0.65f, 0.13f, 1.f); }
	static FLinearColor WhiteColor() { return FLinearColor::White; }
	static FLinearColor RedColor()   { return FLinearColor(0.9f, 0.2f, 0.2f, 1.f); }
	static FLinearColor GreenColor() { return FLinearColor(0.2f, 0.9f, 0.2f, 1.f); }
};
