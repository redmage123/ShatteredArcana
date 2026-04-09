// Copyright Mythforge Studios. All Rights Reserved.
#include "CoMCore.h"
#include "Modules/ModuleManager.h"

class FCoMCoreModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogTemp, Log, TEXT("[CoMCore] Module loaded — UObjects registered."));
	}
};

IMPLEMENT_GAME_MODULE(FCoMCoreModule, CoMCore);
