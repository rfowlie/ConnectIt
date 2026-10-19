// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConnectIt_LevelCatalog.generated.h"

class UConnectIt_MatchPreset;
class UTexture2D;


// One playable level as the main menu presents it.
USTRUCT(BlueprintType)
struct FConnectItLevelCatalogEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (MultiLine = true))
    FText Description;

    // The map to open. Its stage (and default match preset) comes from the
    // level config registered for it in ConnectIt_LevelConfigSettings.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    TSoftObjectPtr<UWorld> Level;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    TSoftObjectPtr<UTexture2D> Thumbnail;
};

// What the main menu offers for a match: the playable levels and the ways to
// play them (match presets -- a preset names its AI director, so "Classic
// Easy" and "Classic Hard" are two presets). Set on ConnectIt_LevelConfigSettings::LevelCatalog
// and read through UConnectIt_MatchSetupSubsystem::GetLevelCatalog.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_LevelCatalog : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
    TArray<FConnectItLevelCatalogEntry> Levels;

    // Presets the menu offers to play any level as (any level with any preset)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
    TArray<TObjectPtr<UConnectIt_MatchPreset>> MatchPresets;
};
