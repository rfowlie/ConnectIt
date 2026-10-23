// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "Board/Powers/ConnectIt_TilePower.h"
#include "ConnectIt_TileCatalog.generated.h"

class AGridTileBase;


// One kind of tile in a catalog: the actor that shows it and what its power does
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItTileCatalogEntry
{
    GENERATED_BODY()

    // The tile actor that shows this kind of tile
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
    TSubclassOf<AGridTileBase> TileClass;

    // Trigger, gate, cooldown, effect and its values. Its Name is the entry's
    // key (leave it empty, or the same).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
    FConnectItTilePower Power;
};

// Every kind of tile a level can have, by power tag: the one place a power is
// defined, and which tile actor shows it. Designers place builder tiles
// (AConnectIt_BuilderTile) that name a tag; when the match starts each
// becomes its entry's tile class, and the rules get the entries' powers.
//
// Chosen in the map's World Settings (Tile Catalog), else the project default
// (Project Settings > Game > ConnectIt > Default Tile Catalog). Trying another skin
// or look is a copy of a catalog with other tile classes slotted in.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_TileCatalog : public UDataAsset
{
    GENERATED_BODY()

public:

    // One entry per power tag (ConnectIt.TilePower.*)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog", meta = (Categories = "ConnectIt.TilePower", ForceInlineRow))
    TMap<FGameplayTag, FConnectItTileCatalogEntry> Tiles;

    // The tile actor for a tile with no power tag
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
    TSubclassOf<AGridTileBase> PlainTileClass;

    // The tile actor for a tile with this power tag: its entry's class, or
    // the plain one for no tag. Null for a tag the catalog doesn't define.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Tile Catalog")
    TSubclassOf<AGridTileBase> GetTileClassFor(FGameplayTag TilePowerTag) const;

    // Whether the catalog defines this power tag
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Tile Catalog")
    bool Defines(FGameplayTag TilePowerTag) const { return Tiles.Contains(TilePowerTag); }

    // Every power, each named by its key: what the rules play with
    TArray<FConnectItTilePower> MakePowers() const;

    // The catalog WorldContextObject's map uses: its World Settings', else
    // the project default. Null (with a warning) if neither is set.
    static const UConnectIt_TileCatalog* FindFor(const UObject* WorldContextObject);

    // The project default (Project Settings > ConnectIt), or null
    static const UConnectIt_TileCatalog* GetProjectDefault();

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
