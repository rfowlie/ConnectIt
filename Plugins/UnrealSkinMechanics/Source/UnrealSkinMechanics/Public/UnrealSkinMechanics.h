// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

UNREALSKINMECHANICS_API DECLARE_LOG_CATEGORY_EXTERN(LogSkinMechanics, Log, All);

class FUnrealSkinMechanicsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
