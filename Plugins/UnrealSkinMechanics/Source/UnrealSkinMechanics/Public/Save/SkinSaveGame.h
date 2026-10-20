// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "SkinSaveGame.generated.h"

// The player's saved skin choice: a preset plus optional per-category overrides
// ("Preset A, but Preset B's pieces"). IDs (table row names), never asset references, so
// a renamed or removed skin falls back instead of breaking the save.
UCLASS()
class UNREALSKINMECHANICS_API USkinSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FName PresetId;

	UPROPERTY()
	TMap<FGameplayTag, FName> Overrides;
};
