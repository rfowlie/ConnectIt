// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConnectIt_LevelCatalogDataAsset.generated.h"

class UConnectIt_MatchPresetDataAsset;
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

    // The map to open. Its board (level config) and default match preset come
    // from its World Settings (AConnectIt_WorldSettings).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    TSoftObjectPtr<UWorld> Level;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    TSoftObjectPtr<UTexture2D> Thumbnail;
};

// What the main menu offers for a match: the playable levels and the ways to
// play them (match presets -- a preset names its AI director, so "Classic
// Easy" and "Classic Hard" are two presets). Referenced directly by the main menu
// (its widget holds the catalog asset).
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_LevelCatalogDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
    TArray<FConnectItLevelCatalogEntry> Levels;

    // Presets the menu offers to play any level as (any level with any preset)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
    TArray<TObjectPtr<UConnectIt_MatchPresetDataAsset>> MatchPresets;
};
