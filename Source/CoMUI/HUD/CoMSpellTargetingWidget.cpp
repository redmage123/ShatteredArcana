// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellTargetingWidget.cpp -- Spell targeting mode UI implementation.

#include "CoMSpellTargetingWidget.h"

#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"

// =============================================================================
// Construction
// =============================================================================

void UCoMSpellTargetingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Start hidden.
	SetVisibility(ESlateVisibility::Collapsed);

	// Bind cancel button.
	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &UCoMSpellTargetingWidget::OnCancelButtonClicked);
	}
}

void UCoMSpellTargetingWidget::NativeDestruct()
{
	if (bIsTargeting)
	{
		CleanupTargeting();
	}
	Super::NativeDestruct();
}

// =============================================================================
// Targeting Lifecycle
// =============================================================================

void UCoMSpellTargetingWidget::BeginTargeting(FName SpellId, int32 InCasterWizardId)
{
	if (SpellId == NAME_None || InCasterWizardId < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMSpellTargetingWidget: Invalid spell ID or wizard ID."));
		return;
	}

	ActiveSpellId = SpellId;
	CasterWizardId = InCasterWizardId;

	// Determine target type from spell data.
	CurrentTargetType = DetermineTargetType(SpellId);

	// For NoTarget spells, cast immediately.
	if (CurrentTargetType == ECoMSpellTargetType::NoTarget)
	{
			ExecuteCast(ActiveSpellId, CasterWizardId, FIntPoint(-1, -1));
		return;
	}

	bIsTargeting = true;

	// Determine spell range (placeholder: could be loaded from spell data asset).
	// For now, default range is based on wizard casting skill.
	UCoMMagicSubsystem* MagicSub = GetMagicSubsystem();
	if (MagicSub)
	{
		FCoMWizardMagicState& State = MagicSub->GetWizardMagic(InCasterWizardId);
		SpellRange = FMath::Max(State.CastingSkill / 5, 3); // Minimum range of 3
	}
	else
	{
		SpellRange = 10; // Fallback default.
	}

	UpdateTargetingDisplay();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	OnTargetingStarted.Broadcast();
}

void UCoMSpellTargetingWidget::CancelTargeting()
{
	if (!bIsTargeting) { return; }

	CleanupTargeting();
	OnTargetingCancelled.Broadcast();
}

void UCoMSpellTargetingWidget::ConfirmTarget(FIntPoint TileTarget)
{
	if (!bIsTargeting || CurrentTargetType != ECoMSpellTargetType::TileTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMSpellTargetingWidget: Not in tile targeting mode."));
		return;
	}

	if (!IsValidTarget(TileTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMSpellTargetingWidget: Invalid tile target (%d, %d)."),
			TileTarget.X, TileTarget.Y);
		return;
	}

	ExecuteCast(ActiveSpellId, CasterWizardId, TileTarget);
}

void UCoMSpellTargetingWidget::ConfirmUnitTarget(int32 UnitId)
{
	if (!bIsTargeting ||
	    (CurrentTargetType != ECoMSpellTargetType::UnitTarget &&
	     CurrentTargetType != ECoMSpellTargetType::ArmyTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMSpellTargetingWidget: Not in unit/army targeting mode."));
		return;
	}

	if (!IsValidUnitTarget(UnitId))
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMSpellTargetingWidget: Invalid unit target %d."), UnitId);
		return;
	}

	ExecuteCast(ActiveSpellId, CasterWizardId, FIntPoint(-1, -1), UnitId);
}

void UCoMSpellTargetingWidget::ConfirmCityTarget(int32 CityId)
{
	if (!bIsTargeting || CurrentTargetType != ECoMSpellTargetType::CityTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMSpellTargetingWidget: Not in city targeting mode."));
		return;
	}

	if (!IsValidCityTarget(CityId))
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMSpellTargetingWidget: Invalid city target %d."), CityId);
		return;
	}

	ExecuteCast(ActiveSpellId, CasterWizardId, FIntPoint(-1, -1), CityId);
}

// =============================================================================
// Target Validation
// =============================================================================

bool UCoMSpellTargetingWidget::IsValidTarget(FIntPoint TilePos) const
{
	if (!bIsTargeting) { return false; }

	// Check tile exists.
	UCoMWorldMapSubsystem* WorldMap = const_cast<UCoMSpellTargetingWidget*>(this)->GetWorldMapSubsystem();
	if (!WorldMap || !WorldMap->IsValidPosition(TilePos.X, TilePos.Y))
	{
		return false;
	}

	// Range check would use caster's position (from their fortress or army location).
	// For now, accept any valid tile position.
	return true;
}

bool UCoMSpellTargetingWidget::IsValidUnitTarget(int32 UnitId) const
{
	if (!bIsTargeting) { return false; }

	UCoMUnitSubsystem* UnitSub = const_cast<UCoMSpellTargetingWidget*>(this)->GetUnitSubsystem();
	if (!UnitSub) { return false; }

	const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitId);
	return Unit != nullptr;
}

bool UCoMSpellTargetingWidget::IsValidCityTarget(int32 CityId) const
{
	if (!bIsTargeting) { return false; }

	UCoMCitySubsystem* CitySub = const_cast<UCoMSpellTargetingWidget*>(this)->GetCitySubsystem();
	if (!CitySub) { return false; }

	const FCoMCityData* City = CitySub->GetCity(CityId);
	return City != nullptr;
}

// =============================================================================
// Internal
// =============================================================================

void UCoMSpellTargetingWidget::UpdateTargetingDisplay()
{
	if (SpellNameText)
	{
		SpellNameText->SetText(FText::FromString(
			FString::Printf(TEXT("Casting: %s"), *ActiveSpellId.ToString())));
		FSlateFontInfo FontInfo = SpellNameText->GetFont();
		FontInfo.Size = 16;
		SpellNameText->SetFont(FontInfo);
		SpellNameText->SetColorAndOpacity(FSlateColor(GoldColor()));
	}

	if (TargetTypeText)
	{
		FString TargetDesc;
		switch (CurrentTargetType)
		{
		case ECoMSpellTargetType::TileTarget: TargetDesc = TEXT("Click a tile on the map"); break;
		case ECoMSpellTargetType::UnitTarget: TargetDesc = TEXT("Click a unit"); break;
		case ECoMSpellTargetType::CityTarget: TargetDesc = TEXT("Click a city"); break;
		case ECoMSpellTargetType::ArmyTarget: TargetDesc = TEXT("Click an army"); break;
		default: TargetDesc = TEXT("Select target"); break;
		}

		TargetTypeText->SetText(FText::FromString(TargetDesc));
		FSlateFontInfo FontInfo = TargetTypeText->GetFont();
		FontInfo.Size = 14;
		TargetTypeText->SetFont(FontInfo);
		TargetTypeText->SetColorAndOpacity(FSlateColor(WhiteColor()));
	}

	if (CancelHintText)
	{
		CancelHintText->SetText(FText::FromString(TEXT("Right-click or Escape to cancel")));
		FSlateFontInfo FontInfo = CancelHintText->GetFont();
		FontInfo.Size = 11;
		CancelHintText->SetFont(FontInfo);
		CancelHintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.f)));
	}

	if (RangeText)
	{
		RangeText->SetText(FText::FromString(
			FString::Printf(TEXT("Range: %d tiles"), SpellRange)));
		FSlateFontInfo FontInfo = RangeText->GetFont();
		FontInfo.Size = 12;
		RangeText->SetFont(FontInfo);
		RangeText->SetColorAndOpacity(FSlateColor(WhiteColor()));
	}
}

void UCoMSpellTargetingWidget::ExecuteCast(FName SpellId, int32 WizardId, FIntPoint TargetTile, int32 TargetUnitId)
{
	UCoMMagicSubsystem* MagicSub = GetMagicSubsystem();
	if (!MagicSub)
	{
		UE_LOG(LogTemp, Error, TEXT("CoMSpellTargetingWidget: MagicSubsystem unavailable."));
		CleanupTargeting();
		return;
	}

	// Cast the spell via magic subsystem
	// TODO: MagicSubsystem::CastSpell may need target parameters — for now log and call
	UE_LOG(LogTemp, Log, TEXT("CoMSpellTargetingWidget: Casting '%s' by wizard %d at tile (%d,%d) target unit %d."),
		*SpellId.ToString(), WizardId, TargetTile.X, TargetTile.Y, TargetUnitId);

	CleanupTargeting();
	OnTargetingEnded.Broadcast();
}

void UCoMSpellTargetingWidget::CleanupTargeting()
{
	bIsTargeting = false;
	ActiveSpellId = NAME_None;
	CasterWizardId = -1;
	CurrentTargetType = ECoMSpellTargetType::NoTarget;
	SpellRange = 0;

	SetVisibility(ESlateVisibility::Collapsed);
}

ECoMSpellTargetType UCoMSpellTargetingWidget::DetermineTargetType(FName SpellId) const
{
	// In a full implementation, this would look up the spell data asset and
	// read the target type from its definition. For now, use the spell scope
	// from the MagicSubsystem as a heuristic.
	//
	// Spells with scope Overworld are typically TileTarget or CityTarget.
	// Combat scope spells are UnitTarget.
	// Global scope spells are NoTarget.
	//
	// TODO: Replace with proper spell data asset lookup when the spell
	// database is fully populated.

	// Default to TileTarget for overworld spells. The spell data asset
	// should define ECoMSpellTargetType directly in a future sprint.
	return ECoMSpellTargetType::TileTarget;
}

void UCoMSpellTargetingWidget::OnCancelButtonClicked()
{
	CancelTargeting();
}

// =============================================================================
// Subsystem Accessors
// =============================================================================

UCoMMagicSubsystem* UCoMSpellTargetingWidget::GetMagicSubsystem()
{
	if (CachedMagicSub.IsValid()) { return CachedMagicSub.Get(); }
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI) { CachedMagicSub = GI->GetSubsystem<UCoMMagicSubsystem>(); return CachedMagicSub.Get(); }
	return nullptr;
}

UCoMWorldMapSubsystem* UCoMSpellTargetingWidget::GetWorldMapSubsystem()
{
	if (CachedWorldMapSub.IsValid()) { return CachedWorldMapSub.Get(); }
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI) { CachedWorldMapSub = GI->GetSubsystem<UCoMWorldMapSubsystem>(); return CachedWorldMapSub.Get(); }
	return nullptr;
}

UCoMUnitSubsystem* UCoMSpellTargetingWidget::GetUnitSubsystem()
{
	if (CachedUnitSub.IsValid()) { return CachedUnitSub.Get(); }
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI) { CachedUnitSub = GI->GetSubsystem<UCoMUnitSubsystem>(); return CachedUnitSub.Get(); }
	return nullptr;
}

UCoMCitySubsystem* UCoMSpellTargetingWidget::GetCitySubsystem()
{
	if (CachedCitySub.IsValid()) { return CachedCitySub.Get(); }
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI) { CachedCitySub = GI->GetSubsystem<UCoMCitySubsystem>(); return CachedCitySub.Get(); }
	return nullptr;
}
