// Copyright Mythforge Studios. All Rights Reserved.
// CoMRHIAbstraction.h — RHI backend detection and capability queries (COM-015)
// Owner: Lead Engineer
// Do NOT call DX12/Vulkan/Metal/OpenGL APIs directly.
// All rendering is through UE5 RHI exclusively.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMRHIAbstraction.generated.h"

// ─── Enumerations ────────────────────────────────────────────────────────────

/** Which UE5 RHI backend is active at runtime. */
UENUM(BlueprintType)
enum class ECoMRHIBackend : uint8
{
	Unknown    UMETA(DisplayName = "Unknown"),
	D3D12      UMETA(DisplayName = "DirectX 12"),
	Vulkan     UMETA(DisplayName = "Vulkan 1.3"),
	OpenGL     UMETA(DisplayName = "OpenGL 4.6"),
	Metal      UMETA(DisplayName = "Metal 3"),
};

/** Active GPU upscaler. TSR is the universal fallback when no vendor upscaler is present. */
UENUM(BlueprintType)
enum class ECoMUpscalerType : uint8
{
	None       UMETA(DisplayName = "None / Native"),
	TSR        UMETA(DisplayName = "TSR (UE5 built-in)"),  // always available
	DLSS       UMETA(DisplayName = "DLSS (NVIDIA)"),
	FSR        UMETA(DisplayName = "FSR 2.0 (AMD)"),
	XeSS       UMETA(DisplayName = "XeSS (Intel)"),
	MetalFX    UMETA(DisplayName = "MetalFX (Apple)"),
};

/** Optional SM6 features — require an OpenGL fallback path when absent. */
UENUM(BlueprintType)
enum class ECoMRHIFeature : uint8
{
	WaveIntrinsics     UMETA(DisplayName = "SM6 Wave Intrinsics"),
	MeshShaders        UMETA(DisplayName = "Mesh Shaders"),
	FP16Arithmetic     UMETA(DisplayName = "16-bit FP Arithmetic"),
	BindlessResources  UMETA(DisplayName = "Bindless Resources"),
	RayTracing         UMETA(DisplayName = "Ray Tracing"),
	VariableRateShading UMETA(DisplayName = "Variable Rate Shading"),
};

/** Graphics scalability tier — maps to DefaultScalability.ini group. */
UENUM(BlueprintType)
enum class ECoMScalabilityTier : uint8
{
	Low        UMETA(DisplayName = "Low"),       // Intel integrated / Xe @ 30fps
	Medium     UMETA(DisplayName = "Medium"),    // GTX 1660-class @ 60fps
	High       UMETA(DisplayName = "High"),      // RTX 3070 / RX 6700 XT @ 60fps
	Epic       UMETA(DisplayName = "Epic"),      // RTX 4080+ @ 60fps+
	Cinematic  UMETA(DisplayName = "Cinematic"), // Enthusiast; RT optional
};

// ─── Delegates ───────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRHIBackendDetected, ECoMRHIBackend, Backend);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpscalerActivated, ECoMUpscalerType, Upscaler);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScalabilityChanged, ECoMScalabilityTier, Tier);

// ─── UCoMRHIAbstraction ──────────────────────────────────────────────────────

/**
 * Game-instance subsystem that detects the active UE5 RHI backend, queries
 * SM6 feature support, selects the best available upscaler, and exposes a
 * unified capability interface to the rest of the game.
 *
 * All platform-conditional logic uses UE5 PLATFORM_* macros and FDynamicRHI
 * queries — never raw DX12/Vulkan/Metal/OpenGL headers.
 *
 * Initialization order: UCoMRHIAbstraction → UCoMPSOCache (depends on
 * GetPSOBundlePath() result from this subsystem).
 */
UCLASS(DisplayName = "CoM RHI Abstraction")
class COMRENDERING_API UCoMRHIAbstraction : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── USubsystem ──────────────────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	// ── Backend & Feature Queries ───────────────────────────────────────────

	/** Returns which RHI is active (detected once at Initialize). */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering")
	ECoMRHIBackend GetActiveBackend() const { return ActiveBackend; }

	/** Human-readable name of the active RHI backend. */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering")
	FString GetBackendDisplayName() const;

	/**
	 * Returns true when the active RHI supports the queried SM6 feature.
	 * OpenGL returns false for all SM6-exclusive features.
	 * Use this to decide whether to enable high-quality shader variants or
	 * fall back to the _OpenGL permutation.
	 */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering")
	bool SupportsFeature(ECoMRHIFeature Feature) const;

	/** True when the GPU fully supports SM6 (Wave, Mesh, FP16, Bindless). */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering")
	bool IsFullSM6() const;

	// ── Upscaler ────────────────────────────────────────────────────────────

	/** Returns the best upscaler that was successfully initialised. */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering|Upscaling")
	ECoMUpscalerType GetActiveUpscaler() const { return ActiveUpscaler; }

	/** Override the upscaler (user setting change). Falls back to TSR if the
	 *  requested type is unavailable. Applied on next frame begin. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Rendering|Upscaling")
	void SetPreferredUpscaler(ECoMUpscalerType Preferred);

	// ── Scalability ──────────────────────────────────────────────────────────

	/** Current scalability tier applied to the engine scalability system. */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering|Scalability")
	ECoMScalabilityTier GetCurrentScalabilityTier() const { return CurrentTier; }

	/**
	 * Change the scalability tier at runtime. Applies the matching
	 * DefaultScalability.ini group via UGameUserSettings and re-applies CVars.
	 * Fires OnScalabilityChanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Rendering|Scalability")
	void SetScalabilityTier(ECoMScalabilityTier NewTier);

	/**
	 * Auto-detect the recommended tier from available GPU VRAM.
	 * Thresholds (approximate): <4 GB → Low, 4-8 GB → Medium,
	 * 8-16 GB → High, 16-24 GB → Epic, >24 GB → Cinematic.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Rendering|Scalability")
	ECoMScalabilityTier AutoDetectScalabilityTier() const;

	// ── PSO Support ──────────────────────────────────────────────────────────

	/**
	 * Returns the expected path for the PSO bundle file that matches the
	 * active backend (e.g. "Build/PSO/ShatteredArcana-Vulkan.pso").
	 * Used by UCoMPSOCache to locate and load the correct bundle.
	 */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering|PSO")
	FString GetPSOBundlePath() const;

	// ── Delegates ────────────────────────────────────────────────────────────

	/** Fired once after RHI backend detection during Initialize. */
	UPROPERTY(BlueprintAssignable, Category = "CoM|Rendering")
	FOnRHIBackendDetected OnRHIBackendDetected;

	/** Fired when an upscaler is successfully activated. */
	UPROPERTY(BlueprintAssignable, Category = "CoM|Rendering|Upscaling")
	FOnUpscalerActivated OnUpscalerActivated;

	/** Fired whenever the scalability tier changes. */
	UPROPERTY(BlueprintAssignable, Category = "CoM|Rendering|Scalability")
	FOnScalabilityChanged OnScalabilityChanged;

private:
	// ── Internal Init Steps ──────────────────────────────────────────────────

	/** Query FDynamicRHI to determine which backend is loaded. */
	ECoMRHIBackend DetectRHIBackend() const;

	/**
	 * Attempt to activate vendor upscalers in priority order:
	 * Platform-specific first (DLSS/MetalFX), then FSR/XeSS, then TSR.
	 * TSR is always available as the final fallback.
	 */
	ECoMUpscalerType SelectBestUpscaler() const;

	/** Apply CVar overrides for the given scalability tier. */
	void ApplyScalabilityCVars(ECoMScalabilityTier Tier) const;

	// ── State ────────────────────────────────────────────────────────────────

	UPROPERTY()
	ECoMRHIBackend ActiveBackend = ECoMRHIBackend::Unknown;

	UPROPERTY()
	ECoMUpscalerType ActiveUpscaler = ECoMUpscalerType::TSR;

	UPROPERTY()
	ECoMScalabilityTier CurrentTier = ECoMScalabilityTier::High;

	UPROPERTY()
	ECoMUpscalerType PreferredUpscaler = ECoMUpscalerType::TSR;
};
