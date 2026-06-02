// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMItemVaultWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

#include "CoMUI/CoMUISubsystem.h"
#include "CoMCore/Items/CoMItemSubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"

#define LOCTEXT_NAMESPACE "CoMItemVault"

namespace VaultColours
{
	static const FLinearColor BgDark    = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f);
	static const FLinearColor PanelBg   = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold      = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver    = FLinearColor(0.816f, 0.816f, 0.863f, 1.0f);
	static const FLinearColor Grey      = FLinearColor(0.500f, 0.500f, 0.550f, 1.0f);
	static const FLinearColor TileEmpty = FLinearColor(0.10f, 0.08f, 0.18f, 0.85f);
	static const FLinearColor TileEquip = FLinearColor(0.16f, 0.13f, 0.22f, 0.95f);
	static const FLinearColor BtnNormal = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover  = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPress  = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);
}

void UCoMVaultItemRow::HandleClick()
{
	if (Owner.IsValid())
	{
		Owner->HandleItemClicked(InstanceID);
	}
}

void UCoMVaultItemRow::HandleDestroyClick()
{
	if (Owner.IsValid())
	{
		Owner->HandleDestroyClicked(InstanceID);
	}
}

void UCoMVaultItemRow::HandleRenameCommit(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType != ETextCommit::OnEnter) return;
	if (Owner.IsValid())
	{
		Owner->HandleRenameCommitted(InstanceID, Text);
	}
}

// Per-realm glow tints (CoM convention).
static FLinearColor RealmGlowColour(ECoMSpellRealm Realm)
{
	switch (Realm)
	{
		case ECoMSpellRealm::Life:    return FLinearColor(1.00f, 0.85f, 0.30f, 1.0f);  // gold
		case ECoMSpellRealm::Death:   return FLinearColor(0.55f, 0.20f, 0.75f, 1.0f);  // violet
		case ECoMSpellRealm::Chaos:   return FLinearColor(0.95f, 0.25f, 0.20f, 1.0f);  // red
		case ECoMSpellRealm::Nature:  return FLinearColor(0.30f, 0.85f, 0.35f, 1.0f);  // green
		case ECoMSpellRealm::Sorcery: return FLinearColor(0.30f, 0.55f, 0.95f, 1.0f);  // blue
		case ECoMSpellRealm::Spirit:  return FLinearColor(0.80f, 0.95f, 1.00f, 1.0f);  // pale cyan
		case ECoMSpellRealm::Binding: return FLinearColor(0.65f, 0.10f, 0.40f, 1.0f);  // magenta
		case ECoMSpellRealm::Glamour: return FLinearColor(1.00f, 0.55f, 0.85f, 1.0f);  // fey pink
		case ECoMSpellRealm::Arcane:
		default:                      return FLinearColor(0.85f, 0.85f, 0.90f, 1.0f);  // silver
	}
}

// =============================================================================
// Lifecycle
// =============================================================================

TSharedRef<SWidget> UCoMItemVaultWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMItemVaultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton) { CloseButton->OnClicked.AddDynamic(this, &UCoMItemVaultWidget::OnCloseClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMItemVaultWidget::Configure(int32 InOwnerWizardIndex)
{
	OwnerWizardIndex = InOwnerWizardIndex;
	RebuildList();
}

void UCoMItemVaultWidget::SetTargetHero(int32 HeroUnitID)
{
	TargetHeroUnitID = HeroUnitID;
	if (SubtitleText)
	{
		SubtitleText->SetText(HeroUnitID == 0
			? LOCTEXT("VaultPreview", "Preview-only — open from a hero screen to equip")
			: FText::Format(LOCTEXT("VaultEquipFor", "Click an item to equip on Hero #{0}"), FText::AsNumber(HeroUnitID)));
	}
	RebuildList();
}

void UCoMItemVaultWidget::HandleItemClicked(int32 InstanceID)
{
	if (TargetHeroUnitID == 0) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>();
	if (!Items) return;

	Items->EquipItem(TargetHeroUnitID, InstanceID);
	RebuildList();
}

void UCoMItemVaultWidget::HandleDestroyClicked(int32 InstanceID)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMItemSubsystem*  Items = GI->GetSubsystem<UCoMItemSubsystem>();
	UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>();
	if (!Items) return;

	FCoMItemInstance Snapshot;
	if (!Items->GetItem(InstanceID, Snapshot)) return;
	const int32 Refund = Items->DestroyItemForMana(InstanceID);
	if (Refund > 0 && Magic && Snapshot.OwnerWizardIndex >= 0)
	{
		Magic->GetWizardMagic(Snapshot.OwnerWizardIndex).CurrentMana += Refund;
	}
	RebuildList();
}

void UCoMItemVaultWidget::HandleRenameCommitted(int32 InstanceID, const FText& NewName)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>();
	if (!Items) return;
	if (Items->RenameItem(InstanceID, NewName))
	{
		RebuildList();
	}
}

void UCoMItemVaultWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

// =============================================================================
// List builder
// =============================================================================

void UCoMItemVaultWidget::RebuildList()
{
	if (!ItemListScroll) return;
	ItemListScroll->ClearChildren();
	RowHelpers.Reset();

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>();
	if (!Items) return;

	const TArray<FCoMItemInstance> Owned = Items->GetItemsForWizard(OwnerWizardIndex);
	if (Owned.Num() == 0)
	{
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>();
		Empty->SetText(LOCTEXT("VaultEmpty", "Vault is empty. Cast Enchant Item or Create Artifact to forge magical items."));
		Empty->SetColorAndOpacity(FSlateColor(VaultColours::Grey));
		{ FSlateFontInfo F = Empty->GetFont(); F.Size = 14; Empty->SetFont(F); }
		ItemListScroll->AddChild(Empty);
		return;
	}

	for (const FCoMItemInstance& Item : Owned)
	{
		const bool bEquipped = (Item.EquippedByHeroID != 0);
		const bool bForging  = (Item.ForgeTurnsRemaining > 0);
		const FLinearColor GlowTint = RealmGlowColour(Item.DominantRealm);

		// Outer realm-glow border (tints lightly with the dominant realm).
		UBorder* GlowBorder = WidgetTree->ConstructWidget<UBorder>();
		GlowBorder->SetBrushColor(GlowTint * 0.45f + FLinearColor(0,0,0,0.55f));
		GlowBorder->SetPadding(FMargin(3.0f));

		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>();
		RowBorder->SetBrushColor(bEquipped ? VaultColours::TileEquip : VaultColours::TileEmpty);
		RowBorder->SetPadding(FMargin(8.0f, 8.0f));
		GlowBorder->AddChild(RowBorder);

		UHorizontalBox* OuterRow = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Portrait with realm tint.
		{
			USizeBox* PortSize = WidgetTree->ConstructWidget<USizeBox>();
			PortSize->SetWidthOverride(64.f); PortSize->SetHeightOverride(64.f);

			UBorder* PortBorder = WidgetTree->ConstructWidget<UBorder>();
			PortBorder->SetBrushColor(GlowTint);
			PortBorder->SetPadding(FMargin(2.0f));

			UImage* PortImg = WidgetTree->ConstructWidget<UImage>();
			if (!Item.ArtVariant.IsNone())
			{
				const FString TexturePath = FString::Printf(TEXT("/Game/UI/Items/%s.%s"),
					*Item.ArtVariant.ToString(), *Item.ArtVariant.ToString());
				if (UTexture2D* Tex = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath)))
				{
					PortImg->SetBrushFromTexture(Tex);
				}
			}
			PortBorder->AddChild(PortImg);
			PortSize->AddChild(PortBorder);

			UHorizontalBoxSlot* PS = OuterRow->AddChildToHorizontalBox(PortSize);
			if (PS) { PS->SetPadding(FMargin(0, 0, 12, 0)); PS->SetVerticalAlignment(VAlign_Top); }
		}

		UVerticalBox* RowCol = WidgetTree->ConstructWidget<UVerticalBox>();

		// Header: name editable + slot
		{
			UHorizontalBox* HdrRow = WidgetTree->ConstructWidget<UHorizontalBox>();

			UEditableTextBox* NameInput = WidgetTree->ConstructWidget<UEditableTextBox>();
			NameInput->SetText(Item.DisplayName);
			NameInput->SetHintText(LOCTEXT("VaultNameHint", "(rename)"));
			UHorizontalBoxSlot* HS = HdrRow->AddChildToHorizontalBox(NameInput);
			if (HS) { HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); HS->SetVerticalAlignment(VAlign_Center); }

			UTextBlock* SlotTag = WidgetTree->ConstructWidget<UTextBlock>();
			SlotTag->SetText(FText::Format(LOCTEXT("VaultSlotFmt", "[{0}]"),
				FText::FromString(StaticEnum<ECoMItemSlot>()->GetDisplayNameTextByValue((int64)Item.Slot).ToString())));
			SlotTag->SetColorAndOpacity(FSlateColor(VaultColours::Grey));
			{ FSlateFontInfo F = SlotTag->GetFont(); F.Size = 12; SlotTag->SetFont(F); }
			UHorizontalBoxSlot* SS = HdrRow->AddChildToHorizontalBox(SlotTag);
			if (SS) { SS->SetVerticalAlignment(VAlign_Center); SS->SetPadding(FMargin(8, 0, 0, 0)); }

			RowCol->AddChildToVerticalBox(HdrRow);

			// Bind rename commit per-row.
			UCoMVaultItemRow* RenameHelper = NewObject<UCoMVaultItemRow>(this);
			RenameHelper->Owner      = this;
			RenameHelper->InstanceID = Item.InstanceID;
			RowHelpers.Add(RenameHelper);
			NameInput->OnTextCommitted.AddDynamic(RenameHelper, &UCoMVaultItemRow::HandleRenameCommit);
		}

		// Powers list
		for (const FCoMItemPower& P : Item.Powers)
		{
			UTextBlock* Pow = WidgetTree->ConstructWidget<UTextBlock>();
			Pow->SetText(FText::Format(LOCTEXT("VaultPowFmt", "  • {0}"), P.DisplayName));
			Pow->SetColorAndOpacity(FSlateColor(VaultColours::Silver));
			{ FSlateFontInfo F = Pow->GetFont(); F.Size = 12; Pow->SetFont(F); }
			RowCol->AddChildToVerticalBox(Pow);
		}

		// Footer: cost + status (forging vs equipped vs vault)
		{
			UTextBlock* Foot = WidgetTree->ConstructWidget<UTextBlock>();
			FString FootStr;
			if (bForging)
			{
				FootStr = FString::Printf(TEXT("%d mana   |   Forging: %d / %d turns"),
					Item.TotalManaCost, Item.ForgeTurnsTotal - Item.ForgeTurnsRemaining, Item.ForgeTurnsTotal);
			}
			else if (bEquipped)
			{
				FootStr = FString::Printf(TEXT("%d mana   |   Equipped on Hero #%d   |   Cap %d"),
					Item.TotalManaCost, Item.EquippedByHeroID, Item.MaxEnchantments);
			}
			else
			{
				FootStr = FString::Printf(TEXT("%d mana   |   In Vault   |   Cap %d"),
					Item.TotalManaCost, Item.MaxEnchantments);
			}
			Foot->SetText(FText::FromString(FootStr));
			Foot->SetColorAndOpacity(FSlateColor(VaultColours::Gold));
			{ FSlateFontInfo F = Foot->GetFont(); F.Size = 11; Foot->SetFont(F); }
			RowCol->AddChildToVerticalBox(Foot);
		}

		// Action row: Equip (whole-row click handled below) + Destroy.
		{
			UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>();

			USizeBox* DestSz = WidgetTree->ConstructWidget<USizeBox>();
			DestSz->SetWidthOverride(140.f); DestSz->SetHeightOverride(26.f);

			UButton* DestBtn = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle DStyle = DestBtn->GetStyle();
			DStyle.Normal.DrawAs   = ESlateBrushDrawType::Box;
			DStyle.Normal.TintColor = FSlateColor(VaultColours::BtnNormal);
			DStyle.Hovered.DrawAs   = ESlateBrushDrawType::Box;
			DStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.40f, 0.10f, 0.10f, 1.0f));
			DStyle.Pressed.DrawAs   = ESlateBrushDrawType::Box;
			DStyle.Pressed.TintColor = FSlateColor(VaultColours::BtnPress);
			DestBtn->SetStyle(DStyle);

			UTextBlock* DLbl = WidgetTree->ConstructWidget<UTextBlock>();
			const int32 Refund = FMath::Max(0, Item.TotalManaCost / 2);
			DLbl->SetText(FText::Format(LOCTEXT("DestroyFmt", "Destroy ({0} mana)"), FText::AsNumber(Refund)));
			DLbl->SetColorAndOpacity(FSlateColor(VaultColours::Silver));
			DLbl->SetJustification(ETextJustify::Center);
			{ FSlateFontInfo F = DLbl->GetFont(); F.Size = 11; DLbl->SetFont(F); }
			DestBtn->AddChild(DLbl);
			DestBtn->SetIsEnabled(!bEquipped && !bForging);
			DestSz->AddChild(DestBtn);

			UHorizontalBoxSlot* AS = ActionRow->AddChildToHorizontalBox(DestSz);
			if (AS) { AS->SetPadding(FMargin(0, 6, 0, 0)); }

			UCoMVaultItemRow* DestHelper = NewObject<UCoMVaultItemRow>(this);
			DestHelper->Owner      = this;
			DestHelper->InstanceID = Item.InstanceID;
			RowHelpers.Add(DestHelper);
			DestBtn->OnClicked.AddDynamic(DestHelper, &UCoMVaultItemRow::HandleDestroyClick);

			RowCol->AddChildToVerticalBox(ActionRow);
		}

		UHorizontalBoxSlot* CS = OuterRow->AddChildToHorizontalBox(RowCol);
		if (CS) { CS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

		// The whole row is clickable -> equip onto target hero (only when not forging).
		UButton* RowBtn = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style = RowBtn->GetStyle();
		Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(FLinearColor(0,0,0,0));
		Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(FLinearColor(1,1,1,0.05f));
		Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0,0,0,0.20f));
		RowBtn->SetStyle(Style);
		RowBtn->AddChild(OuterRow);
		RowBtn->SetIsEnabled(!bForging);
		RowBorder->AddChild(RowBtn);

		// Per-row click helper (equip).
		UCoMVaultItemRow* Helper = NewObject<UCoMVaultItemRow>(this);
		Helper->Owner      = this;
		Helper->InstanceID = Item.InstanceID;
		RowHelpers.Add(Helper);
		RowBtn->OnClicked.AddDynamic(Helper, &UCoMVaultItemRow::HandleClick);

		ItemListScroll->AddChild(GlowBorder);
	}
}

// =============================================================================
// Layout
// =============================================================================

void UCoMItemVaultWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(VaultColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(Root);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(VaultColours::PanelBg);
	Panel->SetPadding(FMargin(40.0f, 30.0f));
	UOverlaySlot* PSlot = Root->AddChildToOverlay(Panel);
	if (PSlot)
	{
		PSlot->SetHorizontalAlignment(HAlign_Fill);
		PSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChild(Col);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	TitleText->SetText(LOCTEXT("VaultTitle", "Magical Item Vault"));
	TitleText->SetColorAndOpacity(FSlateColor(VaultColours::Gold));
	TitleText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = TitleText->GetFont(); F.Size = 28; TitleText->SetFont(F); }
	UVerticalBoxSlot* TS = Col->AddChildToVerticalBox(TitleText);
	if (TS) { TS->SetHorizontalAlignment(HAlign_Center); TS->SetPadding(FMargin(0, 0, 0, 8)); }

	SubtitleText = WidgetTree->ConstructWidget<UTextBlock>();
	SubtitleText->SetText(LOCTEXT("VaultPreview", "Preview-only — open from a hero screen to equip"));
	SubtitleText->SetColorAndOpacity(FSlateColor(VaultColours::Grey));
	SubtitleText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = SubtitleText->GetFont(); F.Size = 13; SubtitleText->SetFont(F); }
	UVerticalBoxSlot* SS = Col->AddChildToVerticalBox(SubtitleText);
	if (SS) { SS->SetHorizontalAlignment(HAlign_Center); SS->SetPadding(FMargin(0, 0, 0, 16)); }

	ItemListScroll = WidgetTree->ConstructWidget<UScrollBox>();
	ItemListScroll->SetOrientation(Orient_Vertical);
	UVerticalBoxSlot* IS = Col->AddChildToVerticalBox(ItemListScroll);
	if (IS) { IS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

	// Bottom: close
	{
		UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
		USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>();
		Sz->SetWidthOverride(140.0f); Sz->SetHeightOverride(38.0f);

		CloseButton = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style = CloseButton->GetStyle();
		Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(VaultColours::BtnNormal);
		Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(VaultColours::BtnHover);
		Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(VaultColours::BtnPress);
		CloseButton->SetStyle(Style);

		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>();
		Lbl->SetText(LOCTEXT("VaultClose", "Close"));
		Lbl->SetColorAndOpacity(FSlateColor(VaultColours::Silver));
		Lbl->SetJustification(ETextJustify::Center);
		{ FSlateFontInfo F = Lbl->GetFont(); F.Size = 13; Lbl->SetFont(F); }
		CloseButton->AddChild(Lbl);
		Sz->AddChild(CloseButton);
		Bottom->AddChildToHorizontalBox(Sz);

		UVerticalBoxSlot* VS = Col->AddChildToVerticalBox(Bottom);
		if (VS) { VS->SetHorizontalAlignment(HAlign_Right); VS->SetPadding(FMargin(0, 14, 0, 0)); }
	}

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}
}

#undef LOCTEXT_NAMESPACE
