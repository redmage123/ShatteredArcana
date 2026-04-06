// Copyright Mythforge Studios. All Rights Reserved.
// CoMPSOCache.cpp — PSO bundle loading implementation (COM-015)

#include "PSO/CoMPSOCache.h"
#include "RHI/CoMRHIAbstraction.h"
#include "ShaderPipelineCache.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMPSO, Log, All);

// ─── Initialization ───────────────────────────────────────────────────────────

void UCoMPSOCache::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Ensure RHI abstraction is up before we ask for the bundle path.
	Collection.InitializeDependency<UCoMRHIAbstraction>();

	UCoMRHIAbstraction* RHIAbstraction =
		GetGameInstance()->GetSubsystem<UCoMRHIAbstraction>();

	if (!RHIAbstraction)
	{
		UE_LOG(LogCoMPSO, Error,
			TEXT("CoMPSO: UCoMRHIAbstraction not available — PSO cache disabled"));
		HandleCacheMiss(TEXT("<RHI abstraction unavailable>"));
		return;
	}

	const FString BundlePath = RHIAbstraction->GetPSOBundlePath();
	if (BundlePath.IsEmpty())
	{
		UE_LOG(LogCoMPSO, Warning,
			TEXT("CoMPSO: RHI abstraction returned empty PSO bundle path — skipping cache load"));
		HandleCacheMiss(BundlePath);
		return;
	}

	// Attempt to load the bundle.
	const bool bLoaded = TryLoadBundle(BundlePath);
	if (!bLoaded)
	{
		HandleCacheMiss(BundlePath);
	}
}

void UCoMPSOCache::Deinitialize()
{
	if (bCacheLoaded)
	{
		// Close the pipeline file cache cleanly on exit.
		FShaderPipelineCache::Shutdown();
		UE_LOG(LogCoMPSO, Log, TEXT("CoMPSO: FShaderPipelineCache shut down cleanly"));
	}
	Super::Deinitialize();
}

// ─── Bundle Loading ────────────────────────────────────────────────────────────

bool UCoMPSOCache::TryLoadBundle(const FString& BundlePath)
{
	// Guard: bundle file must exist on disk before we try to open it.
	if (!IFileManager::Get().FileExists(*BundlePath))
	{
		UE_LOG(LogCoMPSO, Warning,
			TEXT("CoMPSO: Bundle not found at '%s'"), *BundlePath);
		return false;
	}

	UE_LOG(LogCoMPSO, Log,
		TEXT("CoMPSO: Opening PSO bundle '%s'"), *BundlePath);

	// FShaderPipelineCache::OpenPipelineFileCache expects the name WITHOUT
	// extension — the engine locates the file in Build/PSO/ automatically when
	// r.ShaderPipelineCache.Enabled=1 and r.ShaderPipelineCache.FileName is set.
	//
	// We set the CVar here so the engine uses our resolved bundle path.
	IConsoleVariable* CVarFileName =
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.FileName"));
	if (CVarFileName)
	{
		// Strip extension and directory — the cache system resolves Build/PSO/ itself.
		const FString BundleName = FPaths::GetBaseFilename(BundlePath);
		CVarFileName->SetWithCurrentPriority(*BundleName);
		UE_LOG(LogCoMPSO, Log,
			TEXT("CoMPSO: Set r.ShaderPipelineCache.FileName = %s"), *BundleName);
	}

	// Open the file-backed pipeline cache. This triggers asynchronous replay of
	// pre-recorded PSOs on the RHI thread — no game thread stall.
	const bool bOpened = FShaderPipelineCache::OpenPipelineFileCache(
		GMaxRHIShaderPlatform);

	if (bOpened)
	{
		bCacheLoaded = true;
		LoadedBundlePath = BundlePath;

		// Set replay batch size: how many PSOs to warm per frame during loading.
		// 50 per frame balances first-boot warmup speed vs frame-time spike.
		FShaderPipelineCache::SetBatchMode(FShaderPipelineCache::BatchMode::Fast);

		UE_LOG(LogCoMPSO, Log,
			TEXT("CoMPSO: Pipeline cache opened successfully (async PSO replay active)"));

		OnPSOCacheLoaded.Broadcast(true);
		return true;
	}
	else
	{
		UE_LOG(LogCoMPSO, Warning,
			TEXT("CoMPSO: FShaderPipelineCache::OpenPipelineFileCache failed for '%s'"),
			*BundlePath);
		return false;
	}
}

bool UCoMPSOCache::ReloadBundle(const FString& BundlePath)
{
	// Close the current cache (if any) before reopening.
	if (bCacheLoaded)
	{
		FShaderPipelineCache::Shutdown();
		bCacheLoaded = false;
		LoadedBundlePath.Empty();
		ReplayedPSOCount = 0;
		UE_LOG(LogCoMPSO, Log, TEXT("CoMPSO: Closed previous PSO bundle for reload"));
	}

	const bool bLoaded = TryLoadBundle(BundlePath);
	if (!bLoaded)
	{
		HandleCacheMiss(BundlePath);
	}
	return bLoaded;
}

void UCoMPSOCache::HandleCacheMiss(const FString& BundlePath)
{
	bCacheLoaded = false;
	LoadedBundlePath.Empty();
	ReplayedPSOCount = 0;

	UE_LOG(LogCoMPSO, Warning,
		TEXT("CoMPSO: Cache miss for '%s'. "
		     "Engine will compile PSOs on demand — expect brief stutter on first encounter. "
		     "Re-cook the project with -run=ShaderCompileWorker to generate the bundle."),
		*BundlePath);

	OnPSOCacheLoaded.Broadcast(false);
	OnPSOCacheMiss.Broadcast(BundlePath);
}
