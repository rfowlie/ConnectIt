// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_LevelConfigSettings.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Kismet/GameplayStatics.h"


UConnectIt_LevelConfigDataAsset* UConnectIt_LevelConfigSettings::FindLevelConfig(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    const UConnectIt_LevelConfigSettings* Settings = GetDefault<UConnectIt_LevelConfigSettings>();
    if (!IsValid(Settings)) return nullptr;

    const FName LevelName(*UGameplayStatics::GetCurrentLevelName(WorldContextObject, /*bRemovePrefixString=*/true));

    const TSoftObjectPtr<UConnectIt_LevelConfigDataAsset>* Entry = Settings->LevelConfigs.Find(LevelName);
    if (!Entry)
    {
        // Missing per-level registration is expected to happen (a new/
        // duplicated level nobody's added to ConnectIt_LevelConfigSettings
        // yet) -- fall back to DefaultLevelConfig with a loud Warning
        // instead of silently returning null. A null return here used to
        // cascade into every level-config-dependent system (tile registry,
        // action loadout) being silently unset, with nothing pointing back
        // at "you forgot to register this level" -- see
        // ConnectIt/_decisions/2026-09-14-level-config-default-fallback.md.
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_LevelConfigSettings: No ConnectIt_LevelConfigDataAsset "
                 "registered for level '%s' -- falling back to DefaultLevelConfig. "
                 "Add an entry for this level to silence this and use "
                 "level-specific rules/loadouts."),
            *LevelName.ToString());

        if (Settings->DefaultLevelConfig.IsNull())
        {
            UE_LOG(LogTemp, Error,
                TEXT("ConnectIt_LevelConfigSettings: ...and DefaultLevelConfig is "
                     "also unset -- no config available for level '%s'"),
                *LevelName.ToString());
            return nullptr;
        }

        return Settings->DefaultLevelConfig.LoadSynchronous();
    }

    return Entry->LoadSynchronous();
}
