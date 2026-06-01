// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMAudioSubsystem.h — Centralized audio manager for Shattered Arcana.
// Uses UE5 built-in audio (MetaSounds / Sound Cues). Designed to be
// swappable with Wwise middleware if installed later.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "CoreTypes/CoMEnums.h"
#include "CoMAudioSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Combat intensity levels used by SetCombatIntensity().
// ─────────────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class ECoMCombatIntensity : uint8
{
	Peace     = 0 UMETA(DisplayName = "Peace"),
	Skirmish  = 1 UMETA(DisplayName = "Skirmish"),
	Battle    = 2 UMETA(DisplayName = "Battle"),
	Epic      = 3 UMETA(DisplayName = "Epic"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Delegates
// ─────────────────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMusicChanged, FName, TrackID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmbientChanged, ECoMPlane, Plane);

// ─────────────────────────────────────────────────────────────────────────────

/**
 * UCoMAudioSubsystem
 *
 * GameInstance subsystem that centralizes all audio playback for the game.
 * Manages three independent audio layers:
 *   1. Music    — single looping track with crossfade transitions
 *   2. SFX      — one-shot positioned or 2D sounds
 *   3. Ambient  — looping background atmosphere per plane/biome
 *
 * Sound assets are registered via TMap<FName, USoundBase*> properties
 * that can be populated in a DataAsset or Blueprint.
 *
 * This subsystem uses UE5's built-in audio system. If Wwise is installed
 * later, the public API stays the same — only the internals change to
 * post Wwise events instead of playing USoundBase assets.
 */
UCLASS()
class COMCORE_API UCoMAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── USubsystem interface ─────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── Music ────────────────────────────────────────────────────────────

	/**
	 * Crossfade to a new music track identified by TrackID.
	 * If the track is already playing, this is a no-op.
	 * @param TrackID  Key into the MusicTracks map.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void PlayMusic(FName TrackID);

	/** Play the plane-themed overworld music for the given plane.
	 *  Maps ECoMPlane -> /Game/Audio/Music/plane_<name>. No-ops if the
	 *  asset isn't authored yet. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void PlayPlaneMusic(ECoMPlane Plane);

	/** Fade out and stop all music. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void StopAllMusic();

	// ── Sound Effects ────────────────────────────────────────────────────

	/**
	 * Play a positioned (3D) sound effect.
	 * @param SoundID   Key into the SFXSounds map.
	 * @param Location  World-space position for attenuation.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void PlaySFX(FName SoundID, FVector Location);

	/**
	 * Play a 2D UI sound (no spatialization).
	 * @param SoundID  Key into the SFXSounds map.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void PlayUISound(FName SoundID);

	/**
	 * Play a spell SFX cued from realm + impact flag. Loads the asset on
	 * demand from /Game/Audio/SpellSFX/<realm>_<cast|impact>.* and caches it.
	 * No-op when the asset is missing (logs a single warning per key).
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void PlaySpellSFX(ECoMSpellRealm Realm, bool bImpact, FVector Location);

	// ── Ambient ──────────────────────────────────────────────────────────

	/**
	 * Switch the ambient loop to match the given plane.
	 * Crossfades from the current ambient to the new one.
	 * @param Plane  The ECoMPlane whose ambient preset should play.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void SetAmbientPreset(ECoMPlane Plane);

	/** Fade out and stop all ambient sounds. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void StopAllAmbient();

	// ── Combat intensity ─────────────────────────────────────────────────

	/**
	 * Set the combat music intensity.
	 * 0 = peace (overworld music), 1 = skirmish, 2 = battle, 3 = epic.
	 * Maps to different combat music tracks via CombatMusicTracks.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void SetCombatIntensity(int32 Level);

	/** Returns the current combat intensity level. */
	UFUNCTION(BlueprintPure, Category = "CoM|Audio")
	int32 GetCombatIntensity() const { return CurrentCombatIntensity; }

	// ── Volume controls ──────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void SetMasterVolume(float Vol);

	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void SetMusicVolume(float Vol);

	UFUNCTION(BlueprintCallable, Category = "CoM|Audio")
	void SetSFXVolume(float Vol);

	UFUNCTION(BlueprintPure, Category = "CoM|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "CoM|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "CoM|Audio")
	float GetSFXVolume() const { return SFXVolume; }

	// ── Delegates ────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "CoM|Audio")
	FOnMusicChanged OnMusicChanged;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Audio")
	FOnAmbientChanged OnAmbientChanged;

	// ── Sound asset maps ─────────────────────────────────────────────────
	// Populate these from a DataAsset, DataTable, or Blueprint defaults.

	/** Music tracks: TrackID -> USoundBase (looping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|Audio|Library")
	TMap<FName, USoundBase*> MusicTracks;

	/** SFX one-shots: SoundID -> USoundBase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|Audio|Library")
	TMap<FName, USoundBase*> SFXSounds;

	/** Per-plane ambient loops: ECoMPlane -> USoundBase (looping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|Audio|Library")
	TMap<ECoMPlane, USoundBase*> AmbientLoops;

	/**
	 * Combat music by intensity level (0-3).
	 * Key 1 = skirmish, 2 = battle, 3 = epic.
	 * Peace (level 0) reverts to the overworld music track.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|Audio|Library")
	TMap<int32, USoundBase*> CombatMusicTracks;

private:
	// ── Internal helpers ─────────────────────────────────────────────────

	/** Get (or create) the music audio component for the given slot (0 = A, 1 = B). */
	UAudioComponent* GetOrCreateMusicComponent(int32 Slot);

	/** Get (or create) the ambient audio component for the given slot (0 = A, 1 = B). */
	UAudioComponent* GetOrCreateAmbientComponent(int32 Slot);

	/** Apply effective volume to a component (master * category). */
	void ApplyMusicVolume(UAudioComponent* Comp) const;
	void ApplyAmbientVolume(UAudioComponent* Comp) const;

	/** Perform the crossfade between MusicComponents[ActiveMusicSlot] and the other slot. */
	void CrossfadeMusic(USoundBase* NewSound);

	/** Perform crossfade on the ambient component. */
	void CrossfadeAmbient(USoundBase* NewSound);

	// ── State ────────────────────────────────────────────────────────────

	/** Dual-slot music components for crossfading (A/B). */
	UPROPERTY()
	TArray<UAudioComponent*> MusicComponents;

	/** Which music slot (0 or 1) is currently active. */
	int32 ActiveMusicSlot = 0;

	/** Dual-slot ambient components for crossfading (A/B). */
	UPROPERTY()
	TArray<UAudioComponent*> AmbientComponents;

	/** Which ambient slot (0 or 1) is currently active. */
	int32 ActiveAmbientSlot = 0;

	/** Currently playing music track ID. */
	FName CurrentMusicTrackID = NAME_None;

	/** Current plane for ambient. */
	ECoMPlane CurrentAmbientPlane = ECoMPlane::MAX;

	/** Current combat intensity. */
	int32 CurrentCombatIntensity = 0;

	/** Volume levels (0.0 - 1.0). */
	float MasterVolume = 1.0f;
	float MusicVolume  = 0.7f;
	float SFXVolume    = 1.0f;

	/** Crossfade duration in seconds. */
	static constexpr float CrossfadeDuration = 1.5f;
};
