// VRM4U Copyright (c) 2019 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once
//#include "Engine.h"
#include "ModuleManager.h"

class FVRM4UModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};