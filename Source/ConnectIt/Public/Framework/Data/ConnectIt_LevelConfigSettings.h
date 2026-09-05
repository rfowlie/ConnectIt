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

    UPROPERTY(EditAnywhere, Config, Category = "ConnectIt")
    TMap<FName, TSoftObjectPtr<UConnectIt_LevelConfigDataAsset>> LevelConfigs;
};
