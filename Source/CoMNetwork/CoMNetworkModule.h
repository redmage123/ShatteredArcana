// Copyright Mythforge Studios. All Rights Reserved.
// CoMNetworkModule.h — Module declaration for CoMNetwork
// Owner: Network Architect
// NEVER import CoMAI, CoMUI, or CoMRendering

#pragma once
#include "Modules/ModuleManager.h"

class FCoMNetworkModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
