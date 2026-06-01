// Copyright Mythforge Studios. All Rights Reserved.
// CoMUISubsystem.cpp -- Central UI manager implementation.

#include "CoMUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "HUD/CoMHUDWidget.h"
#include "Panels/CoMCityScreenWidget.h"
#include "Panels/CoMSpellBookWidget.h"
#include "Panels/CoMEnchantmentPanelWidget.h"
#include "Panels/CoMCivilopediaWidget.h"
#include "Panels/CoMDiplomacyWidget.h"
#include "Panels/CoMArmyPanelWidget.h"
#include "Panels/CoMCreditsWidget.h"
#include "Panels/CoMSettingsWidget.h"
#include "Panels/CoMWizardCreationWidget.h"
#include "Panels/CoMWizardConfigWidget.h"
#include "HUD/CoMMainMenuWidget.h"
#include "HUD/CoMLoadScreenWidget.h"
#include "Panels/CoMVictoryScreenWidget.h"
#include "HUD/CoMTooltipWidget.h"
#include "HUD/CoMTurnNotificationWidget.h"
#include "HUD/CoMSpellTargetingWidget.h"
#include "Panels/CoMPreBattleWidget.h"
#include "Panels/CoMUnitCardWidget.h"
#include "Panels/CoMArmyStackWidget.h"
#include "Panels/CoMHeroScreenWidget.h"
#include "Panels/CoMItemForgeWidget.h"
#include "Panels/CoMItemVaultWidget.h"
#include "Panels/CoMTutorialWidget.h"
#include "Panels/CoMHeroOfferWidget.h"
#include "HUD/CoMSpellVFXWidget.h"
#include "HUD/CoMSpellCastCinematicWidget.h"
#include "Framework/CoMGameInstance.h"
#include "CoMCore/Combat/CoMCombatSubsystem.h"
#include "CoMCore/Magic/CoMSpellVFXSubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"
#include "Engine/Texture2D.h"

void UCoMUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Set widget class defaults so they work without Blueprint configuration.
	if (!MainMenuWidgetClass)      { MainMenuWidgetClass      = UCoMMainMenuWidget::StaticClass(); }
	if (!HUDWidgetClass)           { HUDWidgetClass           = UCoMHUDWidget::StaticClass(); }
	if (!CityScreenWidgetClass)    { CityScreenWidgetClass    = UCoMCityScreenWidget::StaticClass(); }
	if (!SpellBookWidgetClass)     { SpellBookWidgetClass     = UCoMSpellBookWidget::StaticClass(); }
	if (!EnchantmentPanelWidgetClass) { EnchantmentPanelWidgetClass = UCoMEnchantmentPanelWidget::StaticClass(); }
	if (!CivilopediaWidgetClass)      { CivilopediaWidgetClass      = UCoMCivilopediaWidget::StaticClass(); }
	if (!DiplomacyWidgetClass)     { DiplomacyWidgetClass     = UCoMDiplomacyWidget::StaticClass(); }
	if (!ArmyPanelWidgetClass)     { ArmyPanelWidgetClass     = UCoMArmyPanelWidget::StaticClass(); }
	if (!SettingsWidgetClass)      { SettingsWidgetClass       = UCoMSettingsWidget::StaticClass(); }
	if (!CreditsWidgetClass)       { CreditsWidgetClass       = UCoMCreditsWidget::StaticClass(); }
	if (!WizardCreationWidgetClass){ WizardCreationWidgetClass = UCoMWizardCreationWidget::StaticClass(); }
	if (!WizardConfigWidgetClass) { WizardConfigWidgetClass  = UCoMWizardConfigWidget::StaticClass(); }
	if (!LoadScreenWidgetClass)    { LoadScreenWidgetClass    = UCoMLoadScreenWidget::StaticClass(); }
	if (!VictoryScreenWidgetClass) { VictoryScreenWidgetClass = UCoMVictoryScreenWidget::StaticClass(); }
	if (!TooltipWidgetClass)       { TooltipWidgetClass       = UCoMTooltipWidget::StaticClass(); }
	if (!TurnNotificationWidgetClass) { TurnNotificationWidgetClass = UCoMTurnNotificationWidget::StaticClass(); }
	if (!SpellTargetingWidgetClass){ SpellTargetingWidgetClass = UCoMSpellTargetingWidget::StaticClass(); }
	if (!PreBattleWidgetClass)     { PreBattleWidgetClass     = UCoMPreBattleWidget::StaticClass(); }
	if (!UnitCardWidgetClass)      { UnitCardWidgetClass      = UCoMUnitCardWidget::StaticClass(); }
	if (!ArmyStackWidgetClass)     { ArmyStackWidgetClass     = UCoMArmyStackWidget::StaticClass(); }
	if (!HeroScreenWidgetClass)    { HeroScreenWidgetClass    = UCoMHeroScreenWidget::StaticClass(); }
	if (!SpellVFXWidgetClass)      { SpellVFXWidgetClass      = UCoMSpellVFXWidget::StaticClass(); }
	if (!ItemForgeWidgetClass)     { ItemForgeWidgetClass     = UCoMItemForgeWidget::StaticClass(); }
	if (!ItemVaultWidgetClass)     { ItemVaultWidgetClass     = UCoMItemVaultWidget::StaticClass(); }
	if (!TutorialWidgetClass)      { TutorialWidgetClass      = UCoMTutorialWidget::StaticClass(); }
	if (!SpellCastCinematicWidgetClass) { SpellCastCinematicWidgetClass = UCoMSpellCastCinematicWidget::StaticClass(); }
	if (!HeroOfferWidgetClass)     { HeroOfferWidgetClass     = UCoMHeroOfferWidget::StaticClass(); }

	// Bind to GameInstance delegates so game modes can trigger UI
	// without a compile-time dependency on CoMUI.
	if (UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance()))
	{
		CoMGI->OnMainMenuRequested.AddUObject(this, &UCoMUISubsystem::ShowMainMenu);
		CoMGI->OnHUDRequested.AddUObject(this, &UCoMUISubsystem::ShowHUD);
		CoMGI->OnHideMenusRequested.AddUObject(this, &UCoMUISubsystem::HandleHideMenus);
		CoMGI->OnCityScreenRequested.AddUObject(this, &UCoMUISubsystem::ShowCityScreen);
		CoMGI->OnArmyStackRequested.AddUObject(this, &UCoMUISubsystem::ShowArmyStack);
	}
}

void UCoMUISubsystem::Deinitialize()
{
	// Unbind delegates before teardown.
	if (UCoMGameInstance* CoMGI = Cast<UCoMGameInstance>(GetGameInstance()))
	{
		CoMGI->OnMainMenuRequested.RemoveAll(this);
		CoMGI->OnHUDRequested.RemoveAll(this);
		CoMGI->OnHideMenusRequested.RemoveAll(this);
		CoMGI->OnCityScreenRequested.RemoveAll(this);
		CoMGI->OnArmyStackRequested.RemoveAll(this);
	}

	HideAll();
	Super::Deinitialize();
}

// =============================================================================
// Template helpers
// =============================================================================

template<typename T>
T* UCoMUISubsystem::CreateAndShowWidget(TSubclassOf<T> WidgetClass, T*& InstanceRef, int32 ZOrder)
{
	// If already in viewport, ensure visible and return.
	if (InstanceRef && InstanceRef->IsInViewport())
	{
		InstanceRef->SetVisibility(ESlateVisibility::Visible);
		return InstanceRef;
	}

	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMUISubsystem: Widget class not set for %s"), *T::StaticClass()->GetName());
		return nullptr;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}

	UWorld* World = GI->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	InstanceRef = CreateWidget<T>(World, WidgetClass);
	if (InstanceRef)
	{
		InstanceRef->AddToViewport(ZOrder);
	}

	return InstanceRef;
}

template<typename T>
void UCoMUISubsystem::RemoveWidget(T*& InstanceRef)
{
	if (InstanceRef)
	{
		if (InstanceRef->IsInViewport())
		{
			InstanceRef->RemoveFromParent();
		}
		InstanceRef = nullptr;
	}
}

// =============================================================================
// Main Menu
// =============================================================================

void UCoMUISubsystem::ShowMainMenu()
{
	CreateAndShowWidget<UCoMMainMenuWidget>(MainMenuWidgetClass, MainMenuInstance, 100);
}

void UCoMUISubsystem::HideMainMenu()
{
	RemoveWidget(MainMenuInstance);
}

// =============================================================================
// HUD
// =============================================================================

void UCoMUISubsystem::ShowHUD()
{
	CreateAndShowWidget<UCoMHUDWidget>(HUDWidgetClass, HUDWidgetInstance, 0);

	// Bind HUD side-button delegates.
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnEndTurnRequested.AddDynamic(this, &UCoMUISubsystem::OnEndTurnFromHUD);
		HUDWidgetInstance->OnSpellBookRequested.AddDynamic(this, &UCoMUISubsystem::OnSpellBookFromHUD);
		HUDWidgetInstance->OnDiplomacyRequested.AddDynamic(this, &UCoMUISubsystem::OnDiplomacyFromHUD);
		HUDWidgetInstance->OnEnchantmentsRequested.AddDynamic(this, &UCoMUISubsystem::OnEnchantmentsFromHUD);
		HUDWidgetInstance->OnCivilopediaRequested.AddDynamic(this, &UCoMUISubsystem::OnCivilopediaFromHUD);
	}

	// Create the turn notification overlay (lives on top of HUD).
	if (!TurnNotificationInstance)
	{
		UCoMTurnNotificationWidget* NotifWidget = CreateAndShowWidget<UCoMTurnNotificationWidget>(
			TurnNotificationWidgetClass, TurnNotificationInstance, 50);
		if (NotifWidget)
		{
			NotifWidget->BindToSubsystems();
		}
	}

	// Bind to combat subsystem's pre-battle delegate so we can show the popup.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMCombatSubsystem* CombatSub = GI->GetSubsystem<UCoMCombatSubsystem>())
		{
			CombatSub->OnPreBattleChoice.AddDynamic(this, &UCoMUISubsystem::ShowPreBattlePopup);
		}
	}

	// Create the tooltip widget (highest Z-order).
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}

	// Create the spell targeting widget (below tooltip, above panels).
	if (!SpellTargetingInstance)
	{
		CreateAndShowWidget<UCoMSpellTargetingWidget>(SpellTargetingWidgetClass, SpellTargetingInstance, 150);
	}

	// Create the spell VFX overlay (Z-order 300 — above everything).
	if (!SpellVFXInstance)
	{
		CreateAndShowWidget<UCoMSpellVFXWidget>(SpellVFXWidgetClass, SpellVFXInstance, 300);
	}

	// Bind to the VFX subsystem's effect-requested delegate.
	if (UGameInstance* VFXGI = GetGameInstance())
	{
		if (UCoMSpellVFXSubsystem* VFXSub = VFXGI->GetSubsystem<UCoMSpellVFXSubsystem>())
		{
			VFXSub->OnEffectRequested.AddDynamic(this, &UCoMUISubsystem::OnSpellVFXRequested);
		}

		// Magic: cinematic-cast overlay for summons, globals, artifact creation.
		if (UCoMMagicSubsystem* MagicSub = VFXGI->GetSubsystem<UCoMMagicSubsystem>())
		{
			MagicSub->OnSpellCastCinematicRequested.AddDynamic(
				this, &UCoMUISubsystem::OnSpellCastCinematicRequested);

			// Global enchantments: announce every wizard's cast to all players.
			MagicSub->OnGlobalEnchantmentChanged.AddDynamic(
				this, &UCoMUISubsystem::OnGlobalEnchantmentChanged);
		}

		// Heroes: pop offer popup when a tavern offer fires for the local player.
		if (UCoMHeroSubsystem* HeroSub = VFXGI->GetSubsystem<UCoMHeroSubsystem>())
		{
			HeroSub->OnHeroOffered.AddDynamic(this, &UCoMUISubsystem::OnHeroOffered);
		}
	}

	// First-turn onboarding modal (no-op after first dismissal this session).
	ShowTutorialIfNeeded();
}

void UCoMUISubsystem::HideHUD()
{
	// Unbind notification delegates before removal.
	if (TurnNotificationInstance)
	{
		TurnNotificationInstance->UnbindFromSubsystems();
	}

	RemoveWidget(TurnNotificationInstance);
	RemoveWidget(TooltipInstance);
	RemoveWidget(SpellTargetingInstance);
	RemoveWidget(SpellVFXInstance);

	// Unbind from VFX subsystem delegate.
	if (UGameInstance* VFXGI = GetGameInstance())
	{
		if (UCoMSpellVFXSubsystem* VFXSub = VFXGI->GetSubsystem<UCoMSpellVFXSubsystem>())
		{
			VFXSub->OnEffectRequested.RemoveAll(this);
		}
	}

	RemoveWidget(HUDWidgetInstance);
}

// =============================================================================
// City Screen
// =============================================================================

void UCoMUISubsystem::ShowCityScreen(int32 CityId)
{
	// Close any other panel (except HUD) before opening city screen.
	HideAllPanels();

	UCoMCityScreenWidget* Widget = CreateAndShowWidget<UCoMCityScreenWidget>(
		CityScreenWidgetClass, CityScreenInstance, 10);

	if (Widget)
	{
		Widget->SetCity(CityId);
	}
}

void UCoMUISubsystem::HideCityScreen()
{
	RemoveWidget(CityScreenInstance);
}

// =============================================================================
// Spell Book
// =============================================================================

void UCoMUISubsystem::ShowSpellBook(int32 WizardId)
{
	HideAllPanels();

	UCoMSpellBookWidget* Widget = CreateAndShowWidget<UCoMSpellBookWidget>(
		SpellBookWidgetClass, SpellBookInstance, 10);

	if (Widget)
	{
		Widget->SetWizardId(WizardId);
	}
}

void UCoMUISubsystem::HideSpellBook()
{
	RemoveWidget(SpellBookInstance);
}

void UCoMUISubsystem::ShowEnchantments(int32 ViewerWizardId)
{
	UCoMEnchantmentPanelWidget* Widget = CreateAndShowWidget<UCoMEnchantmentPanelWidget>(
		EnchantmentPanelWidgetClass, EnchantmentPanelInstance, 11);

	if (Widget)
	{
		Widget->Configure(ViewerWizardId);
	}
}

void UCoMUISubsystem::ShowCivilopedia()
{
	CreateAndShowWidget<UCoMCivilopediaWidget>(
		CivilopediaWidgetClass, CivilopediaInstance, 11);
}

void UCoMUISubsystem::HideCivilopedia()
{
	RemoveWidget(CivilopediaInstance);
}

// =============================================================================
// Diplomacy
// =============================================================================

void UCoMUISubsystem::ShowDiplomacy(int32 WizardId)
{
	HideAllPanels();

	UCoMDiplomacyWidget* Widget = CreateAndShowWidget<UCoMDiplomacyWidget>(
		DiplomacyWidgetClass, DiplomacyInstance, 10);

	if (Widget)
	{
		Widget->SetPlayerWizardId(WizardId);
	}
}

void UCoMUISubsystem::HideDiplomacy()
{
	RemoveWidget(DiplomacyInstance);
}

// =============================================================================
// Army Panel
// =============================================================================

void UCoMUISubsystem::ShowArmyPanel(int32 ArmyId)
{
	// Army panel can coexist with the HUD, but close other full-screen panels.
	HideCityScreen();
	HideSpellBook();
	HideDiplomacy();

	UCoMArmyPanelWidget* Widget = CreateAndShowWidget<UCoMArmyPanelWidget>(
		ArmyPanelWidgetClass, ArmyPanelInstance, 5);

	if (Widget)
	{
		Widget->SetArmy(ArmyId);
	}
}

void UCoMUISubsystem::HideArmyPanel()
{
	RemoveWidget(ArmyPanelInstance);
}

// =============================================================================
// Wizard Creation
// =============================================================================

void UCoMUISubsystem::ShowWizardCreation()
{
	HideAllPanels();
	HideMainMenu();
	CreateAndShowWidget<UCoMWizardCreationWidget>(
		WizardCreationWidgetClass, WizardCreationInstance, 100);
}

void UCoMUISubsystem::HideWizardCreation()
{
	HideWizardConfig();
	RemoveWidget(WizardCreationInstance);
}

// =============================================================================
// Wizard Config (Screen 2)
// =============================================================================

void UCoMUISubsystem::ShowWizardConfig(int32 PortraitIndex)
{
	// Hide any previous config instance.
	HideWizardConfig();

	UGameInstance* GI = GetGameInstance();
	if (!GI || !WizardConfigWidgetClass) { return; }

	UWorld* World = GI->GetWorld();
	if (!World) { return; }

	WizardConfigInstance = CreateWidget<UCoMWizardConfigWidget>(World, WizardConfigWidgetClass);
	if (WizardConfigInstance)
	{
		WizardConfigInstance->SetPortraitIndex(PortraitIndex);
		WizardConfigInstance->AddToViewport(101);
	}
}

void UCoMUISubsystem::HideWizardConfig()
{
	RemoveWidget(WizardConfigInstance);
}

// =============================================================================
// Load Screen
// =============================================================================

void UCoMUISubsystem::ShowLoadScreen()
{
	HideAllPanels();
	HideMainMenu();
	CreateAndShowWidget<UCoMLoadScreenWidget>(
		LoadScreenWidgetClass, LoadScreenInstance, 100);
}

void UCoMUISubsystem::HideLoadScreen()
{
	RemoveWidget(LoadScreenInstance);
}

// =============================================================================
// Settings
// =============================================================================

void UCoMUISubsystem::ShowSettings()
{
	HideAllPanels();
	CreateAndShowWidget<UCoMSettingsWidget>(SettingsWidgetClass, SettingsInstance, 20);
}

void UCoMUISubsystem::HideSettings()
{
	const bool bWasShowing = (SettingsInstance != nullptr && SettingsInstance->IsInViewport());

	RemoveWidget(SettingsInstance);

	// Re-show the main menu if it was collapsed when we opened settings.
	if (bWasShowing && MainMenuInstance && MainMenuInstance->IsInViewport())
	{
		MainMenuInstance->SetVisibility(ESlateVisibility::Visible);
	}
}

// =============================================================================
// Credits
// =============================================================================

void UCoMUISubsystem::ShowCredits()
{
	HideAllPanels();

	UCoMCreditsWidget* Widget = CreateAndShowWidget<UCoMCreditsWidget>(
		CreditsWidgetClass, CreditsInstance, 20);

	if (Widget)
	{
		Widget->StartCredits();
	}
}

void UCoMUISubsystem::HideCredits()
{
	RemoveWidget(CreditsInstance);
}

// =============================================================================
// Victory / Defeat Screen
// =============================================================================

void UCoMUISubsystem::ShowVictoryScreen(int32 WinnerWizardId, ECoMVictoryType VictoryType)
{
	HideAllPanels();

	UCoMVictoryScreenWidget* Widget = CreateAndShowWidget<UCoMVictoryScreenWidget>(
		VictoryScreenWidgetClass, VictoryScreenInstance, 50);

	if (Widget)
	{
		Widget->ShowVictory(WinnerWizardId, VictoryType);
	}
}

void UCoMUISubsystem::ShowDefeatScreen(int32 ConquerorWizardId)
{
	HideAllPanels();

	UCoMVictoryScreenWidget* Widget = CreateAndShowWidget<UCoMVictoryScreenWidget>(
		VictoryScreenWidgetClass, VictoryScreenInstance, 50);

	if (Widget)
	{
		Widget->ShowDefeat(ConquerorWizardId);
	}
}

void UCoMUISubsystem::HideVictoryScreen()
{
	RemoveWidget(VictoryScreenInstance);
}

// =============================================================================
// Tooltip
// =============================================================================

void UCoMUISubsystem::ShowTooltipTile(ECoMPlane Plane, ECoMMapLayer Layer, FIntPoint TilePos, int32 WizardId)
{
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}
	if (TooltipInstance)
	{
		TooltipInstance->ShowTileTooltip(Plane, Layer, TilePos, WizardId);
	}
}

void UCoMUISubsystem::ShowTooltipCity(int32 CityId)
{
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}
	if (TooltipInstance)
	{
		TooltipInstance->ShowCityTooltip(CityId);
	}
}

void UCoMUISubsystem::ShowTooltipArmy(int32 ArmyId)
{
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}
	if (TooltipInstance)
	{
		TooltipInstance->ShowArmyTooltip(ArmyId);
	}
}

void UCoMUISubsystem::ShowTooltipUnit(int32 UnitId)
{
	if (!TooltipInstance)
	{
		CreateAndShowWidget<UCoMTooltipWidget>(TooltipWidgetClass, TooltipInstance, 200);
	}
	if (TooltipInstance)
	{
		TooltipInstance->ShowUnitTooltip(UnitId);
	}
}

void UCoMUISubsystem::HideTooltip()
{
	if (TooltipInstance)
	{
		TooltipInstance->Hide();
	}
}

// =============================================================================
// Pre-Battle Popup
// =============================================================================

void UCoMUISubsystem::ShowPreBattlePopup(int32 AttackerArmyId, int32 DefenderArmyId)
{
	UCoMPreBattleWidget* Widget = CreateAndShowWidget<UCoMPreBattleWidget>(
		PreBattleWidgetClass, PreBattleInstance, 80);

	if (Widget)
	{
		Widget->SetBattle(AttackerArmyId, DefenderArmyId);
	}
}

void UCoMUISubsystem::HidePreBattlePopup()
{
	RemoveWidget(PreBattleInstance);
}

// =============================================================================
// Spell Targeting
// =============================================================================

void UCoMUISubsystem::BeginSpellTargeting(FName SpellId, int32 CasterWizardId)
{
	if (!SpellTargetingInstance)
	{
		CreateAndShowWidget<UCoMSpellTargetingWidget>(SpellTargetingWidgetClass, SpellTargetingInstance, 150);
	}
	if (SpellTargetingInstance)
	{
		SpellTargetingInstance->BeginTargeting(SpellId, CasterWizardId);
	}
}

void UCoMUISubsystem::CancelSpellTargeting()
{
	if (SpellTargetingInstance)
	{
		SpellTargetingInstance->CancelTargeting();
	}
}

// =============================================================================
// Unit Card
// =============================================================================

void UCoMUISubsystem::ShowUnitCard(int32 UnitId)
{
	UCoMUnitCardWidget* Widget = CreateAndShowWidget<UCoMUnitCardWidget>(
		UnitCardWidgetClass, UnitCardInstance, 15);

	if (Widget)
	{
		Widget->SetUnit(UnitId);
	}
}

void UCoMUISubsystem::HideUnitCard()
{
	RemoveWidget(UnitCardInstance);
}

// =============================================================================
// Army Stack
// =============================================================================

void UCoMUISubsystem::ShowArmyStack(int32 ArmyId)
{
	UCoMArmyStackWidget* Widget = CreateAndShowWidget<UCoMArmyStackWidget>(
		ArmyStackWidgetClass, ArmyStackInstance, 12);

	if (Widget)
	{
		Widget->SetArmy(ArmyId);
	}
}

void UCoMUISubsystem::HideArmyStack()
{
	RemoveWidget(ArmyStackInstance);
}

// =============================================================================
// Hero Screen
// =============================================================================

void UCoMUISubsystem::ShowHeroScreen(int32 HeroUnitId)
{
	HideAllPanels();

	UCoMHeroScreenWidget* Widget = CreateAndShowWidget<UCoMHeroScreenWidget>(
		HeroScreenWidgetClass, HeroScreenInstance, 10);

	if (Widget)
	{
		Widget->SetHero(HeroUnitId);
	}
}

void UCoMUISubsystem::HideHeroScreen()
{
	RemoveWidget(HeroScreenInstance);
}

// =============================================================================
// Spell VFX Overlay
// =============================================================================

void UCoMUISubsystem::ShowSpellVFX()
{
	CreateAndShowWidget<UCoMSpellVFXWidget>(SpellVFXWidgetClass, SpellVFXInstance, 300);
}

void UCoMUISubsystem::HideSpellVFX()
{
	if (SpellVFXInstance)
	{
		SpellVFXInstance->StopAllSpellAnimations();
	}
	RemoveWidget(SpellVFXInstance);
}

void UCoMUISubsystem::OnSpellVFXRequested(int32 PlaybackID, FName EffectID,
                                           FVector WorldPosition, FVector TargetPosition)
{
	if (!SpellVFXInstance)
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	UCoMSpellVFXSubsystem* VFXSub = GI->GetSubsystem<UCoMSpellVFXSubsystem>();
	if (!VFXSub)
	{
		return;
	}

	FCoMSpellVFXDef Def;
	if (!VFXSub->GetEffectDef(EffectID, Def))
	{
		return;
	}

	// Load the sprite sheet texture.
	UTexture2D* SheetTexture = Cast<UTexture2D>(
		StaticLoadObject(UTexture2D::StaticClass(), nullptr, *Def.SpriteSheetPath));

	if (!SheetTexture)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CoMUISubsystem::OnSpellVFXRequested: Failed to load sprite sheet '%s' for effect '%s'"),
			*Def.SpriteSheetPath, *EffectID.ToString());
		return;
	}

	// Convert world position to screen position.
	// For now, use a simple projection: center of screen as fallback.
	FVector2D ScreenPos(400.0f, 300.0f);

	APlayerController* PC = GI->GetFirstLocalPlayerController();
	if (PC)
	{
		FVector2D ProjectedPos;
		if (PC->ProjectWorldLocationToScreen(WorldPosition, ProjectedPos, false))
		{
			ScreenPos = ProjectedPos;
		}
	}

	SpellVFXInstance->PlaySpellAnimation(PlaybackID, SheetTexture,
	                                Def.FrameCount, Def.FrameDuration,
	                                ScreenPos, Def.Scale);
}

void UCoMUISubsystem::OnSpellCastCinematicRequested(int32 CasterWizardIndex,
                                                      ECoMSpellRealm Realm,
                                                      FString SpellDisplayName,
                                                      bool bIsArtifact)
{
	// Map caster wizard index to a portrait index. For now this is identity
	// (wizard 0 = portrait 0). When per-wizard portrait selection is in
	// FCoMNewGameSettings, look it up here.
	const int32 PortraitIdx = FMath::Clamp(CasterWizardIndex, 0, 13);
	ShowSpellCastCinematic(PortraitIdx, Realm, SpellDisplayName, bIsArtifact);
}

void UCoMUISubsystem::OnHeroOffered(const FCoMHeroOffer& Offer)
{
	// Only surface to the local player (wizard 0); AI auto-accepts.
	if (Offer.WizardIndex != 0) return;
	ShowHeroOffers(0);
}

void UCoMUISubsystem::OnGlobalEnchantmentChanged(int32 CasterWizardIndex, FName SpellID,
                                                   FString SpellDisplayName, bool bActive)
{
	// Caster of Magic style: every player is told when a global enchantment is
	// cast or ends, so they can react (counter, dispel, gloat).
	if (!HUDWidgetInstance) { return; }

	const FString Who = (CasterWizardIndex == 0)
		? TEXT("You have")
		: FString::Printf(TEXT("Wizard %d has"), CasterWizardIndex);

	const FString Msg = bActive
		? FString::Printf(TEXT("%s cast the global enchantment \"%s\"!"), *Who, *SpellDisplayName)
		: FString::Printf(TEXT("The global enchantment \"%s\" has ended."), *SpellDisplayName);

	HUDWidgetInstance->AddNotification(Msg);
}

void UCoMUISubsystem::ShowHeroOffers(int32 WizardIndex)
{
	UCoMHeroOfferWidget* W = CreateAndShowWidget<UCoMHeroOfferWidget>(
		HeroOfferWidgetClass, HeroOfferInstance, 12);
	if (W) { W->Configure(WizardIndex); }
}

void UCoMUISubsystem::HideHeroOffers()
{
	RemoveWidget(HeroOfferInstance);
}

// =============================================================================
// Bulk operations
// =============================================================================

void UCoMUISubsystem::ShowItemForge(int32 OwnerWizardIndex, bool bArtifactMode)
{
	HideAllPanels();
	UCoMItemForgeWidget* W = CreateAndShowWidget<UCoMItemForgeWidget>(
		ItemForgeWidgetClass, ItemForgeInstance, 10);
	if (W)
	{
		W->Configure(OwnerWizardIndex, bArtifactMode);
	}
}

void UCoMUISubsystem::HideItemForge()
{
	RemoveWidget(ItemForgeInstance);
}

void UCoMUISubsystem::ShowItemVault(int32 OwnerWizardIndex, int32 TargetHeroUnitID)
{
	HideAllPanels();
	UCoMItemVaultWidget* W = CreateAndShowWidget<UCoMItemVaultWidget>(
		ItemVaultWidgetClass, ItemVaultInstance, 10);
	if (W)
	{
		W->Configure(OwnerWizardIndex);
		W->SetTargetHero(TargetHeroUnitID);
	}
}

void UCoMUISubsystem::HideItemVault()
{
	RemoveWidget(ItemVaultInstance);
}

void UCoMUISubsystem::ShowTutorialIfNeeded()
{
	if (bTutorialShown) return;
	bTutorialShown = true;
	CreateAndShowWidget<UCoMTutorialWidget>(TutorialWidgetClass, TutorialInstance, 50);
}

void UCoMUISubsystem::HideTutorial()
{
	RemoveWidget(TutorialInstance);
}

void UCoMUISubsystem::ShowSpellCastCinematic(int32 WizardPortraitIndex, ECoMSpellRealm Realm,
                                              const FString& SpellDisplayName, bool bIsArtifact)
{
	UCoMSpellCastCinematicWidget* W = CreateAndShowWidget<UCoMSpellCastCinematicWidget>(
		SpellCastCinematicWidgetClass, CastCinematicInstance, 250);
	if (W)
	{
		W->Configure(WizardPortraitIndex, Realm, SpellDisplayName, bIsArtifact);
	}
}

void UCoMUISubsystem::HideAllPanels()
{
	HideCityScreen();
	HideSpellBook();
	HideDiplomacy();
	HideArmyPanel();
	HideSettings();
	HideCredits();
	HideWizardCreation();
	HideWizardConfig();
	HideLoadScreen();
	HideVictoryScreen();
	HideUnitCard();
	HideArmyStack();
	HideHeroScreen();
	HideItemForge();
	HideItemVault();
	HideTutorial();
	HideHeroOffers();
}

void UCoMUISubsystem::HideAll()
{
	HideMainMenu();
	HideAllPanels();
	HideSpellVFX();
	HideHUD();
}

void UCoMUISubsystem::HandleHideMenus()
{
	HideMainMenu();
	HideWizardCreation();
	HideWizardConfig();
	HideLoadScreen();
}

void UCoMUISubsystem::OnEndTurnFromHUD()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>();
	if (TurnSub)
	{
		TurnSub->ProcessFullTurn();
	}

	// Refresh HUD with fresh data.
	if (HUDWidgetInstance)
	{
		int32 Gold = 0, Mana = 0, Food = 0, Prod = 0, Turn = 0;
		FString WizName = TEXT("Player");

		if (TurnSub)
		{
			Turn = TurnSub->GetCurrentTurn();
		}

		HUDWidgetInstance->UpdateResources(Gold, Mana, Food, Prod);
		HUDWidgetInstance->UpdateTurnInfo(Turn, WizName);
		HUDWidgetInstance->AddNotification(FString::Printf(TEXT("Turn %d complete."), Turn));
	}
}

void UCoMUISubsystem::OnSpellBookFromHUD()    { ShowSpellBook(0); }
void UCoMUISubsystem::OnDiplomacyFromHUD()     { ShowDiplomacy(0); }
void UCoMUISubsystem::OnEnchantmentsFromHUD()  { ShowEnchantments(0); }
void UCoMUISubsystem::OnCivilopediaFromHUD()   { ShowCivilopedia(); }
