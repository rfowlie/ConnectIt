// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "Board/Powers/ConnectIt_TilePower.h"
#include "ConnectIt_TileCatalogDataAsset.generated.h"

class AConnectIt_TileBase;


// One kind of tile in a catalog: the actor that shows it and what its power does
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItTileCatalogEntry
{
    GENERATED_BODY()

    // The tile actor that shows this kind of tile (a ConnectIt tile: it has
    // the tile hooks visuals call)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
    TSubclassOf<AConnectIt_TileBase> TileClass;

    // Trigger, cooldown, and the effect with its own values. The effect names
    // the entry's power tag.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
    FConnectItTilePower TilePower;
};

// Every kind of tile a level can have, by power tag: the one place a power is
// defined, and which tile actor shows it. Each entry's tag is its effect's, so
// the two can't disagree; one entry per tag. Designers place builder tiles
// (AConnectIt_BuilderTile) that name a tag; when the match starts each
// becomes its entry's tile class, and the rules get the entries' powers.
//
// Chosen in the map's World Settings (Tile Catalog), else the project default
// (Project Settings > Game > ConnectIt > Default Tile Catalog). Trying another skin
// or look is a copy of a catalog with other tile classes slotted in.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_TileCatalogDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    // The tile actor for a tile with no power tag
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
    TSubclassOf<AConnectIt_TileBase> DefaultTileClass;
    
    // One entry per power (ConnectIt.TilePower.*), known by its effect's tag
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
    TArray<FConnectItTileCatalogEntry> Entries;

    // The entry for a power tag (by its effect's tag), or null
    const FConnectItTileCatalogEntry* FindEntry(const FGameplayTag& TilePowerTag) const;

    // The tile actor for a tile with this power tag: its entry's class, or
    // the default one for no tag. Null for a tag the catalog doesn't define.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Tile Catalog")
    TSubclassOf<AConnectIt_TileBase> GetTileClassFor(FGameplayTag TilePowerTag) const;

    // Whether the catalog defines this power tag
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Tile Catalog")
    bool Defines(FGameplayTag TilePowerTag) const { return FindEntry(TilePowerTag) != nullptr; }

    // Every entry's power (each known by its effect's tag): what the rules
    // play with
    TArray<FConnectItTilePower> MakeTilePowers() const;

    // The catalog WorldContextObject's map uses: its World Settings', else
    // the project default. Null (with a warning) if neither is set.
    static const UConnectIt_TileCatalogDataAsset* FindFor(const UObject* WorldContextObject);

    // The project default (Project Settings > ConnectIt), or null
    static const UConnectIt_TileCatalogDataAsset* GetProjectDefault();

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

    // Tag -> index in Entries, from each entry's effect tag (the first entry
    // for a tag wins). Built on first use, again after an edit.
    mutable TMap<FGameplayTag, int32> EntryByTag;
    mutable bool bEntryByTagBuilt = false;
};
