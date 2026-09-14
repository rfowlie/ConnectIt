// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ConnectIt_LevelConfigSettings.generated.h"

class UConnectIt_LevelConfigDataAsset;

// Project-settings mapping of level name -> ConnectIt_LevelConfigDataAsset.
// No AssetManager/PrimaryAssetType machinery exists anywhere in this project
// to extend (confirmed during design) -- UDeveloperSettings is the
// standard, lower-ceremony way to hold a single project-wide lookup table
// like this without inventing a bootstrapping problem of its own (i.e.
// "which asset holds the mapping"). Both AConnectIt_GameMode (server) and
// AConnectIt_PlayerController (client) resolve the same entry
// independently -- see UConnectIt_GameUtilityLibrary::GetLevelConfig.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ConnectIt Level Config"))
class CONNECTIT_API UConnectIt_LevelConfigSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:


    // Used by GetLevelConfig when the current level's name has no entry above
    // -- e.g. a newly duplicated/created level nobody's registered yet.
    // Logged as a Warning when this fallback is taken (register the level
    // properly to silence it) rather than the previous hard Error-and-null,
    // which used to leave every level-config-dependent system (tile
    // registry, action loadout) silently unset with no obvious cause.
    UPROPERTY(EditAnywhere, Config, Category = "ConnectIt")
    TSoftObjectPtr<UConnectIt_LevelConfigDataAsset> DefaultLevelConfig;
    
    UPROPERTY(EditAnywhere, Config, Category = "ConnectIt")
    TMap<FName, TSoftObjectPtr<UConnectIt_LevelConfigDataAsset>> LevelConfigs;

};
