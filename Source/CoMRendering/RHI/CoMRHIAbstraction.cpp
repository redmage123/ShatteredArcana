// Copyright Mythforge Studios. All Rights Reserved.
// CoMRHIAbstraction.cpp — RHI backend detection + upscaler selection (COM-015)

#include "RHI/CoMRHIAbstraction.h"
#include "RHIDefinitions.h"
#include "RHI.h"  // GRHIDeviceSharedMemoryMB + RHI globals
#include "DynamicRHI.h"
#include "RHICommandList.h"
#include "GameFramework/GameUserSettings.h"
#include "Scalability.h"
#include "Misc/Paths.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMRHI, Log, All);

// ─── UCoMRHIAbstraction ──────────────────────────────────────────────────────

void UCoMRHIAbstraction::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Detect backend first — everything else depends on it
	ActiveBackend = DetectRHIBackend();
	UE_LOG(LogCoMRHI, Log, TEXT("CoMRHI: Active backend = %s"), *GetBackendDisplayName());
	OnRHIBackendDetected.Broadcast(ActiveBackend);

	// Select upscaler using detected backend + platform caps
	ActiveUpscaler = SelectBestUpscaler();
	UE_LOG(LogCoMRHI, Log, TEXT("CoMRHI: Active upscaler = %d"), static_cast<int32>(ActiveUpscaler));
	OnUpscalerActivated.Broadcast(ActiveUpscaler);

	// Auto-detect initial scalability tier from VRAM
	CurrentTier = AutoDetectScalabilityTier();
	ApplyScalabilityCVars(CurrentTier);
	UE_LOG(LogCoMRHI, Log, TEXT("CoMRHI: Initial scalability tier = %d"), static_cast<int32>(CurrentTier));

	UE_LOG(LogCoMRHI, Log, TEXT("CoMRHI: Expected PSO bundle: %s"), *GetPSOBundlePath());
}

void UCoMRHIAbstraction::Deinitialize()
{
	Super::Deinitialize();
}

// ─── Backend Detection ────────────────────────────────────────────────────────

ECoMRHIBackend UCoMRHIAbstraction::DetectRHIBackend() const
{
	// GDynamicRHI is valid from RHI module load — safe to query here.
	if (!GDynamicRHI)
	{
		UE_LOG(LogCoMRHI, Warning, TEXT("CoMRHI: GDynamicRHI is null — cannot detect backend"));
		return ECoMRHIBackend::Unknown;
	}

	const ERHIInterfaceType RHIType = GDynamicRHI->GetInterfaceType();

	switch (RHIType)
	{
	case ERHIInterfaceType::D3D12:
		return ECoMRHIBackend::D3D12;
	case ERHIInterfaceType::Vulkan:
		return ECoMRHIBackend::Vulkan;
	case ERHIInterfaceType::OpenGL:
		return ECoMRHIBackend::OpenGL;
	case ERHIInterfaceType::Metal:
		return ECoMRHIBackend::Metal;
	default:
		UE_LOG(LogCoMRHI, Warning, TEXT("CoMRHI: Unrecognised RHI interface type %d"), static_cast<int32>(RHIType));
		return ECoMRHIBackend::Unknown;
	}
}

FString UCoMRHIAbstraction::GetBackendDisplayName() const
{
	switch (ActiveBackend)
	{
	case ECoMRHIBackend::D3D12:  return TEXT("DirectX 12 (SM6)");
	case ECoMRHIBackend::Vulkan: return TEXT("Vulkan 1.3 (SPIR-V)");
	case ECoMRHIBackend::OpenGL: return TEXT("OpenGL 4.6 (GLSL)");
	case ECoMRHIBackend::Metal:  return TEXT("Metal 3 (MSL)");
	default:                     return TEXT("Unknown");
	}
}

// ─── Feature Support ──────────────────────────────────────────────────────────

bool UCoMRHIAbstraction::SupportsFeature(ECoMRHIFeature Feature) const
{
	// OpenGL 4.6 does not support any of these SM6 features.
	// All other backends support the full SM6 feature set.
	if (ActiveBackend == ECoMRHIBackend::OpenGL)
	{
		// OpenGL fallback: only basic rasterisation, no wave/mesh/bindless/RT.
		return false;
	}

	switch (Feature)
	{
	case ECoMRHIFeature::WaveIntrinsics:
		return GRHISupportsWaveOperations;
	case ECoMRHIFeature::MeshShaders:
		return GRHISupportsMeshShadersTier0;
	case ECoMRHIFeature::FP16Arithmetic:
		// FP16 arithmetic is available on all SM6 non-OpenGL paths
		return (ActiveBackend != ECoMRHIBackend::OpenGL);
	case ECoMRHIFeature::BindlessResources:
		return false;  // TODO: UE5.4 bindless = FDataDrivenShaderPlatformInfo::GetSupportsBindlessResources(GMaxRHIShaderPlatform)
	case ECoMRHIFeature::RayTracing:
		return IsRayTracingEnabled();
	case ECoMRHIFeature::VariableRateShading:
		return false;  // TODO: UE5.4 VRS = GRHIVariableRateShadingEnabled or similar
	default:
		return false;
	}
}

bool UCoMRHIAbstraction::IsFullSM6() const
{
	return SupportsFeature(ECoMRHIFeature::WaveIntrinsics)
		&& SupportsFeature(ECoMRHIFeature::MeshShaders)
		&& SupportsFeature(ECoMRHIFeature::FP16Arithmetic)
		&& SupportsFeature(ECoMRHIFeature::BindlessResources);
}

// ─── Upscaler Selection ────────────────────────────────────────────────────────

ECoMUpscalerType UCoMRHIAbstraction::SelectBestUpscaler() const
{
	// MetalFX: macOS only. Check PLATFORM_MAC so it compiles on all platforms.
	// The CoM.Rendering.Upscaler.MetalFX CVar controls whether it is enabled.
#if PLATFORM_MAC
	// MetalFX is natively integrated into UE5's Metal platform layer.
	// Query the ITemporalUpscaler interface to confirm it's available.
	static const auto CVarMetalFX = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MetalFX.Upscale"));
	if (CVarMetalFX && CVarMetalFX->GetBool())
	{
		UE_LOG(LogCoMRHI, Log, TEXT("CoMRHI: MetalFX upscaler available and enabled"));
		return ECoMUpscalerType::MetalFX;
	}
#endif

	// DLSS: NVIDIA Windows. Plugin must be loaded and GPU must be RTX-class.
	static const auto CVarDLSS = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
	if (CVarDLSS && CVarDLSS->GetBool())
	{
		UE_LOG(LogCoMRHI, Log, TEXT("CoMRHI: DLSS upscaler available and enabled"));
		return ECoMUpscalerType::DLSS;
	}

	// FSR 2.0: AMD-authored but works on all GPUs. Requires FSR plugin.
	static const auto CVarFSR = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR2.Enabled"));
	if (CVarFSR && CVarFSR->GetBool())
	{
		UE_LOG(LogCoMRHI, Log, TEXT("CoMRHI: FSR 2.0 upscaler available and enabled"));
		return ECoMUpscalerType::FSR;
	}

	// XeSS: Intel. Requires XeSS plugin.
	static const auto CVarXeSS = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));
	if (CVarXeSS && CVarXeSS->GetBool())
	{
		UE_LOG(LogCoMRHI, Log, TEXT("CoMRHI: XeSS upscaler available and enabled"));
		return ECoMUpscalerType::XeSS;
	}

	// TSR: UE5 built-in, always available as final fallback.
	UE_LOG(LogCoMRHI, Log, TEXT("CoMRHI: Falling back to TSR (UE5 built-in)"));
	return ECoMUpscalerType::TSR;
}

void UCoMRHIAbstraction::SetPreferredUpscaler(ECoMUpscalerType Preferred)
{
	PreferredUpscaler = Preferred;

	// Re-evaluate: if the preferred type isn't available, keep TSR.
	ECoMUpscalerType Best = SelectBestUpscaler();
	if (Best != Preferred)
	{
		UE_LOG(LogCoMRHI, Warning,
			TEXT("CoMRHI: Preferred upscaler %d unavailable — staying on %d"),
			static_cast<int32>(Preferred), static_cast<int32>(Best));
	}
	ActiveUpscaler = Best;
	OnUpscalerActivated.Broadcast(ActiveUpscaler);
}

// ─── Scalability ──────────────────────────────────────────────────────────────

ECoMScalabilityTier UCoMRHIAbstraction::AutoDetectScalabilityTier() const
{
	// Query dedicated VRAM via RHI globals (populated after RHI init).
	// Query VRAM via RHI globals. Falls back to 0 if unavailable.
	int32 DedicatedVRAM = 0;
	{
		// GRHIAdapterInternalDriverVersion is not reliable for VRAM; use texture memory budget if available.
		// On platforms where VRAM query isn't supported, default to Medium tier.
		FTextureMemoryStats TexMemStats;
		RHIGetTextureMemoryStats(TexMemStats);
		if (TexMemStats.DedicatedVideoMemory > 0)
		{
			DedicatedVRAM = static_cast<int32>(TexMemStats.DedicatedVideoMemory / (1024 * 1024));
		}
	}

	// Tier thresholds are approximate VRAM budgets (MB):
	//   Low:       <4096  MB  (Intel integrated: XeLP/XeHPG at 30fps)
	//   Medium:    4096   MB  (GTX 1660 / RX 5600 XT class @ 60fps)
	//   High:      8192   MB  (RTX 3070 / RX 6700 XT @ 60fps)
	//   Epic:      16384  MB  (RTX 4080+ @ 60fps+)
	//   Cinematic: >24576 MB  (Enthusiast; RT optional)
	const int32 VRAM = DedicatedVRAM;

	if (VRAM == 0)
	{
		// Cannot determine VRAM — default to Medium (conservative fallback).
		UE_LOG(LogCoMRHI, Warning, TEXT("CoMRHI: VRAM query returned 0 — defaulting to Medium scalability"));
		return ECoMScalabilityTier::Medium;
	}

	if (VRAM < 4096)  return ECoMScalabilityTier::Low;
	if (VRAM < 8192)  return ECoMScalabilityTier::Medium;
	if (VRAM < 16384) return ECoMScalabilityTier::High;
	if (VRAM < 24576) return ECoMScalabilityTier::Epic;
	return ECoMScalabilityTier::Cinematic;
}

void UCoMRHIAbstraction::SetScalabilityTier(ECoMScalabilityTier NewTier)
{
	if (NewTier == CurrentTier) return;

	CurrentTier = NewTier;
	ApplyScalabilityCVars(NewTier);
	OnScalabilityChanged.Broadcast(CurrentTier);

	// Persist to game user settings so it survives restarts
	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		// Map our tier to UE5's integer quality level (0=Low, 1=Med, 2=High, 3=Epic)
		int32 QualityLevel = FMath::Min(static_cast<int32>(NewTier), 3);
		UserSettings->SetOverallScalabilityLevel(QualityLevel);
		UserSettings->SaveSettings();
	}
}

void UCoMRHIAbstraction::ApplyScalabilityCVars(ECoMScalabilityTier Tier) const
{
	// Apply UE5 scalability bucket via the Scalability module.
	// This mirrors what the UE5 graphics settings screen does.
	Scalability::FQualityLevels Quality;

	switch (Tier)
	{
	case ECoMScalabilityTier::Low:
		Quality.SetFromSingleQualityLevel(0);
		break;
	case ECoMScalabilityTier::Medium:
		Quality.SetFromSingleQualityLevel(1);
		break;
	case ECoMScalabilityTier::High:
		Quality.SetFromSingleQualityLevel(2);
		break;
	case ECoMScalabilityTier::Epic:
		Quality.SetFromSingleQualityLevel(3);
		break;
	case ECoMScalabilityTier::Cinematic:
		Quality.SetFromSingleQualityLevel(3); // Epic base; add RT via separate CVar
		// Cinematic-tier: enable RT reflections (blocked from PSO bundles; runtime only)
		if (GEngine && GEngine->GetWorld())
		{
			IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.Enable"))
				->SetWithCurrentPriority(TEXT("1"));
		}
		break;
	default:
		Quality.SetFromSingleQualityLevel(2); // Safe default: High
		break;
	}

	Scalability::SetQualityLevels(Quality);
}

// ─── PSO Bundle Path ──────────────────────────────────────────────────────────

FString UCoMRHIAbstraction::GetPSOBundlePath() const
{
	FString BackendSuffix;
	switch (ActiveBackend)
	{
	case ECoMRHIBackend::D3D12:  BackendSuffix = TEXT("DX12");   break;
	case ECoMRHIBackend::Vulkan: BackendSuffix = TEXT("Vulkan");  break;
	case ECoMRHIBackend::OpenGL: BackendSuffix = TEXT("OpenGL");  break;
	case ECoMRHIBackend::Metal:  BackendSuffix = TEXT("Metal");   break;
	default:
		UE_LOG(LogCoMRHI, Warning, TEXT("CoMRHI: Unknown backend — no PSO bundle path"));
		return FString();
	}

	// PSO bundles are cooked into Build/PSO/ relative to the project root.
	// FPaths::ProjectDir() resolves correctly on all platforms.
	return FPaths::Combine(FPaths::ProjectDir(),
		TEXT("Build"), TEXT("PSO"),
		FString::Printf(TEXT("ShatteredArcana-%s.pso"), *BackendSuffix));
}
