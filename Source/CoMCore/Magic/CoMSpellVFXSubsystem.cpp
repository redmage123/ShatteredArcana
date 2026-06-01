// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellVFXSubsystem.cpp -- Spell visual effects subsystem implementation.

#include "CoMSpellVFXSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMSpellVFXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RegisterAllDefaultEffects();

	UE_LOG(LogTemp, Log, TEXT("CoMSpellVFXSubsystem: Initialized with %d effect definitions."), EffectLibrary.Num());
}

void UCoMSpellVFXSubsystem::Deinitialize()
{
	StopAllEffects();

	// Clear the tick timer if it is running.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWorld* World = GI->GetWorld())
		{
			World->GetTimerManager().ClearTimer(TickTimerHandle);
		}
	}

	Super::Deinitialize();
}

// =============================================================================
// Tick
// =============================================================================

void UCoMSpellVFXSubsystem::TickEffects(float DeltaTime)
{
	for (int32 Idx = ActiveEffects.Num() - 1; Idx >= 0; --Idx)
	{
		FCoMActiveVFX& VFX = ActiveEffects[Idx];
		if (!VFX.bIsPlaying)
		{
			continue;
		}

		const FCoMSpellVFXDef* Def = EffectLibrary.Find(VFX.EffectID);
		if (!Def)
		{
			VFX.bIsPlaying = false;
			continue;
		}

		VFX.FrameTimeAccumulator += DeltaTime;

		// Advance frames.
		while (VFX.FrameTimeAccumulator >= Def->FrameDuration)
		{
			VFX.FrameTimeAccumulator -= Def->FrameDuration;
			VFX.CurrentFrame++;

			if (VFX.CurrentFrame >= Def->FrameCount)
			{
				// Animation finished.
				VFX.bIsPlaying = false;
				OnSpellVFXFinished.Broadcast(VFX.EffectID);
				break;
			}
		}
	}

	// Compact finished effects.
	ActiveEffects.RemoveAll([](const FCoMActiveVFX& V) { return !V.bIsPlaying; });

	// Stop the timer when there are no active effects to avoid wasted ticks.
	if (ActiveEffects.Num() == 0)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UWorld* World = GI->GetWorld())
			{
				World->GetTimerManager().ClearTimer(TickTimerHandle);
			}
		}
	}
}

// =============================================================================
// Registration
// =============================================================================

void UCoMSpellVFXSubsystem::RegisterEffect(const FCoMSpellVFXDef& Def)
{
	EffectLibrary.Add(Def.EffectID, Def);
}

bool UCoMSpellVFXSubsystem::GetEffectDef(FName EffectID, FCoMSpellVFXDef& OutDef) const
{
	const FCoMSpellVFXDef* Found = EffectLibrary.Find(EffectID);
	if (Found)
	{
		OutDef = *Found;
		return true;
	}
	return false;
}

// =============================================================================
// Playback
// =============================================================================

int32 UCoMSpellVFXSubsystem::PlayEffect(FName EffectID, FVector WorldPosition, FVector TargetPosition)
{
	if (!EffectLibrary.Contains(EffectID))
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMSpellVFXSubsystem::PlayEffect: Unknown EffectID '%s'"), *EffectID.ToString());
		return -1;
	}

	FCoMActiveVFX NewVFX;
	NewVFX.PlaybackID = NextPlaybackID++;
	NewVFX.EffectID = EffectID;
	NewVFX.WorldPosition = WorldPosition;
	NewVFX.TargetPosition = TargetPosition;
	NewVFX.CurrentFrame = 0;
	NewVFX.FrameTimeAccumulator = 0.0f;
	NewVFX.bIsPlaying = true;

	ActiveEffects.Add(NewVFX);

	// Broadcast so the UI layer can begin rendering the animation.
	OnEffectRequested.Broadcast(NewVFX.PlaybackID, EffectID, WorldPosition, TargetPosition);

	// Ensure tick timer is running.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWorld* World = GI->GetWorld())
		{
			if (!TickTimerHandle.IsValid())
			{
				World->GetTimerManager().SetTimer(
					TickTimerHandle,
					FTimerDelegate::CreateUObject(this, &UCoMSpellVFXSubsystem::TickEffects, 0.016f),
					0.016f, // ~60 FPS tick
					true);
			}
		}
	}

	return NewVFX.PlaybackID;
}

int32 UCoMSpellVFXSubsystem::PlayEffectAtTile(FName EffectID, int32 TileX, int32 TileY)
{
	// Convert tile coordinates to world position (64 UU per tile, centered).
	constexpr float TileSize = 64.0f;
	const FVector WorldPos(
		(static_cast<float>(TileX) + 0.5f) * TileSize,
		(static_cast<float>(TileY) + 0.5f) * TileSize,
		0.0f);

	return PlayEffect(EffectID, WorldPos);
}

void UCoMSpellVFXSubsystem::StopEffect(int32 PlaybackID)
{
	for (FCoMActiveVFX& VFX : ActiveEffects)
	{
		if (VFX.PlaybackID == PlaybackID)
		{
			VFX.bIsPlaying = false;
			break;
		}
	}
}

void UCoMSpellVFXSubsystem::StopAllEffects()
{
	for (FCoMActiveVFX& VFX : ActiveEffects)
	{
		VFX.bIsPlaying = false;
	}
	ActiveEffects.Empty();
}

bool UCoMSpellVFXSubsystem::IsPlaying(int32 PlaybackID) const
{
	for (const FCoMActiveVFX& VFX : ActiveEffects)
	{
		if (VFX.PlaybackID == PlaybackID)
		{
			return VFX.bIsPlaying;
		}
	}
	return false;
}

// =============================================================================
// Default Effect Registration
// =============================================================================

FCoMSpellVFXDef UCoMSpellVFXSubsystem::MakeEffect(
	FName EffectID, ECoMSpellRealm Realm,
	const FString& SheetPath, int32 Frames,
	float FrameDur, float Scale,
	bool bProjectile, bool bArea,
	bool bBuff, bool bImpact) const
{
	FCoMSpellVFXDef Def;
	Def.EffectID = EffectID;
	Def.Realm = Realm;
	Def.SpriteSheetPath = SheetPath;
	Def.FrameCount = Frames;
	Def.FrameDuration = FrameDur;
	Def.Scale = Scale;
	Def.SoundCueID = EffectID; // Default: SFX ID matches effect ID.
	Def.bIsProjectile = bProjectile;
	Def.bIsAreaEffect = bArea;
	Def.bIsBuff = bBuff;
	Def.bIsImpact = bImpact;
	return Def;
}

void UCoMSpellVFXSubsystem::RegisterRealmEffects(
	ECoMSpellRealm Realm, const FString& RealmFolder,
	const TArray<FName>& EffectNames,
	const TArray<bool>& IsProjectile,
	const TArray<bool>& IsArea,
	const TArray<bool>& IsBuff)
{
	const FString BasePath = FString::Printf(TEXT("/Game/Textures/SpellVFX/%s/"), *RealmFolder);

	for (int32 Idx = 0; Idx < EffectNames.Num(); ++Idx)
	{
		const FName& eName = EffectNames[Idx];
		const FString SheetPath = BasePath + eName.ToString() + TEXT("_sheet");
		// IDs use a dot separator to match the lookup keys formed in
		// CoMMagicSubsystem (e.g. "life.shield", "chaos.fireball"). The old
		// underscore form ("life_shield") never matched -> every cast warned.
		const FName FullID = FName(*(RealmFolder + TEXT(".") + eName.ToString()));

		const bool bProj  = IsProjectile.IsValidIndex(Idx) ? IsProjectile[Idx] : false;
		const bool bArea  = IsArea.IsValidIndex(Idx)       ? IsArea[Idx]       : false;
		const bool bBf    = IsBuff.IsValidIndex(Idx)       ? IsBuff[Idx]       : false;
		const bool bImpact = (eName == FName("impact_flash"));

		RegisterEffect(MakeEffect(FullID, Realm, SheetPath, 8, 0.1f, 1.0f,
		                          bProj, bArea, bBf, bImpact));
	}
}

void UCoMSpellVFXSubsystem::RegisterAllDefaultEffects()
{
	// ── Life ──────────────────────────────────────────────────────────────────
	RegisterRealmEffects(ECoMSpellRealm::Life, TEXT("life"),
		{ FName("heal"), FName("shield"), FName("smite"), FName("resurrect"), FName("impact_flash") },
		{ false, false, true,  false, false },  // projectile
		{ false, false, false, false, false },  // area
		{ true,  true,  false, true,  false }); // buff

	// ── Death ─────────────────────────────────────────────────────────────────
	RegisterRealmEffects(ECoMSpellRealm::Death, TEXT("death"),
		{ FName("drain"), FName("curse"), FName("raise_dead"), FName("shadow_bolt"), FName("impact_flash") },
		{ false, false, false, true,  false },
		{ false, true,  false, false, false },
		{ false, false, true,  false, false });

	// ── Chaos ─────────────────────────────────────────────────────────────────
	RegisterRealmEffects(ECoMSpellRealm::Chaos, TEXT("chaos"),
		{ FName("fireball"), FName("lightning"), FName("meteor"), FName("doom_bolt"), FName("impact_flash") },
		{ true,  true,  false, true,  false },
		{ false, false, true,  false, false },
		{ false, false, false, false, false });

	// ── Nature ────────────────────────────────────────────────────────────────
	RegisterRealmEffects(ECoMSpellRealm::Nature, TEXT("nature"),
		{ FName("entangle"), FName("earthquake"), FName("summon_beast"), FName("growth"), FName("impact_flash") },
		{ false, false, false, false, false },
		{ true,  true,  false, true,  false },
		{ false, false, true,  true,  false });

	// ── Sorcery ───────────────────────────────────────────────────────────────
	RegisterRealmEffects(ECoMSpellRealm::Sorcery, TEXT("sorcery"),
		{ FName("dispel"), FName("teleport"), FName("confusion"), FName("counter_spell"), FName("impact_flash") },
		{ false, false, false, false, false },
		{ true,  false, true,  false, false },
		{ false, false, false, true,  false });

	// ── Arcane ────────────────────────────────────────────────────────────────
	RegisterRealmEffects(ECoMSpellRealm::Arcane, TEXT("arcane"),
		{ FName("magic_missile"), FName("enchant"), FName("detect"), FName("ward"), FName("impact_flash") },
		{ true,  false, false, false, false },
		{ false, false, true,  false, false },
		{ false, true,  false, true,  false });

	// ── Binding ───────────────────────────────────────────────────────────────
	RegisterRealmEffects(ECoMSpellRealm::Binding, TEXT("binding"),
		{ FName("chains"), FName("dominate"), FName("soul_trap"), FName("contract"), FName("impact_flash") },
		{ false, false, false, false, false },
		{ true,  false, true,  false, false },
		{ false, true,  false, true,  false });

	// ── Spirit ────────────────────────────────────────────────────────────────
	RegisterRealmEffects(ECoMSpellRealm::Spirit, TEXT("spirit"),
		{ FName("ghost_touch"), FName("dream"), FName("possession"), FName("astral_bolt"), FName("impact_flash") },
		{ false, false, false, true,  false },
		{ false, true,  false, false, false },
		{ false, false, true,  false, false });

	// ── Glamour ───────────────────────────────────────────────────────────────
	RegisterRealmEffects(ECoMSpellRealm::Glamour, TEXT("glamour"),
		{ FName("illusion"), FName("charm"), FName("true_sight"), FName("time_stop"), FName("impact_flash") },
		{ false, false, false, false, false },
		{ true,  false, true,  true,  false },
		{ false, true,  false, false, false });

	// ── Universal (no realm) ─────────────────────────────────────────────────
	{
		const FString BasePath = TEXT("/Game/Textures/SpellVFX/universal/");
		RegisterEffect(MakeEffect(FName("casting_glow"),  ECoMSpellRealm::None, BasePath + TEXT("casting_glow_sheet"),  8, 0.1f, 1.0f, false, false, true,  false));
		RegisterEffect(MakeEffect(FName("mana_drain"),    ECoMSpellRealm::None, BasePath + TEXT("mana_drain_sheet"),    8, 0.1f, 1.0f, false, false, false, false));
		RegisterEffect(MakeEffect(FName("level_up"),      ECoMSpellRealm::None, BasePath + TEXT("level_up_sheet"),      8, 0.12f,1.2f, false, false, true,  false));
		RegisterEffect(MakeEffect(FName("death_effect"),  ECoMSpellRealm::None, BasePath + TEXT("death_effect_sheet"),  8, 0.08f,1.0f, false, false, false, true ));
		RegisterEffect(MakeEffect(FName("summon_portal"), ECoMSpellRealm::None, BasePath + TEXT("summon_portal_sheet"), 8, 0.1f, 1.5f, false, true,  false, false));
	}

	// ── Iconic per-spell overrides (registered as "iconic.<slug>") ───────────
	// These get a dedicated keyframe + 8-frame burst sheet so the most-cast
	// spells don't all look like the realm-default missile or shield. The
	// SpellID -> "iconic.<slug>" mapping lives in CoMMagicSubsystem; this
	// block only owns the asset registration.
	{
		const FString BasePath = TEXT("/Game/Textures/SpellVFX/iconic/");
		auto Iconic = [this, &BasePath](const TCHAR* Slug, ECoMSpellRealm Realm,
		                                 bool bProj, bool bArea, bool bBuff)
		{
			const FName ID(*(FString(TEXT("iconic.")) + Slug));
			RegisterEffect(MakeEffect(ID, Realm,
				BasePath + Slug + TEXT("_sheet"),
				8, 0.1f, 1.2f, bProj, bArea, bBuff, false));
		};
		Iconic(TEXT("spell_of_mastery"), ECoMSpellRealm::Arcane,  false, true,  false);
		Iconic(TEXT("just_cause"),       ECoMSpellRealm::Life,    false, false, true);
		Iconic(TEXT("eternal_night"),    ECoMSpellRealm::Death,   false, true,  false);
		Iconic(TEXT("armageddon"),       ECoMSpellRealm::Chaos,   false, true,  false);
		Iconic(TEXT("call_the_void"),    ECoMSpellRealm::Chaos,   false, true,  false);
		Iconic(TEXT("gaia_force"),       ECoMSpellRealm::Nature,  false, true,  true);
		Iconic(TEXT("heavenly_light"),   ECoMSpellRealm::Life,    false, false, true);
		Iconic(TEXT("wall_of_fire"),     ECoMSpellRealm::Chaos,   false, false, true);
		Iconic(TEXT("suppress_magic"),   ECoMSpellRealm::Arcane,  false, true,  false);
		Iconic(TEXT("flying_fortress"),  ECoMSpellRealm::Sorcery, false, false, true);
		Iconic(TEXT("volcano"),          ECoMSpellRealm::Chaos,   false, true,  false);
		Iconic(TEXT("great_tree"),       ECoMSpellRealm::Nature,  false, true,  true);
	}
}
