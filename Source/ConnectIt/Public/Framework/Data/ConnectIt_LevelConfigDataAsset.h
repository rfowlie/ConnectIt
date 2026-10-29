// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConnectIt_LevelConfigDataAsset.generated.h"

class AConnectIt_PieceBase;
class AGridTileBase;
class UConnectIt_TileRegistry;
class UConnectIt_PieceRegistry;
class AConnectIt_BoardVisualizer;
class UGridDefinition;

// A board: the tile and piece registries, grid geometry and pieces a map is
// played on. Reusable -- any number of maps can name the same one in their
// World Settings (AConnectIt_WorldSettings::LevelConfig).
//
// How a match on it is played (rules, directors) is not here: that is a
// UConnectIt_MatchPresetDataAsset, which the map names as its default in its World
// Settings too; the main menu can choose a different one, so a level built for
// Adventure can also be played as a versus match with other rules.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_LevelConfigDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    // --- Piece Pool ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
    TSubclassOf<AConnectIt_PieceBase> PieceActorClass = nullptr;

    // Per-client pool size -- each client manages its own pool
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config",
        meta = (ClampMin = 1))
    int32 PiecePoolInitialSize = 32;

    // --- Board Registries ---
    // Instanced concrete-class UObject polymorphism, typed directly to a concrete registry base rather than an
    // interface -- a designer picks a whole UConnectIt_TileRegistry/
    // UConnectIt_PieceRegistry subclass with its own inline-editable
    // sub-properties, the same pattern ABoardManagerBase used to expose
    // these with. Typed to the ConnectIt-specific subclasses (not the
    // plugin's own UGridTileRegistryBase/UGridPieceRegistryBase) so the
    // Details-panel class picker only offers registries that can actually
    // resolve UConnectIt_BoardStateComponent -- see those classes' header
    // comments. Treated purely as a template by
    // UConnectIt_BoardRegistrySubsystem, which duplicates its own per-world
    // runtime instance from these rather than ever using them live -- see
    // that class's header comment.

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ConnectIt|Board")
    TObjectPtr<UConnectIt_TileRegistry> TileRegistry;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ConnectIt|Board")
    TObjectPtr<UConnectIt_PieceRegistry> PieceRegistry;

    // The board's visuals: one per map, spawned after the registries
    // (AConnectIt_BoardVisualizer). None = the C++ base (the actors' own
    // hooks only); pick a Blueprint subclass for this look's effects.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Board")
    TSubclassOf<AConnectIt_BoardVisualizer> VisualizerClass;

    // Grid geometry (cell size, World<->Grid conversion) both registries
    // above share -- a sibling template, same Instanced/duplicate-per-world
    // pattern, not a sub-property of either registry. See UGridDefinition's
    // own class comment for why this isn't just TileRegistry's business.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ConnectIt|Board")
    TObjectPtr<UGridDefinition> GridDefinition;

    // Superseded by the tile catalog's Plain Tile Class (the map's World
    // Settings name the catalog): only used when there is no catalog at all. The tile actor spawned for a layout cell with no
    // power tag. Clear it once the level has a catalog.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Board")
    TSubclassOf<AGridTileBase> DefaultTileClass;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext& Context) const override;
#endif
};
