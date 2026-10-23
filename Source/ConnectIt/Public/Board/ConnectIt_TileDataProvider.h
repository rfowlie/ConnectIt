// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_TileDataProvider.generated.h"

// The starting values a designer can set on a placed tile. A separate struct
// from FConnectItTileData because that one's fields are BlueprintReadOnly, so a
// Blueprint can't Make it.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItTileInitialData
{
    GENERATED_BODY()

    // Starting scoring multiplier (a tile normally starts at 1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile", meta = (ClampMin = 0))
    float Multiplier = 1.0f;

    // Inactive tiles can't be selected
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
    bool bIsActive = true;

    // Whether a board shift may move this tile's data (false = fixed anchor)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
    bool bCanShift = true;

    // Starts locked: in play, but no placing on it (FConnectItTileData::bIsLocked)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
    bool bIsLocked = false;

    // Faction slot that owns a piece on this tile at the start of the match,
    // or -1 for an empty tile. Must be a valid faction slot for the level.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile", meta = (ClampMin = -1))
    int32 StartingFactionPiece = -1;

    // The tile's power (ConnectIt.TilePower.*), defined in the level's tile
    // catalog -- which also decides the tile actor that shows it. Only the
    // tag: a power's values are never set per tile. Empty = no power.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile", meta = (Categories = "ConnectIt.TilePower"))
    FGameplayTag TilePowerTag;

    // A Chained tile's partner (ConnectIt.TilePower.Chained), by grid
    // position: resolved to the partner's tile id when the tile joins the
    // board. A builder tile sets these from its Chain Partner Tile picker; a
    // layout cell carries them as captured. Ignored on other tiles.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile|Chain")
    bool bHasChainPartner = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile|Chain", meta = (EditCondition = "bHasChainPartner"))
    FGridPosition ChainPartner;

    // Whether StartingFactionPiece is a piece a board with NumFactions can hold
    bool HasStartingPiece(int32 NumFactions) const
    {
        return StartingFactionPiece >= 0 && StartingFactionPiece < NumFactions;
    }

    // The board tile these values start as. A starting piece for a faction the
    // board doesn't have is left off (the tile starts empty).
    FConnectItTileData ToTileData(int32 NumFactions) const
    {
        FConnectItTileData Tile;
        Tile.Multiplier = Multiplier;
        Tile.bIsActive  = bIsActive;
        Tile.bCanShift  = bCanShift;
        Tile.bIsLocked  = bIsLocked;
        Tile.SetFactionPiece(HasStartingPiece(NumFactions) ? StartingFactionPiece : -1);
        Tile.TilePowerTag = TilePowerTag;
        return Tile;
    }
};

UINTERFACE(Blueprintable)
class CONNECTIT_API UConnectIt_TileDataProvider : public UInterface
{
    GENERATED_BODY()
};

// Implemented by a tile Blueprint (no reparenting needed) to hand its
// designer-set starting values to the board state. UConnectIt_BoardStateComponent::
// InitialiseBoardState asks every tile that implements this; a tile that doesn't
// starts with the defaults (multiplier from the GameMode, active, shiftable, empty).
class CONNECTIT_API IConnectIt_TileDataProvider
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|Tile")
    FConnectItTileInitialData GetInitialTileData() const;
    virtual FConnectItTileInitialData GetInitialTileData_Implementation() const
    {
        return FConnectItTileInitialData();
    }

    // The tile is being set up from a board layout cell: take these starting
    // values (what Get Initial Tile Data should return from now on) and this
    // look (VisualVariant, empty for the default). Called on tiles spawned
    // from a layout -- by the layout tool when it loads a layout into a
    // level, and at runtime when a phased level's board changes layout. A
    // tile that ignores it keeps its own values. Default: nothing.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|Tile")
    void ApplyLayoutCell(const FConnectItTileInitialData& Data, FGameplayTag VisualVariant);
    virtual void ApplyLayoutCell_Implementation(const FConnectItTileInitialData& Data, FGameplayTag VisualVariant) {}
};
