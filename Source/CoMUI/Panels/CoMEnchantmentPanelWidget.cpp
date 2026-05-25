// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMEnchantmentPanelWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/Data/CoMSpellDatabase.h"
#include "CoMCore/Data/CoMGlobalEnchantmentData.h"

namespace EnchColors
{
	static const FLinearColor PanelBg = FLinearColor(0.035f, 0.025f, 0.080f, 0.97f);
	static const FLinearColor RowBg    = FLinearColor(0.055f, 0.040f, 0.110f, 0.95f);
	static const FLinearColor Gold      = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver    = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor Grey       = FLinearColor(0.55f, 0.55f, 0.62f, 1.0f);
	static const FLinearColor CancelRed = FLinearColor(0.45f, 0.08f, 0.08f, 1.0f);
}

void UCoMEnchantRow::OnCancelClicked()
{
	if (Owner.IsValid())
	{
		Owner->CancelEnchantment(OwnerWizard, SpellID);
	}
}

TSharedRef<SWidget> UCoMEnchantmentPanelWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMEnchantmentPanelWidget::Configure(int32 InViewerWizardIndex)
{
	ViewerWizardIndex = InViewerWizardIndex;
	Refresh();
}

void UCoMEnchantmentPanelWidget::BuildLayout()
{
	// Nesting: gold border -> sized box -> inner panel border -> content VBox.
	UBorder* OuterGold = WidgetTree->ConstructWidget<UBorder>();
	OuterGold->SetBrushColor(EnchColors::Gold);
	OuterGold->SetPadding(FMargin(2.0f));
	WidgetTree->RootWidget = OuterGold;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(520.0f);
	Size->SetHeightOverride(640.0f);
	OuterGold->AddChild(Size);

	UBorder* Inner = WidgetTree->ConstructWidget<UBorder>();
	Inner->SetBrushColor(EnchColors::PanelBg);
	Inner->SetPadding(FMargin(14.0f, 12.0f));
	Size->AddChild(Inner);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
	Inner->AddChild(Root);

	// Title row.
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetText(FText::FromString(TEXT("Global Enchantments")));
	Title->SetColorAndOpacity(FSlateColor(EnchColors::Gold));
	{
		FSlateFontInfo F = Title->GetFont();
		F.Size = 22;
		Title->SetFont(F);
	}
	if (UVerticalBoxSlot* TS = Root->AddChildToVerticalBox(Title))
	{
		TS->SetPadding(FMargin(0, 0, 0, 8));
	}

	// Scrollable list.
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	if (UVerticalBoxSlot* SS = Root->AddChildToVerticalBox(Scroll))
	{
		SS->SetSize(ESlateSizeRule::Fill);
	}

	ListBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(ListBox);

	// Close button.
	UButton* Close = WidgetTree->ConstructWidget<UButton>();
	Close->OnClicked.AddDynamic(this, &UCoMEnchantmentPanelWidget::OnCloseClicked);
	UTextBlock* CloseLbl = WidgetTree->ConstructWidget<UTextBlock>();
	CloseLbl->SetText(FText::FromString(TEXT("Close")));
	CloseLbl->SetColorAndOpacity(FSlateColor(EnchColors::Silver));
	Close->AddChild(CloseLbl);
	if (UVerticalBoxSlot* CS = Root->AddChildToVerticalBox(Close))
	{
		CS->SetPadding(FMargin(0, 8, 0, 0));
		CS->SetHorizontalAlignment(HAlign_Center);
	}

	BuildRows();
}

void UCoMEnchantmentPanelWidget::BuildRows()
{
	if (!ListBox) { return; }
	ListBox->ClearChildren();
	Rows.Reset();

	UGameInstance* GI = GetGameInstance();
	UCoMMagicSubsystem* Magic = GI ? GI->GetSubsystem<UCoMMagicSubsystem>() : nullptr;
	if (!Magic) { return; }

	TArray<int32> Owners;
	TArray<FName>  SpellIDs;
	Magic->GetAllActiveEnchantments(Owners, SpellIDs);

	if (SpellIDs.Num() == 0)
	{
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>();
		Empty->SetText(FText::FromString(TEXT("No global enchantments are active.")));
		Empty->SetColorAndOpacity(FSlateColor(EnchColors::Grey));
		ListBox->AddChildToVerticalBox(Empty);
		return;
	}

	for (int32 i = 0; i < SpellIDs.Num(); ++i)
	{
		const int32 OwnerW = Owners[i];
		const FName  SpellID = SpellIDs[i];
		const FCoMGlobalEnchantmentDef& Def = CoMGlobalEnchantmentData::Get(SpellID);
		const FCoMSpellInfo Info = CoMSpellDatabase::GetSpellInfo(SpellID);

		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>();
		RowBorder->SetBrushColor(EnchColors::RowBg);
		RowBorder->SetPadding(FMargin(8.0f, 6.0f));
		if (UVerticalBoxSlot* RS = ListBox->AddChildToVerticalBox(RowBorder))
		{
			RS->SetPadding(FMargin(0, 0, 0, 6));
		}

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		RowBorder->AddChild(Row);

		// Card art.
		UImage* Card = WidgetTree->ConstructWidget<UImage>();
		const FString TexPath = FString::Printf(
			TEXT("/Game/UI/Enchantments/enchant_%s.enchant_%s"),
			*Def.CardImageSlug, *Def.CardImageSlug);
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *TexPath))
		{
			Card->SetBrushFromTexture(Tex);
		}
		Card->SetDesiredSizeOverride(FVector2D(64.0f, 94.0f));
		if (UHorizontalBoxSlot* CSlot = Row->AddChildToHorizontalBox(Card))
		{
			CSlot->SetPadding(FMargin(0, 0, 10, 0));
			CSlot->SetVerticalAlignment(VAlign_Center);
		}

		// Text column.
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
		if (UHorizontalBoxSlot* ColSlot = Row->AddChildToHorizontalBox(Col))
		{
			ColSlot->SetSize(ESlateSizeRule::Fill);
			ColSlot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
		Name->SetText(Def.IsValid() ? Def.DisplayName : FText::FromName(SpellID));
		Name->SetColorAndOpacity(FSlateColor(EnchColors::Gold));
		Col->AddChildToVerticalBox(Name);

		UTextBlock* Sub = WidgetTree->ConstructWidget<UTextBlock>();
		const FString Who = (OwnerW == ViewerWizardIndex)
			? TEXT("You") : FString::Printf(TEXT("Wizard %d"), OwnerW);
		const int32 Upkeep = (Info.UpkeepMana > 0) ? Info.UpkeepMana : 5;
		Sub->SetText(FText::FromString(FString::Printf(
			TEXT("%s  -  upkeep %d mana/turn"), *Who, Upkeep)));
		Sub->SetColorAndOpacity(FSlateColor(EnchColors::Silver));
		Col->AddChildToVerticalBox(Sub);

		UTextBlock* Flavor = WidgetTree->ConstructWidget<UTextBlock>();
		Flavor->SetText(Def.FlavorText);
		Flavor->SetColorAndOpacity(FSlateColor(EnchColors::Grey));
		Flavor->SetAutoWrapText(true);
		Col->AddChildToVerticalBox(Flavor);

		// Cancel button (only for the viewer's own enchantments).
		if (OwnerW == ViewerWizardIndex)
		{
			UCoMEnchantRow* RowObj = NewObject<UCoMEnchantRow>(this);
			RowObj->Owner       = this;
			RowObj->OwnerWizard = OwnerW;
			RowObj->SpellID     = SpellID;
			Rows.Add(RowObj);

			UButton* Cancel = WidgetTree->ConstructWidget<UButton>();
			Cancel->SetBackgroundColor(EnchColors::CancelRed);
			Cancel->OnClicked.AddDynamic(RowObj, &UCoMEnchantRow::OnCancelClicked);
			UTextBlock* CancelLbl = WidgetTree->ConstructWidget<UTextBlock>();
			CancelLbl->SetText(FText::FromString(TEXT("Cancel")));
			CancelLbl->SetColorAndOpacity(FSlateColor(EnchColors::Silver));
			Cancel->AddChild(CancelLbl);
			if (UHorizontalBoxSlot* BSlot = Row->AddChildToHorizontalBox(Cancel))
			{
				BSlot->SetVerticalAlignment(VAlign_Center);
				BSlot->SetHorizontalAlignment(HAlign_Right);
			}
		}
	}
}

void UCoMEnchantmentPanelWidget::Refresh()
{
	BuildRows();
}

void UCoMEnchantmentPanelWidget::CancelEnchantment(int32 OwnerWizard, FName SpellID)
{
	UGameInstance* GI = GetGameInstance();
	if (UCoMMagicSubsystem* Magic = GI ? GI->GetSubsystem<UCoMMagicSubsystem>() : nullptr)
	{
		Magic->CancelGlobalEnchantment(OwnerWizard, SpellID);
	}
	Refresh();
}

void UCoMEnchantmentPanelWidget::OnCloseClicked()
{
	RemoveFromParent();
}

// Console: open the panel directly (no Blueprint class needed).
//   com.show_enchantments <viewerWizard=0>
static FAutoConsoleCommandWithWorldAndArgs GShowEnchantmentsCmd(
	TEXT("com.show_enchantments"),
	TEXT("Open the global enchantments panel. Arg: <viewerWizardIndex=0>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) { return; }
			const int32 Viewer = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 0;
			UCoMEnchantmentPanelWidget* W = CreateWidget<UCoMEnchantmentPanelWidget>(
				World, UCoMEnchantmentPanelWidget::StaticClass());
			if (W)
			{
				W->Configure(Viewer);
				W->AddToViewport(100);
			}
		}));
