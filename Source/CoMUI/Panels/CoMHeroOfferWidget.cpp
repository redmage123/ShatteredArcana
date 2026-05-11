// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMHeroOfferWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

#include "CoMUI/CoMUISubsystem.h"

#define LOCTEXT_NAMESPACE "CoMHeroOffer"

namespace OfferColours
{
	static const FLinearColor BgDark   = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f);
	static const FLinearColor Panel    = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold     = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver   = FLinearColor(0.816f, 0.816f, 0.863f, 1.0f);
	static const FLinearColor Grey     = FLinearColor(0.500f, 0.500f, 0.550f, 1.0f);
	static const FLinearColor Btn      = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor Tile     = FLinearColor(0.10f, 0.08f, 0.18f, 0.90f);
	static const FLinearColor Green    = FLinearColor(0.55f, 0.85f, 0.40f, 1.0f);
	static const FLinearColor Red      = FLinearColor(0.85f, 0.30f, 0.30f, 1.0f);
}

void UCoMHeroOfferRow::HandleClick()
{
	if (Owner.IsValid()) { Owner->HandleRowClick(OfferID, bHire); }
}

TSharedRef<SWidget> UCoMHeroOfferWidget::RebuildWidget()
{
	if (WidgetTree) { BuildLayout(); }
	return Super::RebuildWidget();
}

void UCoMHeroOfferWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	if (CloseButton) { CloseButton->OnClicked.AddDynamic(this, &UCoMHeroOfferWidget::OnCloseClicked); }
}

void UCoMHeroOfferWidget::Configure(int32 InWizardIndex)
{
	WizardIndex = InWizardIndex;
	RebuildList();
}

void UCoMHeroOfferWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

void UCoMHeroOfferWidget::HandleRowClick(int32 OfferID, bool bHire)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMHeroSubsystem* Heroes = GI->GetSubsystem<UCoMHeroSubsystem>();
	if (!Heroes) return;

	if (bHire) { Heroes->AcceptHeroOffer(OfferID); }
	else       { Heroes->DeclineHeroOffer(OfferID); }
	RebuildList();
}

void UCoMHeroOfferWidget::RebuildList()
{
	if (!ListScroll) return;
	ListScroll->ClearChildren();
	RowHelpers.Reset();

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMHeroSubsystem* Heroes = GI->GetSubsystem<UCoMHeroSubsystem>();
	if (!Heroes) return;

	const TArray<FCoMHeroOffer> Offers = Heroes->GetOffersForWizard(WizardIndex);

	if (Offers.Num() == 0)
	{
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>();
		Empty->SetText(LOCTEXT("NoOffers", "No tavern offers right now. Build fame to attract heroes."));
		Empty->SetColorAndOpacity(FSlateColor(OfferColours::Grey));
		{ FSlateFontInfo F = Empty->GetFont(); F.Size = 13; Empty->SetFont(F); }
		ListScroll->AddChild(Empty);
		return;
	}

	auto MakeBtn = [this](const FString& Label, FLinearColor LblColor) -> UButton*
	{
		UButton* B = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle S = B->GetStyle();
		S.Normal.DrawAs    = ESlateBrushDrawType::Box;
		S.Normal.TintColor = FSlateColor(OfferColours::Btn);
		S.Hovered.DrawAs   = ESlateBrushDrawType::Box;
		S.Hovered.TintColor = FSlateColor(OfferColours::BtnHover);
		S.Pressed.DrawAs   = ESlateBrushDrawType::Box;
		S.Pressed.TintColor = FSlateColor(OfferColours::Btn);
		B->SetStyle(S);

		UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
		L->SetText(FText::FromString(Label));
		L->SetColorAndOpacity(FSlateColor(LblColor));
		L->SetJustification(ETextJustify::Center);
		{ FSlateFontInfo F = L->GetFont(); F.Size = 13; L->SetFont(F); }
		B->AddChild(L);
		return B;
	};

	for (const FCoMHeroOffer& O : Offers)
	{
		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>();
		RowBorder->SetBrushColor(OfferColours::Tile);
		RowBorder->SetPadding(FMargin(10, 8));

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		RowBorder->AddChild(Row);

		// Identity column
		UVerticalBox* Ident = WidgetTree->ConstructWidget<UVerticalBox>();

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
		Name->SetText(FText::FromString(O.HeroName));
		Name->SetColorAndOpacity(FSlateColor(OfferColours::Gold));
		{ FSlateFontInfo F = Name->GetFont(); F.Size = 15; Name->SetFont(F); }
		Ident->AddChildToVerticalBox(Name);

		const FString TierStr = StaticEnum<ECoMHeroTier>()
			->GetDisplayNameTextByValue((int64)O.Tier).ToString();
		const FString ClassStr = StaticEnum<ECoMHeroClass>()
			->GetDisplayNameTextByValue((int64)O.HeroClass).ToString();
		UTextBlock* Sub = WidgetTree->ConstructWidget<UTextBlock>();
		Sub->SetText(FText::FromString(FString::Printf(
			TEXT("%s — %s    %d gold"), *TierStr, *ClassStr, O.GoldCost)));
		Sub->SetColorAndOpacity(FSlateColor(OfferColours::Silver));
		{ FSlateFontInfo F = Sub->GetFont(); F.Size = 12; Sub->SetFont(F); }
		Ident->AddChildToVerticalBox(Sub);

		UHorizontalBoxSlot* IS = Row->AddChildToHorizontalBox(Ident);
		if (IS) { IS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); IS->SetVerticalAlignment(VAlign_Center); }

		// Buttons
		USizeBox* HireSize = WidgetTree->ConstructWidget<USizeBox>();
		HireSize->SetWidthOverride(96.0f); HireSize->SetHeightOverride(34.0f);
		UButton* HireBtn = MakeBtn(TEXT("Hire"), OfferColours::Green);
		HireSize->AddChild(HireBtn);
		Row->AddChildToHorizontalBox(HireSize);

		USizeBox* DeclSize = WidgetTree->ConstructWidget<USizeBox>();
		DeclSize->SetWidthOverride(96.0f); DeclSize->SetHeightOverride(34.0f);
		UButton* DeclBtn = MakeBtn(TEXT("Decline"), OfferColours::Red);
		DeclSize->AddChild(DeclBtn);
		UHorizontalBoxSlot* DS = Row->AddChildToHorizontalBox(DeclSize);
		if (DS) { DS->SetPadding(FMargin(6, 0, 0, 0)); }

		UCoMHeroOfferRow* HireHelper = NewObject<UCoMHeroOfferRow>(this);
		HireHelper->Owner   = this;
		HireHelper->OfferID = O.OfferID;
		HireHelper->bHire   = true;
		RowHelpers.Add(HireHelper);
		HireBtn->OnClicked.AddDynamic(HireHelper, &UCoMHeroOfferRow::HandleClick);

		UCoMHeroOfferRow* DeclHelper = NewObject<UCoMHeroOfferRow>(this);
		DeclHelper->Owner   = this;
		DeclHelper->OfferID = O.OfferID;
		DeclHelper->bHire   = false;
		RowHelpers.Add(DeclHelper);
		DeclBtn->OnClicked.AddDynamic(DeclHelper, &UCoMHeroOfferRow::HandleClick);

		ListScroll->AddChild(RowBorder);
	}
}

void UCoMHeroOfferWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(OfferColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0));

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(Root);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(OfferColours::Panel);
	Panel->SetPadding(FMargin(40, 28));
	UOverlaySlot* PS = Root->AddChildToOverlay(Panel);
	if (PS) { PS->SetHorizontalAlignment(HAlign_Fill); PS->SetVerticalAlignment(VAlign_Fill); }

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChild(Col);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	TitleText->SetText(LOCTEXT("OfferTitle", "Tavern Offers"));
	TitleText->SetColorAndOpacity(FSlateColor(OfferColours::Gold));
	TitleText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = TitleText->GetFont(); F.Size = 28; TitleText->SetFont(F); }
	UVerticalBoxSlot* TS = Col->AddChildToVerticalBox(TitleText);
	if (TS) { TS->SetHorizontalAlignment(HAlign_Center); TS->SetPadding(FMargin(0, 0, 0, 16)); }

	ListScroll = WidgetTree->ConstructWidget<UScrollBox>();
	ListScroll->SetOrientation(Orient_Vertical);
	UVerticalBoxSlot* LS = Col->AddChildToVerticalBox(ListScroll);
	if (LS) { LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

	// Bottom close.
	UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
	USizeBox* CSize = WidgetTree->ConstructWidget<USizeBox>();
	CSize->SetWidthOverride(140.0f); CSize->SetHeightOverride(38.0f);
	CloseButton = WidgetTree->ConstructWidget<UButton>();
	{
		FButtonStyle S = CloseButton->GetStyle();
		S.Normal.DrawAs    = ESlateBrushDrawType::Box;
		S.Normal.TintColor = FSlateColor(OfferColours::Btn);
		S.Hovered.DrawAs   = ESlateBrushDrawType::Box;
		S.Hovered.TintColor = FSlateColor(OfferColours::BtnHover);
		S.Pressed.DrawAs   = ESlateBrushDrawType::Box;
		S.Pressed.TintColor = FSlateColor(OfferColours::Btn);
		CloseButton->SetStyle(S);

		UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
		L->SetText(LOCTEXT("OfferClose", "Close"));
		L->SetColorAndOpacity(FSlateColor(OfferColours::Silver));
		L->SetJustification(ETextJustify::Center);
		{ FSlateFontInfo F = L->GetFont(); F.Size = 13; L->SetFont(F); }
		CloseButton->AddChild(L);
	}
	CSize->AddChild(CloseButton);
	Bottom->AddChildToHorizontalBox(CSize);
	UVerticalBoxSlot* BS = Col->AddChildToVerticalBox(Bottom);
	if (BS) { BS->SetHorizontalAlignment(HAlign_Right); BS->SetPadding(FMargin(0, 14, 0, 0)); }

	if (WidgetTree) { WidgetTree->RootWidget = BackgroundBorder; }
}

#undef LOCTEXT_NAMESPACE
