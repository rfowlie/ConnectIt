// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "ConnectIt_WorldSettings.generated.h"

class UConnectIt_LevelConfigDataAsset;
class UConnectIt_MatchPreset;
class UConnectIt_TileCatalog;

// A ConnectIt map's own setup, in its World Settings: which board it is played
// on, how it plays by default, and which tile catalog its tiles come from. The map is the one thing that is truly per
// level, so this is where the pairing lives -- both assets stay reusable: a
// level config (the board) can serve many maps, a match preset (how a match
// is played) many levels. The main menu can still choose another preset for
// the map (FConnectItMatchSettings::MatchPreset). Duplicating a map copies
// these with it.
//
// Set as the project's World Settings class (DefaultEngine.ini,
// [/Script/Engine.Engine] WorldSettingsClassName). Read through
// FindLevelConfig / FindDefaultMatchPreset.
UCLASS()
class CONNECTIT_API AConnectIt_WorldSettings : public AWorldSettings
{
    GENERATED_BODY()

public:

    // The board this map is played on (tile and piece registries, grid
    // geometry, pieces). Every match map needs one; a menu map has none.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt")
    TObjectPtr<UConnectIt_LevelConfigDataAsset> LevelConfig = nullptr;

    // How this map plays when the main menu chooses nothing else (its
    // Adventure setup, or what opening the map directly in the editor plays)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt")
    TObjectPtr<UConnectIt_MatchPreset> DefaultMatchPreset = nullptr;

    // Every kind of tile this map's builder tiles can name: each power tag's
    // definition and the tile actor that shows it (UConnectIt_TileCatalog --
    // read through UConnectIt_TileCatalog::FindFor). None = the project
    // default (Project Settings > Game > ConnectIt).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt")
    TObjectPtr<UConnectIt_TileCatalog> TileCatalog = nullptr;

    // The level config (board) of the map WorldContextObject is in; null if
    // its World Settings name none (or aren't ConnectIt's) -- callers that
    // need a board report it. A fixed template, read by world bootstrap that
    // needs the board before anything has replicated
    // (UConnectIt_BoardRegistrySubsystem) and the piece-pool accessors of
    // UConnectIt_GameUtilityLibrary. Everything else reads live data.
    static UConnectIt_LevelConfigDataAsset* FindLevelConfig(const UObject* WorldContextObject);

    // How the map WorldContextObject is in plays when the main menu chooses
    // nothing; null, with a warning, if its World Settings name none. Read by
    // the GameMode at match setup.
    static UConnectIt_MatchPreset* FindDefaultMatchPreset(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "ConnectIt | WorldSettings")
    static const UConnectIt_TileCatalog* GetWorldTileCatalog(const UObject* WorldContextObject);

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
