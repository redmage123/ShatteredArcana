// Copyright Mythforge Studios. All Rights Reserved.
// CoMUnitAnimComponent.cpp — Unit animation state management.

#include "Units/CoMUnitAnimComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequence.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

UCoMUnitAnimComponent::UCoMUnitAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCoMUnitAnimComponent::BeginPlay()
{
	Super::BeginPlay();
	LoadAnimationLibrary();
}

USkeletalMeshComponent* UCoMUnitAnimComponent::GetMesh()
{
	if (CachedMesh) return CachedMesh;

	AActor* Owner = GetOwner();
	if (Owner)
	{
		CachedMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
	return CachedMesh;
}

// =============================================================================
// Animation library loading
// =============================================================================

void UCoMUnitAnimComponent::LoadAnimationLibrary()
{
	// Load all animation assets from the knight animations directory
	static const TPair<FName, FString> AnimAssets[] =
	{
		// Idle
		{ FName("Idle_01"),    TEXT("/Game/Characters/Knight/Animations/Anim_Idle_01") },
		{ FName("Idle_02"),    TEXT("/Game/Characters/Knight/Animations/Anim_Idle_02") },
		{ FName("Idle_03"),    TEXT("/Game/Characters/Knight/Animations/Anim_Idle_03") },
		{ FName("Idle_04"),    TEXT("/Game/Characters/Knight/Animations/Anim_Idle_04") },

		// Walk
		{ FName("Walk_01"),    TEXT("/Game/Characters/Knight/Animations/Anim_Walk_01") },
		{ FName("Walk_02"),    TEXT("/Game/Characters/Knight/Animations/Anim_Walk_02") },

		// Run
		{ FName("Run_01"),     TEXT("/Game/Characters/Knight/Animations/Anim_Run_01") },
		{ FName("Run_02"),     TEXT("/Game/Characters/Knight/Animations/Anim_Run_02") },

		// Attack - slash
		{ FName("Attack_01"),  TEXT("/Game/Characters/Knight/Animations/Anim_Attack_01") },
		{ FName("Attack_02"),  TEXT("/Game/Characters/Knight/Animations/Anim_Attack_02") },
		{ FName("Attack_03"),  TEXT("/Game/Characters/Knight/Animations/Anim_Attack_03") },
		{ FName("Attack_04"),  TEXT("/Game/Characters/Knight/Animations/Anim_Attack_04") },
		{ FName("Slash_01"),   TEXT("/Game/Characters/Knight/Animations/Anim_Attack_Slash_01") },
		{ FName("Slash_02"),   TEXT("/Game/Characters/Knight/Animations/Anim_Attack_Slash_02") },
		{ FName("Slash_03"),   TEXT("/Game/Characters/Knight/Animations/Anim_Attack_Slash_03") },
		{ FName("Slash_04"),   TEXT("/Game/Characters/Knight/Animations/Anim_Attack_Slash_04") },
		{ FName("Slash_05"),   TEXT("/Game/Characters/Knight/Animations/Anim_Attack_Slash_05") },

		// Block
		{ FName("Block_01"),   TEXT("/Game/Characters/Knight/Animations/Anim_Block_01") },
		{ FName("Block_02"),   TEXT("/Game/Characters/Knight/Animations/Anim_Block_02") },
		{ FName("Block_Idle"), TEXT("/Game/Characters/Knight/Animations/Anim_Block_Idle") },

		// Death
		{ FName("Death_01"),   TEXT("/Game/Characters/Knight/Animations/Anim_Death_01") },
		{ FName("Death_02"),   TEXT("/Game/Characters/Knight/Animations/Anim_Death_02") },

		// Hit reaction
		{ FName("Hit_01"),     TEXT("/Game/Characters/Knight/Animations/Anim_Hit_01") },
		{ FName("Hit_02"),     TEXT("/Game/Characters/Knight/Animations/Anim_Hit_02") },
		{ FName("Hit_03"),     TEXT("/Game/Characters/Knight/Animations/Anim_Hit_03") },

		// Casting
		{ FName("Cast_01"),    TEXT("/Game/Characters/Knight/Animations/Anim_Cast_01") },
		{ FName("Cast_02"),    TEXT("/Game/Characters/Knight/Animations/Anim_Cast_02") },

		// Turn
		{ FName("Turn_01"),    TEXT("/Game/Characters/Knight/Animations/Anim_Turn_01") },
		{ FName("Turn_02"),    TEXT("/Game/Characters/Knight/Animations/Anim_Turn_02") },

		// Strafe
		{ FName("Strafe_01"),  TEXT("/Game/Characters/Knight/Animations/Anim_Strafe_01") },
		{ FName("Strafe_02"),  TEXT("/Game/Characters/Knight/Animations/Anim_Strafe_02") },

		// Crouch
		{ FName("Crouch_Start"),     TEXT("/Game/Characters/Knight/Animations/Anim_Crouch_Start") },
		{ FName("Crouch_Idle"),      TEXT("/Game/Characters/Knight/Animations/Anim_Crouch_Idle") },
		{ FName("Crouch_Block_01"),  TEXT("/Game/Characters/Knight/Animations/Anim_Crouch_Block_01") },

		// Special
		{ FName("Kick"),       TEXT("/Game/Characters/Knight/Animations/Anim_Kick") },
		{ FName("PowerUp"),    TEXT("/Game/Characters/Knight/Animations/Anim_PowerUp") },
		{ FName("Sheath_01"),  TEXT("/Game/Characters/Knight/Animations/Anim_Sheath_01") },
	};

	for (const auto& [Name, Path] : AnimAssets)
	{
		FString FullPath = Path + TEXT(".") + FPaths::GetBaseFilename(Path);
		UAnimationAsset* Anim = LoadObject<UAnimationAsset>(nullptr, *FullPath);
		if (Anim)
		{
			AnimationLibrary.Add(Name, Anim);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[UnitAnimComponent] Loaded %d animations"), AnimationLibrary.Num());
}

// =============================================================================
// State management
// =============================================================================

void UCoMUnitAnimComponent::SetAnimState(ECoMUnitAnimState NewState)
{
	if (bIsDead && NewState != ECoMUnitAnimState::Dead) return;
	if (NewState == CurrentState) return;

	PreviousState = CurrentState;
	CurrentState = NewState;

	USkeletalMeshComponent* Mesh = GetMesh();
	if (!Mesh) return;

	FName AnimName;
	bool bLoop = true;

	switch (NewState)
	{
	case ECoMUnitAnimState::Idle:
		AnimName = SelectVariant(TEXT("Idle"));
		break;
	case ECoMUnitAnimState::Walking:
		AnimName = SelectVariant(TEXT("Walk"));
		PlayFootstepSound();
		break;
	case ECoMUnitAnimState::Running:
		AnimName = SelectVariant(TEXT("Run"));
		PlayFootstepSound();
		break;
	case ECoMUnitAnimState::Attacking:
		AnimName = SelectVariant(TEXT("Attack"));
		bLoop = false;
		PlaySwingSound();
		break;
	case ECoMUnitAnimState::Blocking:
		AnimName = SelectVariant(TEXT("Block"));
		break;
	case ECoMUnitAnimState::CastingSpell:
		AnimName = SelectVariant(TEXT("Cast"));
		bLoop = false;
		PlayCastSound();
		break;
	case ECoMUnitAnimState::Hit:
		AnimName = SelectVariant(TEXT("Hit"));
		bLoop = false;
		PlayImpactSound();
		break;
	case ECoMUnitAnimState::Dying:
		AnimName = SelectVariant(TEXT("Death"));
		bLoop = false;
		PlayDeathSound();
		break;
	case ECoMUnitAnimState::Crouching:
		AnimName = FName("Crouch_Idle");
		break;
	case ECoMUnitAnimState::Turning:
		AnimName = SelectVariant(TEXT("Turn"));
		bLoop = false;
		break;
	case ECoMUnitAnimState::Strafing:
		AnimName = SelectVariant(TEXT("Strafe"));
		break;
	default:
		AnimName = FName("Idle_01");
		break;
	}

	UAnimationAsset* Anim = GetAnimation(AnimName);
	if (Anim)
	{
		Mesh->PlayAnimation(Anim, bLoop);
		Mesh->SetPlayRate(PlaybackSpeed);
	}
}

void UCoMUnitAnimComponent::PlayAttackVariant(int32 Variant)
{
	PreviousState = CurrentState;
	CurrentState = ECoMUnitAnimState::Attacking;

	FName AnimName = FName(*FString::Printf(TEXT("Attack_%02d"), Variant + 1));
	UAnimationAsset* Anim = GetAnimation(AnimName);
	if (!Anim)
	{
		AnimName = FName(*FString::Printf(TEXT("Slash_%02d"), Variant + 1));
		Anim = GetAnimation(AnimName);
	}

	if (Anim)
	{
		if (USkeletalMeshComponent* Mesh = GetMesh())
		{
			Mesh->PlayAnimation(Anim, false);
			Mesh->SetPlayRate(PlaybackSpeed);
		}
	}
	PlaySwingSound();
}

void UCoMUnitAnimComponent::PlayDeath(int32 Variant)
{
	bIsDead = true;
	CurrentState = ECoMUnitAnimState::Dying;

	FName AnimName = FName(*FString::Printf(TEXT("Death_%02d"), Variant + 1));
	UAnimationAsset* Anim = GetAnimation(AnimName);
	if (Anim)
	{
		if (USkeletalMeshComponent* Mesh = GetMesh())
		{
			Mesh->PlayAnimation(Anim, false);
		}
	}
	PlayDeathSound();
}

void UCoMUnitAnimComponent::PlayHitReaction(int32 Variant)
{
	PreviousState = CurrentState;
	CurrentState = ECoMUnitAnimState::Hit;

	FName AnimName = FName(*FString::Printf(TEXT("Hit_%02d"), Variant + 1));
	UAnimationAsset* Anim = GetAnimation(AnimName);
	if (Anim)
	{
		if (USkeletalMeshComponent* Mesh = GetMesh())
		{
			Mesh->PlayAnimation(Anim, false);
		}
	}
	PlayImpactSound();
}

void UCoMUnitAnimComponent::PlayCastSpell(int32 Variant)
{
	PreviousState = CurrentState;
	CurrentState = ECoMUnitAnimState::CastingSpell;

	FName AnimName = FName(*FString::Printf(TEXT("Cast_%02d"), Variant + 1));
	UAnimationAsset* Anim = GetAnimation(AnimName);
	if (Anim)
	{
		if (USkeletalMeshComponent* Mesh = GetMesh())
		{
			Mesh->PlayAnimation(Anim, false);
		}
	}
	PlayCastSound();
}

// =============================================================================
// Helpers
// =============================================================================

UAnimationAsset* UCoMUnitAnimComponent::GetAnimation(const FName& CategoryName) const
{
	if (const UAnimationAsset* const* Found = AnimationLibrary.Find(CategoryName))
	{
		return const_cast<UAnimationAsset*>(*Found);
	}
	return nullptr;
}

FName UCoMUnitAnimComponent::SelectVariant(const FString& Prefix) const
{
	TArray<FName> Matches;
	for (const auto& [Name, Anim] : AnimationLibrary)
	{
		if (Name.ToString().StartsWith(Prefix))
		{
			Matches.Add(Name);
		}
	}

	if (Matches.Num() == 0)
	{
		return FName(*FString::Printf(TEXT("%s_01"), *Prefix));
	}

	return Matches[FMath::RandRange(0, Matches.Num() - 1)];
}

// =============================================================================
// Sound effects
// =============================================================================

void UCoMUnitAnimComponent::PlaySFX(const FString& SoundPath)
{
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *SoundPath);
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation());
	}
}

void UCoMUnitAnimComponent::PlayFootstepSound()
{
	// Random footstep from combat SFX
	static const FString FootstepSounds[] = {
		TEXT("/Game/Audio/SFX/Combat/impactGeneric_light_000"),
		TEXT("/Game/Audio/SFX/Combat/impactGeneric_light_001"),
		TEXT("/Game/Audio/SFX/Combat/impactGeneric_light_002"),
	};
	int32 Idx = FMath::RandRange(0, 2);
	PlaySFX(FootstepSounds[Idx]);
}

void UCoMUnitAnimComponent::PlaySwingSound()
{
	static const FString SwingSounds[] = {
		TEXT("/Game/Audio/SFX/Combat/sword_swing"),
		TEXT("/Game/Audio/SFX/Combat/oga_swing"),
		TEXT("/Game/Audio/SFX/Combat/oga_swing2"),
	};
	PlaySFX(SwingSounds[FMath::RandRange(0, 2)]);
}

void UCoMUnitAnimComponent::PlayImpactSound()
{
	static const FString ImpactSounds[] = {
		TEXT("/Game/Audio/SFX/Combat/impactMetal_heavy_000"),
		TEXT("/Game/Audio/SFX/Combat/impactMetal_heavy_001"),
		TEXT("/Game/Audio/SFX/Combat/impactMetal_medium_000"),
		TEXT("/Game/Audio/SFX/Combat/unit_hit"),
	};
	PlaySFX(ImpactSounds[FMath::RandRange(0, 3)]);
}

void UCoMUnitAnimComponent::PlayDeathSound()
{
	PlaySFX(TEXT("/Game/Audio/SFX/Combat/unit_death"));
}

void UCoMUnitAnimComponent::PlayCastSound()
{
	static const FString CastSounds[] = {
		TEXT("/Game/Audio/SFX/Combat/oga_magic1"),
		TEXT("/Game/Audio/SFX/Combat/oga_spell"),
	};
	PlaySFX(CastSounds[FMath::RandRange(0, 1)]);
}
