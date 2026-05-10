// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMAudioSubsystem.cpp — Implementation of the centralized audio manager.

#include "Audio/CoMAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMAudio, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// USubsystem interface
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Pre-allocate the two music slots (A/B for crossfading).
	MusicComponents.SetNum(2);
	MusicComponents[0] = nullptr;
	MusicComponents[1] = nullptr;
	ActiveMusicSlot = 0;

	CurrentMusicTrackID = NAME_None;
	CurrentAmbientPlane = ECoMPlane::MAX;
	CurrentCombatIntensity = 0;

	UE_LOG(LogCoMAudio, Log, TEXT("UCoMAudioSubsystem initialized."));
}

void UCoMAudioSubsystem::Deinitialize()
{
	StopAllMusic();
	StopAllAmbient();

	for (UAudioComponent*& Comp : MusicComponents)
	{
		if (IsValid(Comp))
		{
			Comp->DestroyComponent();
		}
		Comp = nullptr;
	}

	for (UAudioComponent*& Comp : AmbientComponents)
	{
		if (IsValid(Comp))
		{
			Comp->DestroyComponent();
		}
		Comp = nullptr;
	}

	UE_LOG(LogCoMAudio, Log, TEXT("UCoMAudioSubsystem deinitialized."));
	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Music
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAudioSubsystem::PlayMusic(FName TrackID)
{
	if (TrackID == CurrentMusicTrackID)
	{
		return; // Already playing this track
	}

	USoundBase** Found = MusicTracks.Find(TrackID);
	if (!Found || !*Found)
	{
		UE_LOG(LogCoMAudio, Warning,
			TEXT("PlayMusic: Track '%s' not found in MusicTracks map."),
			*TrackID.ToString());
		return;
	}

	CurrentMusicTrackID = TrackID;
	CrossfadeMusic(*Found);

	OnMusicChanged.Broadcast(TrackID);

	UE_LOG(LogCoMAudio, Log,
		TEXT("PlayMusic: Now playing '%s'."), *TrackID.ToString());
}

void UCoMAudioSubsystem::StopAllMusic()
{
	for (int32 i = 0; i < MusicComponents.Num(); ++i)
	{
		UAudioComponent* Comp = MusicComponents[i];
		if (Comp && Comp->IsPlaying())
		{
			Comp->FadeOut(CrossfadeDuration, 0.0f);
		}
	}
	CurrentMusicTrackID = NAME_None;
	CurrentCombatIntensity = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Sound Effects
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAudioSubsystem::PlaySFX(FName SoundID, FVector Location)
{
	USoundBase** Found = SFXSounds.Find(SoundID);
	if (!Found || !*Found)
	{
		UE_LOG(LogCoMAudio, Warning,
			TEXT("PlaySFX: Sound '%s' not found in SFXSounds map."),
			*SoundID.ToString());
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogCoMAudio, Warning,
			TEXT("PlaySFX: No world available for sound '%s'."),
			*SoundID.ToString());
		return;
	}

	const float EffectiveVolume = MasterVolume * SFXVolume;
	UGameplayStatics::PlaySoundAtLocation(
		World,
		*Found,
		Location,
		FRotator::ZeroRotator,
		EffectiveVolume,
		1.0f   // pitch
	);
}

void UCoMAudioSubsystem::PlayUISound(FName SoundID)
{
	USoundBase** Found = SFXSounds.Find(SoundID);
	if (!Found || !*Found)
	{
		UE_LOG(LogCoMAudio, Warning,
			TEXT("PlayUISound: Sound '%s' not found in SFXSounds map."),
			*SoundID.ToString());
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	const float EffectiveVolume = MasterVolume * SFXVolume;
	UGameplayStatics::PlaySound2D(
		World,
		*Found,
		EffectiveVolume,
		1.0f   // pitch
	);
}

void UCoMAudioSubsystem::PlaySpellSFX(ECoMSpellRealm Realm, bool bImpact, FVector Location)
{
	// Build a canonical asset key. Cache loaded sounds in SFXSounds so we
	// only LoadObject once per realm/kind pair.
	const TCHAR* RealmName = TEXT("arcane");
	switch (Realm)
	{
	case ECoMSpellRealm::Life:    RealmName = TEXT("life");    break;
	case ECoMSpellRealm::Death:   RealmName = TEXT("death");   break;
	case ECoMSpellRealm::Chaos:   RealmName = TEXT("chaos");   break;
	case ECoMSpellRealm::Nature:  RealmName = TEXT("nature");  break;
	case ECoMSpellRealm::Sorcery: RealmName = TEXT("sorcery"); break;
	case ECoMSpellRealm::Arcane:  RealmName = TEXT("arcane");  break;
	case ECoMSpellRealm::Binding: RealmName = TEXT("binding"); break;
	case ECoMSpellRealm::Spirit:  RealmName = TEXT("spirit");  break;
	case ECoMSpellRealm::Glamour: RealmName = TEXT("glamour"); break;
	default: break;
	}
	const FString Kind = bImpact ? TEXT("impact") : TEXT("cast");
	const FName CacheKey(*FString::Printf(TEXT("spell_%s_%s"), RealmName, *Kind));

	USoundBase* Sound = nullptr;
	if (USoundBase** Cached = SFXSounds.Find(CacheKey))
	{
		Sound = *Cached;
	}
	else
	{
		const FString AssetPath = FString::Printf(
			TEXT("/Game/Audio/SpellSFX/%s_%s.%s_%s"),
			RealmName, *Kind, RealmName, *Kind);
		Sound = LoadObject<USoundBase>(nullptr, *AssetPath);
		// Cache positive AND negative results to avoid spamming load attempts.
		SFXSounds.Add(CacheKey, Sound);
		if (!Sound)
		{
			UE_LOG(LogCoMAudio, Warning,
				TEXT("PlaySpellSFX: missing asset %s"), *AssetPath);
		}
	}

	if (!Sound) return;

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) return;

	const float EffectiveVolume = MasterVolume * SFXVolume;
	UGameplayStatics::PlaySoundAtLocation(World, Sound, Location,
		FRotator::ZeroRotator, EffectiveVolume, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Ambient
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAudioSubsystem::SetAmbientPreset(ECoMPlane Plane)
{
	if (Plane == CurrentAmbientPlane)
	{
		return;
	}

	USoundBase** Found = AmbientLoops.Find(Plane);
	if (!Found || !*Found)
	{
		UE_LOG(LogCoMAudio, Warning,
			TEXT("SetAmbientPreset: No ambient loop for plane %d."),
			static_cast<int32>(Plane));
		return;
	}

	CurrentAmbientPlane = Plane;
	CrossfadeAmbient(*Found);

	OnAmbientChanged.Broadcast(Plane);

	UE_LOG(LogCoMAudio, Log,
		TEXT("SetAmbientPreset: Switched to plane %d ambient."),
		static_cast<int32>(Plane));
}

void UCoMAudioSubsystem::StopAllAmbient()
{
	for (UAudioComponent* Comp : AmbientComponents)
	{
		if (IsValid(Comp) && Comp->IsPlaying())
		{
			Comp->FadeOut(CrossfadeDuration, 0.0f);
		}
	}
	CurrentAmbientPlane = ECoMPlane::MAX;
}

// ─────────────────────────────────────────────────────────────────────────────
// Combat intensity
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAudioSubsystem::SetCombatIntensity(int32 Level)
{
	Level = FMath::Clamp(Level, 0, 3);

	if (Level == CurrentCombatIntensity)
	{
		return;
	}

	CurrentCombatIntensity = Level;

	if (Level == 0)
	{
		// Peace — revert to overworld music if available
		if (MusicTracks.Contains(FName("Music_Overworld")))
		{
			PlayMusic(FName("Music_Overworld"));
		}
		else
		{
			StopAllMusic();
		}
		return;
	}

	// Map intensity 1-3 to combat music tracks
	USoundBase** Found = CombatMusicTracks.Find(Level);
	if (Found && *Found)
	{
		FName CombatTrackName = FName(*FString::Printf(TEXT("Combat_Intensity_%d"), Level));
		CurrentMusicTrackID = CombatTrackName;
		CrossfadeMusic(*Found);
		OnMusicChanged.Broadcast(CombatTrackName);
	}
	else
	{
		UE_LOG(LogCoMAudio, Warning,
			TEXT("SetCombatIntensity: No combat music for intensity %d."), Level);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Volume controls
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAudioSubsystem::SetMasterVolume(float Vol)
{
	MasterVolume = FMath::Clamp(Vol, 0.0f, 1.0f);

	// Update all active components
	for (UAudioComponent* Comp : MusicComponents)
	{
		if (Comp && Comp->IsPlaying())
		{
			ApplyMusicVolume(Comp);
		}
	}
	for (UAudioComponent* Comp : AmbientComponents)
	{
		if (IsValid(Comp) && Comp->IsPlaying())
		{
			ApplyAmbientVolume(Comp);
		}
	}
}

void UCoMAudioSubsystem::SetMusicVolume(float Vol)
{
	MusicVolume = FMath::Clamp(Vol, 0.0f, 1.0f);
	for (UAudioComponent* Comp : MusicComponents)
	{
		if (Comp && Comp->IsPlaying())
		{
			ApplyMusicVolume(Comp);
		}
	}
}

void UCoMAudioSubsystem::SetSFXVolume(float Vol)
{
	SFXVolume = FMath::Clamp(Vol, 0.0f, 1.0f);
	// SFX are one-shots — volume applied at play time. Nothing to update.
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

UAudioComponent* UCoMAudioSubsystem::GetOrCreateMusicComponent(int32 Slot)
{
	check(Slot >= 0 && Slot < 2);

	if (IsValid(MusicComponents[Slot]))
	{
		return MusicComponents[Slot];
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	// Create an audio component attached to the world's game state (persistent actor).
	AActor* Owner = World->GetGameState();
	if (!Owner)
	{
		// Fallback: use the first player controller
		Owner = World->GetFirstPlayerController();
	}
	if (!Owner)
	{
		UE_LOG(LogCoMAudio, Warning,
			TEXT("GetOrCreateMusicComponent: No suitable owner actor found."));
		return nullptr;
	}

	UAudioComponent* Comp = NewObject<UAudioComponent>(Owner, NAME_None, RF_Transient);
	Comp->bAutoActivate = false;
	Comp->bIsUISound = true; // Music should not be attenuated by distance
	Comp->bAllowSpatialization = false;
	Comp->RegisterComponent();

	MusicComponents[Slot] = Comp;
	return Comp;
}

UAudioComponent* UCoMAudioSubsystem::GetOrCreateAmbientComponent(int32 Slot)
{
	if (AmbientComponents.Num() < 2)
	{
		AmbientComponents.SetNum(2);
		AmbientComponents[0] = nullptr;
		AmbientComponents[1] = nullptr;
	}

	if (IsValid(AmbientComponents[Slot]))
	{
		return AmbientComponents[Slot];
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	AActor* Owner = World->GetGameState();
	if (!Owner)
	{
		Owner = World->GetFirstPlayerController();
	}
	if (!Owner)
	{
		return nullptr;
	}

	UAudioComponent* Comp = NewObject<UAudioComponent>(Owner, NAME_None, RF_Transient);
	Comp->bAutoActivate = false;
	Comp->bIsUISound = true;
	Comp->bAllowSpatialization = false;
	Comp->RegisterComponent();

	AmbientComponents[Slot] = Comp;
	return Comp;
}

void UCoMAudioSubsystem::ApplyMusicVolume(UAudioComponent* Comp) const
{
	if (Comp)
	{
		Comp->SetVolumeMultiplier(MasterVolume * MusicVolume);
	}
}

void UCoMAudioSubsystem::ApplyAmbientVolume(UAudioComponent* Comp) const
{
	if (Comp)
	{
		// Ambient is slightly quieter than music by default
		Comp->SetVolumeMultiplier(MasterVolume * MusicVolume * 0.6f);
	}
}

void UCoMAudioSubsystem::CrossfadeMusic(USoundBase* NewSound)
{
	if (!NewSound)
	{
		return;
	}

	// Fade out the currently active slot
	UAudioComponent* OldComp = GetOrCreateMusicComponent(ActiveMusicSlot);
	if (OldComp && OldComp->IsPlaying())
	{
		OldComp->FadeOut(CrossfadeDuration, 0.0f);
	}

	// Switch to the other slot and play the new sound
	ActiveMusicSlot = (ActiveMusicSlot + 1) % 2;
	UAudioComponent* NewComp = GetOrCreateMusicComponent(ActiveMusicSlot);
	if (!NewComp)
	{
		UE_LOG(LogCoMAudio, Warning,
			TEXT("CrossfadeMusic: Failed to create audio component for slot %d."),
			ActiveMusicSlot);
		return;
	}

	NewComp->SetSound(NewSound);
	ApplyMusicVolume(NewComp);
	NewComp->FadeIn(CrossfadeDuration);
}

void UCoMAudioSubsystem::CrossfadeAmbient(USoundBase* NewSound)
{
	if (!NewSound)
	{
		return;
	}

	// Fade out the currently active ambient component.
	UAudioComponent* OldComp = GetOrCreateAmbientComponent(ActiveAmbientSlot);
	if (OldComp && OldComp->IsPlaying())
	{
		OldComp->FadeOut(CrossfadeDuration, 0.0f);
	}

	// Switch to the other slot and fade in the new sound.
	ActiveAmbientSlot = 1 - ActiveAmbientSlot;
	UAudioComponent* NewComp = GetOrCreateAmbientComponent(ActiveAmbientSlot);
	if (!NewComp)
	{
		return;
	}

	NewComp->SetSound(NewSound);
	ApplyAmbientVolume(NewComp);
	NewComp->FadeIn(CrossfadeDuration);
}
