// Copyright Mythforge Studios. All Rights Reserved.
// CoMPSOCache.h — PSO bundle loading and fallback management (COM-015)
// Owner: Lead Engineer
//
// UCoMPSOCache loads the correct platform PSO bundle at startup so the engine
// can warm the shader pipeline cache and avoid first-boot stutter.
// On cache miss it logs a warning and continues — the engine will compile PSOs
// on demand (performance hit) but the game remains functional.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMPSOCache.generated.h"

// ─── Delegates ────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPSOCacheLoaded, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPSOCacheMiss, FString, BundlePath);

// ─── UCoMPSOCache ─────────────────────────────────────────────────────────────

/**
 * Game-instance subsystem that loads the platform-appropriate PSO bundle
 * (ShatteredArcana-{DX12|Vulkan|OpenGL|Metal}.pso) and activates the
 * UE5 shader pipeline cache so that PSOs do not stutter on first draw.
 *
 * Must initialize AFTER UCoMRHIAbstraction (uses its GetPSOBundlePath()).
 *
 * If the bundle file does not exist (e.g. dev build, first launch, or the PSO
 * was not cooked for this platform), OnCacheMiss is called and the subsystem
 * remains functional — the engine stubs in on-demand PSO compilation.
 */
UCLASS(DisplayName = "CoM PSO Cache")
class COMRENDERING_API UCoMPSOCache : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── USubsystem ──────────────────────────────────────────────────────────

	/** Depends on UCoMRHIAbstraction being initialized first. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	// ── State Queries ────────────────────────────────────────────────────────

	/** True when a PSO bundle was found and successfully opened. */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering|PSO")
	bool IsCacheLoaded() const { return bCacheLoaded; }

	/** Absolute path of the bundle that was loaded (empty on cache miss). */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering|PSO")
	FString GetLoadedBundlePath() const { return LoadedBundlePath; }

	/** How many PSOs were replayed from the loaded bundle (0 on cache miss). */
	UFUNCTION(BlueprintPure, Category = "CoM|Rendering|PSO")
	int32 GetReplayedPSOCount() const { return ReplayedPSOCount; }

	// ── Manual Reload ────────────────────────────────────────────────────────

	/**
	 * Attempt to reload the PSO bundle for the given path.
	 * Useful after a DLC install or patch that ships updated .pso files.
	 * Returns true on success.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Rendering|PSO")
	bool ReloadBundle(const FString& BundlePath);

	// ── Delegates ────────────────────────────────────────────────────────────

	/** Fired when a PSO bundle is loaded (bSuccess=false means cache miss). */
	UPROPERTY(BlueprintAssignable, Category = "CoM|Rendering|PSO")
	FOnPSOCacheLoaded OnPSOCacheLoaded;

	/** Fired specifically on cache miss — bundle file not found. */
	UPROPERTY(BlueprintAssignable, Category = "CoM|Rendering|PSO")
	FOnPSOCacheMiss OnPSOCacheMiss;

private:
	// ── Internal ─────────────────────────────────────────────────────────────

	/** Open the bundle at BundlePath via FShaderPipelineCache. Returns true on success. */
	bool TryLoadBundle(const FString& BundlePath);

	/** Called when the target .pso file does not exist on disk. */
	void HandleCacheMiss(const FString& BundlePath);

	// ── State ─────────────────────────────────────────────────────────────────

	UPROPERTY()
	bool bCacheLoaded = false;

	UPROPERTY()
	FString LoadedBundlePath;

	UPROPERTY()
	int32 ReplayedPSOCount = 0;
};
