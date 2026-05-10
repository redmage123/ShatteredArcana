// Copyright Mythforge Studios. All Rights Reserved.

#include "Overworld/CoMSiteIconActor.h"
#include "CoMCore/CoreTypes/CoMStructs.h"

#include "Components/BillboardComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* SiteShortLabel(ECoMSiteType T)
	{
		switch (T)
		{
		case ECoMSiteType::AncientRuins:        return TEXT("Ruins");
		case ECoMSiteType::ForgottenTemple:     return TEXT("Temple");
		case ECoMSiteType::BurialMound:         return TEXT("Mound");
		case ECoMSiteType::TowerOfWizardry:     return TEXT("Tower");
		case ECoMSiteType::ManaNode:            return TEXT("Node");
		case ECoMSiteType::DragonLair:          return TEXT("Lair");
		case ECoMSiteType::DemonGate:           return TEXT("Gate");
		case ECoMSiteType::FeyGlade:            return TEXT("Glade");
		case ECoMSiteType::CrystalCavern:       return TEXT("Cavern");
		case ECoMSiteType::VolcanicForge:       return TEXT("Forge");
		case ECoMSiteType::SunkenCity:          return TEXT("Sunken");
		case ECoMSiteType::FlyingCitadel:       return TEXT("Citadel");
		case ECoMSiteType::LibraryOfTheLost:    return TEXT("Library");
		case ECoMSiteType::ColiseumOfChampions: return TEXT("Coliseum");
		case ECoMSiteType::MerchantVault:       return TEXT("Vault");
		case ECoMSiteType::OraclesSpire:        return TEXT("Spire");
		default:                                return TEXT("Site");
		}
	}

	FColor RealmColor(ECoMSpellRealm Realm)
	{
		switch (Realm)
		{
		case ECoMSpellRealm::Life:    return FColor(255, 250, 220);
		case ECoMSpellRealm::Death:   return FColor(120, 0, 120);
		case ECoMSpellRealm::Chaos:   return FColor(255, 50, 0);
		case ECoMSpellRealm::Nature:  return FColor(0, 200, 0);
		case ECoMSpellRealm::Sorcery: return FColor(60, 100, 255);
		case ECoMSpellRealm::Arcane:  return FColor(220, 160, 0);
		case ECoMSpellRealm::Binding: return FColor(160, 0, 0);
		case ECoMSpellRealm::Spirit:  return FColor(160, 100, 255);
		case ECoMSpellRealm::Glamour: return FColor(255, 100, 200);
		default:                      return FColor::White;
		}
	}
}

ACoMSiteIconActor::ACoMSiteIconActor()
{
	PrimaryActorTick.bCanEverTick = false;

	IconSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("SiteIcon"));
	SetRootComponent(IconSprite);
	IconSprite->SetHiddenInGame(false);
	IconSprite->SetRelativeScale3D(FVector(1.6f, 1.6f, 1.6f));

	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultIcon(
		TEXT("/Engine/EditorResources/S_Note.S_Note"));
	if (DefaultIcon.Succeeded())
	{
		IconSprite->SetSprite(DefaultIcon.Object);
	}

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SiteLabel"));
	Label->SetupAttachment(IconSprite);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, -36.0f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(20.0f);
	Label->SetTextRenderColor(FColor(220, 200, 130));
}

void ACoMSiteIconActor::InitFromSite(const FCoMSite& Site)
{
	SiteID       = Site.SiteID;
	bIsManaNode  = false;

	const FString Body = Site.bCleared
		? FString::Printf(TEXT("%s (cleared)"), SiteShortLabel(Site.Type))
		: FString::Printf(TEXT("%s [%d]"), SiteShortLabel(Site.Type), Site.GuardPower);
	Label->SetText(FText::FromString(Body));

	// Cleared sites are dim grey; uncleared use site-specific tint.
	if (Site.bCleared)
	{
		Label->SetTextRenderColor(FColor(120, 120, 120));
	}
	else
	{
		Label->SetTextRenderColor(FColor(230, 200, 100));
	}
}

void ACoMSiteIconActor::InitFromManaNode(FIntPoint Position, ECoMSpellRealm Realm,
                                          int32 OwnerWizardIndex, bool bGuarded)
{
	SiteID      = -1;
	bIsManaNode = true;

	FString Body = TEXT("Node");
	if (OwnerWizardIndex >= 0)
	{
		Body = FString::Printf(TEXT("Node (W%d)"), OwnerWizardIndex);
	}
	else if (bGuarded)
	{
		Body = TEXT("Node (guarded)");
	}
	Label->SetText(FText::FromString(Body));
	Label->SetTextRenderColor(RealmColor(Realm));
}
