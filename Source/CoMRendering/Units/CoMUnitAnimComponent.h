// Copyright Mythforge Studios. All Rights Reserved.
// CoMUnitAnimComponent.h — Component managing unit animation state on the overworld.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoMUnitAnimComponent.generated.h"

class USkeletalMeshComponent;

/**
 * Unit animation states for overworld and tactical combat.
 */
UENUM(BlueprintType)
enum class ECoMUnitAnimState : uint8
{
	Idle         UMETA(DisplayName = "Idle"),
	Walking      UMETA(DisplayName = "Walking"),
	Running      UMETA(DisplayName = "Running"),
	Attacking    UMETA(DisplayName = "Attacking"),
	Blocking     UMETA(DisplayName = "Blocking"),
	CastingSpell UMETA(DisplayName = "Casting Spell"),
	Hit          UMETA(DisplayName = "Hit Reaction"),
	Dying        UMETA(DisplayName = "Dying"),
	Dead         UMETA(DisplayName = "Dead"),
	Crouching    UMETA(DisplayName = "Crouching"),
	Turning      UMETA(DisplayName = "Turning"),
	Strafing     UMETA(DisplayName = "Strafing"),

	MAX UMETA(Hidden)
};

/**
 * UCoMUnitAnimComponent
 *
 * Manages animation playback for a single unit actor. Handles state transitions,
 * animation montage selection, and blending between animation variants.
 *
 * Attach to any actor with a USkeletalMeshComponent.
 * Call SetAnimState() to trigger transitions. The component selects the
 * appropriate animation asset and plays it via the skeletal mesh.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class COMRENDERING_API UCoMUnitAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoMUnitAnimComponent();

	virtual void BeginPlay() override;

	// ── State Control ────────────────────────────────────────────────────

	/** Set the current animation state. Triggers a transition if different from current. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void SetAnimState(ECoMUnitAnimState NewState);

	/** Get the current animation state. */
	UFUNCTION(BlueprintPure, Category = "CoM|Animation")
	ECoMUnitAnimState GetAnimState() const { return CurrentState; }

	/** Play a specific attack variant (0-based index). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void PlayAttackVariant(int32 Variant);

	/** Play a death animation and lock state. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void PlayDeath(int32 Variant = 0);

	/** Play a hit reaction, then return to previous state. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void PlayHitReaction(int32 Variant = 0);

	/** Play a spell casting animation. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void PlayCastSpell(int32 Variant = 0);

	// ── Configuration ────────────────────────────────────────────────────

	/** Animation asset paths by category. Populated at BeginPlay from the animation library. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|Animation")
	TMap<FName, UAnimationAsset*> AnimationLibrary;

	/** Playback speed multiplier for the current animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|Animation")
	float PlaybackSpeed = 1.0f;

	// ── Audio Integration ────────────────────────────────────────────────

	/** Play footstep sound. Called from animation notifies or at regular intervals during walk/run. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void PlayFootstepSound();

	/** Play weapon swing sound. Called during attack animations. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void PlaySwingSound();

	/** Play impact sound. Called when attack connects. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void PlayImpactSound();

	/** Play death sound. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void PlayDeathSound();

	/** Play spell casting sound. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Animation")
	void PlayCastSound();

private:
	/** Current animation state. */
	ECoMUnitAnimState CurrentState = ECoMUnitAnimState::Idle;

	/** Previous state (for returning after hit reactions). */
	ECoMUnitAnimState PreviousState = ECoMUnitAnimState::Idle;

	/** Whether the unit is dead (locks out other state changes). */
	bool bIsDead = false;

	/** Cached skeletal mesh component. */
	UPROPERTY()
	USkeletalMeshComponent* CachedMesh = nullptr;

	/** Find and cache the skeletal mesh component on the owning actor. */
	USkeletalMeshComponent* GetMesh();

	/** Load an animation asset by category name. Returns nullptr if not found. */
	UAnimationAsset* GetAnimation(const FName& CategoryName) const;

	/** Select a random variant for a given category prefix. */
	FName SelectVariant(const FString& Prefix) const;

	/** Play a sound effect from the SFX library. */
	void PlaySFX(const FString& SoundPath);

	/** Populate the animation library from Content/Characters/Knight/Animations/. */
	void LoadAnimationLibrary();
};
