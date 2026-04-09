// Copyright Mythforge Studios. All Rights Reserved.
// CoMPlayerController.h — Human and AI player controllers. COM-032
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CoMPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * ACoMHumanPlayerController
 *
 * Controller for human wizard players.  Responsibilities:
 *  - Register DefaultMappingContext with the Enhanced Input subsystem on possess
 *  - Possess ACoMCameraPawn and route camera input
 *  - Stub: EndTurn input action (full UI wire-up in Sprint 2)
 *
 * Input asset references (DefaultMappingContext, IA_EndTurn) are left null
 * by default and must be assigned via editor Blueprint subclass or project
 * DefaultGame.ini after IMC assets are created in Sprint 2.
 */
UCLASS()
class COMCORE_API ACoMHumanPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACoMHumanPlayerController();

	// ─── Enhanced Input assets (assign in BP or DefaultGame.ini) ─────────────

	/** Primary mapping context applied on possess. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Enhanced Input action for the End Turn command. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_EndTurn;

	/** Enhanced Input action for QuickSave (F5). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_QuickSave;

	/** Enhanced Input action for QuickLoad (F9). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_QuickLoad;

	/** Priority for DefaultMappingContext; raise to override UI contexts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 StartupInputPriority = 0;

protected:
	virtual void BeginPlay() override;

	/** Register DefaultMappingContext with UEnhancedInputLocalPlayerSubsystem. */
	virtual void OnPossess(APawn* InPawn) override;

	/** Bind Enhanced Input actions to handler functions. */
	virtual void SetupInputComponent() override;

private:
	/** Stub: triggered by IA_EndTurn.  Full logic (validation + subsystem call) in Sprint 2. */
	void Input_EndTurn(const FInputActionValue& Value);

	/** Triggered by IA_QuickSave (F5). Saves game to the QuickSave slot. */
	void Input_QuickSave(const FInputActionValue& Value);

	/** Triggered by IA_QuickLoad (F9). Loads game from the QuickSave slot. */
	void Input_QuickLoad(const FInputActionValue& Value);
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * ACoMAIPlayerController
 *
 * Stub controller for AI-driven wizard players.
 * The AI decision subsystem is wired in Phase 12 (Sprint 6+).
 * Exists now so GameMode can assign AI slots without a Blueprint.
 */
UCLASS()
class COMCORE_API ACoMAIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACoMAIPlayerController();

	// TODO (Phase 12): Wire UCoMAIBrainSubsystem here.
};
