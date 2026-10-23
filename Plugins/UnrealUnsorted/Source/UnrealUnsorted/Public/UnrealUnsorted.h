// Holding area for code migrated from other plugins/projects. Nothing may depend on this module.

#pragma once

#include "Modules/ModuleManager.h"

class FUnrealUnsortedModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
