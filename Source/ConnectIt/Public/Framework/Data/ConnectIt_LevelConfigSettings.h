// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ConnectIt_LevelConfigSettings.generated.h"

class UConnectIt_LevelConfigDataAsset;
class UConnectIt_LevelCatalog;

// Project-settings mapping of level name -> ConnectIt_LevelConfigDataAsset.
// No AssetManager/PrimaryAssetType machinery exists anywhere in this project
// to extend (confirmed during design) -- UDeveloperSettings is the
// standard, lower-ceremony way to hold a single project-wide lookup table
// like this without inventing a bootstrapping problem of its own (i.e.
// "which asset holds the mapping"). The asset is only a level's STARTING
// TEMPLATE -- see FindLevelConfig below for who may read it and where live
// match data is read instead.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ConnectIt Level Config"))
class CONNECTIT_API UConnectIt_LevelConfigSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:

    // The level config asset registered for the level WorldContextObject is in
    // (DefaultLevelConfig, with a warning, if the level has no entry; null,
    // with an error, if there is none at all).
    //
    // This is the level's STARTING TEMPLATE, not the live match: rules, the
    // player's loadout and the AI opponent can all differ once a match is
    // running. C++ only, and meant for exactly two readers --
    //   * AConnectIt_GameMode, which resolves the match setup from it once
    //     and publishes the live values (GameState / PlayerStates), and
    //   * world bootstrap that needs fixed level setup before anything has
    //     replicated (UConnectIt_BoardRegistrySubsystem).
    // Everything else reads live data: UConnectIt_GameUtilityLibrary's
    // GetMatchRules / GetLocalPlayerLoadout / GetOpponentProfile, or its
    // fixed-setup accessors (GetPieceActorClass, ...).
    static UConnectIt_LevelConfigDataAsset* FindLevelConfig(const UObject* WorldContextObject);


    // Used by FindLevelConfig when the current level's name has no entry above
    // -- e.g. a newly duplicated/created level nobody's registered yet.
    // Logged as a Warning when this fallback is taken (register the level
    // properly to silence it) rather than the previous hard Error-and-null,
    // which used to leave every level-config-dependent system (tile
    // registry, action loadout) silently unset with no obvious cause.
    UPROPERTY(EditAnywhere, Config, Category = "ConnectIt")
    TSoftObjectPtr<UConnectIt_LevelConfigDataAsset> DefaultLevelConfig;
    
    UPROPERTY(EditAnywhere, Config, Category = "ConnectIt")
    TMap<FName, TSoftObjectPtr<UConnectIt_LevelConfigDataAsset>> LevelConfigs;

    // What the main menu offers: playable levels and AI opponents. Read via
    // UConnectIt_MatchSetupSubsystem::GetLevelCatalog.
    UPROPERTY(EditAnywhere, Config, Category = "ConnectIt|Main Menu")
    TSoftObjectPtr<UConnectIt_LevelCatalog> LevelCatalog;

};
