// Editor-only tools. Nothing at runtime may depend on this module.

#pragma once

#include "Modules/ModuleManager.h"

class FUnrealEditorMechanicsModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
