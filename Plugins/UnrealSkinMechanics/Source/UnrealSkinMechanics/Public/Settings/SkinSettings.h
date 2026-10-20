// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "SkinSettings.generated.h"

class UDataTable;

// Project-settings entry point for the skin system. Stored in the project's
// Config/DefaultGame.ini under [/Script/UnrealSkinMechanics.SkinSettings].
//
// Skin tables are project-defined: each table's row struct is whatever the game needs
// to describe one skin of that category. The plugin only uses row NAMES as stable IDs.
// One table per category -- to let a content pack extend a category, point the map at a
// UCompositeDataTable that merges the pack's table into the base one.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Skin Mechanics"))
class UNREALSKINMECHANICS_API USkinSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	static const USkinSettings* Get() { return GetDefault<USkinSettings>(); }

	// Category tag (e.g. Visual.Piece) -> the DataTable of that category's skins.
	// The keys are also the list of categories the player can customise.
	UPROPERTY(EditAnywhere, Config, Category = "Skins")
	TMap<FGameplayTag, TSoftObjectPtr<UDataTable>> SkinTables;

	// DataTable of presets. Its row struct must derive from FSkinPresetRowBase (checked
	// at runtime -- the asset picker can't filter on derived structs).
	UPROPERTY(EditAnywhere, Config, Category = "Skins")
	TSoftObjectPtr<UDataTable> PresetTable;

	// Preset used when the player has no saved selection, or a saved ID no longer
	// resolves. The fallback of last resort.
	UPROPERTY(EditAnywhere, Config, Category = "Skins")
	FName DefaultPresetId;

	// Save-game slot the player's selection is stored in.
	UPROPERTY(EditAnywhere, Config, Category = "Persistence")
	FString SaveSlotName = TEXT("SkinSelection");

	UPROPERTY(EditAnywhere, Config, Category = "Persistence")
	int32 SaveUserIndex = 0;
};
