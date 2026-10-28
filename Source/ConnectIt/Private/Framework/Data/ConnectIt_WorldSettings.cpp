// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_MatchPresetDataAsset.h"
#include "Board/Powers/ConnectIt_TileCatalogDataAsset.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"

#if WITH_EDITOR
#include "GameMapsSettings.h"
#include "Logging/TokenizedMessage.h"
#endif


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

UConnectIt_MatchPresetDataAsset* AConnectIt_WorldSettings::FindDefaultMatchPreset(const UObject* WorldContextObject)
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

const UConnectIt_TileCatalogDataAsset* AConnectIt_WorldSettings::GetWorldTileCatalog(const UObject* WorldContextObject)
{
    const UWorld* World = GEngine && IsValid(WorldContextObject)
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
        : nullptr;
    if (!World) return nullptr;
    const AConnectIt_WorldSettings* WorldSettings = World ? Cast<AConnectIt_WorldSettings>(World->GetWorldSettings()) : nullptr;
    return WorldSettings && IsValid(WorldSettings->TileCatalog) ? WorldSettings->TileCatalog : nullptr;
}

#if WITH_EDITOR
EDataValidationResult AConnectIt_WorldSettings::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // The map's game mode: this map's override, else the project default
    const UClass* GameModeClass = DefaultGameMode.Get();
    if (!GameModeClass)
    {
        const FString ProjectGameMode = UGameMapsSettings::GetGlobalDefaultGameMode();
        GameModeClass = ProjectGameMode.IsEmpty() ? nullptr : LoadClass<AGameModeBase>(nullptr, *ProjectGameMode);
    }

    // Match data is only checked on a match map (the hub and the menus have
    // none; data set on them anyway is harmless)
    if (!GameModeClass || !GameModeClass->IsChildOf(AConnectIt_GameMode::StaticClass()))
    {
        Context.AddMessage(EMessageSeverity::Info, FText::FromString(FString::Printf(TEXT(
            "ConnectIt_WorldSettings: not a match map (game mode: %s) -- match data not checked."),
            GameModeClass ? *GameModeClass->GetName() : TEXT("none"))));
        return Result;
    }

    Context.AddMessage(EMessageSeverity::Info, FText::FromString(FString::Printf(TEXT(
        "ConnectIt_WorldSettings: checked as a match map (game mode: %s)."), *GameModeClass->GetName())));

    if (!IsValid(LevelConfig))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_WorldSettings: No Level Config -- this match map has no board.")));
    }

    // Not invalid: the project default stands in
    if (IsValid(LevelConfig) && !IsValid(TileCatalog))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_WorldSettings: No Tile Catalog -- this map uses the project default (Project Settings > "
            "ConnectIt).")));
    }

    // Not invalid: the main menu can choose a preset
    if (!IsValid(DefaultMatchPreset))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_WorldSettings: No Default Match Preset -- this map has no match setup unless the main "
            "menu chooses one.")));
    }
    return Result;
}
#endif
