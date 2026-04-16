// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellVFXSubsystem.h -- Spell visual effects subsystem: metadata, playback management.
// Phase 7 -- Shattered Arcana

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMSpellVFXSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// SPELL VFX DEFINITION
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct COMCORE_API FCoMSpellVFXDef
{
	GENERATED_BODY()

	/** Unique ID for this effect (e.g. "chaos_fireball"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName EffectID;

	/** Magic realm this effect belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoMSpellRealm Realm = ECoMSpellRealm::None;

	/** Content path to the horizontal sprite sheet texture (e.g. /Game/Textures/SpellVFX/chaos/fireball_sheet). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SpriteSheetPath;

	/** Number of frames in the sprite sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FrameCount = 8;

	/** Duration of each frame in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FrameDuration = 0.1f;

	/** Display scale multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Scale = 1.0f;

	/** Optional SFX cue ID to play when this effect fires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SoundCueID;

	// ── Effect Category Flags ────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsProjectile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAreaEffect = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsBuff = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsImpact = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// ACTIVE VFX INSTANCE (a currently-playing effect)
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT()
struct FCoMActiveVFX
{
	GENERATED_BODY()

	/** Unique playback handle. */
	UPROPERTY()
	int32 PlaybackID = 0;

	/** Which effect definition this plays. */
	UPROPERTY()
	FName EffectID;

	/** World-space origin. */
	UPROPERTY()
	FVector WorldPosition = FVector::ZeroVector;

	/** World-space target (for projectiles). */
	UPROPERTY()
	FVector TargetPosition = FVector::ZeroVector;

	/** Current frame index. */
	UPROPERTY()
	int32 CurrentFrame = 0;

	/** Time elapsed on the current frame. */
	UPROPERTY()
	float FrameTimeAccumulator = 0.0f;

	/** True while this playback is still running. */
	UPROPERTY()
	bool bIsPlaying = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// DELEGATES
// ─────────────────────────────────────────────────────────────────────────────

/** Broadcast when a playing effect finishes its animation. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpellVFXFinished, FName, EffectID);

/** Broadcast when a new effect is requested (UI layer listens to this). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnSpellVFXRequested,
	int32, PlaybackID,
	FName, EffectID,
	FVector, WorldPosition,
	FVector, TargetPosition);

// ─────────────────────────────────────────────────────────────────────────────
// SPELL VFX SUBSYSTEM
// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class COMCORE_API UCoMSpellVFXSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// -- Lifecycle ────────────────────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Tick all active effects forward. Called from a world timer. */
	void TickEffects(float DeltaTime);

	// -- Effect Registration ──────────────────────────────────────────────────

	/** Register (or overwrite) a spell VFX definition. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellVFX")
	void RegisterEffect(const FCoMSpellVFXDef& Def);

	/** Look up a definition by EffectID. Returns nullptr if not found. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellVFX")
	bool GetEffectDef(FName EffectID, FCoMSpellVFXDef& OutDef) const;

	// -- Playback Control ─────────────────────────────────────────────────────

	/**
	 * Trigger an effect at a world position. Returns a playback handle.
	 * TargetPosition is used only for projectile effects.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellVFX")
	int32 PlayEffect(FName EffectID, FVector WorldPosition, FVector TargetPosition = FVector::ZeroVector);

	/**
	 * Convenience: trigger an effect at a map tile (converts tile to world coords).
	 * Uses a simple tile-size multiplier (64 UU per tile).
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellVFX")
	int32 PlayEffectAtTile(FName EffectID, int32 TileX, int32 TileY);

	/** Cancel a playing effect immediately. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellVFX")
	void StopEffect(int32 PlaybackID);

	/** Cancel all playing effects. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellVFX")
	void StopAllEffects();

	/** Returns true if the given playback handle is still animating. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellVFX")
	bool IsPlaying(int32 PlaybackID) const;

	// -- Delegates ────────────────────────────────────────────────────────────

	/** Fired when an effect finishes playing. */
	UPROPERTY(BlueprintAssignable, Category = "CoM|SpellVFX")
	FOnSpellVFXFinished OnSpellVFXFinished;

	/** Fired when a new effect is requested (UI layer listens to this). */
	UPROPERTY(BlueprintAssignable, Category = "CoM|SpellVFX")
	FOnSpellVFXRequested OnEffectRequested;

private:
	// -- Registration helpers ─────────────────────────────────────────────────

	/** Register all default spell effects for every realm + universal. */
	void RegisterAllDefaultEffects();

	/** Register a set of effects for one realm. */
	void RegisterRealmEffects(ECoMSpellRealm Realm, const FString& RealmFolder,
	                          const TArray<FName>& EffectNames,
	                          const TArray<bool>& IsProjectile,
	                          const TArray<bool>& IsArea,
	                          const TArray<bool>& IsBuff);

	/** Helper to build a single FCoMSpellVFXDef. */
	FCoMSpellVFXDef MakeEffect(FName EffectID, ECoMSpellRealm Realm,
	                            const FString& SheetPath, int32 Frames,
	                            float FrameDur, float Scale,
	                            bool bProjectile, bool bArea,
	                            bool bBuff, bool bImpact) const;

	// -- Data ─────────────────────────────────────────────────────────────────

	/** All registered effect definitions, keyed by EffectID. */
	UPROPERTY()
	TMap<FName, FCoMSpellVFXDef> EffectLibrary;

	/** Currently playing effects. */
	UPROPERTY()
	TArray<FCoMActiveVFX> ActiveEffects;

	/** Monotonically increasing playback handle counter. */
	int32 NextPlaybackID = 1;

	/** Timer handle for the tick timer. */
	FTimerHandle TickTimerHandle;
};
