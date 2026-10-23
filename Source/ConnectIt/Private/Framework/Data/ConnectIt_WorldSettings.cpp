// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_MatchPreset.h"
#include "Board/Powers/ConnectIt_TileCatalog.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"


namespace
{
    const UWorld* GetContextWorld(const UObject* WorldContextObject)
    {
        return GEngine && IsValid(WorldContextObject)
            ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
            : nullptr;
    }
}

UConnectIt_LevelConfigDataAsset* AConnectIt_WorldSettings::FindLevelConfig(const UObject* WorldContextObject)
{
    const UWorld* World = GetContextWorld(WorldContextObject);
    if (!World) return nullptr;

    // Silent when there is none: a menu map has no board; the callers that
    // need one say so
    const AConnectIt_WorldSettings* WorldSettings = Cast<AConnectIt_WorldSettings>(World->GetWorldSettings());
    return WorldSettings ? WorldSettings->LevelConfig.Get() : nullptr;
}

UConnectIt_MatchPreset* AConnectIt_WorldSettings::FindDefaultMatchPreset(const UObject* WorldContextObject)
{
    const UWorld* World = GetContextWorld(WorldContextObject);
    if (!World) return nullptr;

    const AConnectIt_WorldSettings* WorldSettings = Cast<AConnectIt_WorldSettings>(World->GetWorldSettings());
    if (WorldSettings && IsValid(WorldSettings->DefaultMatchPreset))
    {
        return WorldSettings->DefaultMatchPreset;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("ConnectIt_WorldSettings: map '%s' names no Default Match Preset (World Settings > ConnectIt)"),
        *World->GetMapName());
    return nullptr;
}

#if WITH_EDITOR
EDataValidationResult AConnectIt_WorldSettings::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // Not invalid: a menu map has no board -- but a match map needs one
    if (!IsValid(LevelConfig))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_WorldSettings: No Level Config -- this map has no board (fine for a menu map, not for a match).")));
    }

    // Not invalid: the project default stands in
    if (IsValid(LevelConfig) && !IsValid(TileCatalog))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_WorldSettings: No Tile Catalog -- this map uses the project default (Project Settings > "
            "ConnectIt).")));
    }

    // Not invalid: the main menu can choose a preset (a menu-only map)
    if (!IsValid(DefaultMatchPreset))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_WorldSettings: No Default Match Preset -- this map has no match setup unless the main "
            "menu chooses one.")));
    }
    return Result;
}
#endif
